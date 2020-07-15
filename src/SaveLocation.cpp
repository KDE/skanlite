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
#include "ui_SaveLocation.h"

#include <QFileDialog>
#include <QPushButton>
#include <QComboBox>
#include <QShowEvent>
#include <QTimer>

SaveLocation::SaveLocation(QWidget *parent)
    : QDialog(parent)
{
    m_ui = new Ui_SaveLocation();
    m_ui->setupUi(this);

    m_ui->u_urlRequester->setMode(KFile::Directory);
    connect(m_ui->u_urlRequester, &KUrlRequester::textChanged, this, &SaveLocation::updateGui);
    connect(m_ui->u_imgPrefix, &QLineEdit::textChanged, this, &SaveLocation::updateGui);
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    connect(m_ui->u_imgFormat, static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::activated), this, &SaveLocation::updateGui);
#else
    connect(m_ui->u_imgFormat, static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::textActivated), this, &SaveLocation::updateGui);
#endif
    connect(m_ui->u_numStartFrom, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &SaveLocation::updateGui);
}

SaveLocation::~SaveLocation()
{
}

void SaveLocation::updateGui()
{
    if (sender() != m_ui->u_numStartFrom) {
        m_ui->u_numStartFrom->setValue(1); // Reset the counter whenever the directory or the prefix is changed
    }
    const QString name = QString::fromLatin1("%1%2.%3")
    .arg(m_ui->u_imgPrefix->text())
    .arg(m_ui->u_numStartFrom->value(), 4, 10, QLatin1Char('0'))
    .arg(m_ui->u_imgFormat->currentText());

    QUrl folderUrl = m_ui->u_urlRequester->url();
    folderUrl.setPath(folderUrl.path() + QLatin1Char('/'));
    m_ui->u_resultValue->setText(folderUrl.toString(QUrl::PreferLocalFile | QUrl::NormalizePathSegments)+name);
}

QUrl SaveLocation::folderUrl() const
{
    QUrl folderUrl = m_ui->u_urlRequester->url();
    folderUrl.setPath(folderUrl.path() + QLatin1Char('/'));
    return folderUrl.adjusted(QUrl::NormalizePathSegments);
}

QString SaveLocation::imagePrefix() const { return m_ui->u_imgPrefix->text(); }
QString SaveLocation::imageFormat() const { return m_ui->u_imgFormat->currentText().toLower(); }
int     SaveLocation::startNumber() const { return m_ui->u_numStartFrom->value(); }
int     SaveLocation::startNumberMax() const { return m_ui->u_numStartFrom->maximum(); }


void SaveLocation::setFolderUrl(const QUrl &url)               { m_ui->u_urlRequester->setUrl(url); }
void SaveLocation::setImagePrefix(const QString &prefix)       { m_ui->u_imgPrefix->setText(prefix); }
void SaveLocation::setImageFormats(const QStringList &formats) { m_ui->u_imgFormat->addItems(formats); }
void SaveLocation::setImageFormat(const QString &format)       { m_ui->u_imgFormat->setCurrentText(format); }
void SaveLocation::setStartNumber(int number)                  { m_ui->u_numStartFrom->setValue(number); }

void SaveLocation::setOpenRequesterOnShow(bool open)
{
    m_openRequesterOnShow = open;
}

void SaveLocation::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);

    if (m_openRequesterOnShow) {
        QTimer::singleShot(0, this, [this]() { m_ui->u_urlRequester->button()->click(); });
    }
}
