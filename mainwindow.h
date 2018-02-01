#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <string>
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
#include <QDebug>

#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#include "parser.h"

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

    Parser parser;

    vector<string> products;

private slots:
    void replyFinished(QNetworkReply*);

    void on_actionOpen_triggered();

    void productClicked(QListWidgetItem*);
};

#endif // MAINWINDOW_H
