/* ============================================================
 * Date        : 2010-07-07
 * Description : Save location settings dialog.
 *
 * SPDX-FileCopyrightText: 2008-2012 Kåre Särs <kare.sars@iki .fi>
 * SPDX-FileCopyrightText: 2014 Gregor Mitsch : port to KDE5 frameworks
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 *
 * ============================================================ */

#ifndef SAVE_LOCATION_H
#define SAVE_LOCATION_H

#include <QDialog>

class Ui_SaveLocation;
class QShowEvent;

class SaveLocation : public QDialog
{
    Q_OBJECT
public:
    explicit SaveLocation(QWidget *parent = nullptr);
    ~SaveLocation();

    QUrl folderUrl() const;
    QString imagePrefix() const;
    QString imageMimetype() const;
    QString imageSuffix() const;
    int startNumber() const;
    int startNumberMax() const;

    void setFolderUrl(const QUrl &url);
    void setImagePrefix(const QString &prefix);
    void setImageFormat(const QString &suffix);
    void setImageFormatIndex(int index);
    void addImageFormat(const QString &type, const QString &mimetype);
    void setStartNumber(int number);

    void setOpenRequesterOnShow(bool open);

protected:
    void showEvent(QShowEvent *event) override;

private Q_SLOTS:
    void updateGui();

private:
    Ui_SaveLocation *m_ui = nullptr;
    bool m_openRequesterOnShow = false;
};

#endif

