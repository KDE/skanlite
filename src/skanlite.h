/* ============================================================
 *
 * SPDX-FileCopyrightText: 2007-2012 Kåre Särs <kare.sars@iki .fi>
 * SPDX-FileCopyrightText: 2014 Gregor Mitsch : port to KDE5 frameworks
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 *
 * ============================================================ */

#ifndef Skanlite_h
#define Skanlite_h

#include <QDialog>
#include <QTimer>
#include <QUrl>

#include "DBusInterface.h"
#include "ui_settings.h"

class ShowImageDialog;
class SaveLocation;
class QProgressBar;

namespace KSaneIface
{
class KSaneWidget;
}

class Skanlite : public QDialog
{
    Q_OBJECT

public:
    explicit Skanlite(const QString &device, QWidget *parent);

private:
    // Order of items in save mode combo-box
    enum SaveMode {
        SaveModeManual = 0,
        SaveModeAskFirst = 1,
    };

    void readSettings();
    void loadScannerOptions();

    void processSelectionOptions(QMap<QString, QString> &opts, bool ignoreSelection);

private Q_SLOTS:
    void showSettingsDialog();
    void imageReady(const QImage &image);
    void saveImage();
    void updateSaveProgress();
    void imageSaved(const QUrl &url, const QString &name, bool success);
    void saveWindowSize();
    void saveScannerDevice();

    void saveScannerOptions();
    void defaultScannerOptions();
    void applyScannerOptions(const QMap<QString, QString> &opts);

    void alertUser(int type, const QString &strStatus);
    void buttonPressed(const QString &optionName, const QString &optionLabel, bool pressed);

    void showHelp();

    // slots to communicate with D-Bus interface
    void getScannerOptions();
    void setScannerOptions(const QStringList &options, bool ignoreSelection);
    void getDefaultScannerOptions();
    void saveScannerOptionsToProfile(const QStringList &options, const QString &profile, bool ignoreSelection);
    void switchToProfile(const QString &profile, bool ignoreSelection);
    void getDeviceName();
    void getSelection();
    void setSelection(const QStringList &options);

    void reselectScannerDevice();
    void updateWindowTitle(const QString &deviceName, const QString &deviceVendor = QString(), const QString &deviceModel = QString());

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    KSaneIface::KSaneWidget *m_ksanew = nullptr;
    QProgressBar *m_saveProgressBar = nullptr;
    QPushButton *m_btnReselectDevice = nullptr;
    QUrl m_currentSaveUrl;
    QTimer m_saveUpdateTimer;
    Ui::SkanliteSettings m_settingsUi;
    QDialog *m_settingsDialog = nullptr;
    ShowImageDialog *m_showImgDialog = nullptr;
    SaveLocation *m_saveLocation = nullptr;
    QString m_deviceName;
    QString m_deviceVendor;
    QString m_deviceModel;
    QMap<QString, QString> m_defaultScanOpts;
    QMap<QString, QString> m_pendingApplyScanOpts;
    QImage m_img;

    DBusInterface m_dbusInterface;
    QStringList m_filterList;
    QStringList m_filter16BitList;
    bool m_firstImage;
};

#endif
