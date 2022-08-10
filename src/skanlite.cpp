/* ============================================================
*
* SPDX-FileCopyrightText: 2007-2012 Kåre Särs <kare.sars@iki .fi>
* SPDX-FileCopyrightText: 2009 Arseniy Lartsev <receive-spam at yandex dot ru>
* SPDX-FileCopyrightText: 2014 Gregor Mitsch : port to KDE5 frameworks
*
* SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*
* ============================================================ */

#include "skanlite.h"

#include "SaveLocation.h"
#include "showimagedialog.h"
#include "SkanliteImageSaver.h"

#include <QApplication>
#include <QScrollArea>
#include <QStringList>
#include <QFileDialog>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QMessageBox>
#include <QTemporaryFile>
#include <QImageWriter>
#include <QMimeType>
#include <QMimeDatabase>
#include <QCloseEvent>
#include <QProgressBar>

#include <KAboutData>
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
#include <KHelpMenu>
#include <KSaneWidget>

#include <skanlite_debug.h>

#include <errno.h>
using namespace KSaneIface;
Skanlite::Skanlite(const QString &device, QWidget *parent)
    : QDialog(parent)
    , m_dbusInterface(this)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QDialogButtonBox *dlgButtonBoxBottom = new QDialogButtonBox(this);
    dlgButtonBoxBottom->setStandardButtons(QDialogButtonBox::Help);
    dlgButtonBoxBottom->button(QDialogButtonBox::Help)->setAutoDefault(false);

    KHelpMenu *helpMenu = new KHelpMenu(this, KAboutData::applicationData(), false);
    dlgButtonBoxBottom->button(QDialogButtonBox::Help)->setMenu(helpMenu->menu());

    QPushButton *btnConfigure = dlgButtonBoxBottom->addButton(i18n("Configure..."), QDialogButtonBox::ButtonRole::ResetRole);
    btnConfigure->setIcon(QIcon::fromTheme(QStringLiteral("configure")));
    btnConfigure->setAutoDefault(false);

    QPushButton *btnReselectDevice = dlgButtonBoxBottom->addButton(i18n("Reselect scanner device"), QDialogButtonBox::ButtonRole::ActionRole);
    btnReselectDevice->setIcon(QIcon::fromTheme(QStringLiteral("view-refresh")));
    btnReselectDevice->setAutoDefault(false);

    m_firstImage = true;

    m_ksanew = new KSaneIface::KSaneWidget(this);
    connect(m_ksanew, &KSaneWidget::scannedImageReady, this, &Skanlite::imageReady);
    connect(m_ksanew, &KSaneWidget::userMessage, this, &Skanlite::alertUser);
    connect(m_ksanew, &KSaneWidget::buttonPressed, this, &Skanlite::buttonPressed);
    connect(m_ksanew, &KSaneWidget::scanDone, this, [this](){
        if (!m_pendingApplyScanOpts.isEmpty()) {
            applyScannerOptions(m_pendingApplyScanOpts);
        }
    });

    m_saveProgressBar = new QProgressBar(this);
    m_saveProgressBar->setVisible(false);
    m_saveProgressBar->setFormat(i18n("Saving: %v kB"));
    m_saveProgressBar->setTextVisible(true);

    m_saveUpdateTimer.setInterval(200);
    m_saveUpdateTimer.setSingleShot(false);
    connect(&m_saveUpdateTimer, &QTimer::timeout, this, &Skanlite::updateSaveProgress);

    mainLayout->addWidget(m_ksanew);
    mainLayout->addWidget(m_saveProgressBar);
    mainLayout->addWidget(dlgButtonBoxBottom);

    // read the size here...
    KConfigGroup window(KSharedConfig::openStateConfig(), "Window");
    QSize rect = window.readEntry("Geometry", QSize(740, 400));
    resize(rect);

    // open scanner device from command line, otherwise try remembered one
    QString deviceName;
    QString deviceVendor;
    QString deviceModel;
    if (device.isEmpty()) {
        KConfigGroup general(KSharedConfig::openStateConfig(), QStringLiteral("General"));
        deviceName = general.readEntry(QStringLiteral("deviceName"));
        deviceVendor = general.readEntry(QStringLiteral("deviceVendor"));
        deviceModel = general.readEntry(QStringLiteral("deviceModel"));
    } else {
        deviceName = device;
    }

    connect(dlgButtonBoxBottom, &QDialogButtonBox::rejected, this, &QDialog::close);
    connect(this, &QDialog::finished, this, &Skanlite::saveWindowSize);
    connect(this, &QDialog::finished, this, &Skanlite::saveScannerDevice);
    connect(this, &QDialog::finished, this, &Skanlite::saveScannerOptions);
    connect(btnConfigure, &QPushButton::clicked, this, &Skanlite::showSettingsDialog);
    connect(btnReselectDevice, &QPushButton::clicked, this, &Skanlite::reselectScannerDevice);
    connect(dlgButtonBoxBottom, &QDialogButtonBox::helpRequested, this, &Skanlite::showHelp);

    //
    // Create the settings dialog
    //
    {
        m_settingsDialog = new QDialog(this);

        QVBoxLayout *mainLayout = new QVBoxLayout(m_settingsDialog);

        QWidget *settingsWidget = new QWidget(m_settingsDialog);
        m_settingsUi.setupUi(settingsWidget);
        m_settingsUi.revertOptions->setIcon(QIcon::fromTheme(QStringLiteral("edit-undo")));
        m_saveLocation = new SaveLocation(this);

        // add the supported image types
        const QList<QByteArray> tmpList = QImageWriter::supportedMimeTypes();
        m_filterList.clear();
        for (const auto &ba : tmpList) {
            if (ba.isEmpty()) {
                continue;
            }
            m_filterList.append(QString::fromLatin1(ba));
        }

        qCDebug(SKANLITE_LOG) << m_filterList;

        // Put first class citizens at first place
        m_filterList.removeAll(QStringLiteral("image/jpeg"));
        m_filterList.removeAll(QStringLiteral("image/tiff"));
        m_filterList.removeAll(QStringLiteral("image/png"));
        m_filterList.insert(0, QStringLiteral("image/png"));
        m_filterList.insert(1, QStringLiteral("image/jpeg"));
        m_filterList.insert(2, QStringLiteral("image/tiff"));
        m_filterList.insert(3, QStringLiteral("application/pdf"));

        m_filter16BitList << QStringLiteral("image/png");
        m_filter16BitList << QStringLiteral("image/tiff");

        // fill m_filterList (...)
        {
            QStringList namedMimeTypes;
            for (const QString &mimeStr : qAsConst(m_filterList)) {
                QMimeType mimeType = QMimeDatabase().mimeTypeForName(mimeStr);
                namedMimeTypes.append(mimeType.name());

                m_settingsUi.imgFormat->addItem(mimeType.preferredSuffix(), mimeType.name());
                m_saveLocation->addImageFormat(mimeType.preferredSuffix(), mimeType.name());
            }
            m_filterList << std::move(namedMimeTypes);
        }

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
        m_saveLocation->setImageFormatIndex(m_settingsUi.imgFormat->currentIndex());
    }

    // open the scan device
    if (!m_ksanew->openDevice(deviceName)) {
        QString dev = m_ksanew->selectDevice(nullptr);
        if (dev.isEmpty()) {
            // either no scanner was found or then cancel was pressed.
            exit(0);
        }
        if (!m_ksanew->openDevice(dev)) {
            // could not open a scanner
            KMessageBox::error(nullptr, i18n("Opening the selected scanner failed."));
            exit(1);
        }
        else {
            updateWindowTitle(dev, m_ksanew->deviceVendor(), m_ksanew->deviceModel());
            m_deviceName = dev;
        }
    }
    else {
        if (deviceVendor.isEmpty()) {
            updateWindowTitle(deviceName);
        } else {
            updateWindowTitle(deviceName, deviceVendor, deviceModel);
        }
        m_deviceName = deviceName;
        m_deviceModel = deviceModel;
        m_deviceVendor = deviceVendor;
    }

    // prepare the Show Image Dialog
    m_showImgDialog = new ShowImageDialog(this);
    connect(m_showImgDialog, &ShowImageDialog::saveRequested, this, &Skanlite::saveImage);
    connect(m_showImgDialog, &ShowImageDialog::rejected, m_ksanew, &KSaneWidget::scanCancel);

    // save the default sane options for later use
    m_ksanew->getOptVals(m_defaultScanOpts);

    // load saved options
    loadScannerOptions();

    m_firstImage = true;
    m_ksanew->setFocus();

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
    KHelpClient::invokeHelp(QStringLiteral("index"), QStringLiteral("skanlite"));
}

void Skanlite::closeEvent(QCloseEvent *event)
{
    saveWindowSize();
    saveScannerDevice();
    saveScannerOptions();
    event->accept();
}

void Skanlite::saveWindowSize()
{
    KConfigGroup window(KSharedConfig::openStateConfig(), "Window");
    window.writeEntry("Geometry", size());
    window.sync();
}

void Skanlite::saveScannerDevice()
{
    KConfigGroup general(KSharedConfig::openStateConfig(), "General");
    general.writeEntry(QStringLiteral("deviceName"), m_deviceName);
    general.writeEntry(QStringLiteral("deviceModel"), m_deviceModel);
    general.writeEntry(QStringLiteral("deviceVendor"), m_deviceVendor);
    general.sync();
}

void Skanlite::reselectScannerDevice()
{
    m_ksanew->closeDevice();
    m_deviceName.clear();
    m_deviceVendor.clear();
    m_deviceModel.clear();
    // open the scan device dialog
    QString dev = m_ksanew->selectDevice(nullptr);
    if (m_ksanew->openDevice(dev) == false) {
        // could not open a scanner
        KMessageBox::error(nullptr, i18n("Opening the selected scanner failed."));
    }
    else {
        updateWindowTitle(dev, m_ksanew->deviceVendor(), m_ksanew->deviceModel());
        m_deviceName = dev;
        m_deviceModel = m_ksanew->deviceModel();
        m_deviceVendor = m_ksanew->deviceVendor();
    }
}

// Pops up message box similar to what perror() would print
//************************************************************
static void perrorMessageBox(const QString &text)
{
    if (errno != 0) {
        KMessageBox::error(nullptr, i18n("%1: %2", text, QString::fromLocal8Bit(strerror(errno))));
    }
    else {
        KMessageBox::error(nullptr, text);
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
    QString format = saving.readEntry("ImgFormat", "image/png");
    int index = m_settingsUi.imgFormat->findData(format);
    if (index >= 0) {
        m_settingsUi.imgFormat->setCurrentIndex(index);
    }

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
        saving.writeEntry("ImgFormat", m_settingsUi.imgFormat->currentData().toString());
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
        m_saveLocation->setImageFormatIndex(m_settingsUi.imgFormat->currentIndex());

        m_firstImage = true;
    }
    else {
        //Forget Changes
        readSettings();
    }
}

void Skanlite::imageReady(const QImage &image)
{
    // save the image data
    m_img = image;
    if (m_settingsUi.showB4Save->isChecked() == true) {
        // show the image in the preview
        m_showImgDialog->setQImage(&m_img);
        m_showImgDialog->zoom2Fit();
        m_showImgDialog->exec();
        // save has been done as a result of save or then we got cancel
    }
    else {
        saveImage();
    }
}

bool urlExists(const QUrl& url)
{
    if (url.isLocalFile()) {
        if (!QFileInfo::exists(url.toLocalFile())) {
            return false;
        }
    }
    else {
        KIO::StatJob *statJob = KIO::statDetails(url, KIO::StatJob::DestinationSide, KIO::StatNoDetails);
        KJobWidgets::setWindow(statJob, QApplication::activeWindow());
        if (!statJob->exec()) {
            return false;
        }
    }
    return true;
}

void Skanlite::saveImage()
{
    QUrl dirUrl = m_saveLocation->folderUrl();
    bool dirExists = urlExists(dirUrl);

    // Ask the first time if we are in "ask on first" mode
    if (m_settingsUi.saveModeCB->currentIndex() == SaveModeAskFirst) {
        while (m_firstImage || !dirExists) {
            m_saveLocation->setOpenRequesterOnShow(!dirExists);
            if (m_saveLocation->exec() != QFileDialog::Accepted) {
                m_ksanew->scanCancel(); // In case we are cancelling a document feeder scan
                return;
            }
            dirUrl = m_saveLocation->folderUrl();
            dirExists = urlExists(dirUrl); // check that we actually got an existing folder
            m_firstImage = false;
        }
    }
    else if (!dirExists) {
        // The save-folder from settings does not exist! Use the users home directory.
        dirUrl = QUrl::fromUserInput(QDir::homePath() + QLatin1Char('/'));
        m_saveLocation->setFolderUrl(dirUrl);
    }

    QString prefix = m_saveLocation->imagePrefix();
    QString imageMimetype = m_saveLocation->imageMimetype();
    int fileNumber = m_saveLocation->startNumber();
    QStringList filterList;

    if ((m_img.format() == QImage::Format_Grayscale16) ||
        (m_img.format() == QImage::Format_RGBX64))
    {
        filterList = m_filter16BitList;
        if (imageMimetype != QLatin1String("image/png") && imageMimetype != QLatin1String("image/tiff")) {
            imageMimetype = QStringLiteral("image/png");
            KMessageBox::information(this, i18n("The image will be saved in the PNG format, as the selected image type does not support saving 16 bit color images."));
        }
    } else {
        filterList = m_filterList;
    }

    // find next available file name for name suggestion
    QUrl fileUrl;
    QString fname;
    for (int i = fileNumber; i <= m_saveLocation->startNumberMax(); ++i) {
        fname = QStringLiteral("%1%2.%3")
                .arg(prefix)
                .arg(i, 4, 10, QLatin1Char('0'))
                .arg(m_saveLocation->imageSuffix());

        fileUrl = dirUrl;
        fileUrl.setPath(fileUrl.path() + fname);
        fileUrl = fileUrl.adjusted(QUrl::NormalizePathSegments);
        if (!urlExists(fileUrl)) {
            break;
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

        saveDialog.setMimeTypeFilters(filterList);
        saveDialog.selectMimeTypeFilter(imageMimetype);

        if (saveDialog.exec() != QFileDialog::Accepted) {
            return;
        }

        fileUrl = saveDialog.selectedUrls().at(0);
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
        fileFormat = QStringLiteral("png");
    }
    if (suffix == QLatin1String("pdf")) {
        fileFormat = QStringLiteral("pdf");
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

    SkanliteImageSaver *imageSaver = new SkanliteImageSaver(this);
    connect(imageSaver, &SkanliteImageSaver::imageSaved, this, &Skanlite::imageSaved);
    connect(imageSaver, &SkanliteImageSaver::finished, &SkanliteImageSaver::deleteLater);

    imageSaver->saveQImage(fileUrl, localName, m_img, fileFormat, quality);

    m_showImgDialog->blockSignals(true);
    m_showImgDialog->close(); // calling close() on a closed window does nothing.
    // NOTE we need to block the signals since close() will emit rejected()
    m_showImgDialog->blockSignals(false);

    // Disable parts of the interface and indicate that we are saving the image
    m_currentSaveUrl = fileUrl;
    m_ksanew->setDisabled(true);
    m_saveProgressBar->setMaximum(0);
    m_saveProgressBar->setValue(0);
    m_saveProgressBar->setVisible(true);
    m_saveUpdateTimer.start();

    // Save the file base name without number
    QString baseName = QFileInfo(fileUrl.fileName()).completeBaseName();
    while ((!baseName.isEmpty()) && (baseName[baseName.size() - 1].isNumber())) {
        baseName.remove(baseName.size() - 1, 1);
    }
    m_saveLocation->setImagePrefix(baseName);

    // Save the number
    if (fileNumber) {
        m_saveLocation->setStartNumber(fileNumber + 1);
    }

    if (m_settingsUi.saveModeCB->currentIndex() == SaveModeManual) {
        // Save last used dir, prefix and suffix.
        m_saveLocation->setFolderUrl(KIO::upUrl(fileUrl));
        m_saveLocation->setImageFormat(QFileInfo(fileUrl.fileName()).suffix());
    }


}

void Skanlite::updateSaveProgress()
{
    QFileInfo saveInfo(m_currentSaveUrl.toLocalFile());
    quint64 size = saveInfo.size()/1024;
    m_saveProgressBar->setMaximum(size);
    m_saveProgressBar->setValue(size);
}

void Skanlite::imageSaved(const QUrl &fileUrl, const QString &localName, bool success)
{
    if (!success) {
        perrorMessageBox(i18n("Failed to save image"));
        return;
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
            KMessageBox::error(nullptr, i18n("Failed to upload image"));
        }
        else {
            Q_EMIT m_dbusInterface.imageSaved(fileUrl.toString());
        }
    }
    else {
        Q_EMIT m_dbusInterface.imageSaved(localName);
    }
    m_ksanew->setDisabled(false);
    m_ksanew->setFocus();
    m_saveUpdateTimer.stop();
    m_saveProgressBar->setVisible(false);
}

void writeScannerOptions(const QString &groupName, const QMap <QString, QString> &opts)
{
    KConfigGroup options(KSharedConfig::openStateConfig(), groupName);
    QMap<QString, QString>::const_iterator it = opts.constBegin();
    while (it != opts.constEnd()) {
        options.writeEntry(it.key(), it.value());
        ++it;
    }
    options.sync();
}

void readScannerOptions(const QString &groupName, QMap <QString, QString> &opts)
{
    KConfigGroup scannerOptions(KSharedConfig::openStateConfig(), groupName);
    opts = scannerOptions.entryMap();
}

void Skanlite::saveScannerOptions()
{
    KConfigGroup saving(KSharedConfig::openConfig(), "Image Saving");
    saving.writeEntry("NumberStartsFrom", m_saveLocation->startNumber());

    if (!m_ksanew) {
        return;
    }

    if (!m_deviceName.isEmpty()) {
        KConfigGroup options(KSharedConfig::openStateConfig(), QStringLiteral("Options For %1").arg(m_deviceName));
        QMap <QString, QString> opts;
        m_ksanew->getOptVals(opts);
        writeScannerOptions(QStringLiteral("Options For %1").arg(m_deviceName), opts);
    }
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
    if (!m_deviceName.isEmpty()) {
        KConfigGroup saving(KSharedConfig::openConfig(), "Image Saving");
        m_saveLocation->setStartNumber(saving.readEntry("NumberStartsFrom", 1));

        if (!m_ksanew) {
            return;
        }

        QMap <QString, QString> opts;
        readScannerOptions(QStringLiteral("Options For %1").arg(m_deviceName), opts);
        applyScannerOptions(opts);
    }
}

void Skanlite::alertUser(int type, const QString &strStatus)
{
    switch (type) {
    case KSaneWidget::ErrorGeneral:
        KMessageBox::error(nullptr, strStatus, QStringLiteral("Skanlite Test"));
        break;
    default:
        KMessageBox::information(nullptr, strStatus, QStringLiteral("Skanlite Test"));
    }
}

void Skanlite::buttonPressed(const QString &optionName, const QString &optionLabel, bool pressed)
{
    qCDebug(SKANLITE_LOG) << "Button" << optionName << optionLabel << ((pressed) ? "pressed" : "released");
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
    for (const QString &s : settings) {
        int i = s.lastIndexOf(QLatin1Char('='));
        opts[s.left(i)] = s.right(s.length()-i-1);
    }
}

static const auto selectionSettings = { QLatin1String("tl-x"), QLatin1String("tl-y"),
                                        QLatin1String("br-x"), QLatin1String("br-y") };

void filterSelectionSettings(QMap<QString, QString> &opts)
{
    for (const auto &s : selectionSettings) {
        opts.remove(s);
    }
}

bool containsSelectionSettings(const QMap<QString, QString> &opts)
{
    for (const auto &s : selectionSettings) {
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

void Skanlite::updateWindowTitle(const QString &deviceName, const QString &deviceVendor, const QString &deviceModel)
{
    if (!deviceVendor.isEmpty() &&  !deviceModel.isEmpty()) {
        setWindowTitle(i18nc("@title:window %1 = scanner maker, %2 = scanner model", "%1 %2 - Skanlite", deviceVendor, deviceModel));
    } else if (!deviceName.isEmpty()) {
        setWindowTitle(i18nc("@title:window %1 = scanner device", "%1 - Skanlite", deviceName));
    } else {
        setWindowTitle(i18n("Skanlite"));
    }
}

// D-Bus interface related slots

void Skanlite::getScannerOptions()
{
    if (!m_deviceName.isEmpty()) {
        QMap <QString, QString> opts;
        m_ksanew->getOptVals(opts);
        m_dbusInterface.setReply(serializeScannerOptions(opts));
    }
}

void Skanlite::setScannerOptions(const QStringList &options, bool ignoreSelection)
{
    if (!m_deviceName.isEmpty()) {
        QMap <QString, QString> opts;
        deserializeScannerOptions(options, opts);
        processSelectionOptions(opts, ignoreSelection);
        applyScannerOptions(opts);
    }
}


void Skanlite::getDefaultScannerOptions()
{
    m_dbusInterface.setReply(serializeScannerOptions(m_defaultScanOpts));
}

static const QLatin1String defaultProfileGroup("Options For %1 - Profile %2"); // 1 - device, 2 - arg

void Skanlite::saveScannerOptionsToProfile(const QStringList &options, const QString &profile, bool ignoreSelection)
{
    if (!m_deviceName.isEmpty()) {
        QMap <QString, QString> opts;
        deserializeScannerOptions(options, opts);
        processSelectionOptions(opts, ignoreSelection);
        writeScannerOptions(QString(defaultProfileGroup).arg(m_deviceName, profile), opts);
    }
}

void Skanlite::switchToProfile(const QString &profile, bool ignoreSelection)
{
    if (!m_deviceName.isEmpty()) {
        QMap <QString, QString> opts;
        readScannerOptions(QString(defaultProfileGroup).arg(m_deviceName, profile), opts);

        if (opts.empty()) {
            opts = m_defaultScanOpts;
        }

        processSelectionOptions(opts, ignoreSelection);
        applyScannerOptions(opts);
    }
}

void Skanlite::getDeviceName()
{
    if (!m_deviceName.isEmpty()) {
        m_dbusInterface.setReply(QStringList(m_deviceName));
    }
}

void Skanlite::getSelection()
{
    if (!m_deviceName.isEmpty()) {
        QMap <QString, QString> opts;
        m_ksanew->getOptVals(opts);

        QStringList reply;
        for (const auto &key : selectionSettings ) {
            if (opts.contains(key)) {
                reply.append(key + QLatin1Char('=') + opts[key]);
            }
        }
        m_dbusInterface.setReply(reply);
    }
}

void Skanlite::setSelection(const QStringList &options)
{ // here options contains selection related subset of options
    setScannerOptions(options, false);
}
