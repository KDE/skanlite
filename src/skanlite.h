/* ============================================================
 *
 * Copyright (C) 2007-2012 by Kåre Särs <kare.sars@iki .fi>
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

#include <libksane/ksane.h>
#include <QDir>

#include "ui_settings.h"
#include "ImageViewer.h"

class KFileDialog;
class SaveLocation;

using namespace KSaneIface;

class Skanlite : public KDialog
{
    Q_OBJECT

    public:
        explicit Skanlite(const QString& device, QWidget *parent = 0);

    private:
        // Order of items in save mode combo-box
        enum SaveMode {
            SaveModeManual = 0,
            SaveModeAskFirst = 1,
        };

        void readSettings();
        void doSaveImage(bool askFilename = true);
        void loadScannerOptions();

    private Q_SLOTS:
        void showSettingsDialog();
        void getDir();
        void imageReady(QByteArray &, int, int, int, int);
        void saveImage();
        void showAboutDialog();
        void saveWindowSize();

        void saveScannerOptions();
        void defaultScannerOptions();

        void availableDevices(const QList<KSaneWidget::DeviceInfo> &deviceList);

        void alertUser(int type, const QString &strStatus);
        void buttonPressed(const QString &optionName, const QString &optionLabel, bool pressed);

    protected:
        void closeEvent(QCloseEvent *event);

    private:
        KSaneWidget             *m_ksanew;
        Ui::SkanliteSettings     m_settingsUi;
        KDialog                 *m_settingsDialog;
        KDialog                 *m_showImgDialog;
        KFileDialog             *m_saveDialog;
        SaveLocation            *m_saveLocation;
        QString                  m_deviceName;
        QMap<QString,QString>    m_defaultScanOpts;
        QImage                   m_img;
        QByteArray               m_data;
        int                      m_width;
        int                      m_height;
        int                      m_bytesPerLine;
        int                      m_format;

        ImageViewer              m_imageViewer;
        QStringList              m_filterList;
        QStringList              m_filter16BitList;
        QStringList              m_typeList;
        bool                     m_firstImage;
};


#endif

