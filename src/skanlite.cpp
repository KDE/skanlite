/* ============================================================
*
* Copyright (C) 2007-2008 by Kare Sars <kare dot sars at iki dot fi>
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

#include <QApplication>
#include <QScrollArea>

#include <KAboutApplicationDialog>
#include <KAction>
#include <KComponentData>
#include <KDebug>
#include <KFileDialog>
#include <KGlobal>
#include <KMessageBox>
#include <KStandardAction>
#include <kdeversion.h>

// Order of items in save mode combo-box
enum {
    SAVE_MODE_MANUAL = 0,
    SAVE_MODE_ASK_FIRST = 1,
    SAVE_MODE_AUTO = 2
};

#include <errno.h>

Skanlite::Skanlite(const QString &device, QWidget *parent)
    : KDialog(parent)
{
    setButtons(KDialog::Help | KDialog::User1 | KDialog::User2 | KDialog::Close);
    setButtonText(KDialog::User1, i18n("Settings"));
    setButtonIcon(KDialog::User1, KIcon("configure"));
    setButtonText(KDialog::User2, i18n("About"));
    setHelp("index", "skanlite");

    m_ksanew = new KSaneIface::KSaneWidget(this);
    setMainWidget(m_ksanew);

    connect (this, SIGNAL(user1Clicked()), this, SLOT(showSettingsDialog()));
    connect (this, SIGNAL(user2Clicked()), this, SLOT(showAboutDialog()));
    
    // Create the settings dialog
    m_settingsDialog = new KDialog(0);
    m_settingsDialog->setButtons(KDialog::Ok | KDialog::Cancel);

    QWidget *settingsWidget = new QWidget(m_settingsDialog);
    m_settingsUi.setupUi(settingsWidget);

    // add the supported image types
    m_typeList << "PNG";
    m_typeList << "JPG";
    m_typeList << "JPEG";
    m_typeList << "BMP";
    m_typeList << "PPM";
    m_typeList << "XBM";
    m_typeList << "XPM";
    m_settingsUi.imgFormat->addItems(m_typeList);

    m_settingsDialog->setMainWidget(settingsWidget);
    m_settingsDialog->setWindowTitle(i18n("Skanlite Settings"));


    connect(m_settingsUi.getDirButton, SIGNAL(clicked(void)), this, SLOT(setDir(void)));
    readSettings();

    // default dir for save dialog
    m_currentDir = m_settingsUi.saveDirLEdit->text();

    connect(m_ksanew, SIGNAL(imageReady(QByteArray &, int, int, int, int)),
            this, SLOT(imageReady(QByteArray &, int, int, int, int)));

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
    }
    setWindowTitle(m_ksanew->make()+ ' ' + m_ksanew->model() + " - Skanlite");

    addAction(KStandardAction::quit(qApp, SLOT(quit()), this));

    // prepare the Show Image Dialog
    buildShowImage();

    m_firstImage = true;
}

QString getSystemErrorMessage()
{
    if (errno != 0) {
        return QString::fromLocal8Bit(strerror(errno));
    }
    else {
        return QString();
    }
}

// Pops up message box similar to what perror() would print
void perrorMessageBox(const QString &text)
{
    QString errmsg = getSystemErrorMessage();
    if (errmsg.isEmpty()) {
        KMessageBox::sorry(0, text);
    }
    else {
        KMessageBox::sorry(0, text + ": " + errmsg);
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
    m_settingsUi.saveModeCB->setCurrentIndex(saving.readEntry("SaveMode", (int)SAVE_MODE_MANUAL));
    m_settingsUi.saveDirLEdit->setText(saving.readEntry("Location", QDir::homePath()));
    m_settingsUi.imgPrefix->setText(saving.readEntry("NamePrefix", "Image-"));
    for (int i=0; i<m_settingsUi.imgFormat->count(); i++) {
        if (saving.readEntry("ImgFormat", "PNG") == m_settingsUi.imgFormat->itemText(i)) {
            m_settingsUi.imgFormat->setCurrentIndex(i);
            break;
        }
    }
    m_settingsUi.imgQuality->setValue(saving.readEntry("ImgQuality", 90));
    m_settingsUi.setQuality->setChecked(saving.readEntry("SetQuality", false));
    m_settingsUi.showB4Save->setChecked(saving.readEntry("ShowBeforeSave", true));

    #if KDE_IS_VERSION(4, 3, 65)
    KConfigGroup general(KGlobal::config(), "General");
    m_settingsUi.previewDPI->setCurrentItem(general.readEntry("PreviewDPI", "100"), true);
    m_settingsUi.setPreviewDPI->setChecked(general.readEntry("SetPreviewDPI", false));
    
    if (m_settingsUi.setPreviewDPI->isChecked()) {
        m_ksanew->setPreviewResolution(m_settingsUi.previewDPI->currentText().toFloat());
    }
    else {
        m_ksanew->setPreviewResolution(0.0);
    }
    #else
    m_settingsUi.generalGB->hide();
    #endif
    
}

//************************************************************
void Skanlite::showSettingsDialog(void)
{
    readSettings();
    int saveMode = m_settingsUi.saveModeCB->currentIndex();

    // show the dialog
    if (m_settingsDialog->exec()) {
        
        // now save the settings
        KConfigGroup saving(KGlobal::config(), "Image Saving");
        saving.writeEntry("SaveMode", m_settingsUi.saveModeCB->currentIndex());
        saving.writeEntry("Location", m_settingsUi.saveDirLEdit->text());
        saving.writeEntry("NamePrefix", m_settingsUi.imgPrefix->text());
        saving.writeEntry("ImgFormat", m_settingsUi.imgFormat->currentText());
        saving.writeEntry("SetQuality", m_settingsUi.setQuality->isChecked());
        saving.writeEntry("ImgQuality", m_settingsUi.imgQuality->value());
        saving.writeEntry("ShowBeforeSave", m_settingsUi.showB4Save->isChecked());
        if ((saveMode != SAVE_MODE_ASK_FIRST) &&
            (m_settingsUi.saveModeCB->currentIndex() == SAVE_MODE_ASK_FIRST))
        {
            m_firstImage = true;
        }
        saving.sync();
        
        #if KDE_IS_VERSION(4, 3, 65)
        KConfigGroup general(KGlobal::config(), "General");
        general.writeEntry("PreviewDPI", m_settingsUi.previewDPI->currentText());
        general.writeEntry("SetPreviewDPI", m_settingsUi.setPreviewDPI->isChecked());
        general.sync();

        // the previewDPI has to be set here
        if (m_settingsUi.setPreviewDPI->isChecked()) {
            m_ksanew->setPreviewResolution(m_settingsUi.previewDPI->currentText().toFloat());
        }
        else {
            m_ksanew->setPreviewResolution(0.0);
        }
        #endif
    }
    else {
        //Forget Changes
        readSettings();
    }
}


//************************************************************
void Skanlite::buildShowImage(void)
{
    m_showImgDialog = new KDialog(0);
    m_showImgDialog->setButtons(KDialog::User1 | KDialog::Close);
    m_showImgDialog->setButtonText(KDialog::User1, i18n("Save"));
    m_showImgDialog->setButtonIcon(KDialog::User1, KIcon("document-save"));

    m_showImgDialog->setMainWidget(&m_imageViewer);
}


//************************************************************
void Skanlite::imageReady(QByteArray &data, int w, int h, int bpl, int f)
{
    /* copy the image data into img */
    m_img = m_ksanew->toQImage(data, w, h, bpl, (KSaneIface::KSaneWidget::ImageFormat)f);

    if (m_settingsUi.showB4Save->isChecked() == true) {
        m_imageViewer.setQImage(&m_img);

        disconnect(m_showImgDialog, SIGNAL(user1Clicked()), NULL, NULL);
        if (m_settingsUi.saveModeCB->currentIndex() != SAVE_MODE_AUTO) {
            connect (m_showImgDialog, SIGNAL(user1Clicked()), this, SLOT(saveImage()));
        }
        else {
            connect (m_showImgDialog, SIGNAL(user1Clicked()), this, SLOT(autoSaveImage()));
        }
        m_showImgDialog->exec();
    }
    else {
        if (m_settingsUi.saveModeCB->currentIndex() != SAVE_MODE_AUTO) {
            saveImage();
        }
        else {
            autoSaveImage();
        }
    }
}

//************************************************************
void Skanlite::saveImage()
{
    bool askFilename =
      (m_settingsUi.saveModeCB->currentIndex() == SAVE_MODE_MANUAL) || m_firstImage;
    doSaveImage(askFilename);
    m_firstImage = false;
}

//************************************************************
void Skanlite::doSaveImage(bool askFilename)
{
    //QString filter = "*.png\n*.jpg\n*.jpeg\n*.bmp\n*.ppm\n*.xbm\n*";
    QString filter = "image/png image/jpg image/jpeg image/bmp image/ppm image/xbm";

    QFile file;
    QString fname;
    int i;

    // find next available file name for name suggestion
    for (i=1; i<10000; i++) {
        fname = m_currentDir;
        if (fname.endsWith('/') == false) {
            fname += '/';
        }
        fname += m_settingsUi.imgPrefix->text();
        fname += QString().sprintf("%03d.",i);
        fname += m_settingsUi.imgFormat->currentText().toLower();
        file.setFileName(fname);
        if (file.exists() == false) {
            break;
        }
    }

    if (askFilename || file.exists()) {
        do {
            fname = KFileDialog::getSaveFileName(fname, filter);
            //kDebug() << "-----Save-----" << fname;
            
            if (fname.isEmpty()) {
                //kDebug() << "!!!!!!!!!!!!!Nothing to save!!!!!!!!!!!";
                return;
            }
            
            file.setFileName(fname);
            if (file.exists()) {
                if (KMessageBox::warningContinueCancel(this,
                    i18n("Do you want to overwrite \"%1\"?", file.fileName()),
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

    // Save last used dir, but remove the file name.
    m_currentDir = fname.left(fname.lastIndexOf('/'));

    // Figure out Image format
    QString type(m_settingsUi.imgFormat->currentText());
    for (int i=0; i<m_typeList.size(); ++i) {
        if (fname.endsWith(m_typeList.at(i), Qt::CaseInsensitive)) {
            type = m_typeList.at(i);
            break;
        }
    }

    // Get the quality
    int quality = -1;
    if (m_settingsUi.setQuality->isChecked()) {
        quality = m_settingsUi.imgQuality->value();
    }

    // Save
    //kDebug() << "-------" << fname << type.toLatin1() << quality;
    if (m_img.save(fname, type.toLatin1(), quality)) {
        m_showImgDialog->close();
    }
    else {
        perrorMessageBox(i18n("Failed to save image"));
    }

}

//************************************************************
void Skanlite::autoSaveImage()
{
    QFile file;
    QString fname;
    int quality;
    int i;

    if (m_settingsUi.setQuality->isChecked()) {
        quality = m_settingsUi.imgQuality->value();
    }
    else {
        quality = -1;
    }

    for (i=1; i<10000; i++) {
        fname = m_settingsUi.saveDirLEdit->text();
        if (fname.endsWith('/') == false) {
            fname += '/';
        }
        fname += m_settingsUi.imgPrefix->text();
        fname += QString().sprintf("%03d.",i);
        fname += m_settingsUi.imgFormat->currentText().toLower();

        file.setFileName(fname);
        if (file.exists() == false) {
            break;
        }
    }
    if (i==10000) {
        saveImage();
        return;
    }

    if (m_img.save(fname,
                 qPrintable(m_settingsUi.imgFormat->currentText()),
                 quality))
    {
        m_showImgDialog->close();
    }
    else {
        perrorMessageBox(i18n("Failed to save image"));
    }
}

//************************************************************
void Skanlite::setDir(void)
{
    QString dir = KFileDialog::getExistingDirectory(KUrl(m_settingsUi.saveDirLEdit->text()));
    if (!dir.isEmpty()) {
        m_settingsUi.saveDirLEdit->setText(dir);
    }
}

//************************************************************
void Skanlite::showAboutDialog(void)
{
    KAboutApplicationDialog oKAboutDiag(KGlobal::mainComponent().aboutData(), 0);
    oKAboutDiag.exec();
}


