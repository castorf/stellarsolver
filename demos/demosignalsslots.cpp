#include "demosignalsslots.h"

//Includes for this project
#include "structuredefinitions.h"
#include "stellarsolver.h"
#include "testerutils/fileio.h"

DemoSignalsSlots::DemoSignalsSlots()
{
    //Setting up simultaneous solvers.  Not usually recommended, but it does work.
    setupStellarSolver("randomsky.fits");
}

void DemoSignalsSlots::setupStellarSolver(QString fileName)
{
    fileio imageLoader;
    if(!imageLoader.loadImage(fileName))
    {
        printf("Error in loading file");
        exit(1);
    }
    FITSImage::Statistic stats = imageLoader.getStats();
    uint8_t *imageBuffer = imageLoader.getImageBuffer();

    StellarSolver *stellarSolver = new StellarSolver(stats, imageBuffer, nullptr);
    stellarSolver->setProperty("ExtractorType", SSolver::EXTRACTOR_INTERNAL);
    stellarSolver->setProperty("SolverType", SSolver::SOLVER_STELLARSOLVER);
    stellarSolver->setProperty("ProcessType", SSolver::SOLVE);
    stellarSolver->setParameterProfile(SSolver::Parameters::SINGLE_THREAD_SOLVING);
    stellarSolver->setIndexFolderPaths(QStringList() << "astrometry");
    stellarSolver->setLogLevel(SSolver::LOG_ALL);
    if(imageLoader.position_given)
    {
        printf("Using Position: %f hours, %f degrees\n", imageLoader.ra, imageLoader.dec);
        stellarSolver->setSearchPositionRaDec(imageLoader.ra, imageLoader.dec);
    }
    if(imageLoader.scale_given)
    {
        stellarSolver->setSearchScale(imageLoader.scale_low, imageLoader.scale_high, imageLoader.scale_units);
        printf("Using Scale: %f to %f, %s\n", imageLoader.scale_low, imageLoader.scale_high, SSolver::getScaleUnitString(imageLoader.scale_units).toUtf8().data());
    }
    printf("Starting to solve. . .\n");
    fflush( stdout );

    connect(stellarSolver, &StellarSolver::finished, this, &DemoSignalsSlots::stellarSolverFinished);
    connect(stellarSolver, &StellarSolver::logOutput, this, &DemoSignalsSlots::logOutput);
    stellarSolver->start();

}

void DemoSignalsSlots::stellarSolverFinished()
{
    StellarSolver *reportingSolver = qobject_cast<StellarSolver*>(sender());
    FITSImage::Solution solution = reportingSolver->getSolution();
    printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");

    printf("Field center: (RA,Dec) = (%f, %f) deg.\n", solution.ra, solution.dec);
    printf("Field size: %f x %f arcminutes\n", solution.fieldWidth, solution.fieldHeight);
    printf("Pixel Scale: %f\"\n", solution.pixscale);
    printf("Field rotation angle: up is %f degrees E of N\n", solution.orientation);
    printf("Field parity: %s\n", FITSImage::getParityText(solution.parity).toUtf8().data());
    fflush( stdout );
    exit(0);
}

void DemoSignalsSlots::logOutput(QString text)
{
    printf("%s \n", text.toUtf8().data());
}


int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
#if defined(__linux__)
    setlocale(LC_NUMERIC, "C");
#endif
    DemoSignalsSlots *demo = new DemoSignalsSlots();
    app.exec();

    delete demo;

    return 0;
}
