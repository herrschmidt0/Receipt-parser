#ifndef DEFINES_H
#define DEFINES_H

struct Product
{
    string name;
    int price;

    string originalLine;
    int confidence;

    Product() : price(0), confidence(50) {}
};

#define alpha 0.60

#define CONFIDENCE3 25
#define CONFIDENCE2 50
#define CONFIDENCE1 75

#define PRICE_CONF_FOUND 75
#define PRICE_CONF_ERROR 25

#endif // DEFINES_H
