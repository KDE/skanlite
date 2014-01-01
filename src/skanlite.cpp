/* ============================================================
*
* Copyright (C) 2007-2012 by Kåre Särs <kare.sars@iki .fi>
* Copyright (C) 2009 by Arseniy Lartsev <receive-spam at yandex dot ru>
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
#include "skanlite.moc"

#include "KSaneImageSaver.h"
#include "SaveLocation.h"


#include <QApplication>
#include <QScrollArea>
#include <QStringList>

#include <KAboutApplicationDialog>
#include <KAction>
#include <KComponentData>
#include <KDebug>
#include <KFileDialog>
#include <KUrl>
#include <KGlobal>
#include <KMessageBox>
#include <KStandardAction>
#include <KImageIO>
#include <kdeversion.h>
#include <kio/netaccess.h>
#include <KTemporaryFile>

#include <errno.h>

Skanlite::Skanlite(const QString &device, QWidget *parent)
    : KDialog(parent)
{
    setAttribute(Qt::WA_DeleteOnClose);

    setButtons(KDialog::Help | KDialog::User2 | KDialog::User1 | KDialog::Close);

    setButtonText(KDialog::User1, i18n("Settings"));
    setButtonIcon(KDialog::User1, KIcon("configure"));
    setButtonText(KDialog::User2, i18n("About"));
    setHelp("index", "skanlite");

    m_firstImage = true;

    m_ksanew = new KSaneIface::KSaneWidget(this);
    connect(m_ksanew, SIGNAL(imageReady(QByteArray &, int, int, int, int)),
            this,     SLOT(imageReady(QByteArray &, int, int, int, int)));
    connect(m_ksanew, SIGNAL(availableDevices(QList<KSaneWidget::DeviceInfo>)),
            this,     SLOT(availableDevices(QList<KSaneWidget::DeviceInfo>)));
    connect(m_ksanew, SIGNAL(userMessage(int, QString)),
            this,     SLOT(alertUser(int, QString)));
    connect(m_ksanew, SIGNAL(buttonPressed(QString, QString, bool)),
            this,     SLOT(buttonPressed(QString, QString, bool)));

    setMainWidget(m_ksanew);

    m_ksanew->initGetDeviceList();

    // read the size here...
    KConfigGroup window(KGlobal::config(), "Window");
    QSize rect = window.readEntry("Geometry", QSize(740,400));
    resize(rect);

    connect (this, SIGNAL(closeClicked()), this, SLOT(saveWindowSize()));
    connect (this, SIGNAL(closeClicked()), this, SLOT(saveScannerOptions()));
    connect (this, SIGNAL(user1Clicked()), this, SLOT(showSettingsDialog()));
    connect (this, SIGNAL(user2Clicked()), this, SLOT(showAboutDialog()));

    // Create the settings dialog
    m_settingsDialog = new KDialog(this);
    m_settingsDialog->setButtons(KDialog::Ok | KDialog::Cancel);

    QWidget *settingsWidget = new QWidget(m_settingsDialog);
    m_settingsUi.setupUi(settingsWidget);
    m_settingsUi.revertOptions->setIcon(KIcon("edit-undo"));
    m_saveLocation = new SaveLocation(this);

    // add the supported image types
    m_filterList = KImageIO::mimeTypes(KImageIO::Writing);
    // Put first class citizens at first place
    m_filterList.removeAll("image/jpeg");
    m_filterList.removeAll("image/tiff");
    m_filterList.removeAll("image/png");
    m_filterList.insert(0, "image/png");
    m_filterList.insert(1, "image/jpeg");
    m_filterList.insert(2, "image/tiff");

    m_filter16BitList << "image/png";
    //m_filter16BitList << "image/tiff";

    QString mime;
    QStringList type;
    foreach (mime , m_filterList) {
        type = KImageIO::typeForMime(mime);
        if (type.size() > 0) {
            m_typeList << type.first();
        }
    }
    m_settingsUi.imgFormat->addItems(m_typeList);
    m_saveLocation->u_imgFormat->addItems(m_typeList);

    m_settingsDialog->setMainWidget(settingsWidget);
    m_settingsDialog->setWindowTitle(i18n("Skanlite Settings"));


    connect(m_settingsUi.getDirButton, SIGNAL(clicked()), this, SLOT(getDir()));
    connect(m_settingsUi.revertOptions, SIGNAL(clicked()), this, SLOT(defaultScannerOptions()));
    readSettings();

    // default directory for the save dialog
    m_saveLocation->u_saveDirLEdit->setText(m_settingsUi.saveDirLEdit->text());
    m_saveLocation->u_imgPrefix->setText(m_settingsUi.imgPrefix->text());
    m_saveLocation->u_imgFormat->setCurrentItem(m_settingsUi.imgFormat->currentText());

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
            m_deviceName = QString("%1:%2").arg(m_ksanew->make()).arg(m_ksanew->model());
        }
    }
    else {
        setWindowTitle(i18nc("@title:window %1 = scanner device", "%1 - Skanlite", device));
        m_deviceName = device;
    }

    addAction(KStandardAction::quit(qApp, SLOT(quit()), this));

    // prepare the Show Image Dialog
    m_showImgDialog = new KDialog(this);
    m_showImgDialog->setButtons(KDialog::User1 | KDialog::Close);
    m_showImgDialog->setButtonText(KDialog::User1, i18n("Save"));
    m_showImgDialog->setButtonIcon(KDialog::User1, KIcon("document-save"));
    m_showImgDialog->setDefaultButton(KDialog::User1);
    m_showImgDialog->resize(640,  480);
    m_showImgDialog->setMainWidget(&m_imageViewer);
    connect(m_showImgDialog, SIGNAL(user1Clicked()), this, SLOT(saveImage()));


    // prepare the save dialog
    m_saveDialog = new KFileDialog(m_settingsUi.saveDirLEdit->text(), QString(), this);
    m_saveDialog->setOperationMode(KFileDialog::Saving);
    m_saveDialog->setMode(KFile::File);
    m_saveDialog->setCaption(i18n("New Image File Name"));

    // save the default sane options for later use
    m_ksanew->getOptVals(m_defaultScanOpts);

    // load saved options
    loadScannerOptions();

    m_ksanew->initGetDeviceList();

    m_firstImage = true;
}

//************************************************************
void Skanlite::closeEvent(QCloseEvent *event)
{
    saveWindowSize();
    saveScannerOptions();
    event->accept();
}

//************************************************************
void Skanlite::saveWindowSize()
{
    KConfigGroup window(KGlobal::config(), "Window");
    window.writeEntry("Geometry", size());
    window.sync();
}
// Pops up message box similar to what perror() would print
//************************************************************
static void perrorMessageBox(const QString &text)
{
    if (errno != 0) {
        KMessageBox::sorry(0, i18n("%1: %2", text, QString::fromLocal8Bit(strerror(errno))));
    }
    else {
        KMessageBox::sorry(0, text);
    }
}

//************************************************************
void Skanlite::readSettings(void)
{
    // enable the widgets to allow modifying
    m_settingsUi.setQuality->setChecked(true);
    m_settingsUi.setPreviewDPI->setChecked(true);

    // read the saved parameters
    KConfigGroup saving(KGlobal::config(), "Image Saving");
    m_settingsUi.saveModeCB->setCurrentIndex(saving.readEntry("SaveMode", (int)SaveModeManual));
    if (m_settingsUi.saveModeCB->currentIndex() != SaveModeAskFirst) m_firstImage = false;
    m_settingsUi.saveDirLEdit->setText(saving.readEntry("Location", QDir::homePath()));
    m_settingsUi.imgPrefix->setText(saving.readEntry("NamePrefix", i18nc("prefix for auto naming", "Image-")));
    m_settingsUi.imgFormat->setCurrentItem(saving.readEntry("ImgFormat", "png"));
    m_settingsUi.imgQuality->setValue(saving.readEntry("ImgQuality", 90));
    m_settingsUi.setQuality->setChecked(saving.readEntry("SetQuality", false));
    m_settingsUi.showB4Save->setChecked(saving.readEntry("ShowBeforeSave", true));

    KConfigGroup general(KGlobal::config(), "General");
    m_settingsUi.previewDPI->setCurrentItem(general.readEntry("PreviewDPI", "100"), true);
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

//************************************************************
void Skanlite::showSettingsDialog(void)
{
    readSettings();

    // show the dialog
    if (m_settingsDialog->exec()) {
        // save the settings
        KConfigGroup saving(KGlobal::config(), "Image Saving");
        saving.writeEntry("SaveMode", m_settingsUi.saveModeCB->currentIndex());
        saving.writeEntry("Location", m_settingsUi.saveDirLEdit->text());
        saving.writeEntry("NamePrefix", m_settingsUi.imgPrefix->text());
        saving.writeEntry("ImgFormat", m_settingsUi.imgFormat->currentText());
        saving.writeEntry("SetQuality", m_settingsUi.setQuality->isChecked());
        saving.writeEntry("ImgQuality", m_settingsUi.imgQuality->value());
        saving.writeEntry("ShowBeforeSave", m_settingsUi.showB4Save->isChecked());
        saving.sync();

        KConfigGroup general(KGlobal::config(), "General");
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
        m_saveLocation->u_saveDirLEdit->setText(m_settingsUi.saveDirLEdit->text());
        m_saveLocation->u_imgPrefix->setText(m_settingsUi.imgPrefix->text());
        m_saveLocation->u_imgFormat->setCurrentItem(m_settingsUi.imgFormat->currentText());

        m_firstImage = true;
    }
    else {
        //Forget Changes
        readSettings();
    }
}


//************************************************************
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
        m_showImgDialog->setDefaultButton(KDialog::User1);
        m_showImgDialog->exec();
        // save has been done as a result of save or then we got cancel
    }
    else {
        m_img = QImage(); // clear the image to ensure we save the correct one.
        saveImage();
    }
}

//************************************************************
void Skanlite::saveImage()
{
    // ask the first time if we are in "ask on first" mode
    if ((m_settingsUi.saveModeCB->currentIndex() == SaveModeAskFirst) && m_firstImage) {
        if (m_saveLocation->exec() != KFileDialog::Accepted) return;
        m_firstImage = false;
    }

    QString dir = m_saveLocation->u_saveDirLEdit->text();
    QString prefix = m_saveLocation->u_imgPrefix->text();
    QString type = m_saveLocation->u_imgFormat->currentText().toLower();
    int fileNumber = m_saveLocation->u_numStartFrom->value();
    QStringList filterList = m_filterList;
    if ((m_format==KSaneIface::KSaneWidget::FormatRGB_16_C) ||
        (m_format==KSaneIface::KSaneWidget::FormatGrayScale16))
    {
        filterList = m_filter16BitList;
        if (type != "png") {
            type = "png";
            KMessageBox::information(this, i18n("The image will be saved in the PNG format, as Skanlite only supports saving 16 bit color images in the PNG format."));
        }
    }


    // find next available file name for name suggestion
    KUrl fileUrl;
    QString fname;
    for (int i=fileNumber; i<=m_saveLocation->u_numStartFrom->maximum(); ++i) {
        fname = QString("%1%2.%3")
        .arg(prefix)
        .arg(i, 4, 10, QChar('0'))
        .arg(type);

        fileUrl = KUrl(QString("%1/%2").arg(dir).arg(fname));
        if (fileUrl.isLocalFile()) {
            if (!QFileInfo(fileUrl.toLocalFile()).exists()) {
                break;
            }
        }
        else {
            if (!KIO::NetAccess::exists(fileUrl, true, this)) {
                break;
            }
        }
    }

    if (m_settingsUi.saveModeCB->currentIndex() == SaveModeManual) {
        // ask for a filename if requested.
        m_saveDialog->setSelection(fileUrl.url());
        m_saveDialog->setMimeFilter(filterList, "image/"+type);

        do {
            if (m_saveDialog->exec() != KFileDialog::Accepted) return;

            fileUrl = m_saveDialog->selectedUrl();
            //kDebug() << "-----Save-----" << fname;

            if (KIO::NetAccess::exists(fileUrl, true, this)) {
                if (KMessageBox::warningContinueCancel(this,
                    i18n("Do you want to overwrite \"%1\"?", fileUrl.fileName()),
                     QString(),
                     KGuiItem(i18n("Overwrite")),
                     KStandardGuiItem::cancel(),
                     QString("editorWindowSaveOverwrite")
                     ) ==  KMessageBox::Continue)
                     {
                         break;
                     }
            }
            else {
                break;
            }
        } while (1);
    }

    m_firstImage = false;

    // Get the quality
    int quality = -1;
    if (m_settingsUi.setQuality->isChecked()) {
        quality = m_settingsUi.imgQuality->value();
    }

    QFileInfo fileInfo(fileUrl.pathOrUrl());

    //kDebug() << "suffix" << fileInfo.suffix() << "localFile" << fileUrl.pathOrUrl();
    fname = fileUrl.pathOrUrl();
    KTemporaryFile tmp;
    if (!fileUrl.isLocalFile()) {
        tmp.setSuffix('.'+fileInfo.suffix());
        tmp.open();
        fname = tmp.fileName();
        tmp.close(); // we just want the filename
    }

    // Save
    if ((m_format==KSaneIface::KSaneWidget::FormatRGB_16_C) ||
        (m_format==KSaneIface::KSaneWidget::FormatGrayScale16))
    {
        KSaneImageSaver saver;
        if (saver.savePngSync(fname, m_data, m_width, m_height, m_format)) {
            m_showImgDialog->close(); // closing the window if it is closed should not be a problem.
        }
        else {
            perrorMessageBox(i18n("Failed to save image"));
        }
    }
    else  {
        // create the image if needed.
        if (m_img.width() < 1) {
            m_img = m_ksanew->toQImage(m_data, m_width, m_height, m_bytesPerLine, (KSaneIface::KSaneWidget::ImageFormat)m_format);
        }
        if (m_img.save(fname, 0, quality)) {
            m_showImgDialog->close(); // calling close() on a closed window does nothing.
        }
        else {
            perrorMessageBox(i18n("Failed to save image"));
        }
    }

    if (!fileUrl.isLocalFile()) {
        if (!KIO::NetAccess::upload( fname, fileUrl, this )) {
            KMessageBox::sorry(0, i18n("Failed to upload image"));
        }
    }

    // Save the file base name without number
    QString baseName = fileInfo.completeBaseName();
    while ((baseName.size() > 1) && (baseName[baseName.size()-1].isNumber())) {
        baseName.remove(baseName.size()-1, 1);
    }
    m_saveLocation->u_imgPrefix->setText(baseName);

    // Save the number
    QString fileNumStr = fileInfo.completeBaseName();
    fileNumStr.remove(baseName);
    fileNumber = fileNumStr.toInt();
    if (fileNumber) {
        m_saveLocation->u_numStartFrom->setValue(fileNumber+1);
    }

    if (m_settingsUi.saveModeCB->currentIndex() == SaveModeManual) {
        // Save last used dir, prefix and suffix.
        m_saveLocation->u_saveDirLEdit->setText(fileUrl.upUrl().pathOrUrl());
        m_saveLocation->u_imgFormat->setCurrentItem(fileInfo.suffix());
    }
}


//************************************************************
void Skanlite::getDir(void)
{
    QString dir = KFileDialog::getExistingDirectory(KUrl(m_settingsUi.saveDirLEdit->text()));
    if (!dir.isEmpty()) {
        m_settingsUi.saveDirLEdit->setText(dir);
    }
}

//************************************************************
void Skanlite::showAboutDialog(void)
{
    KAboutApplicationDialog(KGlobal::mainComponent().aboutData(), 0).exec();
}

//************************************************************
void Skanlite::saveScannerOptions()
{
    KConfigGroup saving(KGlobal::config(), "Image Saving");
    saving.writeEntry("NumberStartsFrom", m_saveLocation->u_numStartFrom->value());

    if (!m_ksanew) return;

    KConfigGroup options(KGlobal::config(), QString("Options For %1").arg(m_deviceName));
    QMap <QString, QString> opts;
    m_ksanew->getOptVals(opts);
    QMap<QString, QString>::const_iterator it = opts.constBegin();
    while (it != opts.constEnd()) {
        options.writeEntry(it.key(), it.value());
        ++it;
    }
    options.sync();
}

//************************************************************
void Skanlite::defaultScannerOptions()
{
    if (!m_ksanew) return;

    m_ksanew->setOptVals(m_defaultScanOpts);
}

//************************************************************
void Skanlite::loadScannerOptions()
{
    KConfigGroup saving(KGlobal::config(), "Image Saving");
    m_saveLocation->u_numStartFrom->setValue(saving.readEntry("NumberStartsFrom", 1));

    if (!m_ksanew) return;

    KConfigGroup scannerOptions(KGlobal::config(), QString("Options For %1").arg(m_deviceName));
    m_ksanew->setOptVals(scannerOptions.entryMap());
}

//************************************************************
void Skanlite::availableDevices(const QList<KSaneWidget::DeviceInfo> &deviceList)
{
    for (int i=0; i<deviceList.size(); i++) {
        kDebug() << deviceList[i].name;
    }
}

//************************************************************
void Skanlite::alertUser(int type, const QString &strStatus)
{
    switch (type) {
        case KSaneWidget::ErrorGeneral:
            KMessageBox::sorry(0, strStatus, "Skanlite Test");
            break;
        default:
            KMessageBox::information(0, strStatus, "Skanlite Test");
    }
}

//************************************************************
void Skanlite::buttonPressed(const QString &optionName, const QString &optionLabel, bool pressed)
{
    kDebug() << "Button" << optionName << optionLabel << ((pressed) ? "pressed" : "released");
}
