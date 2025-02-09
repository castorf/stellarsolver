#include <QApplication>

//Includes for this project
#include "structuredefinitions.h"
#include "stellarsolver.h"
#include "testerutils/fileio.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
#if defined(__linux__)
    setlocale(LC_NUMERIC, "C");
#endif
    fileio imageLoader;
    if(!imageLoader.loadImage("pleiades.jpg"))
    {
        printf("Error in loading FITS file");
        exit(1);
    }
    FITSImage::Statistic stats = imageLoader.getStats();
    uint8_t *imageBuffer = imageLoader.getImageBuffer();

    StellarSolver stellarSolver(stats, imageBuffer);
    stellarSolver.setIndexFolderPaths(QStringList() << "astrometry");

    printf("Starting to solve. . .\n");
    fflush( stdout );

    if(!stellarSolver.solve())
    {
        printf("Solver Failed");
        exit(0);
    }

    FITSImage::Solution solution = stellarSolver.getSolution();
    printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");

    printf("Field center: (RA,Dec) = (%f, %f) deg.\n", solution.ra, solution.dec);
    printf("Field size: %f x %f arcminutes\n", solution.fieldWidth, solution.fieldHeight);
    printf("Pixel Scale: %f\"\n", solution.pixscale);
    printf("Field rotation angle: up is %f degrees E of N\n", solution.orientation);
    printf("Field parity: %s\n", FITSImage::getParityText(solution.parity).toUtf8().data());
    fflush( stdout );

    stellarSolver.setParameterProfile(SSolver::Parameters::ALL_STARS);

    if(!stellarSolver.extract(true))
    {
        printf("Solver Failed");
        exit(0);
    }

    QList<FITSImage::Star> starList = stellarSolver.getStarList();
    printf("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    printf("Stars found: %u\n", starList.count());
    for(int i=0; i < starList.count(); i++)
    {
        FITSImage::Star star = starList.at(i);
        char *ra = StellarSolver::raString(star.ra).toUtf8().data();
        char *dec = StellarSolver::decString(star.dec).toUtf8().data();
        printf("Star #%u: (%f x, %f y), (ra: %s,dec: %s), mag: %f, peak: %f, hfr: %f \n", i, star.x, star.y, ra , dec, star.mag, star.peak, star.HFR);
    }
    return 0;
}
