#include "glimpse.h"
#include "glimpse.moc"

#include <QScrollArea>

#include <KFileDialog>
#include <KMessageBox>
#include <KDebug>
#include <KGlobal>
#include <KAboutApplicationDialog>
#include <KComponentData>

Glimpse::Glimpse(const QString &device, QWidget *parent)
    : QDialog(parent)
{
    ksanew = new KSaneIface::KSaneWidget(this);


    QVBoxLayout *layout = new QVBoxLayout(this);
    QHBoxLayout *btn_layout = new QHBoxLayout;

    QFrame *separator = new QFrame(this);
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);

    layout->setMargin(2);
    layout->setSpacing(2);
    layout->addWidget(ksanew);
    layout->addWidget(separator);
    layout->addLayout(btn_layout);


    // add the settings and close buttons to the bottom
    QPushButton *settingsBtn = new QPushButton(this);
    settingsBtn->setText(i18n("Settings"));
    settingsBtn->setIcon(SmallIcon("configure"));
    QPushButton *aboutBtn = new QPushButton(this);
    aboutBtn->setText(i18n("About"));
    QPushButton *closeBtn = new QPushButton(this);
    closeBtn->setText(i18n("Close"));
    closeBtn->setIcon(SmallIcon("dialog-close"));

    btn_layout->addWidget(settingsBtn);
    btn_layout->addWidget(aboutBtn);
    btn_layout->addStretch();
    btn_layout->addWidget(closeBtn);

    connect (settingsBtn, SIGNAL(clicked()), this, SLOT(showSettingsDialog()));
    connect (aboutBtn, SIGNAL(clicked()), this, SLOT(showAboutDialog()));
    connect (closeBtn, SIGNAL(clicked()), this, SLOT(close()));

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
        QString dev = ksanew->selectDevice(NULL);
        if (ksanew->openDevice(dev) == false) {
            // could not open a scanner
            KMessageBox::sorry(0, i18n("Opening the selected scanner failed!"));
            exit(1);
        }
    }
    setWindowTitle(ksanew->make()+ " " + ksanew->model() + " - Glimpse");

    // prepare the Show Image Dialog
    buildShowImage();

}

//************************************************************
void Glimpse::readSettings(void)
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
void Glimpse::showSettingsDialog(void)
{
    readSettings();

    //settingsUi.okButton->setIcon(SmallIcon("dialog-ok-apply"));
    //settingsUi.cancelButton->setIcon(SmallIcon("dialog-cancel"));
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
void Glimpse::buildShowImage(void)
{
    showImgDialog = new QDialog(0);

    QVBoxLayout *s_layout = new QVBoxLayout;
    s_layout->setSpacing(2);
    s_layout->setMargin(0);
    showImgDialog->setLayout(s_layout);

    QScrollArea *img_area = new QScrollArea;
    s_layout->addWidget(img_area);
    imgLabel = new QLabel(showImgDialog);
    img_area->setWidget(imgLabel);

    QHBoxLayout *btn_layout = new QHBoxLayout;
    s_layout->insertLayout(1, btn_layout, 0);

    // add the Save and close buttons to the bottom
    btn_layout->addStretch();
    saveBtn = new QPushButton;
    saveBtn->setText(i18n("Save"));
    saveBtn->setIcon(SmallIcon("document-save"));

    btn_layout->addWidget(saveBtn);
    QPushButton *close = new QPushButton;
    close->setText(i18n("Close"));
    close->setIcon(SmallIcon("dialog-close"));
    btn_layout->addWidget(close);

    connect (close, SIGNAL(clicked()), showImgDialog, SLOT(close()));
}


//************************************************************
void Glimpse::imageReady(QByteArray &data, int w, int h, int bpl, int f)
{
    /* copy the image data into img */
    ksanew->makeQImage(data, w, h, bpl, (KSaneIface::KSaneWidget::ImageFormat)f, img);

    if (settingsUi.showB4Save->isChecked() == true) {
        imgLabel->setPixmap(QPixmap::fromImage(img));
        imgLabel->resize(img.size());

        if (settingsUi.saveModeCB->currentIndex() == 0) {
            connect (saveBtn, SIGNAL(clicked()), this, SLOT(saveImage()));
        }
        else {
            connect (saveBtn, SIGNAL(clicked()), this, SLOT(autoSaveImage()));
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
void Glimpse::saveImage()
{
    //QString filter = "*.png\n*.jpg\n*.jpeg\n*.bmp\n*.ppm\n*.xbm\n*";
    QString filter = "image/png image/jpg image/jpeg image/bmp image/ppm image/xbm";

    QFile file;
    QString fname;
    int i;

    // find next available file name for name suggestion
    for (i=1; i<10000; i++) {
        fname = currentDir;
        if (fname.endsWith("/") == false) {
            fname += "/";
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
                i18n("Warning"),
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
        disconnect(saveBtn, NULL, NULL, NULL);
        showImgDialog->close();
    }
    else {
        KMessageBox::sorry(0, i18n("Saving Failed!"));
    }

}

//************************************************************
void Glimpse::autoSaveImage()
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
        if (fname.endsWith("/") == false) {
            fname += "/";
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
        disconnect(saveBtn, NULL, NULL, NULL);
        showImgDialog->close();
    }
    else {
        KMessageBox::sorry(0, i18n("Auto Saving Failed!"));
    }
}

//************************************************************
void Glimpse::setDir(void)
{
    QString dir = KFileDialog::getExistingDirectory(KUrl(settingsUi.saveDirLEdit->text()));
    if (dir != QString("")) {
        settingsUi.saveDirLEdit->setText(dir);
    }
}

//************************************************************
void Glimpse::showAboutDialog(void)
{
    KAboutApplicationDialog::KAboutApplicationDialog(
            KGlobal::mainComponent().aboutData(), 0).exec();
}


