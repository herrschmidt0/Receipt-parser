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

#include "defines.h"

using namespace std;

class Abbreviation: public QObject
{
    Q_OBJECT
public:
    Abbreviation() : dictionaryFile("roviditesek.txt")
    {
        string s, l;
        DictElem elem;

        /** Beolvassa szótár szavai egy vektorba **/
        while(!dictionaryFile.eof())
        {
            dictionaryFile >> s >> l;
            elem.Long = l;
            elem.Short = s;
            dictionary.push_back(elem);
        }
    }

    /*
     * Felismeri a rövidített szavakat és szótár használatával kibontja azokat
     */
    void resolveAbbrevs(string& input, Product&product)
    {
        size_t i_pos, pos = 0;
        bool foundInDictionary;

        /** Szó és pont közti space-ek törlése **/
        int j;
        for(size_t i=1; i< input.length(); ++i)
        {
            if(input[i]=='.')
            {
                j = i-1;
                while(j>=0 && std::isspace(input[j]))
                    --j;
                input.erase(j+1, i-j-1);
            }
        }

        //qDebug() <<QString::fromStdString(input);

        /** Végigiterál a soron **/
        while(pos < input.length())
        {
            i_pos = pos;

            while(pos < input.length() && input[pos]!='.' && !isspace(input[pos]))
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
                    /*qDebug()<<QString::fromStdString(dictionary[i].Short)
                    << QString::fromStdString(input.substr(i_pos, pos-i_pos));*/

                   if(dictionary[i].Short == input.substr(i_pos, pos-i_pos))
                   {
                      input.erase(i_pos, pos-i_pos+1);
                      input.insert(i_pos, dictionary[i].Long);
                      pos = i_pos + dictionary[i].Long.length() + 1;
                      foundInDictionary = true;
                   }
                }

                /** Ha nem talált pontos megfelelőt a szótárban, akkor a Google Autocomplete szolgáltatással próbálkozik **/
                if(!foundInDictionary && input.substr(i_pos, pos-i_pos).length()>=3)
                {
                    QString url = "http://suggestqueries.google.com/complete/search?client=firefox&hl=hu&q=";
                    url.append(QString::fromStdString(input.substr(i_pos, pos-i_pos)));

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
                    qDebug() << "Response: " << results;

                    for(size_t i=0; i<results.count(); ++i)
                    {
                        DictElem e(input.substr(i_pos, pos-i_pos), results[i].toString().toStdString());
                        product.abrevs.push_back(e);
                    }
                }
            }

            ++pos;
        }


       //qDebug() <<QString::fromStdString( input);
    }

private:
    std::ifstream dictionaryFile;

    std::vector<DictElem> dictionary;

/*private slots:
    void replyFinished(QNetworkReply *reply)
    {
        QByteArray data = reply->readAll();
        qDebug() << "Response: " <<data;
    }
*/
};

#endif // ABBREVIATION_H
