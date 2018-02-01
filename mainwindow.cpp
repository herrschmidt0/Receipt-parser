#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setlocale(LC_NUMERIC, "C");

    tesseract::TessBaseAPI *api = new tesseract::TessBaseAPI();
   // Initialize tesseract-ocr with English, without specifying tessdata path
   if (api->Init(NULL, "eng")) {
       fprintf(stderr, "Could not initialize tesseract.\n");
       exit(1);
   }

   Pix *image = pixRead("test.jpg");
   api->SetImage(image);
   // Get OCR result
   char *outText;
   outText = api->GetUTF8Text();
   std::cout<<"OCR output: "<<outText;

    parser.executeParser(products);

    foreach(const string& word, products)
    {
        ui->productsList->addItem(QString::fromStdString(word));
    }

    connect(ui->productsList, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(productClicked(QListWidgetItem*)));

}

void MainWindow::productClicked(QListWidgetItem * arg)
{
    //qDebug()<<arg->text();

    /*Networking stuff ***/
    QString url = "https://www.googleapis.com/customsearch/v1?key=AIzaSyAwcMNPEj1XYdw_qS-lXvZsk0w668fDwAg&cx=000804532379202159918:s3m1ywa3e90&q=";
    url.append(arg->text());

    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));

    QNetworkRequest request;
    QSslConfiguration config = QSslConfiguration::defaultConfiguration();
    config.setProtocol(QSsl::TlsV1_2);
    request.setSslConfiguration(config);
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ServerHeader, "application/json");

    manager->get(request);
    /*****************/

    ui->recomsList->clear();
}

void MainWindow::replyFinished(QNetworkReply *reply)
{
   // qDebug() << "Response:";

    QByteArray data = reply->readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

    QJsonValue itemsVal = jsonDoc["items"];
    QJsonArray items = itemsVal.toArray();

    foreach (const QJsonValue & v, items)
    {
        //qDebug() << v["title"];
        ui->recomsList->addItem(v["title"].toString());
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionOpen_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Image"), "/home", tr("Image Files (*.png *.jpg *.bmp)"));

    qDebug()<<fileName;
}
