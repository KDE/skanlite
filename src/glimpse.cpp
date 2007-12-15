#include <QSettings>
#include <QScrollArea>

#include <KFileDialog>
#include <KMessageBox>
#include <KDebug>

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
    if (ksanew->openDevice(device) == FALSE) {
        QString dev = ksanew->selectDevice(NULL);
        if (ksanew->openDevice(dev) == FALSE) {
            // could not open a scanner
            KMessageBox::sorry(0, i18n("Opening the selected scanner failed!"));
            exit(1);
        }
    }

    settingsDialog = new QDialog(NULL);
    settingsUi.setupUi(settingsDialog);
    connect(settingsUi.getDirButton, SIGNAL(clicked(void)), this, SLOT(setDir(void)));
    readSettings();

    QSettings settings("OSS", "KSaneWidget");
    if (settings.value("General/DontAskOnStart", FALSE).toBool() == FALSE) {
        showSettingsDialog();
    }

    // default dir for save dialog
    currentDir = settings.value("Automatic/Location", "./").toString();

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
    QSettings settings("OSS", "KSaneWidget");

    // enable the widgets to allow modifying
    settingsUi.autoSaveRButton->setChecked(TRUE);
    settingsUi.setQuality->setChecked(TRUE);

    // read the saved parameters
    settingsUi.dontAskOnStart->setChecked(settings.value("General/DontAskOnStart", FALSE).toBool());
    settingsUi.saveDirLEdit->setText(settings.value("Automatic/Location", "./").toString());
    settingsUi.imgPrefix->setText(settings.value("Automatic/NamePrefix", "Image-").toString());
    for (int i=0; i<settingsUi.imgFormat->count(); i++) {
        if (settings.value("Automatic/ImgFormat", "PNG").toString() ==
            settingsUi.imgFormat->itemText(i))
        {
            settingsUi.imgFormat->setCurrentIndex(i);
            break;
        }
    }
    settingsUi.imgQuality->setValue(settings.value("Automatic/ImgQuality", 45).toInt());
    settingsUi.setQuality->setChecked(settings.value("Automatic/SetQuality", FALSE).toBool());
    settingsUi.showB4Save->setChecked(settings.value("Automatic/ShowB4Save", TRUE).toBool());
    if (settings.value("General/SaveMode", "Manual").toString() == QString("Manual")) {
        settingsUi.manualSaveRButton->setChecked(TRUE);
    }
    else {
        settingsUi.autoSaveRButton->setChecked(TRUE);
    }
}

//************************************************************
void Glimpse::showSettingsDialog(void)
{

    readSettings();

    // show the dialog
    if (settingsDialog->exec()) {
        QSettings settings("OSS", "KSaneWidget");
        //printf("save settings\n");
        // Save mode
        if (settingsUi.manualSaveRButton->isChecked()) {
            settings.setValue("General/SaveMode", "Manual");
        }
        else {
            settings.setValue("General/SaveMode", "Automatic");
        }
        settings.setValue("Automatic/Location", settingsUi.saveDirLEdit->text());
        settings.setValue("Automatic/NamePrefix", settingsUi.imgPrefix->text());
        settings.setValue("Automatic/ImgFormat", settingsUi.imgFormat->currentText());
        settings.setValue("Automatic/SetQuality", settingsUi.setQuality->isChecked());
        settings.setValue("Automatic/ImgQuality", settingsUi.imgQuality->value());
        settings.setValue("Automatic/ShowB4Save", settingsUi.showB4Save->isChecked());
        settings.setValue("General/DontAskOnStart", settingsUi.dontAskOnStart->isChecked());
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

    if (settingsUi.showB4Save->isChecked() == TRUE) {
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
        if (fname.endsWith("/") == FALSE) {
            fname += "/";
        }
        fname += settingsUi.imgPrefix->text();
        fname += QString().sprintf("%03d.",i);
        fname += settingsUi.imgFormat->currentText().toLower();
        //printf("filename = %s\n",qPrintable(fname));
        file.setFileName(fname);
        if (file.exists() == FALSE) {
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
