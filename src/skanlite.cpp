/* ============================================================
*
* Copyright (C) 2007-2012 by Kåre Särs <kare.sars@iki .fi>
* Copyright (C) 2009 by Arseniy Lartsev <receive-spam at yandex dot ru>
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

#include "skanlite.h"

#include "KSaneImageSaver.h"
#include "SaveLocation.h"

#include <QApplication>
#include <QScrollArea>
#include <QStringList>
#include <QFileDialog>
#include <QUrl>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QMessageBox>
#include <QTemporaryFile>
#include <QDebug>
#include <QImageWriter>
#include <QMimeType>
#include <QMimeDatabase>

#include <KAboutApplicationDialog>
#include <KLocalizedString>
#include <KMessageBox>
#include <KIO/StatJob>
#include <KIO/Job>
#include <KJobWidgets>
#include <kio/global.h>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KHelpClient>

#include <errno.h>

Skanlite::Skanlite(const QString &device, QWidget *parent)
    : QDialog(parent)
    , m_aboutData(nullptr)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QDialogButtonBox *dlgButtonBoxBottom = new QDialogButtonBox(this);
    dlgButtonBoxBottom->setStandardButtons(QDialogButtonBox::Help | QDialogButtonBox::Close);
    // was "User2:
    QPushButton *btnAbout = dlgButtonBoxBottom->addButton(i18n("About"), QDialogButtonBox::ButtonRole::ActionRole);
    // was "User1":
    QPushButton *btnSettings = dlgButtonBoxBottom->addButton(i18n("Settings"), QDialogButtonBox::ButtonRole::ActionRole);
    btnSettings->setIcon(QIcon::fromTheme(QLatin1String("configure")));

    m_firstImage = true;

    m_ksanew = new KSaneIface::KSaneWidget(this);
    connect(m_ksanew, &KSaneWidget::imageReady, this, &Skanlite::imageReady);
    connect(m_ksanew, &KSaneWidget::availableDevices, this, &Skanlite::availableDevices);
    connect(m_ksanew, &KSaneWidget::userMessage, this, &Skanlite::alertUser);
    connect(m_ksanew, &KSaneWidget::buttonPressed, this, &Skanlite::buttonPressed);

    mainLayout->addWidget(m_ksanew);
    mainLayout->addWidget(dlgButtonBoxBottom);

    m_ksanew->initGetDeviceList();

    // read the size here...
    KConfigGroup window(KSharedConfig::openConfig(), "Window");
    QSize rect = window.readEntry("Geometry", QSize(740, 400));
    resize(rect);

    connect(dlgButtonBoxBottom, &QDialogButtonBox::rejected, this, &QDialog::close);
    connect(this, &QDialog::finished, this, &Skanlite::saveWindowSize);
    connect(this, &QDialog::finished, this, &Skanlite::saveScannerOptions);
    connect(btnSettings, &QPushButton::clicked, this, &Skanlite::showSettingsDialog);
    connect(btnAbout, &QPushButton::clicked, this, &Skanlite::showAboutDialog);
    connect(dlgButtonBoxBottom, &QDialogButtonBox::helpRequested, this, &Skanlite::showHelp);

    //
    // Create the settings dialog
    //
    {
        m_settingsDialog = new QDialog(this);

        QVBoxLayout *mainLayout = new QVBoxLayout(m_settingsDialog);

        QWidget *settingsWidget = new QWidget(m_settingsDialog);
        m_settingsUi.setupUi(settingsWidget);
        m_settingsUi.revertOptions->setIcon(QIcon::fromTheme(QLatin1String("edit-undo")));
        m_saveLocation = new SaveLocation(this);

        // add the supported image types
        const QList<QByteArray> tmpList = QImageWriter::supportedMimeTypes();
        m_filterList.clear();
        foreach (auto ba, tmpList) {
            m_filterList.append(QString::fromLatin1(ba));
        }

        qDebug() << m_filterList;

        // Put first class citizens at first place
        m_filterList.removeAll(QLatin1String("image/jpeg"));
        m_filterList.removeAll(QLatin1String("image/tiff"));
        m_filterList.removeAll(QLatin1String("image/png"));
        m_filterList.insert(0, QLatin1String("image/png"));
        m_filterList.insert(1, QLatin1String("image/jpeg"));
        m_filterList.insert(2, QLatin1String("image/tiff"));

        m_filter16BitList << QLatin1String("image/png");
        //m_filter16BitList << QLatin1String("image/tiff");

        // fill m_filterList (...) and m_typeList (list of file suffixes)
        foreach (QString mimeStr, m_filterList) {
            QMimeType mimeType = QMimeDatabase().mimeTypeForName(mimeStr);
            m_filterList.append(mimeType.name());

            QStringList fileSuffixes = mimeType.suffixes();

            if (fileSuffixes.size() > 0) {
                m_typeList << fileSuffixes.first();
            }
        }

        m_settingsUi.imgFormat->addItems(m_typeList);
        m_saveLocation->u_imgFormat->addItems(m_typeList);

        mainLayout->addWidget(settingsWidget);

        QDialogButtonBox *dlgButtonBoxBottom = new QDialogButtonBox(this);
        dlgButtonBoxBottom->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Close);
        connect(dlgButtonBoxBottom, &QDialogButtonBox::accepted, m_settingsDialog, &QDialog::accept);
        connect(dlgButtonBoxBottom, &QDialogButtonBox::rejected, m_settingsDialog, &QDialog::reject);

        mainLayout->addWidget(dlgButtonBoxBottom);

        m_settingsDialog->setWindowTitle(i18n("Skanlite Settings"));

        connect(m_settingsUi.getDirButton, &QPushButton::clicked, this, &Skanlite::getDir);
        connect(m_settingsUi.revertOptions, &QPushButton::clicked, this, &Skanlite::defaultScannerOptions);
        readSettings();

        // default directory for the save dialog
        m_saveLocation->u_urlRequester->setUrl(QUrl::fromUserInput(m_settingsUi.saveDirLEdit->text()));
        m_saveLocation->u_imgPrefix->setText(m_settingsUi.imgPrefix->text());
        m_saveLocation->u_imgFormat->setCurrentText(m_settingsUi.imgFormat->currentText());
    }

    // open the scan device
    if (m_ksanew->openDevice(device) == false) {
        QString dev = m_ksanew->selectDevice(0);
        if (dev.isEmpty()) {
            // either no scanner was found or then cancel was pressed.
            exit(0);
        }
        if (m_ksanew->openDevice(dev) == false) {
            // could not open a scanner
            KMessageBox::sorry(0, i18n("Opening the selected scanner failed."));
            exit(1);
        }
        else {
            setWindowTitle(i18nc("@title:window %1 = scanner maker, %2 = scanner model", "%1 %2 - Skanlite", m_ksanew->make(), m_ksanew->model()));
            m_deviceName = QString::fromLatin1("%1:%2").arg(m_ksanew->make()).arg(m_ksanew->model());
        }
    }
    else {
        setWindowTitle(i18nc("@title:window %1 = scanner device", "%1 - Skanlite", device));
        m_deviceName = device;
    }

    // prepare the Show Image Dialog
    {
        m_showImgDialog = new QDialog(this);

        QVBoxLayout *mainLayout = new QVBoxLayout(m_showImgDialog);

        QDialogButtonBox *imgButtonBox = new QDialogButtonBox(m_showImgDialog);
        // "Close" (now Discard) and "User1"=Save
        imgButtonBox->setStandardButtons(QDialogButtonBox::Discard | QDialogButtonBox::Save);

        mainLayout->addWidget(&m_imageViewer);
        mainLayout->addWidget(imgButtonBox);

        m_showImgDialogSaveButton = imgButtonBox->button(QDialogButtonBox::Save);
        m_showImgDialogSaveButton->setDefault(true); // still needed?

        m_showImgDialog->resize(640, 480);
        connect(imgButtonBox, &QDialogButtonBox::accepted, this, &Skanlite::saveImage);
        //connect(imgButtonBox, &QDialogButtonBox::accepted, m_showImgDialog, &QDialog::accept);
        connect(imgButtonBox->button(QDialogButtonBox::Discard), &QPushButton::clicked, m_ksanew, &KSaneWidget::scanCancel);
        connect(imgButtonBox->button(QDialogButtonBox::Discard), &QPushButton::clicked, m_showImgDialog, &QDialog::reject);
    }

    // save the default sane options for later use
    m_ksanew->getOptVals(m_defaultScanOpts);

    // load saved options
    loadScannerOptions();

    m_ksanew->initGetDeviceList();

    m_firstImage = true;
}

void Skanlite::showHelp()
{
    KHelpClient::invokeHelp(QLatin1String("index"), QLatin1String("skanlite"));
}

void Skanlite::setAboutData(KAboutData *aboutData)
{
    m_aboutData = aboutData;
}

void Skanlite::closeEvent(QCloseEvent *event)
{
    saveWindowSize();
    saveScannerOptions();
    event->accept();
}

void Skanlite::saveWindowSize()
{
    KConfigGroup window(KSharedConfig::openConfig(), "Window");
    window.writeEntry("Geometry", size());
    window.sync();
}

// Pops up message box similar to what perror() would print
//************************************************************
static void perrorMessageBox(const QString &text)
{
    if (errno != 0) {
        KMessageBox::sorry(0, i18n("%1: %2", text, QString::fromLocal8Bit(strerror(errno))));
    } else {
        KMessageBox::sorry(0, text);
    }
}

void Skanlite::readSettings(void)
{
    // enable the widgets to allow modifying
    m_settingsUi.setQuality->setChecked(true);
    m_settingsUi.setPreviewDPI->setChecked(true);

    // read the saved parameters
    KConfigGroup saving(KSharedConfig::openConfig(), "Image Saving");
    m_settingsUi.saveModeCB->setCurrentIndex(saving.readEntry("SaveMode", (int)SaveModeManual));
    if (m_settingsUi.saveModeCB->currentIndex() != SaveModeAskFirst) {
        m_firstImage = false;
    }
    m_settingsUi.saveDirLEdit->setText(saving.readEntry("Location", QDir::homePath()));
    m_settingsUi.imgPrefix->setText(saving.readEntry("NamePrefix", i18nc("prefix for auto naming", "Image-")));
    m_settingsUi.imgFormat->setCurrentText(saving.readEntry("ImgFormat", "png"));
    m_settingsUi.imgQuality->setValue(saving.readEntry("ImgQuality", 90));
    m_settingsUi.setQuality->setChecked(saving.readEntry("SetQuality", false));
    m_settingsUi.showB4Save->setChecked(saving.readEntry("ShowBeforeSave", true));

    KConfigGroup general(KSharedConfig::openConfig(), "General");

    //m_settingsUi.previewDPI->setCurrentItem(general.readEntry("PreviewDPI", "100"), true); // FIXME KF5 is the 'true' parameter still needed?
    m_settingsUi.previewDPI->setCurrentText(general.readEntry("PreviewDPI", "100"));

    m_settingsUi.setPreviewDPI->setChecked(general.readEntry("SetPreviewDPI", false));
    if (m_settingsUi.setPreviewDPI->isChecked()) {
        m_ksanew->setPreviewResolution(m_settingsUi.previewDPI->currentText().toFloat());
    } else {
        m_ksanew->setPreviewResolution(0.0);
    }
    m_settingsUi.u_disableSelections->setChecked(general.readEntry("DisableAutoSelection", false));
    m_ksanew->enableAutoSelect(!m_settingsUi.u_disableSelections->isChecked());
}

void Skanlite::showSettingsDialog(void)
{
    readSettings();

    // show the dialog
    if (m_settingsDialog->exec()) {
        // save the settings
        KConfigGroup saving(KSharedConfig::openConfig(), "Image Saving");
        saving.writeEntry("SaveMode", m_settingsUi.saveModeCB->currentIndex());
        saving.writeEntry("Location", m_settingsUi.saveDirLEdit->text());
        saving.writeEntry("NamePrefix", m_settingsUi.imgPrefix->text());
        saving.writeEntry("ImgFormat", m_settingsUi.imgFormat->currentText());
        saving.writeEntry("SetQuality", m_settingsUi.setQuality->isChecked());
        saving.writeEntry("ImgQuality", m_settingsUi.imgQuality->value());
        saving.writeEntry("ShowBeforeSave", m_settingsUi.showB4Save->isChecked());
        saving.sync();

        KConfigGroup general(KSharedConfig::openConfig(), "General");
        general.writeEntry("PreviewDPI", m_settingsUi.previewDPI->currentText());
        general.writeEntry("SetPreviewDPI", m_settingsUi.setPreviewDPI->isChecked());
        general.writeEntry("DisableAutoSelection", m_settingsUi.u_disableSelections->isChecked());
        general.sync();

        // the previewDPI has to be set here
        if (m_settingsUi.setPreviewDPI->isChecked()) {
            m_ksanew->setPreviewResolution(m_settingsUi.previewDPI->currentText().toFloat());
        } else {
            // 0.0 means default value.
            m_ksanew->setPreviewResolution(0.0);
        }
        m_ksanew->enableAutoSelect(!m_settingsUi.u_disableSelections->isChecked());

        // pressing OK in the settings dialog means use those settings.
        m_saveLocation->u_urlRequester->setUrl(QUrl::fromUserInput(m_settingsUi.saveDirLEdit->text()));
        m_saveLocation->u_imgPrefix->setText(m_settingsUi.imgPrefix->text());
        m_saveLocation->u_imgFormat->setCurrentText(m_settingsUi.imgFormat->currentText());

        m_firstImage = true;
    } else {
        //Forget Changes
        readSettings();
    }
}

void Skanlite::imageReady(QByteArray &data, int w, int h, int bpl, int f)
{
    // save the image data
    m_data = data;
    m_width = w;
    m_height = h;
    m_bytesPerLine = bpl;
    m_format = f;

    if (m_settingsUi.showB4Save->isChecked() == true) {
        /* copy the image data into m_img and show it*/
        m_img = m_ksanew->toQImageSilent(data, w, h, bpl, (KSaneIface::KSaneWidget::ImageFormat)f);
        m_imageViewer.setQImage(&m_img);
        m_imageViewer.zoom2Fit();
        m_showImgDialogSaveButton->setFocus();
        m_showImgDialog->exec();
        // save has been done as a result of save or then we got cancel
    } else {
        m_img = QImage(); // clear the image to ensure we save the correct one.
        saveImage();
    }
}

void Skanlite::saveImage()
{
    // ask the first time if we are in "ask on first" mode
    if ((m_settingsUi.saveModeCB->currentIndex() == SaveModeAskFirst) && m_firstImage) {
        if (m_saveLocation->exec() != QFileDialog::Accepted) {
            m_ksanew->scanCancel(); // In case we are cancelling a document feeder scan
            return;
        }
        m_firstImage = false;
    }

    QString dir = m_saveLocation->u_urlRequester->url().url();
    QString prefix = m_saveLocation->u_imgPrefix->text();
    QString imgFormat = m_saveLocation->u_imgFormat->currentText().toLower();
    int fileNumber = m_saveLocation->u_numStartFrom->value();
    QStringList filterList = m_filterList;
    if ((m_format == KSaneIface::KSaneWidget::FormatRGB_16_C) ||
            (m_format == KSaneIface::KSaneWidget::FormatGrayScale16)) {
        filterList = m_filter16BitList;
        if (imgFormat != QLatin1String("png")) {
            imgFormat = QLatin1String("png");
            KMessageBox::information(this, i18n("The image will be saved in the PNG format, as Skanlite only supports saving 16 bit color images in the PNG format."));
        }
    }

    //qDebug() << dir << prefix << imgFormat;

    // find next available file name for name suggestion
    QUrl fileUrl;
    QString fname;
    for (int i = fileNumber; i <= m_saveLocation->u_numStartFrom->maximum(); ++i) {
        fname = QString::fromLatin1("%1%2.%3")
                .arg(prefix)
                .arg(i, 4, 10, QLatin1Char('0'))
                .arg(imgFormat);

        fileUrl = QUrl::fromUserInput(QStringLiteral("%1%2").arg(dir, fname));
        //qDebug() << fileUrl;
        if (fileUrl.isLocalFile()) {
            if (!QFileInfo(fileUrl.toLocalFile()).exists()) {
                break;
            }
        }
        else {
            KIO::StatJob *statJob = KIO::stat(fileUrl, KIO::StatJob::DestinationSide, 0);
            KJobWidgets::setWindow(statJob, QApplication::activeWindow());
            if (!statJob->exec()) {
                break;
            }
        }
    }

    if (m_settingsUi.saveModeCB->currentIndex() == SaveModeManual) {
        // prepare the save dialog
        QFileDialog saveDialog(this, i18n("New Image File Name"));
        saveDialog.setAcceptMode(QFileDialog::AcceptSave);
        saveDialog.setFileMode(QFileDialog::AnyFile);

        // ask for a filename if requested.
        saveDialog.setDirectoryUrl(fileUrl.adjusted(QUrl::RemoveFilename));
        saveDialog.selectUrl(fileUrl);
        // NOTE it is probably a bug that both setDirectoryUrl and selectUrl have
        // to be set to get remote urls to work

        QStringList actualFilterList = filterList;
        QString currentMimeFilter = QLatin1String("image/") + imgFormat;
        saveDialog.setMimeTypeFilters(actualFilterList);
        saveDialog.selectMimeTypeFilter(currentMimeFilter);
        //qDebug() << fileUrl.url() << fileUrl.toLocalFile() << currentMimeFilter;

        do {
            if (saveDialog.exec() != QFileDialog::Accepted) {
                return;
            }

            fileUrl = saveDialog.selectedUrls()[0];

            bool exists;
            if (fileUrl.isLocalFile()) {
                exists = QFileInfo(fileUrl.toLocalFile()).exists();
            }
            else {
                KIO::StatJob *statJob = KIO::stat(fileUrl, KIO::StatJob::DestinationSide, 0);
                KJobWidgets::setWindow(statJob, QApplication::activeWindow());
                exists = statJob->exec();
            }
            if (exists) {
                if (KMessageBox::warningContinueCancel(this,
                    i18n("Do you want to overwrite \"%1\"?", fileUrl.fileName()),
                    QString(),
                    KGuiItem(i18n("Overwrite")),
                    KStandardGuiItem::cancel(),
                    QLatin1String("editorWindowSaveOverwrite")
                ) ==  KMessageBox::Continue)
                {
                    break;
                }
            }
            else {
                break;
            }
        } while (true);
    }

    m_firstImage = false;

    // Get the quality
    int quality = -1;
    if (m_settingsUi.setQuality->isChecked()) {
        quality = m_settingsUi.imgQuality->value();
    }

    //qDebug() << "suffix" << QFileInfo(fileUrl.fileName()).suffix();
    QString localName;
    if (!fileUrl.isLocalFile()) {
        QTemporaryFile tmp;
        tmp.open();
        localName = QStringLiteral("%1.%2").arg(tmp.fileName(), QFileInfo(fileUrl.fileName()).suffix());
        tmp.close(); // we just want the filename
    }
    else {
        localName = fileUrl.toLocalFile();
    }

    // Save
    if ((m_format == KSaneIface::KSaneWidget::FormatRGB_16_C) ||
        (m_format == KSaneIface::KSaneWidget::FormatGrayScale16))
    {
        KSaneImageSaver saver;
        if (saver.savePngSync(localName, m_data, m_width, m_height, m_format)) {
            m_showImgDialog->close(); // closing the window if it is closed should not be a problem.
        }
        else {
            perrorMessageBox(i18n("Failed to save image"));
            return;
        }
    }
    else  {
        // create the image if needed.
        if (m_img.width() < 1) {
            m_img = m_ksanew->toQImage(m_data, m_width, m_height, m_bytesPerLine, (KSaneIface::KSaneWidget::ImageFormat)m_format);
        }
        if (m_img.save(localName, 0, quality)) {
            m_showImgDialog->close(); // calling close() on a closed window does nothing.
        }
        else {
            perrorMessageBox(i18n("Failed to save image"));
            return;
        }
    }

    if (!fileUrl.isLocalFile()) {
        QFile tmpFile(localName);
        tmpFile.open(QIODevice::ReadOnly);
        auto uploadJob = KIO::storedPut(&tmpFile, fileUrl, -1);
        KJobWidgets::setWindow(uploadJob, QApplication::activeWindow());
        bool ok = uploadJob->exec();
        tmpFile.close();
        tmpFile.remove();
        if (!ok) {
            KMessageBox::sorry(0, i18n("Failed to upload image"));
        }
    }

    // Save the file base name without number
    QString baseName = QFileInfo(fileUrl.fileName()).completeBaseName();
    while ((baseName.size() > 1) && (baseName[baseName.size() - 1].isNumber())) {
        baseName.remove(baseName.size() - 1, 1);
    }
    m_saveLocation->u_imgPrefix->setText(baseName);

    // Save the number
    QString fileNumStr = QFileInfo(fileUrl.fileName()).completeBaseName();
    fileNumStr.remove(baseName);
    fileNumber = fileNumStr.toInt();
    if (fileNumber) {
        m_saveLocation->u_numStartFrom->setValue(fileNumber + 1);
    }

    if (m_settingsUi.saveModeCB->currentIndex() == SaveModeManual) {
        // Save last used dir, prefix and suffix.
        m_saveLocation->u_urlRequester->setUrl(KIO::upUrl(fileUrl));
        m_saveLocation->u_imgFormat->setCurrentText(QFileInfo(fileUrl.fileName()).suffix());
    }
}

void Skanlite::getDir(void)
{
    // FIXME KF5 / WAIT: this is not working yet due to a bug in frameworkintegration:
    // see commit: 2c1ee08a21a1f16f9c2523718224598de8fc0d4f for kf5/src/frameworks/frameworkintegration/tests/qfiledialogtest.cpp
    QString dir = QFileDialog::getExistingDirectory(m_settingsDialog, QString(), m_settingsUi.saveDirLEdit->text());
    if (!dir.isEmpty()) {
        m_settingsUi.saveDirLEdit->setText(dir);
    }
}

void Skanlite::showAboutDialog(void)
{
    KAboutApplicationDialog(*m_aboutData).exec();
}

void Skanlite::saveScannerOptions()
{
    KConfigGroup saving(KSharedConfig::openConfig(), "Image Saving");
    saving.writeEntry("NumberStartsFrom", m_saveLocation->u_numStartFrom->value());

    if (!m_ksanew) {
        return;
    }

    KConfigGroup options(KSharedConfig::openConfig(), QString::fromLatin1("Options For %1").arg(m_deviceName));
    QMap <QString, QString> opts;
    m_ksanew->getOptVals(opts);
    QMap<QString, QString>::const_iterator it = opts.constBegin();
    while (it != opts.constEnd()) {
        options.writeEntry(it.key(), it.value());
        ++it;
    }
    options.sync();
}

void Skanlite::defaultScannerOptions()
{
    if (!m_ksanew) {
        return;
    }

    m_ksanew->setOptVals(m_defaultScanOpts);
}

void Skanlite::loadScannerOptions()
{
    KConfigGroup saving(KSharedConfig::openConfig(), "Image Saving");
    m_saveLocation->u_numStartFrom->setValue(saving.readEntry("NumberStartsFrom", 1));

    if (!m_ksanew) {
        return;
    }

    KConfigGroup scannerOptions(KSharedConfig::openConfig(), QString::fromLatin1("Options For %1").arg(m_deviceName));
    m_ksanew->setOptVals(scannerOptions.entryMap());
}

void Skanlite::availableDevices(const QList<KSaneWidget::DeviceInfo> &deviceList)
{
    for (int i = 0; i < deviceList.size(); ++i) {
        qDebug() << deviceList.at(i).name;
    }
}

void Skanlite::alertUser(int type, const QString &strStatus)
{
    switch (type) {
    case KSaneWidget::ErrorGeneral:
        KMessageBox::sorry(0, strStatus, QLatin1String("Skanlite Test"));
        break;
    default:
        KMessageBox::information(0, strStatus, QLatin1String("Skanlite Test"));
    }
}

void Skanlite::buttonPressed(const QString &optionName, const QString &optionLabel, bool pressed)
{
    qDebug() << "Button" << optionName << optionLabel << ((pressed) ? "pressed" : "released");
}
