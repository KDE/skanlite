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

#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QShortcut>

#include <KAboutData>
#include <KLocalizedString>

#include "skanlite.h"

#include <skanlite_debug.h>
#include <skanlite_version.h>

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication app(argc, argv);

    KLocalizedString::setApplicationDomain("skanlite");

    KAboutData aboutData(QStringLiteral("skanlite"), // componentName, k4: appName
                         i18n("Skanlite"), // displayName, k4: programName
                         QStringLiteral(SKANLITE_VERSION_STRING), // version
                         i18n("Scanning application by KDE based on libksane."), // shortDescription
                         KAboutLicense::GPL, // licenseType
                         i18n("(C) 2008-2020 Kåre Särs"), // copyrightStatement
                         QString(), // other Text
                         QString() // homePageAddress
                        );

    aboutData.addAuthor(i18n("Kåre Särs"),
                        i18n("developer"),
                        QStringLiteral("kare.sars@iki.fi"));

    aboutData.addAuthor(i18n("Gregor Mi"),
                        i18n("contributor"));

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

    // Required for showing the translation list KXmlGui is not used
    aboutData.setTranslator(i18nc("NAME OF TRANSLATORS", "Your names"),
                            i18nc("EMAIL OF TRANSLATORS", "Your emails"));

    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("skanlite")));
    KAboutData::setApplicationData(aboutData);

    QCoreApplication::setApplicationVersion(aboutData.version());
    QCommandLineParser parser;
    aboutData.setupCommandLine(&parser);
    QCommandLineOption deviceOption(QStringList() << QStringLiteral("d") << QStringLiteral("device"), i18n("Sane scanner device name. Use 'test' for test device."), i18n("device"));
    parser.addOption(deviceOption);
    parser.process(app); // the --author and --license is shown anyway but they work only with the following line
    aboutData.processCommandLine(&parser);

    const QString deviceName = parser.value(deviceOption);
    qCDebug(SKANLITE_LOG) << QStringLiteral("deviceOption value=%1").arg(deviceName);

    Skanlite skanliteDialog(deviceName, nullptr);

    skanliteDialog.show();

    QShortcut *prevShortcut = new QShortcut(QKeySequence(QStringLiteral("Ctrl+Q")), &skanliteDialog);
    QObject::connect(prevShortcut, &QShortcut::activated, &app, &QApplication::quit);

    return app.exec();
}

