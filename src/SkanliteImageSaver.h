/* ============================================================
* Date        : 2010-07-02
* Description : Image saver class for libksane image data.
*
* SPDX-FileCopyrightText: 2010-2012 Kåre Särs <kare.sars@iki .fi>
* SPDX-FileCopyrightText: 2014 Gregor Mitsch : port to KDE5 frameworks
*
* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*
* ============================================================ */

#ifndef SkanliteImageSaver_h
#define SkanliteImageSaver_h

#include <QByteArray>
#include <QThread>
#include <QString>

class SkanliteImageSaver : public QThread
{
    Q_OBJECT
public:
    explicit SkanliteImageSaver(QObject *parent = nullptr);
    ~SkanliteImageSaver() override;

    bool saveQImage(const QUrl &url, const QString &name, const QImage &image, const QString& fileFormat, int quality);
Q_SIGNALS:
    void imageSaved(const QUrl &url, const QString &name, bool success);

protected:
    void run() Q_DECL_OVERRIDE;

private:
    struct Private;
    Private *const d;
};

#endif

