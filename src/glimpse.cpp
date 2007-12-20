#include <QScrollArea>

#include <KFileDialog>
#include <KMessageBox>
#include <KDebug>
#include <KGlobal>

#include "glimpse.h"
#include "glimpse.moc"

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

    // add the settings and clode buttons to the bottom
    QPushButton *settingsBtn = new QPushButton(this);
    settingsBtn->setText("Settings");
    QPushButton *close = new QPushButton(this);
    close->setText("Close");

    btn_layout->addWidget(settingsBtn);
    btn_layout->addStretch();
    btn_layout->addWidget(close);

    connect (settingsBtn, SIGNAL(clicked()), this, SLOT(showSettingsDialog()));
    connect (close, SIGNAL(clicked()), this, SLOT(close()));

    // open the scan device
    if (ksanew->openDevice(device) == false) {
        QString dev = ksanew->selectDevice(NULL);
        if (ksanew->openDevice(dev) == false) {
            // could not open a scanner
            KMessageBox::sorry(0, i18n("Opening the selected scanner failed!"));
            exit(1);
        }
    }

    settingsDialog = new QDialog(NULL);
    settingsUi.setupUi(settingsDialog);
    connect(settingsUi.getDirButton, SIGNAL(clicked(void)), this, SLOT(setDir(void)));
    readSettings();

    if (settingsUi.dontAskOnStart->isChecked() == false) {
        showSettingsDialog();
    }

    // default dir for save dialog
    currentDir = settingsUi.imgPrefix->text();

    connect(ksanew, SIGNAL(imageReady(QByteArray &, int, int, int, int)),
            this, SLOT(imageReady(QByteArray &, int, int, int, int)));

    setWindowTitle(qApp->applicationName());

    // prepare the Show Image Dialog
    buildShowImage();

    typeList << "PNG";
    typeList << "JPG";
    typeList << "JPEG";
    typeList << "BMP";
    typeList << "PPM";
    typeList << "XBM";
    typeList << "XPM";
    typeList << "PNG";

}

//************************************************************
void Glimpse::readSettings(void)
{
    // enable the widgets to allow modifying
    settingsUi.autoSaveRButton->setChecked(true);
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
    settingsUi.imgQuality->setValue(asave.readEntry("ImgQuality", 85));
    settingsUi.setQuality->setChecked(asave.readEntry("SetQuality", false));
    settingsUi.showB4Save->setChecked(asave.readEntry("ShowB4Save", true));

    KConfigGroup general(KGlobal::config(), "General");
    settingsUi.manualSaveRButton->setChecked(general.readEntry("SaveModeManual", true));
    settingsUi.dontAskOnStart->setChecked(general.readEntry("DontAskOnStart", false));

}

//************************************************************
void Glimpse::showSettingsDialog(void)
{
    readSettings();

    // show the dialog
    if (settingsDialog->exec()) {
        KConfigGroup general(KGlobal::config(), "General");
        general.writeEntry("SaveModeManual", settingsUi.manualSaveRButton->isChecked());
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

    // add the Save and clode buttons to the bottom
    btn_layout->addStretch();
    saveBtn = new QPushButton;
    saveBtn->setText("Save");
    btn_layout->addWidget(saveBtn);
    QPushButton *close = new QPushButton;
    close->setText("Close");
    btn_layout->addWidget(close);

    connect (close, SIGNAL(clicked()), showImgDialog, SLOT(close()));
}


//************************************************************
void Glimpse::imageReady(QByteArray &data, int w, int h, int bpl, int f)
{
    ksanew->makeQImage(data, w, h, bpl, (KSaneIface::KSaneWidget::ImageFormat)f, img);
    imgLabel->setPixmap(QPixmap::fromImage(img));
    imgLabel->resize(img.size());

    if (settingsUi.showB4Save->isChecked() == true) {
        if (settingsUi.manualSaveRButton->isChecked()) {
            connect (saveBtn, SIGNAL(clicked()), this, SLOT(saveImage()));
        }
        else {
            connect (saveBtn, SIGNAL(clicked()), this, SLOT(autoSaveImage()));
        }
        showImgDialog->exec();
    }
    else {
        if (settingsUi.manualSaveRButton->isChecked()) {
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
    //QString filter = "image/png \n*.png\n*.jpg\n*.jpeg\n*.bmp\n*.ppm\n*.xbm\n*";
    QString filter = "image/png image/jpg image/jpeg image/bmp image/ppm image/xbm";
    QString name;
    do {
        name = KFileDialog::getSaveFileName(currentDir, filter);
        kDebug() << "-----Save-----" << name;

        if (name.isEmpty()) {
            kDebug() << "!!!!!!!!!!!!!Nothing to save!!!!!!!!!!!";
            return;
        }

        QFileInfo file(name);
        if (file.exists()) {
            if (KMessageBox::warningContinueCancel(this,
                i18n("About to overwrite file \"%1\"\nAre you sure?", file.fileName()),
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

    // Now we should try to save the file

    QString type("PNG");
    // figure out image type
    for (int i=0; i<typeList.size(); ++i) {
        if (name.endsWith(typeList.at(i), Qt::CaseInsensitive)) {
            type = typeList.at(i);
            break;
        }
    }

    //if (type == "NONE) {do seletc file type} for now default to PNG

    if (img.save(name, type.toLatin1(), -1)) {
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
    //printf("autoSaveImage\n");
    QFile file;
    QString fname;
    int quality;

    if (settingsUi.setQuality->isChecked()) {
        quality = settingsUi.imgQuality->value();
    }
    else {
        quality = -1;
    }

    for (int i=0; i<1000; i++) {
        fname = settingsUi.saveDirLEdit->text();
        if (fname.endsWith("/") == false) {
            fname += "/";
        }
        fname += settingsUi.imgPrefix->text();
        fname += QString().sprintf("%03d.",i);
        fname += settingsUi.imgFormat->currentText().toLower();
        //printf("filename = %s\n",qPrintable(fname));
        file.setFileName(fname);
        if (file.exists() == false) {
            break;
        }
    }

    if (img.save(fname,
        qPrintable(settingsUi.imgFormat->currentText()), quality))
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
/*            ( this, "Choose a directory",
              settingsUi.saveDirLEdit->text(),
              KFileDialog::ShowDirsOnly | KFileDialog::DontResolveSymlinks
            );*/
    if (dir != QString("")) {
        settingsUi.saveDirLEdit->setText(dir);
    }
}
