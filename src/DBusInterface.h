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

#ifndef DBusInterface_h
#define DBusInterface_h

#include <QDBusConnection>
#include <QDebug>
#include <KSaneWidget>
#include <QStringList>

static const bool defaultSelectionFiltering = true;
static const QLatin1String defaultProfile("1");

class DBusInterface : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.skanlite")
    QStringList m_msgBuffer;

public:

    explicit DBusInterface(QObject* parent = nullptr) : QObject(parent) {}

    bool setupDBusInterface();

    const QStringList& reply() { return m_msgBuffer; }
    void setReply(const QStringList &reply) { m_msgBuffer = reply; }

private:
    // helper method for QDbusViewer compatibility. The details are in .cpp
    const QStringList ensureStringList(const QStringList &list);

Q_SIGNALS:
    // used to communicate with Skanlite class
    void requestedScan();
    void requestedPreview();
    void requestedScanCancel();
    void requestedGetScannerOptions();
    void requestedSetScannerOptions(const QStringList &options, bool ignoreSelection);
    void requestedDefaultScannerOptions();
    void requestedDeviceName();
    void requestedSaveScannerOptionsToProfile(const QStringList &options, const QString &profile, bool ignoreSelection);
    void requestedSwitchToProfile(const QString &profile, bool ignoreSelection);
    void requestedGetSelection();
    void requestedSetSelection(const QStringList &options);

public Q_SLOTS:

    // called via D-Bus

    // Perform scan operation if is in idle
    Q_SCRIPTABLE void scan() { emit requestedScan(); }

    // Perform preview operation if is in idle
    Q_SCRIPTABLE void preview() { emit requestedPreview(); }

    // Cancel any ongoing operation
    Q_SCRIPTABLE void scanCancel() { emit requestedScanCancel(); }

    // Return device name, like "Hewlett-Packard:Scanjet 4370"
    Q_SCRIPTABLE QString getDeviceName()
    {
        emit requestedDeviceName();
        return reply().join(QLatin1String());
    }

    // Return current scanner options as returned by KSaneWidget::getOptVals()
    Q_SCRIPTABLE QStringList getScannerOptions()
    {
        emit requestedGetScannerOptions();
        return reply();
    }

    // Replaces current scanner options with argument value. Ignores page selection area info if ignoreSelection is true (by default)
    Q_SCRIPTABLE void setScannerOptions(const QStringList &options, bool ignoreSelection = defaultSelectionFiltering)
    {        
        emit requestedSetScannerOptions(ensureStringList(options), ignoreSelection);
    }

    // Return current scanner options
    Q_SCRIPTABLE QStringList getDefaultScannerOptions()
    {
        emit requestedDefaultScannerOptions();
        return reply();
    }

    // Save options to KConfigGroup named ""Options For %Current_Device% - Profile %profile%""
    // Thus it's device specific. Default profile is "1". Could be any string value.
    // Ignores page selection area info if ignoreSelection is true (by default)
    Q_SCRIPTABLE void saveScannerOptionsToProfile(const QStringList &options, const QString &profile = defaultProfile, bool ignoreSelection = defaultSelectionFiltering)
    {        
        emit requestedSaveScannerOptionsToProfile(ensureStringList(options), profile, ignoreSelection);
    }

    // Gets current options via KSaneWidget::getOptVals() and saves them into provile
    // Acts as a combination of saveScannerOptionsToProfile and requestedSaveScannerOptionsToProfile
    // Made for easy bind both to hotkey in KHotkeys
    Q_SCRIPTABLE void saveCurrentScannerOptionsToProfile(const QString &profile = defaultProfile, bool ignoreSelection = defaultSelectionFiltering)
    {
        emit requestedGetScannerOptions(); // put result in m_msgBuffer
        emit requestedSaveScannerOptionsToProfile(reply(), profile, ignoreSelection);
    }

    // Loads options from specified profile and applies them. Ignores page selection area info if ignoreSelection is true (by default)
    Q_SCRIPTABLE void switchToProfile(const QString &profile = defaultProfile, bool ignoreSelection = defaultSelectionFiltering)
    {
        emit requestedSwitchToProfile(profile, ignoreSelection);
    }

    // Returns current selection area in form "{"tl-x=0", "tl-y=0", "br-x=220", "br-y=248"}"
    Q_SCRIPTABLE QStringList getSelection()
    {
        emit requestedGetSelection();
        return reply();
    }

    // Changes current selection area. Argument must be a list of strings in  form "{"tl-x=0", "tl-y=0", "br-x=220", "br-y=248"}"
    Q_SCRIPTABLE void setSelection(const QStringList &options)
    {        
        emit requestedSetSelection(ensureStringList(options));
    }

Q_SIGNALS:

    Q_SCRIPTABLE void imageSaved(const QString &strFilename);

    // Below are 4 signals which are just forwarded from KSaneWidget.
    // You can take a look in KSaneWidget.h for detailed arguments description

    Q_SCRIPTABLE void scanDone(int status, const QString &strStatus);
    Q_SCRIPTABLE void userMessage(int type, const QString &strStatus);
    Q_SCRIPTABLE void scanProgress(int percent);
    Q_SCRIPTABLE void buttonPressed(const QString &optionName, const QString &optionLabel, bool pressed);
};

#endif
