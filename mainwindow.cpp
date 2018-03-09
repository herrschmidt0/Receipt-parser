#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow),
                                          savedImageId(0)
{
    ui->setupUi(this);

    setWindowTitle("Nyugtafelismerő");
    setWindowState(Qt::WindowMaximized);
    setlocale(LC_NUMERIC, "C");

    api = new tesseract::TessBaseAPI();
    // Initialize tesseract-ocr with English, without specifying tessdata path
    if (api->Init(NULL, "eng")) {
       fprintf(stderr, "Could not initialize tesseract.\n");
       exit(1);
    }
    api->ReadConfigFile("note");

    connect(ui->productsList, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(productClicked(QListWidgetItem*)));
    connect(ui->recomsList, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(recomClicked(QListWidgetItem*)));
}



/* Fájl megnyitása gombra kattintottak */
void MainWindow::on_actionOpen_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Image"), "/home/pt/Documents/nyaam", tr("Image Files (*.png *.jpg *.bmp)"));

    if(!fileName.isEmpty())
    {
        //Fájl beolvasása
        QByteArray fileNameBytes = fileName.toLatin1();

        //Tesseract futtatása
        Pix *image = pixRead(fileNameBytes.data());
        api->SetImage(image);

        char *outText;
        outText = api->GetUTF8Text();
        //std::cout<<"OCR: "<<outText;

        //Sorokra bontja a folytonos szöveget
        std::stringstream ss(outText);
        std::string line;

        //Ha nem üres az eredmény, akkor parszer futtatása
        if (outText != NULL)
        {
            parserInput.clear();
            while(std::getline(ss,line,'\n')){
                parserInput.push_back(line);
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


            QPixmap pixmap(fileName);
            ui->note->setPixmap(pixmap);
            ui->note->setMask(pixmap.mask());

            ui->note->show();
        }
    }
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
    for(size_t i=0; i<parserOutput[id].abrevs.size(); ++i)
    {
        ui->productDetails->addItem("Javaslat " + QString::fromStdString(parserOutput[id].abrevs[i].Short)
                                    + "-ra: " + QString::fromStdString(parserOutput[id].abrevs[i].Long));
    }

}

/* Parszolás eredményének mentése fájlba */
void MainWindow::on_actionSaveParsed_triggered()
{
    QFile file("parszer_eredmenyek");

    if (parserOutput.size() && file.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        QTextStream stream(&file);

        stream << "Nyugta \n \n";

        for(size_t i=0; i<parserOutput.size(); ++i)
        {
            stream << QString::fromUtf8("Név: ")<< QString::fromStdString(parserOutput[i].name)
                   << QString::fromUtf8(" Ár: ") << QString::number(parserOutput[i].price) << '\n';
        }

        stream << "\n \n";
        stream.setGenerateByteOrderMark(true);

        QMessageBox messageBox;
        messageBox.information(0,"Ok","Digitalizálás eredménye mentve!");
        messageBox.setFixedSize(500,200);
    }

    file.close();
}

/* Javaslat hozzáadása a szótárhoz */
void MainWindow::on_actionAddToDictionary_triggered()
{
    int idRow = ui->productDetails->currentRow();

    // A 4. sortól kezdődnek a javaslatok
    if(idRow>=4)
    {
        int idProduct = ui->productsList->currentRow();

        QString word_long = QString::fromStdString(parserOutput[idProduct].abrevs[idRow-4].Long);
        word_long = word_long.normalized(QString::NormalizationForm_D);
        QString word_short = QString::fromStdString(parserOutput[idProduct].abrevs[idRow-4].Short);
        word_short = word_short.normalized(QString::NormalizationForm_D);

        //Hozzáadja a sima szótárhoz, meg a rövidítéses szótárhoz is
        QFile dict("termekek.txt"), abrevDict("roviditesek.txt");

        QTextStream dictStream(&dict);
        QTextStream abrevDictStream(&abrevDict);

        // Hozzáadás a szótárhoz
        if(dict.open(QIODevice::ReadWrite))
        {
            QString lastLine, line, startOfFile;

            line = dictStream.readLine().normalized(QString::NormalizationForm_D);

            while(!dictStream.atEnd() && word_long > line)
            {
                startOfFile.push_back(line+'\n');
                lastLine = line;
                line = dictStream.readLine().normalized(QString::NormalizationForm_D);
            }

            lastLine = lastLine.normalized(QString::NormalizationForm_D);

            qDebug() << lastLine << QString::compare(lastLine,word_long, Qt::CaseInsensitive) << word_long;

            if(lastLine == word_long)
            {
                QMessageBox messageBox;
                messageBox.information(0,"Ütközés","Az adott szó már szerepel a szótárban.");
                messageBox.setFixedSize(500,200);
            }
            else
            {
                QString endOfFile = dictStream.readAll();
                startOfFile.push_back(word_long + '\n');

                dict.resize(0);
                dictStream << (startOfFile + line + '\n' + endOfFile);

                //qDebug() << linesStartingHere;
            }

            dict.close();
        }

       /* if(abrevDict.open(QIODevice::ReadWrite))
        {
            QString line;


            do
            {
              DictElem d(abrevDictStream.readLine().normalized(QString::NormalizationForm_D);

            }while(!abrevDictStream.atEnd() && word_short != line.);
        }*/

        //qDebug()<< QString::fromStdString(parserOutput[idProduct].abrevs[idRow-4].Long);
    }
    else
    {
        QMessageBox messageBox;
        messageBox.critical(0,"Hiba","Nincs kijelölve megfelelő elem!");
        messageBox.setFixedSize(500,200);
    }
}


/* Megnyitották az online keresés ablakot */
void MainWindow::on_actionSearchOnline_triggered()
{
    int id = ui->productsList->currentRow();

    SearchDialog *dialog;
    if(id == -1)
        dialog = new SearchDialog(this);
    else
        dialog = new SearchDialog(this, parserOutput[id].name);

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

/* Kijelölt online találat mentése */
void MainWindow::on_actionSaveOnlineResult_triggered()
{
    QFile file("talalat_eredmenyek");

    int id = ui->recomsList->currentRow();

    if (id!=-1 && recommendations.size() && file.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        QTextStream stream(&file);

        stream << QString::fromUtf8("Keresési eredmény \n\n");

        stream << QString::fromUtf8("Név: ")<< recommendations[id]["title"].toString()
               << QString::fromUtf8("\nLeírás: ") << recommendations[id]["snippet"].toString()
               << "\n\n";

        QMessageBox messageBox;
        messageBox.information(0,"Ok","Kijelölt találat mentve!");
        messageBox.setFixedSize(500,200);
    }
    else if(recommendations.size()==0)
    {
        QMessageBox messageBox;
        messageBox.critical(0,"Hiba","Nincsenek találatok!");
        messageBox.setFixedSize(500,200);
    }
    else if(id==-1)
    {
        QMessageBox messageBox;
        messageBox.critical(0,"Hiba","Nincs kijelölve egy elem sem!");
        messageBox.setFixedSize(500,200);
    }


    file.close();
}

/* Online találatok mentése */
void MainWindow::on_actionSaveAllOnlineResults_triggered()
{
    QFile file("talalat_eredmenyek");

    if (recommendations.size() && file.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        QTextStream stream(&file);

        stream << QString::fromUtf8("Keresési eredmény \n\n");

        for(size_t i=0; i<recommendations.size(); ++i)
        {
            stream << QString::fromUtf8("Név: ")<< recommendations[i]["title"].toString()
               << QString::fromUtf8("\nLeírás: ") << recommendations[i]["snippet"].toString()
               << "\n\n";
        }

        QMessageBox messageBox;
        messageBox.information(0,"Ok","Találatok mentve!");
        messageBox.setFixedSize(500,200);
    }
    else if(recommendations.size()==0)
    {
        QMessageBox messageBox;
        messageBox.critical(0,"Hiba","Nincsenek találatok!");
        messageBox.setFixedSize(500,200);
    }

    file.close();
}



/* Online találatok egy elemére kattintottak */
void MainWindow::recomClicked(QListWidgetItem * arg)
{
    ui->recomDetails->clear();

    /** Találat részleteinek a feltöltése **/
    int id = arg->data(Qt::UserRole).toInt();

    ui->recomDetails->addItem("Cím: " + recommendations[id]["title"].toString());
    ui->recomDetails->addItem("Leírás: \n" + recommendations[id]["snippet"].toString());

    /** Kép letöltése, ha van hozzá url **/
    QJsonArray cse_image = recommendations[id]["pagemap"]["cse_image"].toArray();
    QJsonValue first_el = cse_image[0];

    QString url = first_el["src"].toString();
    qDebug()<<url;

    if(!url.isEmpty())
    {
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
}

/* Kép letöltése kész */
void MainWindow::imageDownloaded(QNetworkReply *reply)
{
    QByteArray jpegData = reply->readAll();

    QPixmap pixmap;
    pixmap.loadFromData(jpegData);
    ui->image->setPixmap(pixmap);
}

/* Online keresés kép eredményének mentése */
void MainWindow::on_actionSaveImage_triggered()
{
    if(ui->image->pixmap() != 0)
    {
        const QPixmap *pixmap = ui->image->pixmap();
        QFile file("image" + QString::number(this->savedImageId)+".png");
        ++savedImageId;

        file.open(QIODevice::WriteOnly);
        pixmap->save(&file, "PNG");
        file.close();

        QMessageBox messageBox;
        messageBox.information(0,"Ok","Kép mentve!");
        messageBox.setFixedSize(500,200);
    }
}

void MainWindow::on_actionEditDict_triggered()
{
    EditDictDialog *dialog = new EditDictDialog(this);
    dialog->open();
}


/* Kilépés */
void MainWindow::on_actionQuit_triggered()
{
    this->close();
}

/* Destruktor */
MainWindow::~MainWindow()
{
    delete ui;
}
