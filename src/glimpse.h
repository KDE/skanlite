/* ============================================================
 *
 * Copyright (C) 2007-2008 by Kare Sars <kare dot sars at iki dot fi>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This appication is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * ============================================================ */

#ifndef GLIMPSE_H
#define GLIMPSE_H

#include <libksane/ksane.h>

#include "ui_settings.h"

namespace KSaneIface
{
    class KSaneWidget;
}

class Glimpse : public KDialog
{
    Q_OBJECT

    public:
        explicit Glimpse(const QString& device, QWidget *parent = 0);

    private:
        void buildShowImage();
        void readSettings();

    private Q_SLOTS:
        void showSettingsDialog();
        void setDir();
        void imageReady(QByteArray &, int, int, int, int);
        void saveImage();
        void autoSaveImage();
        void showAboutDialog();


    private:
        KSaneIface::KSaneWidget *ksanew;
        Ui::GlimpseSettings      settingsUi;
        KDialog                 *settingsDialog;
        KDialog                 *showImgDialog;
        QImage                   img;
        QLabel                  *imgLabel;
        QStringList              filterList;
        QStringList              typeList;
        QString                  currentDir;

};

#endif

