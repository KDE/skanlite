/* ============================================================
* Date        : 2010-07-02
* Description : Image saver class for libksane image data.
*
* Copyright (C) 2010-2012 by Kåre Särs <kare.sars@iki .fi>
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

#ifndef KSaneImageSaver_h
#define KSaneImageSaver_h

#include <QByteArray>
#include <QThread>
#include <QString>

class KSaneImageSaver : public QThread
{
    Q_OBJECT
public:
    explicit KSaneImageSaver(QObject *parent = nullptr);
    ~KSaneImageSaver();

    bool saveQImage(const QUrl &url, const QString &name, const QByteArray &data, int width, int height, int bpl, int dpi, int format, const QString& fileFormat, int quality);
    bool save16BitPng(const QUrl &url, const QString &name, const QByteArray &data, int width, int height, int bpl, int dpi, int format, const QString& fileFormat, int quality);

    bool savePdf(const QUrl &url, const QString &name, const QByteArray &data, int width, int height, int bpl, int dpi, int format, const QString& fileFormat, int quality);
Q_SIGNALS:
    void imageSaved(const QUrl &url, const QString &name, bool success);

protected:
    void run() Q_DECL_OVERRIDE;

private:
    struct Private;
    Private *const d;
};

#endif

