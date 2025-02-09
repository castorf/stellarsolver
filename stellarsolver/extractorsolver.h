/*  ExtractorSolver, StellarSolver Intenal Library developed by Robert Lancaster, 2020

    This application is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
*/
#pragma once

//Includes for this project
#include <QThread>
#include <QRect>
#include <QDir>
#include <QVector>
#include "structuredefinitions.h"
#include "parameters.h"

//CFitsio Includes
#include <fitsio.h>

//Astrometry.net includes
extern "C" {
#include "astrometry/starutil.h"
}

using namespace SSolver;

class ExtractorSolver : public QThread
{
        Q_OBJECT
    public:
        ExtractorSolver(ProcessType type, ExtractorType exType, SolverType solType, const FITSImage::Statistic &statistics,
                         const uint8_t *imageBuffer, QObject *parent = nullptr);
        ~ExtractorSolver();

    // ExtractorSolver Options

        // Primary Options
        ProcessType m_ProcessType;
        ExtractorType m_ExtractorType;
        SolverType m_SolverType;

        // Logging Settings for Astrometry and StellarSolver
        bool m_LogToFile = false;                       // This determines whether or not to save the output from Astrometry.net to a file
        QString m_LogFileName;                          // This is the path to the log file that it will save.
        logging_level m_AstrometryLogLevel = LOG_NONE;  // This is the level of logging reported from Astrometry.net
        SSolverLogLevel m_SSLogLevel {LOG_NORMAL};      // This is the level for the StellarSolver Logging

        // These are for creating temporary files
        QString m_BaseName;                 // This is the base name used for all temporary files.  If it is not set, it will use a number based on the order of solvers created.
        QString m_BasePath;                 // This is the path used for saving any temporary files.  They are by default saved to the default temp directory, you can change it if you want to.

        // Index File Options
        QStringList indexFolderPaths;       // This is the list of folder paths that the solver will use to search for index files
        QStringList indexFiles;             // This is an alternative to the indexFolderPaths variable.  We can just load individual index files instead of searching for them

        // The currently set parameters for StellarSolver
        Parameters m_ActiveParameters;      // The currently set parameters for StellarSolver

        // This is the convolution filter used by SEP to help extract stars
        QVector<float> convFilter = {1, 2, 1,
                                     2, 4, 2,
                                     1, 2, 1
                                    };

        // Astrometry Scale Parameters, These are not saved parameters and change for each image, use the methods to set them
        bool m_UseScale = false;            // Whether or not to use the image scale parameters
        double scalelo = 0;                 // Lower bound of image scale estimate
        double scalehi = 0;                 // Upper bound of image scale estimate
        ScaleUnits scaleunit;               // In what units are the lower and upper bounds?

        // Astrometry Depth Parameters, for searching at different depths
        int depthlo = -1;                   // This is the low depth of this child solver
        int depthhi = -1;                   // This is the high depth of this child solver

        // Astrometry Position Parameters, These are not saved parameters and change for each image, use the methods to set them
        bool m_UsePosition = false;         // Whether or not to use initial information about the position
        double search_ra = HUGE_VAL;        // RA of field center for search, format: decimal degrees
        double search_dec = HUGE_VAL;       // DEC of field center for search, format: decimal degrees

    // ExtractorSolver Methods
        /**
         * @brief extract is the method that does star extraction
         * @return whether or not it was successful, 0 means success
         */
        virtual int extract() = 0;

        /**
         * @brief execute is a function that performs just like start, but in a blocking way, instead of in a different thread.  NOTE: At the moment we are only using this for the Online Solver
         */
        virtual void execute();

        /**
         * @brief abort will stop the extractorsolver by setting a cancel variable, using the quit method, using the kill method, and/or making a cancel file
         */
        virtual void abort() = 0;

        /**
         * @brief spawnChildSolver is a method used by StellarSolver to make the child solvers from this solver
         * @param n is a number to identify this child solver for external solvers so they can have separate files with identifying numbers
         * @return the spawned child solver
         */
        virtual ExtractorSolver* spawnChildSolver(int n) = 0;

        /**
         * @brief cleanupTempFiles will delete the temporary files used by external solvers
         */
        virtual void cleanupTempFiles() = 0;

        /**
         * @brief appendStarsRAandDEC attaches the RA and DEC information to a star list
         * @param stars is the star list to process
         * @return true if it was successful
         */
        virtual bool appendStarsRAandDEC(QList<FITSImage::Star> &stars) = 0;

        /**
         * @brief getScaleUnitString gets a string for the scale units used in the scale for plate solving
         * @return The string for the scale units used.
         */
        inline QString getScaleUnitString()
        {
            switch(scaleunit)
            {
                case DEG_WIDTH:
                    return "degwidth";
                    break;
                case ARCMIN_WIDTH:
                    return "arcminwidth";
                    break;
                case ARCSEC_PER_PIX:
                    return "arcsecperpix";
                    break;
                case FOCAL_MM:
                    return "focalmm";
                    break;
                default:
                    return "";
                    break;
            }
        }

        /**
         * @brief setSearchScale will set the Search Scale range to speed up the solver based on the ScaleUnits QString
         * @param fov_low The low end of the search range in the specified units
         * @param fov_high The high end of the search range in the specified units
         * @param scaleUnits The specified units for the range.
         */
        void setSearchScale(double fov_low, double fov_high, ScaleUnits units);

        /**
         * @brief setSearchPositionInDegrees sets the search RA/DEC/Radius to speed up the solver when plate solving
         * @param ra The Right Ascension in decimal degrees
         * @param dec The Declination in decimal degrees
         */
        void setSearchPositionInDegrees(double ra, double dec);

        /**
         * @brief getBackground gets information about the image background found during star exraction
         * @return The background information
         */
        const FITSImage::Background &getBackground() const
        {
            return m_Background;
        }

        /**
         * @brief getNumStarsFound gets the number of stars found in the star extraction
         * @return The number of stars found
         */
        int getNumStarsFound() const
        {
            return m_ExtractedStars.size();
        };

        /**
         * @brief getStarList gets the list of stars found during star extraction
         * @return A QList full of stars and their properties
         */
        const QList<FITSImage::Star> &getStarList() const
        {
            return m_ExtractedStars;
        }

        /**
         * @brief getSolution gets the Solution information from the latest plate solve
         * @return The Solution information
         */
        FITSImage::Solution getSolution()
        {
            return m_Solution;
        };

        /**
         * @brief getSolutionIndexNumber gets the astrometry index file number used to solve the latest plate solve
         * @return The index number
         */
        short getSolutionIndexNumber()
        {
            return solutionIndexNumber;
        };

        /**
         * @brief getSolutionHealpix gets the healpix identifying which image in the index series solved the image in the latest plate solve
         * @return The healpix number
         */
        short getSolutionHealpix()
        {
            return solutionHealpix;
        };

        /**
         * @brief hasWCSData gets whether or not WCS Data has been retrieved for the image after plate solving
         * @return true means we have WCS data
         */
        bool hasWCSData()
        {
            return m_HasWCS;
        };

        /**
         * @brief solvingDone Whether or not plate solving has been completed
         * @return true means the plate solving is done
         */
        bool solvingDone()
        {
            return m_HasSolved;
        };

        /**
         * @brief extractionDone Whether or not star extraction has been completed
         * @return true means the star extraction is done
         */
        bool extractionDone()
        {
            return m_HasExtracted;
        };

        /**
         * @brief isCalculatingHFR returns whether or not the star extraction is also doing HFR
         * @return true means HFR is being done too
         */
        bool isCalculatingHFR()
        {
            return m_ProcessType == EXTRACT_WITH_HFR;
        };

        /**
         * @brief setUseSubframe sets up a subframe for star extraction
         * @param frame The QRect that represents the subframe in the image
         */
        void setUseSubframe(QRect frame)
        {
            m_UseSubframe = true;
            m_SubFrameRect = frame;
        };

        /**
         * @brief pixelToWCS converts the image X, Y Pixel coordinates to RA, DEC sky coordinates using the WCS data
         * @param pixelPoint The X, Y coordinate in pixels
         * @param skyPoint The RA, DEC coordinates
         * @return A boolean to say whether it succeeded, true means it did
         */
        virtual bool pixelToWCS(const QPointF &pixelPoint, FITSImage::wcs_point &skyPoint) = 0;

        /**
         * @brief wcsToPixel converts the RA, DEC sky coordinates to image X, Y Pixel coordinates using the WCS data
         * @param skyPoint The RA, DEC coordinates
         * @param pixelPoint The X, Y coordinate in pixels
         * @return A boolean to say whether it succeeded, true means it did
         */
        virtual bool wcsToPixel(const FITSImage::wcs_point &skyPoint, QPointF &pixelPoint) = 0;


    protected:  //Note: These items are not private because they are needed by Child Classes

        // Useful State Information
        bool m_HasExtracted = false;            // This boolean is set when the sextraction is done and successful
        bool m_HasSolved = false;               // This boolean is set when the solving is done and successful
        bool m_HasWCS = false;                  // This boolean gets set if the StellarSolver has WCS data to retrieve
        bool m_WasAborted = false;              // This boolean gets set if the StellarSolver was aborted

        // Subframing Options
        bool m_UseSubframe = false;             // Whether or not to use the subframe for star extraction
        QRect m_SubFrameRect;                   // The subframe to use in star extraction

        FITSImage::Statistic m_Statistics;      // This is information about the image
        const uint8_t *m_ImageBuffer { nullptr };   //The generic data buffer containing the image data
        bool usingDownsampledImage = false;     // This boolean gets set internally if we are using a downsampled image buffer for SEP

    // The Results

        FITSImage::Background m_Background;     // This is a report on the background levels found during star extraction
        QList<FITSImage::Star> m_ExtractedStars;// This is the list of stars that get extracted from the image
        FITSImage::Solution m_Solution;         // This is the solution that comes back from the Solver
        short solutionIndexNumber = -1;         // This is the index number of the index used to solve the image.
        short solutionHealpix = -1;             // This is the healpix of the index used to solve the image.

        // This is the cancel file path that astrometry.net monitors.  If it detects this file, it aborts the solve
        QString cancelfn;           //Filename whose creation signals the process to stop
        QString solvedfn;           //Filename whose creation tells astrometry.net it already solved the field.

        bool isChildSolver = false;             // This identifies that this solver is in fact a child solver.

        /**
         * @brief runSEPExtractor is the method that actually runs the internal extractor
         * @return true if it succeeded
         */
        bool runSEPExtractor();

        /**
         * @brief convertToDegreeHeight is a function made for Watney and ASTAP Solvers since they use degree height not the astrometry scale numbers
         * @param scale is the number for scale in the current scaleunit that needs to be converted
         * @return the converted degree height
         */
        inline double convertToDegreeHeight(double scale)
        {
            switch(scaleunit)
            {
                case DEG_WIDTH:
                    return scale;
                    break;
                case ARCMIN_WIDTH:
                    return arcmin2deg(scale);
                    break;
                case ARCSEC_PER_PIX:
                    return arcsec2deg(scale) * (double)m_Statistics.height;
                    break;
                case FOCAL_MM:
                    return rad2deg(atan(36. / (2. * scale)));
                    break;
                default:
                    return scale;
                    break;
            }
        }

    signals:

        /**
         * @brief logOutput signals that there is infomation that should be printed to a log file or log window
         * @param logText is the QString that should be logged
         */
        void logOutput(QString logText);

        /**
         * @brief finished Extraction and/or solving complete, whether successful or not, and StellarSolver has shut down.
         * @param exit_code 0 means success.
         */
        void finished(int exit_code);

};

