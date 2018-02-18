#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle("Nyugtafelismerő");
    setlocale(LC_NUMERIC, "C");

    api = new tesseract::TessBaseAPI();
   // Initialize tesseract-ocr with English, without specifying tessdata path
   if (api->Init(NULL, "eng")) {
       fprintf(stderr, "Could not initialize tesseract.\n");
       exit(1);
   }

    currentId = -1;

    connect(ui->productsList, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(productClicked(QListWidgetItem*)));
    connect(ui->recomsList, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(recomClicked(QListWidgetItem*)));
}

/* Terméksor egy elemére kattintottak */
void MainWindow::productClicked(QListWidgetItem * arg)
{
    /** Termék részleteinek megjelenítése **/
    ui->productDetails->clear();

    int id = arg->data(Qt::UserRole).toInt();
    ui->productDetails->addItem("Javított név: " + QString::fromStdString(parserOutput[id].name));
    ui->productDetails->addItem("Ár: " + QString::number(parserOutput[id].price) + " Ft");
    ui->productDetails->addItem("Konfidencia szint: " + QString::number(parserOutput[id].confidence) + "%");
    ui->productDetails->addItem("Eredeti terméksor: " + QString::fromStdString(parserOutput[id].originalLine));

    /** A keresési ablak mezőjébe be fog másolódni a kiválasztott termék neve **/
    currentId = id;
}


/* Online találatok egy elemére kattintottak */
void MainWindow::recomClicked(QListWidgetItem * arg)
{
    ui->recomDetails->clear();

    /** Találat részleteinek a feltöltése **/
    int id = arg->data(Qt::UserRole).toInt();
    ui->recomDetails->addItem("Cím: " + recommendations[id]["title"].toString());
    ui->recomDetails->addItem("Szöveg: " + recommendations[id]["snippet"].toString());

    /** Kép letöltése **/
    QJsonArray cse_image = recommendations[id]["pagemap"]["cse_image"].toArray();
    QJsonValue first_el = cse_image[0];

    QString url = first_el["src"].toString();
    qDebug()<<url;

    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(imageDownloaded(QNetworkReply*)));

    QNetworkRequest request;
    QSslConfiguration config = QSslConfiguration::defaultConfiguration();
    config.setProtocol(QSsl::TlsV1_2);
    request.setSslConfiguration(config);
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ServerHeader, "application/json");

    manager->get(request);
}

/* Kép letöltése kész */
void MainWindow::imageDownloaded(QNetworkReply *reply)
{
    QByteArray jpegData = reply->readAll();

    QPixmap pixmap;
    pixmap.loadFromData(jpegData);
    ui->image->setPixmap(pixmap);
}

/* Hálózati response kezelése, javaslat lista feltöltése */
void MainWindow::replyFinished(QNetworkReply *reply)
{
    QByteArray data = reply->readAll();
    //qDebug() << "Response: " <<data;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(data);

    QJsonValue itemsVal = jsonDoc["items"];
    QJsonArray items = itemsVal.toArray();

    for(int i=0; i<items.count(); ++i)  //const QJsonValue & v, items)
    {
        QListWidgetItem * item = new QListWidgetItem();
        QJsonValue v = items[i];
        item->setText(v["title"].toString());
        item->setData(Qt::UserRole, QVariant(i));
        ui->recomsList->addItem(item);

        recommendations.push_back(items[i]);
    }
}

/* Fájl megnyitása gombra kattintottak */
void MainWindow::on_actionOpen_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Image"), "/home/pt/Documents/nyaam", tr("Image Files (*.png *.jpg *.bmp)"));

    if(!fileName.isEmpty())
    {
        QByteArray fileNameBytes = fileName.toLatin1();

        //qDebug()<<fileName;

        Pix *image = pixRead(fileNameBytes.data());
        api->SetImage(image);
        // Get OCR result
        char *outText;
        outText = api->GetUTF8Text();
        //std::cout<<"OCR: "<<outText;

        //Sorokra bontja a folytonos szöveget
        std::stringstream ss(outText);
        std::string line;

        if (outText != NULL)
        {
            parserInput.clear();
            while(std::getline(ss,line,'\n')){
                parserInput.push_back(line);
             }
        }

        //Parser futtatása
        parserOutput.clear();
        parser.executeParser(parserInput, parserOutput);

        //Ablak feltöltése az eredménnyel
        ui->productsList->clear();
        ui->productDetails->clear();
        for(int i=0; i<parserOutput.size(); ++i)
        {
            QListWidgetItem *item = new QListWidgetItem();
            item->setText(QString::fromStdString(parserOutput[i].name));
            item->setData(Qt::UserRole, QVariant(i));
            ui->productsList->addItem(item);
        }
    }
}

/* Megnyitották az online keresés ablakot */
void MainWindow::on_actionSearchOnline_triggered()
{
    SearchDialog *dialog;
    if(currentId == -1)
        dialog = new SearchDialog(this, "");
    else
        dialog = new SearchDialog(this, parserOutput[currentId].name);

    dialog->open();

}

/* Elindítottak egy online keresést, a "Küldés" gombra kattintva */
void MainWindow::onExecuteSearch(QString query)
{
    /** Hálózati request végrehajtása **/
    QString url = "https://www.googleapis.com/customsearch/v1?key=AIzaSyAwcMNPEj1XYdw_qS-lXvZsk0w668fDwAg&cx=000804532379202159918:s3m1ywa3e90&q=";
    url.append(query);

    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinished(QNetworkReply*)));

    QNetworkRequest request;
    QSslConfiguration config = QSslConfiguration::defaultConfiguration();
    config.setProtocol(QSsl::TlsV1_2);
    request.setSslConfiguration(config);
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ServerHeader, "application/json");

    manager->get(request);

    ui->recomsList->clear();
    recommendations.clear();
}


MainWindow::~MainWindow()
{
    delete ui;
}

