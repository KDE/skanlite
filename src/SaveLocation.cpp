/* ============================================================
* Date        : 2010-07-07
* Description : Save location settings dialog.
*
* Copyright (C) 2010-2012 by Kare Sars <kare.sars@iki.fi>
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

#include "SaveLocation.h"

#include <QDebug>
#include <QFileDialog>
#include <QPushButton>
#include <QComboBox>

SaveLocation::SaveLocation(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);

    u_urlRequester->setMode(KFile::Directory);
    connect(u_urlRequester, &KUrlRequester::textChanged, this, &SaveLocation::updateGui);    
    connect(u_imgPrefix, &QLineEdit::textChanged, this, &SaveLocation::updateGui);
    connect(u_imgFormat, static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::activated), this, &SaveLocation::updateGui);
    connect(u_numStartFrom, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &SaveLocation::updateGui);
}

SaveLocation::~SaveLocation()
{
}

void SaveLocation::updateGui()
{
    if (sender() != u_numStartFrom) {
        u_numStartFrom->setValue(1); // Reset the counter whenever the directory or the prefix is changed
    }
    const QString name = QString::fromLatin1("%1%2.%3").arg(u_imgPrefix->text()).arg(u_numStartFrom->value(), 4, 10, QLatin1Char('0')).arg(u_imgFormat->currentText());
    QString dir = u_urlRequester->url().toString();
    if (!dir.endsWith(QDir::separator())) dir = dir.append(QDir::separator()); //make sure whole value is processed as path to directory
    u_resultValue->setText(QUrl(dir).resolved(QUrl(name)).toString(QUrl::PreferLocalFile | QUrl::NormalizePathSegments));
}

void SaveLocation::getDir(void)
{
    const QString newDir = QFileDialog::getExistingDirectory(this, QString(), u_urlRequester->url().toLocalFile());
    if (!newDir.isEmpty()) {
        u_urlRequester->setUrl(QUrl::fromLocalFile(newDir));
    }
}
