/* ============================================================
* Date        : 2010-07-02
* Description : Image saver class for libksane image data.
*
* Copyright (C) 2010-2012 by Kåre Särs <kare.sars@iki .fi>
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

#include "SkanliteImageSaver.h"

#include <QMutex>
#include <QDebug>

#include <KSaneWidget>
#include <QUrl>

struct SkanliteImageSaver::Private {
    bool   m_savedOk;
    QMutex m_runMutex;

    QUrl       m_url;
    QString    m_name;
    QImage     m_image;
    int        m_width;
    int        m_height;
    int        m_bpl;
    int        m_dpi;
    int        m_format;
    QString    m_fileFormat;
    int        m_quality;

    SkanliteImageSaver *q;

    bool saveQImage();
};

// ------------------------------------------------------------------------
SkanliteImageSaver::SkanliteImageSaver(QObject *parent) : QThread(parent), d(new Private)
{
    d->q = this;
}

// ------------------------------------------------------------------------
SkanliteImageSaver::~SkanliteImageSaver()
{
    delete d;
}

bool SkanliteImageSaver::saveQImage(const QUrl &url, const QString &name, const QImage &image, int dpi, const QString& fileFormat, int quality)
{
    if (!d->m_runMutex.tryLock()) {
        return false;
    }

    d->m_url    = url;
    d->m_name   = name;
    d->m_image  = image;
    d->m_dpi    = dpi;
    d->m_fileFormat = fileFormat;
    d->m_quality = quality;

    start();
    return true;
}

void SkanliteImageSaver::run()
{
    d->m_savedOk = d->saveQImage();
    Q_EMIT imageSaved(d->m_url, d->m_name, d->m_savedOk);

    d->m_runMutex.unlock();
}

bool SkanliteImageSaver::Private::saveQImage()
{
    return m_image.save(m_name, qPrintable(m_fileFormat), m_quality);
}

