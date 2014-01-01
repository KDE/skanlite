/* ============================================================
* Date        : 2010-07-07
* Description : Save location settings dialog.
*
* Copyright (C) 2010-2012 by Kare Sars <kare.sars@iki.fi>
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
#include "SaveLocation.moc"

#include <KDebug>
#include <KFileDialog>

SaveLocation::SaveLocation(QWidget *parent) : KDialog(parent)
{
    QWidget *container = new QWidget(this);
    setupUi(container);
    setMainWidget(container);
    connect(u_saveDirLEdit, SIGNAL(textChanged(QString)), this, SLOT(update()));
    connect(u_imgPrefix,    SIGNAL(textChanged(QString)), this, SLOT(update()));
    connect(u_imgFormat,    SIGNAL(activated(QString)),   this, SLOT(update()));
    connect(u_numStartFrom, SIGNAL(valueChanged(int)),    this, SLOT(update()));
    connect(u_getDirButton, SIGNAL(clicked()),            this, SLOT(getDir()));
}

// ------------------------------------------------------------------------
SaveLocation::~SaveLocation()
{
}

// ------------------------------------------------------------------------
void SaveLocation::update()
{
    if (sender() != u_numStartFrom) {
        u_numStartFrom->setValue(1); // Reset the counter whenever the directory or the prefix is changed
    }
    QString name = QString("%1%2.%3").arg(u_imgPrefix->text()).arg(u_numStartFrom->value(), 4, 10, QLatin1Char('0')).arg(u_imgFormat->currentText());
    u_resultValue->setText(QFileInfo(u_saveDirLEdit->text(), name).absoluteFilePath());
}

// ------------------------------------------------------------------------
void SaveLocation::getDir(void)
{
    QString newDir = KFileDialog::getExistingDirectory(u_saveDirLEdit->text());
    if (!newDir.isEmpty()) {
        u_saveDirLEdit->setText(newDir);
    }
}
