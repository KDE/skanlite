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

#ifndef Skanlite_h
#define Skanlite_h

#include <QUrl>
#include <QDialog>
#include <QTimer>

#include <KSaneWidget>

#include "ui_settings.h"
#include "DBusInterface.h"

class ShowImageDialog;
class SaveLocation;
class QProgressBar;

using namespace KSaneIface;

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
    void showAboutDialog();
    void saveWindowSize();

    void saveScannerOptions();
    void defaultScannerOptions();
    void applyScannerOptions(const QMap<QString, QString> &opts);

    void availableDevices(const QList<KSaneWidget::DeviceInfo> &deviceList);

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

protected:
    void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;

private:
    KSaneWidget             *m_ksanew = nullptr;
    QProgressBar            *m_saveProgressBar = nullptr;
    QUrl                     m_currentSaveUrl;
    QTimer                   m_saveUpdateTimer;
    Ui::SkanliteSettings     m_settingsUi;
    QDialog                 *m_settingsDialog = nullptr;
    ShowImageDialog         *m_showImgDialog = nullptr;
    SaveLocation            *m_saveLocation = nullptr;
    QString                  m_deviceName;
    QMap<QString, QString>   m_defaultScanOpts;
    QMap<QString, QString>   m_pendingApplyScanOpts;
    QImage                   m_img;

    DBusInterface            m_dbusInterface;
    QStringList              m_filterList;
    QStringList              m_filter16BitList;
    QStringList              m_typeList;
    bool                     m_firstImage;
};

#endif

