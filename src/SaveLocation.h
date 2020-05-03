/* ============================================================
 * Date        : 2010-07-07
 * Description : Save location settings dialog.
 *
 * Copyright (C) 2008-2012 by Kåre Särs <kare.sars@iki .fi>
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
    QString imageFormat() const;
    int startNumber() const;
    int startNumberMax() const;

    void setFolderUrl(const QUrl &url);
    void setImagePrefix(const QString &prefix);
    void setImageFormat(const QString &format);
    void setStartNumber(int number);

    void setImageFormats(const QStringList &formats);

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

