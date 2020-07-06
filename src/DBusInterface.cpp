/* ============================================================
*
* Copyright (C) 2017 by Alexander Trufanov
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

#include "DBusInterface.h"

#include <skanlite_debug.h>

bool DBusInterface::setupDBusInterface()
{
    QDBusConnection session = QDBusConnection::sessionBus();

    if (!session.isConnected()) {
        qCDebug(SKANLITE_LOG) << ("ERROR: Cannot connect to the D-Bus session bus. Continuing...");
        return false;
    }

    if(!session.registerObject(QLatin1String("/"), this, QDBusConnection::ExportScriptableContents)) {
        qCDebug(SKANLITE_LOG) << ("ERROR: Cannot register D-Bus object. Continuing...");
        return false;
    }

    if(!session.registerService(QLatin1String("org.kde.skanlite"))) {
        qCDebug(SKANLITE_LOG) << ("ERROR: Cannot register D-Bus service. Continuing...");
        return false;
    }

    return true;
}


// QDbusViewer (qttools5-dev-tools) is a very convinient tool to test D-Bus messaging (QTBUG-7341)
// But it can't pass string list parameters. It passes them as one string in following format:
// "{"KSane::InvertColors=false", "blue-gamma-table=0:0:100", "br-x=220", "br-y=300", "button 0=false", "button 1=false", "button 2=false", "button 3=false", "depth=8 битов", "green-gamma-table=0:0:100", "mode=Color", "opt_chipid=4", "opt_chipname=RTS8822L-02A", "opt_dbgimages=false", "opt_emulategray=false", "opt_model=HP4370", "opt_negative=false", "opt_nogamma=false", "opt_nowarmup=true", "opt_nowshading=true", "opt_realdepth=false", "opt_scancount=12381", "red-gamma-table=0:0:100", "resolution=50 DPI", "source=Flatbed", "tl-x=0", "tl-y=0"}"
// We check for this format and convert string back to string list if detected.
const QStringList DBusInterface::ensureStringList(const QStringList &list)
{
    if (list.length() == 1) {
        QString s = list[0].trimmed();
        if (s.left(2) == QLatin1String("{\"") && s.right(2) == QLatin1String("\"}")) {
            return s.remove(QLatin1String("{\"")).remove(QLatin1String("\"}")).split(QLatin1String("\", \""));
        }
    }

    return list;
}
