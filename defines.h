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

#define CONFIDENCE3 50
#define CONFIDENCE2 75
#define CONFIDENCE1 100

#endif // DEFINES_H
