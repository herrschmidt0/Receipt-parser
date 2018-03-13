#ifndef DEFINES_H
#define DEFINES_H

using namespace std;

struct DictElem
{
    QString Short;
    QString Long;

    DictElem() = default;
    DictElem(QString s, QString l) : Short(s), Long(l) {}
    DictElem(QString line)
    {
      setFromLine(line);
    }

    void setFromLine(QString line)
    {
        int firstSpacePos = line.indexOf(' ');
        Short = line.mid(0, firstSpacePos);
        Long = line.mid(firstSpacePos+1);
    }
};


struct Product
{
    string name;
    int price;

    string originalLine;
    int confidence;
    vector<DictElem> abrevs;

    Product() : price(0), confidence(50) {}
};

#define alpha 0.60

#define CONFIDENCE3 25
#define CONFIDENCE2 50
#define CONFIDENCE1 75

#define PRICE_CONF_FOUND 75
#define PRICE_CONF_ERROR 25

#endif // DEFINES_H
