/*  ExternalExtractorSolver, StellarSolver Intenal Library developed by Robert Lancaster, 2020

    This application is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/

#pragma once

#include "internalextractorsolver.h"
#include <QProcess>
#include <QPointer>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

using namespace SSolver;

class ExternalExtractorSolver : public InternalExtractorSolver
{
        Q_OBJECT
    public:
        explicit ExternalExtractorSolver(ProcessType type, ExtractorType exType, SolverType solType,
                                          FITSImage::Statistic imagestats, uint8_t const *imageBuffer, QObject *parent = nullptr);
        ~ExternalExtractorSolver();


        // File Options
        QString fileToProcess;                  // This is the file that will be processed by the external SExtractor or solver
        bool fileToProcessIsTempFile = false;   // This indicates that the file is a temp file that might need to be deleted
        QString solutionFile;                   // This is the path to the solution file after solving is done.
        ExternalProgramPaths externalPaths;     // File Paths for the external solvers
        QString starXYLSFilePath;               // This is the path to the generated XYLS file from SEP to solve with the local solver
        bool starXYLSFilePathIsTempFile = false;// This will be set to true if the XYLS file gets generated

        // External Program Options
        bool cleanupTemporaryFiles = true;      // Whether or not to clean up the temporary files created
        bool autoGenerateAstroConfig = true;    // Option to generate astrometry.cfg file to pass options to the solver
        bool onlySendFITSFiles = true;          // Option to only send FITS files for compatibility

        // This is the WCS Struct that is created when the WCS information gets loaded.
        struct wcsprm *m_wcs
        {
            nullptr
        };
        int m_nwcs = 0;

        //These are used for reading and writing the star extractor file
        char* xcol = strdup("X_IMAGE");         // This is the column for the x-coordinates
        char* ycol = strdup("Y_IMAGE");         // This is the column for the y-coordinates
        char* magcol = strdup("MAG_AUTO");      // This is the column for the magnitude
        char* colFormat = strdup("1E");         // This Format means a decimal number
        char* colUnits = strdup("pixels");      // This is the unit for the xy columns in the file
        char* magUnits = strdup("magnitude");   // This is the unit for the magnitude in the file

        /**
         * @brief extract is the method that does star extraction
         * @return whether or not it was successful, 0 means success
         */
        int extract() override;

        /**
         * @brief abort will stop the external solver by issuing a kill command to the external process and using a cancel file
         */
        void abort() override;

        /**
         * @brief spawnChildSolver is a method used by StellarSolver to make the child solvers from this solver
         * @param n is a number to identify this child solver for external solvers so they can have separate files with identifying numbers
         * @return the spawned child solver
         */
        ExtractorSolver * spawnChildSolver(int n) override;

        /**
         * @brief getDefaultExternalPaths gets the default external program paths appropriate for the selected Computer System
         * @param system is the selected operating system and configuration
         * @return The appropriate ExernalProgramPaths Object
         */
        static ExternalProgramPaths getDefaultExternalPaths(ComputerSystemType system);

        /**
         * @brief getDefaultExternalPaths gets the default external program paths for this operating system
         * @return The appropriate ExernalProgramPaths Object (Note: may not be appropriate to this configuration
         */
        static ExternalProgramPaths getDefaultExternalPaths();

        /**
         * @brief setExternalFilePaths sets the external file paths for the external programs
         * @param paths are the paths to set
         */
        void setExternalFilePaths(ExternalProgramPaths paths)
        {
            externalPaths = paths;
        }

        /**
         * @brief runExternalExtractor does the star extraction using the external SExtractor.
         * @return 0 if it succeeds
         */
        int runExternalExtractor();

        /**
         * @brief appendStarsRAandDEC attaches the RA and DEC information to a star list
         * @param stars is the star list to process
         * @return true if it was successful
         */
        bool appendStarsRAandDEC(QList<FITSImage::Star> &stars) override;

        /**
         * @brief loadWCS will load the WCS Information from the WCS file
         * @return 0 if it succeeds
         */
        int loadWCS();

        /**
         * @brief pixelToWCS converts the image X, Y Pixel coordinates to RA, DEC sky coordinates using the WCS data
         * @param pixelPoint The X, Y coordinate in pixels
         * @param skyPoint The RA, DEC coordinates
         * @return A boolean to say whether it succeeded, true means it did
         */
        bool pixelToWCS(const QPointF &pixelPoint, FITSImage::wcs_point &skyPoint) override;

        /**
         * @brief wcsToPixel converts the RA, DEC sky coordinates to image X, Y Pixel coordinates using the WCS data
         * @param skyPoint The RA, DEC coordinates
         * @param pixelPoint The X, Y coordinate in pixels
         * @return A boolean to say whether it succeeded, true means it did
         */
        bool wcsToPixel(const FITSImage::wcs_point &skyPoint, QPointF &pixelPoint) override;

        /**
         * @brief saveAsFITS will save the image buffer to a FITS file for solving by external solvers
         * @return 0 if it succeeds
         */
        int saveAsFITS();

        /**
         * @brief cleanupTempFiles will clean up the temporary files
         */
        void cleanupTempFiles() override;

        /**
         * @brief writeStarExtractorTable will write the stars in the star list to an external xylist file for plate solving by other programs
         * @return 0 if it succeeds
         */
        int writeStarExtractorTable();

        /**
         * @brief getStarsFromXYLSFile gets the star list from an xylist file
         * @return 0 if it succeeds
         */
        int getStarsFromXYLSFile();

    private:
        // These are the variables for the external processes
        QPointer<QProcess> solver;
        QPointer<QProcess> extractorProcess;

        // Note: this method is needed so that the options selected in StellarSolver get passed to the solver
        /**
         * @brief generateAstrometryConfigFile creates the astrometry.cfg file for the local astrometry solver
         * @return true means it was successful
         */
        bool generateAstrometryConfigFile();

        /**
         * @brief getSolverArgsList gets the list of arguments to pass to the local astrometry.net solver
         * @return the QStringList full of arguments
         */
        QStringList getSolverArgsList();

        /**
         * @brief run starts the external solver or star extractor with QProcess.
         */
        void run() override;

        /**
         * @brief runExternalSolver runs the local astrometry.net solver
         * @return 0 if it succeeeds
         */
        int runExternalSolver();

        /**
         * @brief runExternalANSVRSolver runs the local ANSVR solver
         * @return 0 if it succeeeds
         */
        int runExternalASTAPSolver();

        /**
         * @brief runExternalWatneySolver runs the local Watney solver
         * @return 0 if it succeeeds
         */
        int runExternalWatneySolver();

        /**
         * @brief getSolutionInformation gets the Solution Info from the local astrometry.net solution file (WCS)
         * @return
         */
        bool getSolutionInformation();

        /**
         * @brief getASTAPSolutionInformation gets the Solution Info from the local ASTAP solution file (INI)
         * @return
         */
        bool getASTAPSolutionInformation();

        /**
         * @brief getSolutionInformation gets the Solution Info from the local Watney solution file (INI)
         * @return
         */
        bool getWatneySolutionInformation();

        /**
         * @brief logSolver will log the output of the solver to a file or program output
         */
        void logSolver();

        /**
         * @brief logSolver will log the output of the external SExtrator to a file or program output
         */
        void logSextractor();

};

