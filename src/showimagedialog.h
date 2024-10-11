/* ============================================================
 *
 * SPDX-FileCopyrightText: 2007-2012 Kåre Särs <kare.sars@iki .fi>
 * SPDX-FileCopyrightText: 2009 Arseniy Lartsev <receive-spam at yandex dot ru>
 * SPDX-FileCopyrightText: 2014 Gregor Mitsch : port to KDE5 frameworks
 * SPDX-FileCopyrightText: 2018 Alexander Volkov <a.volkov@rusbitech.ru>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
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
