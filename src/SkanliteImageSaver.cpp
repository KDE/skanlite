/* ============================================================
* Date        : 2010-07-02
* Description : Image saver class for libksane image data.
*
* SPDX-FileCopyrightText: 2010-2012 Kåre Särs <kare.sars@iki .fi>
*
* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
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
    int        m_dpi;
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

bool SkanliteImageSaver::saveQImage(const QUrl &url, const QString &name, const QImage &image, const QString& fileFormat, int quality)
{
    if (!d->m_runMutex.tryLock()) {
        return false;
    }

    d->m_url    = url;
    d->m_name   = name;
    d->m_image  = image;
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

