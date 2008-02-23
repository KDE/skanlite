#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <kglobal.h>

#include "glimpse.h"

int main(int argc, char *argv[])
{
    // about data
    KAboutData aboutData("glimpse", "glimpse", ki18n("Glimpse"), "0.1",
                         ki18n("This is an example application for libksane."),
                         KAboutData::License_GPL,
                         ki18n("(C) %{YEAR} Kåre Särs"));

    aboutData.addAuthor(ki18n("Kåre Särs"),
                        ki18n("developer"),
                        "kare.sars@iki.fi", 0);

    KCmdLineArgs::init(argc, argv, &aboutData);

    KCmdLineOptions options;
    options.add("d <device>", ki18n("Sane scanner device name."));
    KCmdLineArgs::addCmdLineOptions(options);
    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    QString device = args->getOption("d");

    KApplication app;
    
    // get the translations for libksane
    KGlobal::locale()->insertCatalog("libksane");

    Glimpse *ksane_test = new Glimpse(device, 0);

    ksane_test->show();

    QObject::connect(&app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()));

    return app.exec();
}

