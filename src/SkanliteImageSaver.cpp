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

#include <png.h>

#include <QMutex>
#include <QDebug>

#include <KSaneWidget>
#include <QUrl>

struct SkanliteImageSaver::Private {
    bool   m_savedOk;
    QMutex m_runMutex;

    QUrl       m_url;
    QString    m_name;
    QByteArray m_data;
    int        m_width;
    int        m_height;
    int        m_bpl;
    int        m_dpi;
    int        m_format;
    QString    m_fileFormat;
    int        m_quality;
    bool       m_savingAsPng16;

    SkanliteImageSaver *q;

    bool saveQImage();
    bool save16BitPng();
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

bool SkanliteImageSaver::saveQImage(const QUrl &url, const QString &name, const QByteArray &data, int width, int height, int bpl, int dpi, int format, const QString& fileFormat, int quality)
{
    if (!d->m_runMutex.tryLock()) {
        return false;
    }

    d->m_url    = url;
    d->m_name   = name;
    d->m_data   = data;
    d->m_width  = width;
    d->m_height = height;
    d->m_bpl    = bpl;
    d->m_dpi    = dpi;
    d->m_format = format;
    d->m_fileFormat = fileFormat;
    d->m_quality = quality;
    d->m_savingAsPng16 = false;

    start();
    return true;
}

bool SkanliteImageSaver::save16BitPng(const QUrl &url, const QString &name, const QByteArray &data, int width, int height, int bpl, int dpi, int format, const QString& fileFormat, int quality)
{
    if (!d->m_runMutex.tryLock()) {
        return false;
    }

    d->m_url    = url;
    d->m_name   = name;
    d->m_data   = data;
    d->m_width  = width;
    d->m_height = height;
    d->m_bpl    = bpl;
    d->m_dpi    = dpi;
    d->m_format = format;
    d->m_fileFormat = fileFormat;
    d->m_quality = quality;
    d->m_savingAsPng16 = true;

    start();
    return true;
}

void SkanliteImageSaver::run()
{
    d->m_savedOk = d->m_savingAsPng16 ? d->save16BitPng() : d->saveQImage();
    emit imageSaved(d->m_url, d->m_name, d->m_savedOk);

    d->m_runMutex.unlock();
}

bool SkanliteImageSaver::Private::saveQImage()
{
    QImage img = KSaneIface::KSaneWidget::toQImageSilent(m_data, m_width, m_height, m_bpl, m_dpi, (KSaneIface::KSaneWidget::ImageFormat) m_format);
    return img.save(m_name, qPrintable(m_fileFormat), m_quality);
}

bool SkanliteImageSaver::Private::save16BitPng()
{
    FILE        *file;
    png_structp  png_ptr;
    png_infop    info_ptr;
    png_color_8  sig_bit;
    png_bytep    row_ptr;
    int          bytesPerPixel;

    // open the file
    file = fopen(qPrintable(m_name), "wb");
    if (!file) {
        return false;
    }

    // create the png struct
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png_ptr) {
        fclose(file);
        return false;
    }

    // create the image information srtuct
    info_ptr = png_create_info_struct(png_ptr);
    if (!png_ptr) {
        fclose(file);
        png_destroy_write_struct(&png_ptr, (png_infopp)nullptr);
        return false;
    }

    // initialize IO
    png_init_io(png_ptr, file);

    // set the image attributes
    switch ((KSaneIface::KSaneWidget::ImageFormat)m_format) {
    case KSaneIface::KSaneWidget::FormatGrayScale16:
        png_set_IHDR(png_ptr, info_ptr, m_width, m_height, 16, PNG_COLOR_TYPE_GRAY,
                     PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
        sig_bit.gray = 16;
        bytesPerPixel = 2;
        break;
    case KSaneIface::KSaneWidget::FormatRGB_16_C:
        png_set_IHDR(png_ptr, info_ptr, m_width, m_height, 16, PNG_COLOR_TYPE_RGB,
                     PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
        sig_bit.red = 16;
        sig_bit.green = 16;
        sig_bit.blue = 16;
        bytesPerPixel = 6;
        break;
    default:
        fclose(file);
        png_destroy_write_struct(&png_ptr, (png_infopp)nullptr);
        return false;
    }

    png_set_sBIT(png_ptr, info_ptr, &sig_bit);

    png_uint_32 dpm = m_dpi * (1000.0 / 25.4);
    png_set_pHYs(png_ptr, info_ptr, dpm, dpm, 1);

    /* Optionally write comments into the image */
//     text_ptr[0].key = "Title";
//     text_ptr[0].text = "Mona Lisa";
//     text_ptr[0].compression = PNG_TEXT_COMPRESSION_NONE;
//     text_ptr[1].key = "Author";
//     text_ptr[1].text = "Leonardo DaVinci";
//     text_ptr[1].compression = PNG_TEXT_COMPRESSION_NONE;
//     text_ptr[2].key = "Description";
//     text_ptr[2].text = "<long text>";
//     text_ptr[2].compression = PNG_TEXT_COMPRESSION_zTXt;
//     png_set_text(png_ptr, info_ptr, text_ptr, 2);

    /* Write the file header information. */
    png_write_info(png_ptr, info_ptr);

    //png_set_shift(png_ptr, &sig_bit);
    //png_set_packing(png_ptr);

    png_set_compression_level(png_ptr, 9);

    // This is not nice :( swap bytes in the 16 bit color....
    // Make sure we have a buffer size that is divisible by 2
    int dataSize = m_data.size();
    if ((dataSize % 2) > 0) {
        dataSize--;
    }
    for (int i = 0; i < dataSize; i += 2) {
        char tmp = m_data[i];
        m_data[i] = m_data[i + 1];
        m_data[i + 1] = tmp;
    }

    row_ptr = (png_bytep)m_data.data();

    for (int i = 0; i < m_height; i++) {
        png_write_rows(png_ptr, &row_ptr, 1);
        row_ptr += m_width * bytesPerPixel;
    }

    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, (png_infopp) & info_ptr);
    png_destroy_info_struct(png_ptr, (png_infopp) & info_ptr);
    fclose(file);

    return true;
}

