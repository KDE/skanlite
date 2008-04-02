/* ============================================================
 *
 * Copyright (C) 2007-2008 by Kare Sars <kare dot sars at iki dot fi>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) version 3, or any
 * later version accepted by the membership of KDE e.V. (or its
 * successor approved by the membership of KDE e.V.), which shall
 * act as a proxy defined in Section 6 of version 3 of the license.
 *
 * This appication is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * ============================================================ */

#include "skanlite.h"
#include "skanlite.moc"

#include <QScrollArea>

#include <KFileDialog>
#include <KMessageBox>
#include <KDebug>
#include <KGlobal>
#include <KAboutApplicationDialog>
#include <KComponentData>

Skanlite::Skanlite(const QString &device, QWidget *parent)
    : KDialog(parent)
{
    setButtons(KDialog::Help | KDialog::User1 | KDialog::User2 | KDialog::Close);
    setButtonText(KDialog::User1, i18n("Settings"));
    setButtonIcon(KDialog::User1, KIcon("configure"));
    setButtonText(KDialog::User2, i18n("About"));
    //setButtonIcon(KDialog::User2, KIcon(""));
    setHelp("skanlite");

    ksanew = new KSaneIface::KSaneWidget(this);
    setMainWidget(ksanew);

    connect (this, SIGNAL(user1Clicked()), this, SLOT(showSettingsDialog()));
    connect (this, SIGNAL(user2Clicked()), this, SLOT(showAboutDialog()));

    // Create the settings dialog
    settingsDialog = new KDialog(0);
    settingsDialog->setButtons(KDialog::Ok | KDialog::Cancel);

    QWidget *settingsWidget = new QWidget(settingsDialog);
    settingsUi.setupUi(settingsWidget);

    // add the supported image types
    typeList << "PNG";
    typeList << "JPG";
    typeList << "JPEG";
    typeList << "BMP";
    typeList << "PPM";
    typeList << "XBM";
    typeList << "XPM";
    settingsUi.imgFormat->addItems(typeList);

    settingsDialog->setMainWidget(settingsWidget);


    connect(settingsUi.getDirButton, SIGNAL(clicked(void)), this, SLOT(setDir(void)));
    readSettings();

    if (settingsUi.dontAskOnStart->isChecked() == false) {
        showSettingsDialog();
    }

    // default dir for save dialog
    currentDir = settingsUi.saveDirLEdit->text();

    connect(ksanew, SIGNAL(imageReady(QByteArray &, int, int, int, int)),
            this, SLOT(imageReady(QByteArray &, int, int, int, int)));

    // open the scan device
    if (ksanew->openDevice(device) == false) {
        QString dev = ksanew->selectDevice(0);
        if (dev.isEmpty()) {
            // either no scanner was found or then cancel was pressed.
            exit(0);
        }
        if (ksanew->openDevice(dev) == false) {
            // could not open a scanner
            KMessageBox::sorry(0, i18n("Opening the selected scanner failed!"));
            exit(1);
        }
    }
    setWindowTitle(ksanew->make()+ ' ' + ksanew->model() + " - Skanlite");

    // prepare the Show Image Dialog
    buildShowImage();

}

//************************************************************
void Skanlite::readSettings(void)
{
    // enable the widgets to allow modifying
    settingsUi.setQuality->setChecked(true);

    // read the saved parameters
    KConfigGroup asave(KGlobal::config(), "AutoSave");
    settingsUi.saveDirLEdit->setText(asave.readEntry("Location", "./"));
    settingsUi.imgPrefix->setText(asave.readEntry("NamePrefix", "Image-"));
    for (int i=0; i<settingsUi.imgFormat->count(); i++) {
        if (asave.readEntry("ImgFormat", "PNG") == settingsUi.imgFormat->itemText(i)) {
            settingsUi.imgFormat->setCurrentIndex(i);
            break;
        }
    }
    settingsUi.imgQuality->setValue(asave.readEntry("ImgQuality", 90));
    settingsUi.setQuality->setChecked(asave.readEntry("SetQuality", false));
    settingsUi.showB4Save->setChecked(asave.readEntry("ShowB4Save", true));

    KConfigGroup general(KGlobal::config(), "General");
    settingsUi.saveModeCB->setCurrentIndex(general.readEntry("SaveModeManual", true) ? 0:1);
    settingsUi.dontAskOnStart->setChecked(general.readEntry("DontAskOnStart", false));

}

//************************************************************
void Skanlite::showSettingsDialog(void)
{
    readSettings();

    // show the dialog
    if (settingsDialog->exec()) {
        KConfigGroup general(KGlobal::config(), "General");
        general.writeEntry("SaveModeManual", (settingsUi.saveModeCB->currentIndex() == 0));
        general.writeEntry("DontAskOnStart", settingsUi.dontAskOnStart->isChecked());

        KConfigGroup asave(KGlobal::config(), "AutoSave");
        asave.writeEntry("Location", settingsUi.saveDirLEdit->text());
        asave.writeEntry("NamePrefix", settingsUi.imgPrefix->text());
        asave.writeEntry("ImgFormat", settingsUi.imgFormat->currentText());
        asave.writeEntry("SetQuality", settingsUi.setQuality->isChecked());
        asave.writeEntry("ImgQuality", settingsUi.imgQuality->value());
        asave.writeEntry("ShowB4Save", settingsUi.showB4Save->isChecked());
    }
    else {
        //Forget Changes
        readSettings();
    }
}


//************************************************************
void Skanlite::buildShowImage(void)
{
    showImgDialog = new KDialog(0);
    showImgDialog->setButtons(KDialog::User1 | KDialog::Close);
    showImgDialog->setButtonText(KDialog::User1, i18n("Save"));
    showImgDialog->setButtonIcon(KDialog::User1, KIcon("document-save"));

    QScrollArea *img_area = new QScrollArea(showImgDialog);
    imgLabel = new QLabel(showImgDialog);
    img_area->setWidget(imgLabel);

    showImgDialog->setMainWidget(img_area);
}


//************************************************************
void Skanlite::imageReady(QByteArray &data, int w, int h, int bpl, int f)
{
    /* copy the image data into img */
    ksanew->makeQImage(data, w, h, bpl, (KSaneIface::KSaneWidget::ImageFormat)f, img);

    if (settingsUi.showB4Save->isChecked() == true) {
        imgLabel->setPixmap(QPixmap::fromImage(img));
        imgLabel->resize(img.size());

        disconnect(showImgDialog, SIGNAL(user1Clicked()), NULL, NULL);
        if (settingsUi.saveModeCB->currentIndex() == 0) {
            connect (showImgDialog, SIGNAL(user1Clicked()), this, SLOT(saveImage()));
        }
        else {
            connect (showImgDialog, SIGNAL(user1Clicked()), this, SLOT(autoSaveImage()));
        }
        showImgDialog->exec();
    }
    else {
        if (settingsUi.saveModeCB->currentIndex() == 0) {
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
    //QString filter = "*.png\n*.jpg\n*.jpeg\n*.bmp\n*.ppm\n*.xbm\n*";
    QString filter = "image/png image/jpg image/jpeg image/bmp image/ppm image/xbm";

    QFile file;
    QString fname;
    int i;

    // find next available file name for name suggestion
    for (i=1; i<10000; i++) {
        fname = currentDir;
        if (fname.endsWith('/') == false) {
            fname += '/';
        }
        fname += settingsUi.imgPrefix->text();
        fname += QString().sprintf("%03d.",i);
        fname += settingsUi.imgFormat->currentText().toLower();
        file.setFileName(fname);
        if (file.exists() == false) {
            break;
        }
    }

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

    // Save last used dir
    currentDir = fname;

    // Figure out Image format
    QString type(settingsUi.imgFormat->currentText());
    for (int i=0; i<typeList.size(); ++i) {
        if (fname.endsWith(typeList.at(i), Qt::CaseInsensitive)) {
            type = typeList.at(i);
            break;
        }
    }

    // Get the quality
    int quality = -1;
    if (settingsUi.setQuality->isChecked()) {
        quality = settingsUi.imgQuality->value();
    }

    // Save
    //kDebug() << "-------" << fname << type.toLatin1() << quality;
    if (img.save(fname, type.toLatin1(), quality)) {
        showImgDialog->close();
    }
    else {
        KMessageBox::sorry(0, i18n("Saving Failed!"));
    }

}

//************************************************************
void Skanlite::autoSaveImage()
{
    QFile file;
    QString fname;
    int quality;
    int i;

    if (settingsUi.setQuality->isChecked()) {
        quality = settingsUi.imgQuality->value();
    }
    else {
        quality = -1;
    }

    for (i=1; i<10000; i++) {
        fname = settingsUi.saveDirLEdit->text();
        if (fname.endsWith('/') == false) {
            fname += '/';
        }
        fname += settingsUi.imgPrefix->text();
        fname += QString().sprintf("%03d.",i);
        fname += settingsUi.imgFormat->currentText().toLower();

        file.setFileName(fname);
        if (file.exists() == false) {
            break;
        }
    }
    if (i==10000) {
        saveImage();
        return;
    }

    if (img.save(fname,
                 qPrintable(settingsUi.imgFormat->currentText()),
                 quality))
    {
        showImgDialog->close();
    }
    else {
        KMessageBox::sorry(0, i18n("Auto Saving Failed!"));
    }
}

//************************************************************
void Skanlite::setDir(void)
{
    QString dir = KFileDialog::getExistingDirectory(KUrl(settingsUi.saveDirLEdit->text()));
    if (!dir.isEmpty()) {
        settingsUi.saveDirLEdit->setText(dir);
    }
}

//************************************************************
void Skanlite::showAboutDialog(void)
{
    KAboutApplicationDialog::KAboutApplicationDialog(
            KGlobal::mainComponent().aboutData(), 0).exec();
}


