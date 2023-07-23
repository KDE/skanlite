/* ============================================================
* Date        : 2010-07-07
* Description : Save location settings dialog.
*
* SPDX-FileCopyrightText: 2010-2012 Kare Sars <kare.sars@iki.fi>
* SPDX-FileCopyrightText: 2014 Gregor Mitsch : port to KDE5 frameworks
*
* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
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
    connect(m_ui->u_imgFormat, static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::textActivated), this, &SaveLocation::updateGui);
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
    const QString name = QStringLiteral("%1%2.%3")
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

QString SaveLocation::imagePrefix() const
{
    return m_ui->u_imgPrefix->text();
}

QString SaveLocation::imageMimetype() const
{
    return m_ui->u_imgFormat->currentData().toString();
}

QString SaveLocation::imageSuffix() const
{
    return m_ui->u_imgFormat->currentText().toLower();
}

int SaveLocation::startNumber() const
{
    return m_ui->u_numStartFrom->value();
}

int SaveLocation::startNumberMax() const
{
    return m_ui->u_numStartFrom->maximum();
}

void SaveLocation::setFolderUrl(const QUrl &url)
{
    m_ui->u_urlRequester->setUrl(url);
}

void SaveLocation::setImagePrefix(const QString &prefix)
{
    m_ui->u_imgPrefix->setText(prefix);
}

void SaveLocation::addImageFormat(const QString &suffix, const QString &mimetype)
{
    m_ui->u_imgFormat->addItem(suffix, mimetype);
}

void SaveLocation::setImageFormatIndex(int index)
{
    m_ui->u_imgFormat->setCurrentIndex(index);
}

void SaveLocation::setImageFormat(const QString &suffix)
{
    int index = m_ui->u_imgFormat->findText(suffix);
    if (index >= 0) {
        m_ui->u_imgFormat->setCurrentIndex(index);
    }
}

void SaveLocation::setStartNumber(int number)
{
    m_ui->u_numStartFrom->setValue(number);
}

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

#include "moc_SaveLocation.cpp"
