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

#include "SaveLocation.h"
#include "showimagedialog.h"

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
#include <QCloseEvent>

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
    , m_dbusInterface(this)
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
    connect(m_ksanew, &KSaneWidget::scanDone, [this](){
        if (!m_pendingApplyScanOpts.isEmpty()) {
            applyScannerOptions(m_pendingApplyScanOpts);
        }
    });

    m_imageSaver = new KSaneImageSaver(this);
    connect(m_imageSaver, &KSaneImageSaver::imageSaved, this, &Skanlite::imageSaved);

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
            if (ba.isEmpty()) {
                continue;
            }
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
        m_saveLocation->setImageFormats(m_typeList);

        mainLayout->addWidget(settingsWidget);

        QDialogButtonBox *dlgButtonBoxBottom = new QDialogButtonBox(this);
        dlgButtonBoxBottom->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Close);
        connect(dlgButtonBoxBottom, &QDialogButtonBox::accepted, m_settingsDialog, &QDialog::accept);
        connect(dlgButtonBoxBottom, &QDialogButtonBox::rejected, m_settingsDialog, &QDialog::reject);

        mainLayout->addWidget(dlgButtonBoxBottom);

        m_settingsDialog->setWindowTitle(i18n("Skanlite Settings"));

        connect(m_settingsUi.revertOptions, &QPushButton::clicked, this, &Skanlite::defaultScannerOptions);
        readSettings();

        // default directory for the save dialog
        m_saveLocation->setFolderUrl(m_settingsUi.saveDirRequester->url());
        m_saveLocation->setImagePrefix(m_settingsUi.imgPrefix->text());
        m_saveLocation->setImageFormat(m_settingsUi.imgFormat->currentText());
    }

    // open the scan device
    if (m_ksanew->openDevice(device) == false) {
        QString dev = m_ksanew->selectDevice(nullptr);
        if (dev.isEmpty()) {
            // either no scanner was found or then cancel was pressed.
            exit(0);
        }
        if (m_ksanew->openDevice(dev) == false) {
            // could not open a scanner
            KMessageBox::sorry(nullptr, i18n("Opening the selected scanner failed."));
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
    m_showImgDialog = new ShowImageDialog(this);
    connect(m_showImgDialog, &ShowImageDialog::saveRequested, this, &Skanlite::saveImage);
    connect(m_showImgDialog, &ShowImageDialog::rejected, m_ksanew, &KSaneWidget::scanCancel);

    // save the default sane options for later use
    m_ksanew->getOptVals(m_defaultScanOpts);

    // load saved options
    loadScannerOptions();

    m_ksanew->initGetDeviceList();

    m_firstImage = true;

    if (m_dbusInterface.setupDBusInterface()) {
        // D-Bus related slots
        connect(&m_dbusInterface, &DBusInterface::requestedScan, m_ksanew, &KSaneWidget::scanFinal);
        connect(&m_dbusInterface, &DBusInterface::requestedPreview, m_ksanew, &KSaneWidget::startPreviewScan);
        connect(&m_dbusInterface, &DBusInterface::requestedScanCancel, m_ksanew, &KSaneWidget::scanCancel);
        connect(&m_dbusInterface, &DBusInterface::requestedSetScannerOptions, this, &Skanlite::setScannerOptions);
        connect(&m_dbusInterface, &DBusInterface::requestedSetSelection, this, &Skanlite::setSelection);

        // D-Bus related slots below must be Qt::DirectConnection to simplify return value forwarding via DBusInterface
        connect(&m_dbusInterface, &DBusInterface::requestedGetScannerOptions, this, &Skanlite::getScannerOptions, Qt::DirectConnection);
        connect(&m_dbusInterface, &DBusInterface::requestedDefaultScannerOptions, this, &Skanlite::getDefaultScannerOptions, Qt::DirectConnection);
        connect(&m_dbusInterface, &DBusInterface::requestedDeviceName, this, &Skanlite::getDeviceName, Qt::DirectConnection);
        connect(&m_dbusInterface, &DBusInterface::requestedSaveScannerOptionsToProfile, this, &Skanlite::saveScannerOptionsToProfile, Qt::DirectConnection);
        connect(&m_dbusInterface, &DBusInterface::requestedSwitchToProfile, this, &Skanlite::switchToProfile, Qt::DirectConnection);
        connect(&m_dbusInterface, &DBusInterface::requestedGetSelection, this, &Skanlite::getSelection, Qt::DirectConnection);

        // D-Bus related signals
        connect(m_ksanew, &KSaneWidget::scanDone, &m_dbusInterface, &DBusInterface::scanDone);
        connect(m_ksanew, &KSaneWidget::userMessage, &m_dbusInterface, &DBusInterface::userMessage);
        connect(m_ksanew, &KSaneWidget::scanProgress, &m_dbusInterface, &DBusInterface::scanProgress);
        connect(m_ksanew, &KSaneWidget::buttonPressed, &m_dbusInterface, &DBusInterface::buttonPressed);
    }
    else {
        // keep working without dbus
    }
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
        KMessageBox::sorry(nullptr, i18n("%1: %2", text, QString::fromLocal8Bit(strerror(errno))));
    }
    else {
        KMessageBox::sorry(nullptr, text);
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
    m_settingsUi.saveDirRequester->setUrl(saving.readEntry("Location", QUrl(QDir::homePath())));
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
    }
    else {
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
        saving.writeEntry("Location", m_settingsUi.saveDirRequester->url());
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
        }
        else {
            // 0.0 means default value.
            m_ksanew->setPreviewResolution(0.0);
        }
        m_ksanew->enableAutoSelect(!m_settingsUi.u_disableSelections->isChecked());

        // pressing OK in the settings dialog means use those settings.
        m_saveLocation->setFolderUrl(m_settingsUi.saveDirRequester->url());
        m_saveLocation->setImagePrefix(m_settingsUi.imgPrefix->text());
        m_saveLocation->setImageFormat(m_settingsUi.imgFormat->currentText());

        m_firstImage = true;
    }
    else {
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
        m_showImgDialog->setQImage(&m_img);
        m_showImgDialog->zoom2Fit();
        m_showImgDialog->exec();
        // save has been done as a result of save or then we got cancel
    }
    else {
        m_img = QImage(); // clear the image to ensure we save the correct one.
        saveImage();
    }
}

bool pathExists(const QUrl& dirUrl, QWidget* parent)
{
    // propose directory creation if doesn't exists
    if (dirUrl.isLocalFile()) {
        QDir path(dirUrl.toLocalFile());
        if (!path.exists()) {
            if (KMessageBox::questionYesNo(parent, i18n("Directory doesn't exist, do you wish to create it?")) == KMessageBox::ButtonCode::Yes ) {
                if (!path.mkpath(QLatin1String("."))) {
                    KMessageBox::error(parent, i18n("Could not create directory %1", path.path()));
                    return false;
                }
            }
        }
    }
    return true;
}

void Skanlite::saveImage()
{
    // ask the first time if we are in "ask on first" mode
    QUrl dirUrl = m_saveLocation->folderUrl();

    while ((m_firstImage && (m_settingsUi.saveModeCB->currentIndex() == SaveModeAskFirst)) ||
        !pathExists(dirUrl, this))
    {
        if (m_saveLocation->exec() != QFileDialog::Accepted) {
            m_ksanew->scanCancel(); // In case we are cancelling a document feeder scan
            return;
        }
        dirUrl = m_saveLocation->folderUrl();
        m_firstImage = false;
    }

    QString prefix = m_saveLocation->imagePrefix();
    QString imgFormat = m_saveLocation->imageFormat().toLower();
    int fileNumber = m_saveLocation->startNumber();
    QStringList filterList = m_filterList;
    bool enforceSavingAsPng16bit = false;
    if ((m_format == KSaneIface::KSaneWidget::FormatRGB_16_C) ||
        (m_format == KSaneIface::KSaneWidget::FormatGrayScale16))
    {
        filterList = m_filter16BitList;
        enforceSavingAsPng16bit = true;
        if (imgFormat != QLatin1String("png")) {
            imgFormat = QLatin1String("png");
            KMessageBox::information(this, i18n("The image will be saved in the PNG format, as Skanlite only supports saving 16 bit color images in the PNG format."));
        }
    }

    // find next available file name for name suggestion
    QUrl fileUrl;
    QString fname;
    for (int i = fileNumber; i <= m_saveLocation->startNumberMax(); ++i) {
        fname = QString::fromLatin1("%1%2.%3")
                .arg(prefix)
                .arg(i, 4, 10, QLatin1Char('0'))
                .arg(imgFormat);

        fileUrl = dirUrl;
        fileUrl.setPath(fileUrl.path() + fname);
        fileUrl = fileUrl.adjusted(QUrl::NormalizePathSegments);
        if (fileUrl.isLocalFile()) {
            if (!QFileInfo(fileUrl.toLocalFile()).exists()) {
                break;
            }
        }
        else {
            KIO::StatJob *statJob = KIO::statDetails(fileUrl, KIO::StatJob::DestinationSide);
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

        if (saveDialog.exec() != QFileDialog::Accepted) {
            return;
        }

        fileUrl = saveDialog.selectedUrls()[0];
    }

    m_firstImage = false;

    // Get the quality
    int quality = -1;
    if (m_settingsUi.setQuality->isChecked()) {
        quality = m_settingsUi.imgQuality->value();
    }

    QString localName;
    QString suffix = QFileInfo(fileUrl.fileName()).suffix();
    QString fileFormat;
    if (suffix.isEmpty()) {
        fileFormat = QLatin1String("png");
    }

    if (!fileUrl.isLocalFile()) {
        QTemporaryFile tmp;
        tmp.open();
        if (suffix.isEmpty()) {
            localName = tmp.fileName();
        }
        else {
            localName = QStringLiteral("%1.%2").arg(tmp.fileName(), suffix);
        }
        tmp.close(); // we just want the filename
    }
    else {
        localName = fileUrl.toLocalFile();
    }


    // Save
    if (enforceSavingAsPng16bit) {
        m_imageSaver->save16BitPng(fileUrl, localName, m_data, m_width, m_height, m_bytesPerLine, (int) m_ksanew->currentDPI(), m_format, fileFormat, quality);
    } else {
        m_imageSaver->saveQImage(fileUrl, localName, m_data, m_width, m_height, m_bytesPerLine, (int) m_ksanew->currentDPI(), m_format, fileFormat, quality);
    }
}

void Skanlite::imageSaved(const QUrl &fileUrl, const QString &localName, bool success)
{
    if (!success) {
        perrorMessageBox(i18n("Failed to save image"));
        return;
    }

    m_showImgDialog->close(); // calling close() on a closed window does nothing.


    if (!fileUrl.isLocalFile()) {
        QFile tmpFile(localName);
        tmpFile.open(QIODevice::ReadOnly);
        auto uploadJob = KIO::storedPut(&tmpFile, fileUrl, -1);
        KJobWidgets::setWindow(uploadJob, QApplication::activeWindow());
        bool ok = uploadJob->exec();
        tmpFile.close();
        tmpFile.remove();
        if (!ok) {
            KMessageBox::sorry(nullptr, i18n("Failed to upload image"));
        }
        else {
            emit m_dbusInterface.imageSaved(fileUrl.toString());
        }
    }
    else {
        emit m_dbusInterface.imageSaved(localName);
    }

    // Save the file base name without number
    QString baseName = QFileInfo(fileUrl.fileName()).completeBaseName();
    while ((!baseName.isEmpty()) && (baseName[baseName.size() - 1].isNumber())) {
        baseName.remove(baseName.size() - 1, 1);
    }
    m_saveLocation->setImagePrefix(baseName);

    // Save the number
    QString fileNumStr = QFileInfo(fileUrl.fileName()).completeBaseName();
    fileNumStr.remove(baseName);
    int fileNumber = fileNumStr.toInt();
    if (fileNumber) {
        m_saveLocation->setStartNumber(fileNumber + 1);
    }

    if (m_settingsUi.saveModeCB->currentIndex() == SaveModeManual) {
        // Save last used dir, prefix and suffix.
        m_saveLocation->setFolderUrl(KIO::upUrl(fileUrl));
        m_saveLocation->setImageFormat(QFileInfo(fileUrl.fileName()).suffix());
    }
}

void Skanlite::showAboutDialog(void)
{
    KAboutApplicationDialog(*m_aboutData).exec();
}

void writeScannerOptions(const QString &groupName, const QMap <QString, QString> &opts)
{
    KConfigGroup options(KSharedConfig::openConfig(), groupName);
    QMap<QString, QString>::const_iterator it = opts.constBegin();
    while (it != opts.constEnd()) {
        options.writeEntry(it.key(), it.value());
        ++it;
    }
    options.sync();
}

void readScannerOptions(const QString &groupName, QMap <QString, QString> &opts)
{
    KConfigGroup scannerOptions(KSharedConfig::openConfig(), groupName);
    opts = scannerOptions.entryMap();
}

void Skanlite::saveScannerOptions()
{
    KConfigGroup saving(KSharedConfig::openConfig(), "Image Saving");
    saving.writeEntry("NumberStartsFrom", m_saveLocation->startNumber());

    if (!m_ksanew) {
        return;
    }

    KConfigGroup options(KSharedConfig::openConfig(), QString::fromLatin1("Options For %1").arg(m_deviceName));
    QMap <QString, QString> opts;
    m_ksanew->getOptVals(opts);
    writeScannerOptions(QString::fromLatin1("Options For %1").arg(m_deviceName), opts);
}

void Skanlite::defaultScannerOptions()
{
    if (!m_ksanew) {
        return;
    }

    applyScannerOptions(m_defaultScanOpts);
}

void Skanlite::applyScannerOptions(const QMap <QString, QString> &opts)
{
    if (m_ksanew->setOptVals(opts) == -1) {
        m_pendingApplyScanOpts = opts;
    } else {
        m_pendingApplyScanOpts.clear();
    }
}

void Skanlite::loadScannerOptions()
{
    KConfigGroup saving(KSharedConfig::openConfig(), "Image Saving");
    m_saveLocation->setStartNumber(saving.readEntry("NumberStartsFrom", 1));

    if (!m_ksanew) {
        return;
    }

    QMap <QString, QString> opts;
    readScannerOptions(QString::fromLatin1("Options For %1").arg(m_deviceName), opts);
    applyScannerOptions(opts);
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
        KMessageBox::sorry(nullptr, strStatus, QLatin1String("Skanlite Test"));
        break;
    default:
        KMessageBox::information(nullptr, strStatus, QLatin1String("Skanlite Test"));
    }
}

void Skanlite::buttonPressed(const QString &optionName, const QString &optionLabel, bool pressed)
{
    qDebug() << "Button" << optionName << optionLabel << ((pressed) ? "pressed" : "released");
}

// D-Bus interface related helper functions

QStringList serializeScannerOptions(const QMap<QString, QString> &opts)
{
    QStringList sl;
    QMap<QString, QString>::const_iterator it = opts.constBegin();
    while (it != opts.constEnd()) {
        sl.append(it.key() + QLatin1Char('=') + it.value());
        ++it;
    }
    return sl;
}

void deserializeScannerOptions(const QStringList &settings, QMap<QString, QString> &opts)
{
    foreach (QString s, settings) {
        int i = s.lastIndexOf(QLatin1Char('='));
        opts[s.left(i)] = s.right(s.length()-i-1);
    }
}

static const QStringList selectionSettings = { QLatin1String("tl-x"), QLatin1String("tl-y"),
                                               QLatin1String("br-x"), QLatin1String("br-y") };

void filterSelectionSettings(QMap<QString, QString> &opts)
{
    foreach (QString s, selectionSettings) {
        opts.remove(s);
    }
}

bool containsSelectionSettings(const QMap<QString, QString> &opts)
{
    foreach (QString s, selectionSettings) {
        if (opts.contains(s)) {
            return true;
        }
    }
    return false;
}

void Skanlite::processSelectionOptions(QMap<QString, QString> &opts, bool ignoreSelection)
{
    if (ignoreSelection) {
        filterSelectionSettings(opts);
    }
    else {
        if (containsSelectionSettings(opts)) { // make sure we really have selection to apply
            m_ksanew->setSelection(QPointF(0,0), QPointF(1,1)); // bcs settings have no effect if nothing was selected beforehand (Bug 377009)
        }
    }
}

// D-Bus interface related slots

void Skanlite::getScannerOptions()
{
    QMap <QString, QString> opts;
    m_ksanew->getOptVals(opts);
    m_dbusInterface.setReply(serializeScannerOptions(opts));
}

void Skanlite::setScannerOptions(const QStringList &options, bool ignoreSelection)
{
    QMap <QString, QString> opts;
    deserializeScannerOptions(options, opts);
    processSelectionOptions(opts, ignoreSelection);
    applyScannerOptions(opts);
}


void Skanlite::getDefaultScannerOptions()
{
    m_dbusInterface.setReply(serializeScannerOptions(m_defaultScanOpts));
}

static const QLatin1String defaultProfileGroup("Options For %1 - Profile %2"); // 1 - device, 2 - arg

void Skanlite::saveScannerOptionsToProfile(const QStringList &options, const QString &profile, bool ignoreSelection)
{
    QMap <QString, QString> opts;
    deserializeScannerOptions(options, opts);
    processSelectionOptions(opts, ignoreSelection);
    writeScannerOptions(QString(defaultProfileGroup).arg(m_deviceName).arg(profile), opts);
}

void Skanlite::switchToProfile(const QString &profile, bool ignoreSelection)
{
    QMap <QString, QString> opts;
    readScannerOptions(QString(defaultProfileGroup).arg(m_deviceName).arg(profile), opts);

    if (opts.empty()) {
        opts = m_defaultScanOpts;
    }

    processSelectionOptions(opts, ignoreSelection);
    applyScannerOptions(opts);
}

void Skanlite::getDeviceName()
{
    m_dbusInterface.setReply(QStringList(m_deviceName));
}

void Skanlite::getSelection()
{
    QMap <QString, QString> opts;
    m_ksanew->getOptVals(opts);

    QStringList reply;
    foreach ( QString key, selectionSettings ) {
        if (opts.contains(key)) {
            reply.append(key + QLatin1Char('=') + opts[key]);
        }
    }
    m_dbusInterface.setReply(reply);
}

void Skanlite::setSelection(const QStringList &options)
{ // here options contains selection related subset of options
    setScannerOptions(options, false);
}
