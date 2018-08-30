/* ============================================================
*
* Copyright (C) 2007-2012 by Kåre Särs <kare.sars@iki .fi>
* Copyright (C) 2009 by Arseniy Lartsev <receive-spam at yandex dot ru>
* Copyright (C) 2014 by Gregor Mitsch: port to KDE5 frameworks
* Copyright (C) 2018 by Alexander Volkov <a.volkov@rusbitech.ru>
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

#ifndef SHOWIMAGEDIALOG_H
#define SHOWIMAGEDIALOG_H

#include <QDialog>

class ImageViewer;

class ShowImageDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ShowImageDialog(QWidget *parent = nullptr);

    void setQImage(QImage *img);

public Q_SLOTS:
    void zoom2Fit();

Q_SIGNALS:
    void saveRequested();

protected:
    void showEvent(QShowEvent *e) override;

private:
    QPushButton *m_saveButton = nullptr;
    ImageViewer *m_imageViewer = nullptr;
};

#endif // SHOWIMAGEDIALOG_H
