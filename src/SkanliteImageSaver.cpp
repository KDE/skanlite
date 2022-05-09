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
#include <QUrl>
#include <QImage>
#include <QPdfWriter>
#include <QPainter>

struct SkanliteImageSaver::Private {
    bool   m_savedOk;
    QMutex m_runMutex;

    QUrl       m_url;
    QString    m_name;
    QImage     m_image;
    QString    m_fileFormat;
    int        m_quality;

    SkanliteImageSaver *q;

    bool saveQImage();
    bool savePDF();
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
    if (d->m_fileFormat == QLatin1String("pdf")) {
        d->m_savedOk = d->savePDF();
    } else {
        d->m_savedOk = d->saveQImage();
    }

    Q_EMIT imageSaved(d->m_url, d->m_name, d->m_savedOk);

    d->m_runMutex.unlock();
}

bool SkanliteImageSaver::Private::saveQImage()
{
    return m_image.save(m_name, qPrintable(m_fileFormat), m_quality);
}

bool SkanliteImageSaver::Private::savePDF()
{
    const QPageSize pageSize = QPageSize(QSizeF(m_image.width() * 1000.0 / m_image.dotsPerMeterX() , m_image.height() * 1000.0 / m_image.dotsPerMeterY()), QPageSize::Millimeter);
    const int dpi = qRound(m_image.dotsPerMeterX() / 1000.0 * 25.4);

    QPainter painter;
    QPdfWriter writer(m_name);
    writer.setPageOrientation(QPageLayout::Portrait);
    writer.setResolution(dpi);
    writer.setPageSize(pageSize);
    writer.setPageMargins(QMarginsF(0, 0, 0, 0));
    writer.setCreator(QStringLiteral("Skanlite"));
    writer.setTitle(m_name);

    QRect paintArea(0, 0, writer.width(), writer.height());
    if (!painter.begin(&writer)) {
        return false;
    }

    painter.drawImage(paintArea, m_image);

    return painter.end();
}
