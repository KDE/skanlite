/* ============================================================
 *
 * SPDX-FileCopyrightText: 2017 Alexander Trufanov
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 *
 * ============================================================ */

#ifndef DBusInterface_h
#define DBusInterface_h

#include <KSaneWidget>
#include <QDBusConnection>
#include <QStringList>

static const bool defaultSelectionFiltering = true;
static const QLatin1String defaultProfile("1");

class DBusInterface : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.skanlite")
    QStringList m_msgBuffer;

public:
    explicit DBusInterface(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    bool setupDBusInterface();

    const QStringList &reply()
    {
        return m_msgBuffer;
    }
    void setReply(const QStringList &reply)
    {
        m_msgBuffer = reply;
    }

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
    Q_SCRIPTABLE void scan()
    {
        Q_EMIT requestedScan();
    }

    // Perform preview operation if is in idle
    Q_SCRIPTABLE void preview()
    {
        Q_EMIT requestedPreview();
    }

    // Cancel any ongoing operation
    Q_SCRIPTABLE void scanCancel()
    {
        Q_EMIT requestedScanCancel();
    }

    // Return device name, like "Hewlett-Packard:Scanjet 4370"
    Q_SCRIPTABLE QString getDeviceName()
    {
        Q_EMIT requestedDeviceName();
        return reply().join(QLatin1String());
    }

    // Return current scanner options as returned by KSaneWidget::getOptVals()
    Q_SCRIPTABLE QStringList getScannerOptions()
    {
        Q_EMIT requestedGetScannerOptions();
        return reply();
    }

    // Replaces current scanner options with argument value. Ignores page selection area info if ignoreSelection is true (by default)
    Q_SCRIPTABLE void setScannerOptions(const QStringList &options, bool ignoreSelection = defaultSelectionFiltering)
    {
        Q_EMIT requestedSetScannerOptions(ensureStringList(options), ignoreSelection);
    }

    // Return current scanner options
    Q_SCRIPTABLE QStringList getDefaultScannerOptions()
    {
        Q_EMIT requestedDefaultScannerOptions();
        return reply();
    }

    // Save options to KConfigGroup named ""Options For %Current_Device% - Profile %profile%""
    // Thus it's device specific. Default profile is "1". Could be any string value.
    // Ignores page selection area info if ignoreSelection is true (by default)
    Q_SCRIPTABLE void
    saveScannerOptionsToProfile(const QStringList &options, const QString &profile = defaultProfile, bool ignoreSelection = defaultSelectionFiltering)
    {
        Q_EMIT requestedSaveScannerOptionsToProfile(ensureStringList(options), profile, ignoreSelection);
    }

    // Gets current options via KSaneWidget::getOptVals() and saves them into provile
    // Acts as a combination of saveScannerOptionsToProfile and requestedSaveScannerOptionsToProfile
    // Made for easy bind both to hotkey in KHotkeys
    Q_SCRIPTABLE void saveCurrentScannerOptionsToProfile(const QString &profile = defaultProfile, bool ignoreSelection = defaultSelectionFiltering)
    {
        Q_EMIT requestedGetScannerOptions(); // put result in m_msgBuffer
        Q_EMIT requestedSaveScannerOptionsToProfile(reply(), profile, ignoreSelection);
    }

    // Loads options from specified profile and applies them. Ignores page selection area info if ignoreSelection is true (by default)
    Q_SCRIPTABLE void switchToProfile(const QString &profile = defaultProfile, bool ignoreSelection = defaultSelectionFiltering)
    {
        Q_EMIT requestedSwitchToProfile(profile, ignoreSelection);
    }

    // Returns current selection area in form "{"tl-x=0", "tl-y=0", "br-x=220", "br-y=248"}"
    Q_SCRIPTABLE QStringList getSelection()
    {
        Q_EMIT requestedGetSelection();
        return reply();
    }

    // Changes current selection area. Argument must be a list of strings in  form "{"tl-x=0", "tl-y=0", "br-x=220", "br-y=248"}"
    Q_SCRIPTABLE void setSelection(const QStringList &options)
    {
        Q_EMIT requestedSetSelection(ensureStringList(options));
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
