#ifndef ABBREVIATION_H
#define ABBREVIATION_H

#include <vector>
#include <fstream>

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QUrl>
#include <QNetworkReply>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>

#include "defines.h"

using namespace std;

class Abbreviation: public QObject
{
    Q_OBJECT
public:
    Abbreviation() : dictionaryFile("roviditesek.txt"), stream(&dictionaryFile)
    {
        QString s, l;
        DictElem elem;

        dictionaryFile.open(QIODevice::ReadOnly);

        /** Beolvassa szótár szavai egy vektorba **/
        while(!stream.atEnd())
        {
            stream >> s >> l;
            elem.Long = l;
            elem.Short = s;
            dictionary.push_back(elem);
        }

        dictionaryFile.close();
    }

    /*
     * Felismeri a rövidített szavakat és szótár használatával kibontja azokat
     */
    void resolveAbbrevs(QString& input, Product&product)
    {
        int i_pos, pos = 0;
        bool foundInDictionary;

        /** Szó és pont közti space-ek törlése **/
        int j;
        for(int i=1; i< input.length(); ++i)
        {
            if(input[i]=='.')
            {
                j = i-1;
                while(j>=0 && input[j].isSpace())
                    --j;
                input.remove(j+1, i-j-1);
            }
        }

        //qDebug() <<QString::fromStdString(input);

        /** Végigiterál a soron **/
        while(pos < input.length())
        {
            i_pos = pos;

            while(pos < input.length() && input[pos]!='.' && !input[pos].isSpace())
            {
                ++pos;
            }

            /** Ezek a ponttal végződő szavak **/
            if(pos < input.length() && input[pos]=='.')
            {
                /** Szóközt tesz a szó végére **/
                input.insert(pos+1, " ");

                /** Megkeresi az adott rövidítést a szótárban, találat esetén cserél **/
                foundInDictionary = false;
                for(size_t i=0; i<dictionary.size(); ++i)
                {                 

                   if(dictionary[i].Short == input.mid(i_pos, pos-i_pos))
                   {
                       qDebug()<<dictionary[i].Short
                       << input.mid(i_pos, pos-i_pos);

                       /*
                      input.erase(i_pos, pos-i_pos+1);
                      input.insert(i_pos, dictionary[i].Long.toStdString());
                      pos = i_pos + dictionary[i].Long.length() + 1;*/

                      product.abrevs.push_back(DictElem(dictionary[i].Short, dictionary[i].Long));
                      foundInDictionary = true;
                   }
                }

                /** Ha nem talált pontos megfelelőt a szótárban, akkor a Google Autocomplete szolgáltatással próbálkozik **/
                if(!foundInDictionary && input.mid(i_pos, pos-i_pos).length()>=3)
                {
                    QString url = "http://suggestqueries.google.com/complete/search?client=firefox&hl=hu&q=";
                    url.append(input.mid(i_pos, pos-i_pos));

                    QNetworkAccessManager *manager = new QNetworkAccessManager();
                    QNetworkRequest request;
                    request.setUrl(QUrl(url));
                    QNetworkReply *netReply = manager->get(request);

                    QEventLoop loop;
                    connect(netReply, SIGNAL(finished()), &loop, SLOT(quit()));
                    loop.exec();

                    QByteArray data = netReply->readAll();
                    QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
                    QJsonArray results = jsonDoc[1].toArray();
                    //qDebug() << "Response: " << results;

                    for(int i=0; i<results.count(); ++i)
                    {
                        DictElem e(input.mid(i_pos, pos-i_pos), results[i].toString());
                        product.abrevs.push_back(e);
                    }
                }
            }

            ++pos;
        }


       //qDebug() <<QString::fromStdString( input);
    }

private:
    QFile dictionaryFile;
    QTextStream stream;

    std::vector<DictElem> dictionary;

};

#endif // ABBREVIATION_H
