// this is a test app to test KSaneWidget
#ifndef SW_TESTER_H
#define SW_TESTER_H

#include <libksane/ksane.h>

#include "ui_settings.h"

namespace KSaneIface
{
    class KSaneWidget;
}

class Glimpse : public QDialog
{
    Q_OBJECT

    public:
        Glimpse(const QString& device, QWidget *parent = 0);

    private:
        void buildShowImage();
        void readSettings();

    private Q_SLOTS:
        void showSettingsDialog();
        void setDir();
        void imageReady(QByteArray &, int, int, int, int);
        void saveImage();
        void autoSaveImage();


    private:
        KSaneIface::KSaneWidget *ksanew;
        Ui::GlimpseSettings      settingsUi;
        KDialog                 *settingsDialog;
        QDialog                 *showImgDialog;
        QImage                   img;
        QLabel                  *imgLabel;
        QPushButton             *saveBtn;
        QStringList              filterList;
        QStringList              typeList;
        QString                  currentDir;

};

#endif

