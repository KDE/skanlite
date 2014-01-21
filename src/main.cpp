/* ============================================================
*
* Copyright (C) 2007-2012 by Kåre Särs <kare.sars@iki .fi>
* Copyright (C) 2014 by Gregor Mitsch: port to KDE5 frameworks
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation; either version 2 of
* the License or (at your option) version 3 or any later version
* accepted by the membership of KDE e.V. (or its successor approved
*  by the membership of KDE e.V.), which shall act as a proxy
* defined in Section 14 of version 3 of the license.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License.
* along with this program.  If not, see <http://www.gnu.org/licenses/>
*
* ============================================================ */
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <kglobal.h>

#include "skanlite.h"
#include "version.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // about data
    KAboutData aboutData("Skanlite", // appname
                         "skanlite", // catalogName
                         i18n("Skanlite"), // rogramName
                         skanlite_version, // version
                         i18n("This is a scanning application for KDE based on libksane."), // shortDescription 
                         KAboutData::License_GPL, // licenseType
                         i18n("(C) 2008-2014 Kåre Särs") // copyrightStatement
                        );

    aboutData.addAuthor(i18n("Kåre Särs"),
                        i18n("developer"),
                        "kare.sars@iki.fi");

    aboutData.addAuthor(i18n("Arseniy Lartsev"),
                        i18n("contributor"));

    aboutData.addCredit(i18n("Gilles Caulier"),
                        i18n("Importing libksane to extragear"));

    aboutData.addCredit(i18n("Anne-Marie Mahfouf"),
                        i18n("Writing the user manual"));

    aboutData.addCredit(i18n("Laurent Montel"),
                        i18n("Importing libksane to extragear"));

    aboutData.addCredit(i18n("Chusslove Illich"),
                        i18n("Help with translations"));

    aboutData.addCredit(i18n("Albert Astals Cid"),
                        i18n("Help with translations"));

    aboutData.setProgramIconName("scanner");

    // FIXME KF5 is the command line parsing still working? (ported KCmdLineOptions to QCommandLineParser)
    //KCmdLineArgs::init(argc, argv, &aboutData);
    //KCmdLineOptions options;
    //options.add("d <device>", ki18n("Sane scanner device name."));
    //KCmdLineArgs::addCmdLineOptions(options);
    //KCmdLineArgs *args = KCmdLineArgs::parsedArgs();
    //QString device = args->getOption("d");
    QString device; // FIXME KF5


    Skanlite skanliteDialog(device, 0);
    skanliteDialog.setAboutData(&aboutData);

    skanliteDialog.show();

    return app.exec();
}

