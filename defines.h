#ifndef DEFINES_H
#define DEFINES_H

using namespace std;

struct DictElem
{
    string Short;
    string Long;

    DictElem() = default;
    DictElem(string s, string l) : Short(s), Long(l) {}
    DictElem(string line)
    {
        int firstSpacePos = line.find_first_of(' ');
        Short = line.substr(0, firstSpacePos);
        Long = line.substr(firstSpacePos+1);
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
