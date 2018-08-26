#include "mainwindow.h"
#include "ui_mainwindow.h"


MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle("Nyugtafelismerő");
    setWindowState(Qt::WindowMaximized);
    setlocale(LC_NUMERIC, "C");

    api = new tesseract::TessBaseAPI();

    QString qtessdataPath = QCoreApplication::applicationDirPath().append("/tessdata");
    QByteArray BApath = qtessdataPath.toLatin1();
    const char* tessdataPath = BApath.data();
    // Initialize tesseract-ocr with English, without specifying tessdata path
    if (api->Init(tessdataPath, "eng")) {
       qDebug() << "Could not initialize tesseract.";
       exit(1);
    }

    QByteArray BAconfigPath = qtessdataPath.append("/note").toLatin1();
    api->ReadConfigFile(BAconfigPath.data());

    connect(ui->productsList, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(productClicked(QListWidgetItem*)));
    connect(ui->recomsList, SIGNAL(itemClicked(QListWidgetItem*)), this, SLOT(recomClicked(QListWidgetItem*)));
}


QString processText(QString fileName, tesseract::TessBaseAPI *api)
{
    //Fájl beolvasása
    QByteArray fileNameBytes = fileName.toLatin1();

    Pix *image = pixRead(fileNameBytes.data());
    api->SetImage(image);

    char *outText;
    try
    {
        outText = api->GetUTF8Text();
    }
    catch(...)
    {
         qDebug() << "OCR kifagyott";
    }


    QString outQString;
    if(outText != NULL)
    {
        outQString = QString(outText);
    }

    delete image;
    delete outText;
    return outQString;
}

/* Fájl megnyitása gombra kattintottak */
void MainWindow::on_actionOpen_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Image"), "/home/pt/Documents/nyaam", tr("Image Files (*.png *.jpg *.bmp)"));

    if(!fileName.isEmpty())
    {
        //timer.start();

        ocrProgress = new QProgressDialog("Nyugtabeolvasás folyamatban...", "Cancel", 0, 100);
        ocrProgress->show();

        //Tesseract futtatása külön szálon       
        tessFuture = QtConcurrent::run(processText, fileName, api);
        QFutureWatcher<QString> *watcher = new QFutureWatcher<QString>();
        watcher->setFuture(tessFuture);

        connect(watcher, SIGNAL(finished()), this, SLOT(handleOCRFinished()));

        //A nyugta képének megjelenítése
        QPixmap pixmap(fileName);
        ui->note->setPixmap(pixmap);
        ui->note->setMask(pixmap.mask());
    }
}

vector<Product> runParser(vector<QString> input, Parser *parser)
{
    vector<Product> output;
    parser->executeParser(input, output);
    return output;
}

void MainWindow::handleOCRFinished()
{
    ocrProgress->setValue(75);

    QString outQString = tessFuture.result();

    //Ha nem üres az eredmény, akkor parszer futtatása
    if (!outQString.isEmpty())
    {
        parserInput.clear();

        //Sorokra bontja a folytonos szöveget
        QTextStream ss(&outQString);

        while(!ss.atEnd())
        {
            parserInput.push_back(ss.readLine());
        }

        //Parser futtatása külön szálon
        parserFuture = QtConcurrent::run(runParser, parserInput, &parser);
        QFutureWatcher<vector<Product>> *watcher = new QFutureWatcher<vector<Product>>();
        watcher->setFuture(parserFuture);

        connect(watcher, SIGNAL(finished()), this, SLOT(handleParserFinished()));
    }
    else
    {
        QMessageBox messageBox;
        messageBox.critical(0,"Hiba","A kép szövege nem olvasható!");
        messageBox.setFixedSize(500,200);
    }

}


void MainWindow::handleParserFinished()
{
    //Ablak feltöltése az eredménnyel
    ui->productsList->clear();
    ui->productDetails->clear();

    parserOutput = parserFuture.result();

    if(parserOutput.size() > 0)
    {
        for(size_t i=0; i<parserOutput.size(); ++i)
        {
            QListWidgetItem *item = new QListWidgetItem();
            item->setText(parserOutput[i].name);
            item->setData(Qt::UserRole, QVariant(int(i)));
            ui->productsList->addItem(item);
        }
    }
    else
    {
        QMessageBox messageBox;
        messageBox.critical(0,"Hiba","A kép nem nyugtát tartalmaz!");
        messageBox.setFixedSize(500,200);
    }

    ocrProgress->setValue(100);
    ocrProgress->deleteLater();
}

/* Terméksor egy elemére kattintottak */
void MainWindow::productClicked(QListWidgetItem * arg)
{
    /** Termék részleteinek megjelenítése **/
    ui->productDetails->clear();

    int id = arg->data(Qt::UserRole).toInt();
    ui->productDetails->addItem("Javított név: " + parserOutput[id].name);
    ui->productDetails->addItem("Ár: " + QString::number(parserOutput[id].price) + " Ft");
    ui->productDetails->addItem("Konfidencia szint: " + QString::number(parserOutput[id].confidence) + "%");
    ui->productDetails->addItem("Eredeti terméksor: " + parserOutput[id].originalLine);
    for(size_t i=0; i<parserOutput[id].abrevs.size(); ++i)
    {
        ui->productDetails->addItem("Javaslat " + parserOutput[id].abrevs[i].Short
                                    + "-ra: " + parserOutput[id].abrevs[i].Long);
    }

}

/* Parszolás eredményének mentése fájlba */
void MainWindow::on_actionSaveParsed_triggered()
{

    if (parserOutput.size())
    {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Mentés"), "/", "");

        if(!fileName.isEmpty())
        {
            QFile file(fileName);
            file.open(QIODevice::WriteOnly);
            QTextStream stream(&file);

            stream << "Nyugta \n \n";

            for(size_t i=0; i<parserOutput.size(); ++i)
            {
                stream << QString::fromUtf8("Név: ")<< parserOutput[i].name
                       << QString::fromUtf8(" Ár: ") << QString::number(parserOutput[i].price) << '\n';
            }

            stream << "\n \n";
            stream.setGenerateByteOrderMark(true);

            QMessageBox messageBox;
            messageBox.information(0,"Ok","Digitalizálás eredménye mentve!");
            messageBox.setFixedSize(500,200);

            file.close();
        }
    }
    else
    {
        QMessageBox messageBox;
        messageBox.critical(0,"Hiba","Nincs beparszolt nyugta!");
        messageBox.setFixedSize(500,200);
    }

}

/* Javaslat hozzáadása a szótárhoz */
void MainWindow::on_actionAddToDictionary_triggered()
{
    QString message;
    int idRow = ui->productDetails->currentRow();

    // A 4. sortól kezdődnek a javaslatok
    if(idRow>=4)
    {
        int idProduct = ui->productsList->currentRow();

        QString word_long = parserOutput[idProduct].abrevs[idRow-4].Long;
        word_long = word_long.normalized(QString::NormalizationForm_D);
        QString word_short = parserOutput[idProduct].abrevs[idRow-4].Short;
        word_short = word_short.normalized(QString::NormalizationForm_D);

        //Hozzáadja a sima szótárhoz, meg a rövidítéses szótárhoz is
        QFile dict("termekek.txt"), abrevDict("roviditesek.txt");

        QTextStream dictStream(&dict);
        QTextStream abrevDictStream(&abrevDict);

        // Hozzáadás a termékszótárhoz
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

            qDebug() << lastLine << QString::compare(lastLine,word_long, Qt::CaseInsensitive) << line;

            if(line == word_long)
            {
                message.push_back("Az adott szó már szerepel a termékszótárban.\n");
            }
            else
            {
                QString endOfFile = dictStream.readAll();
                startOfFile.push_back(word_long + '\n');

                dict.resize(0);
                dictStream << (startOfFile + line + '\n' + endOfFile);

                //qDebug() << linesStartingHere;
                message.push_back("Sikeresen hozzáadva a termékszótárhoz!\n");
            }

            dict.close();
        }

        //Hozzáadaás a rövidítéses szótárhoz
        if(abrevDict.open(QIODevice::ReadWrite))
        {
            DictElem d;

            do
            {
              d.setFromLine(abrevDictStream.readLine().normalized(QString::NormalizationForm_D));

            }while(!abrevDictStream.atEnd() && (word_short != d.Short || word_long != d.Long));

            if(abrevDictStream.atEnd() && (word_short != d.Short || word_long != d.Long))
            {
                abrevDictStream << word_short << ' ' << word_long << '\n';
                message.push_back("Sikeresen hozzáadva a rövidítésekhez!\n");
            }
            else
            {
                 message.push_back("Az adott szó már szerepel a rövidítések között.\n");
            }

            abrevDict.close();
        }

        QMessageBox messageBox;
        messageBox.information(0,"Info", message);
        messageBox.setFixedSize(500, 200);

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

    if(items.count() > 0)
    {
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
    else
    {
        QMessageBox messageBox;
        messageBox.critical(0,"Hiba","Az adott termékre nincs találat!");
        messageBox.setFixedSize(500,200);
    }
}

/* Kijelölt online találat mentése */
void MainWindow::on_actionSaveOnlineResult_triggered()
{
    int id = ui->recomsList->currentRow();

    if (id!=-1 && recommendations.size() )
    {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Mentés"), "/", "");

        if(!fileName.isEmpty())
        {
            QFile file(fileName);
            file.open(QIODevice::WriteOnly);
            QTextStream stream(&file);

            stream << QString::fromUtf8("Keresési eredmény \n\n");

            stream << QString::fromUtf8("Név: ")<< recommendations[id]["title"].toString()
                   << QString::fromUtf8("\nLeírás: ") << recommendations[id]["snippet"].toString()
                   << "\n\n";
            file.close();

            QMessageBox messageBox;
            messageBox.information(0,"Ok","Kijelölt találat mentve!");
            messageBox.setFixedSize(500,200);
        }
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

}

/* Online találatok mentése */
void MainWindow::on_actionSaveAllOnlineResults_triggered()
{

    if (recommendations.size())
    {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Mentés"), "/", "");

        if(!fileName.isEmpty())
        {
            QFile file(fileName);
            QTextStream stream(&file);
            file.open(QIODevice::WriteOnly);

            stream << QString::fromUtf8("Keresési eredmények \n\n");

            for(size_t i=0; i<recommendations.size(); ++i)
            {
                stream << QString::fromUtf8("Név: ")<< recommendations[i]["title"].toString()
                   << QString::fromUtf8("\nLeírás: ") << recommendations[i]["snippet"].toString()
                   << "\n\n";
            }
            file.close();

            QMessageBox messageBox;
            messageBox.information(0,"Ok","Találatok mentve!");
            messageBox.setFixedSize(500,200);
        }
    }
    else if(recommendations.size()==0)
    {
        QMessageBox messageBox;
        messageBox.critical(0,"Hiba","Nincsenek találatok!");
        messageBox.setFixedSize(500,200);
    }

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
    if(ui->image->pixmap() && !ui->image->pixmap()->isNull())
    {
        QString fileName = QFileDialog::getSaveFileName(this, tr("Kép mentése"), "/", "");

        if(!fileName.isEmpty())
        {
            const QPixmap *pixmap = ui->image->pixmap();

            QFile file(fileName);
            file.open(QIODevice::WriteOnly);
            pixmap->save(&file, "PNG");
            file.close();

            QMessageBox messageBox;
            messageBox.information(0,"Ok","Kép mentve!");
            messageBox.setFixedSize(500,200);
        }
    }
    else
    {
        QMessageBox messageBox;
        messageBox.critical(0,"Hiba","Nincs kép!");
        messageBox.setFixedSize(500,200);
    }
}

/* Szótárszerkesztő felület indítása */
void MainWindow::on_actionEditDict_triggered()
{
    EditDictDialog *dialog = new EditDictDialog(this);
    dialog->open();
}

/* Súgó megnyitása */
void MainWindow::on_actionOpenHelp_triggered()
{
   HelpDialog *hd = new HelpDialog(this);
   hd->open();
}

/* Kilépés */
void MainWindow::on_actionQuit_triggered()
{
    this->close();
}

/* Destruktor */
MainWindow::~MainWindow()
{
    delete api;
    delete ocrProgress;
    delete ui;
}


