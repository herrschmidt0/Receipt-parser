#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QTextStream>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QUrl>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonArray>
#include <QFileDialog>
#include <QMainWindow>
#include <QListWidget>
#include <QPixmap>
#include <QBitmap>
#include <QMessageBox>
#include <QFuture>
#include <QtConcurrent>
#include <QProgressDialog>
#include <QElapsedTimer>
#include <QDebug>

#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>

#include "parser.h"
#include "searchdialog.h"
#include "editdictdialog.h"
#include "helpdialog.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    tesseract::TessBaseAPI *api;
    QFuture<QString> tessFuture;
    QProgressDialog *ocrProgress;

    Parser parser;

    //Listák adatai
    vector<QString> parserInput;
    vector<Product> parserOutput;
    vector<QJsonValue> recommendations;

    QElapsedTimer timer;

private slots:
    void replyFinished(QNetworkReply*);
    void imageDownloaded(QNetworkReply*);

    void productClicked(QListWidgetItem*);
    void recomClicked(QListWidgetItem*);

    void on_actionOpen_triggered();
    void on_actionSearchOnline_triggered();
    void on_actionQuit_triggered();
    void on_actionSaveImage_triggered();
    void on_actionSaveParsed_triggered();

    void on_actionSaveOnlineResult_triggered();

    void on_actionEditDict_triggered();

    void on_actionSaveAllOnlineResults_triggered();

    void on_actionAddToDictionary_triggered();

    void on_actionOpenHelp_triggered();

public slots:
    void handleOCRFinished();
    void onExecuteSearch(QString query);
};

#endif // MAINWINDOW_H
