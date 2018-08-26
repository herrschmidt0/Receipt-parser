#ifndef PARSER_H
#define PARSER_H
/*
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <regex>
#include <cctype>*/

#include "spellcheck.h"
#include "abbreviation.h"
#include "defines.h"


using namespace std;

class Parser
{

public:

    Parser() :
                taxcodeRight("^.+[ABCc][0O]{2}$"),
                taxcodeLeft("^[ABCc][0O]{2}.+$"),
                productType1Confidence2("^\\w{3} .+ \\d{2,5}$"),
                productType1Confidence3("^\\w{3,4} .+ \\d{2,5}$"),
                productType2Confidence2("^.+ \\d{2,5} \\w{3,4}$"),
                productType2Confidence3("^.+ \\d{2,5} \\w{3}$"),
                abrevSolver()
    { }

    void executeParser(vector<QString> &input, vector<Product> &output)
    {

        //Mindenféle jelek, nem alfanumerikus karekterek törlése a szövegből
        for(size_t i=0; i<input.size(); ++i){
            removeUnnecessaryCharacters(input[i]);
        }

        for(size_t i=0;i<input.size();++i){

            if(input[i].length()>3){

                Product product;

                qDebug()<<input[i];

                int tc_pos = isProductLine(input[i], product);

                if(tc_pos){

                    product.originalLine = input[i];

                    //std::transform(input[i].begin(), input[i].end(), input[i].begin(), ::tolower);
                    input[i] = input[i].toLower();
                    extractProductInfo(tc_pos, input[i], product);

                    output.push_back(product);
                }

            }
        }

    }

private:

     QRegularExpression taxcodeRight,
                        taxcodeLeft,
                        productType1Confidence2,
                        productType1Confidence3,
                        productType2Confidence2,
                        productType2Confidence3;

     SpellChecker spellchecker;
     Abbreviation abrevSolver;

    /*
        Felismeri, hogy egy adott sor tartalmaz-e termék-ár párt,
        illetve melyik oldalon van az ÁFA kód.
        0 - nem terméksor
        1 - terméksor, ÁFA kód bal oldalon
        2 - terméksor, ÁFA kód jobb oldalon
    */
    int isProductLine(QString line, Product& product){


        /** Egyre szigorúbb minták illesztése (1. típus) **/
        if(productType1Confidence3.match(line).hasMatch()){

            product.confidence = alpha*product.confidence + (1-alpha)*CONFIDENCE3;

            if(productType1Confidence2.match(line).hasMatch()){
                product.confidence = alpha*product.confidence + (1-alpha)*CONFIDENCE2;

                if(taxcodeLeft.match(line).hasMatch()){
                   product.confidence = alpha*product.confidence + (1-alpha)*CONFIDENCE1;
                }
            }

            return 1;
        }

         /** Egyre szigorúbb minták illesztése (2. típus) **/
        if(productType2Confidence3.match(line).hasMatch()){

            product.confidence = alpha*product.confidence + (1-alpha)*CONFIDENCE3;

            if(productType2Confidence2.match(line).hasMatch()){
                product.confidence = alpha*product.confidence + (1-alpha)*CONFIDENCE2;

                if(taxcodeRight.match(line).hasMatch()){
                   product.confidence = alpha*product.confidence + (1-alpha)*CONFIDENCE1;
                }
            }

            return 2;
        }

        /** Csak az áfa kód talál **/
        if(taxcodeLeft.match(line).hasMatch()){
           product.confidence = alpha*product.confidence + (1-alpha)*CONFIDENCE2;
           return 1;
        }

        if(taxcodeRight.match(line).hasMatch()){
           product.confidence = alpha*product.confidence + (1-alpha)*CONFIDENCE1;
           return 2;
        }

        return 0;
    }


    /*
        Kinyeri egy terméksorból a termék nevét és árát.
    */
    void extractProductInfo(int tc_pos, QString line, Product &product){

        // X00 TERMEKNEV AR - formátumú
        if(tc_pos == 1){

            int firstSpacePos = line.indexOf(' ');
            line = line.mid(firstSpacePos+1);
        }
        // TERMEKNEV AR X00 - formátumú
        else{

            int lastSpacePos = line.lastIndexOf(' ');
            line = line.mid(0, lastSpacePos);
            //qDebug() << line;
        }

        int pricePos = line.length() - 1;
        bool spaceFound = false;

        if(pricePos > 0)
        {

            /** Felismeri a több, mint 3 jegyű számokat,
             * ahol esetenként a számjegyek 1 space-el vannak elválasztva **/       
            while(pricePos > 0 && (line[pricePos].isSpace() || line[pricePos].isDigit()))
            {
                if(line[pricePos].isSpace())
                {
                    if(!spaceFound)
                        spaceFound = true;
                    else
                        break;
                }
                else
                   spaceFound = false;

                --pricePos;
            }

            int number = 0;
            for(int i=pricePos; i<line.length(); ++i)
                if(line[i].isDigit())
                {
                    number = number*10 + QString(line[i]).toInt();
                }

            product.price = number;

            //Terméknév
            QString productString = line.left(pricePos);
            abrevSolver.resolveAbbrevs(productString, product);
            product.name = runSpellcheckOnProduct(productString);

        }
        else{
            product.confidence =  alpha*product.confidence + (1-alpha)*PRICE_CONF_ERROR;
        }

    }

    /*
     * Spellchecker futtatása az adott terméksoron, megpróbál minden szót kijavítani
     */
    QString runSpellcheckOnProduct(QString input)
    {
        QTextStream ss(&input);
        QString word;

        vector<QString> results;
        QString output;


        while(!ss.atEnd())
        {
            ss >> word;

            /** Legalább 3 betű hosszú és nincs pont a végén (mert az rövidítés) **/
            if(word.length()>=3 && word[word.length()-1]!='.')
            {
                results.clear();           
                spellchecker.getRecommendations(word, results);

                   /* qDebug()<<QString::fromStdString(word)<<':';
                    for(int i=0;i<results.size();++i)
                        qDebug()<<QString::fromStdString(results[i]);*/

                if(results.size()>0){
                    output.append(results[0] + " ");
                }
                else{
                    output.append(word + " ");
                }


            }
            else{
               output.append(word + " ");
            }

            word.clear();
        }

        return  output;
    }


    /*
     * Mindenféle jelek, nem alfanumerikus karekterek,
     * valamint sor eleji/végi szóközök törlése
    */
    void removeUnnecessaryCharacters(QString& line)
    {
        //std::cout<<line<<'\n';

        if(line.size()>0)
        {       
            for(int i=0; i<line.size(); ++i)
                if(line[i]!='.' && !line[i].isLetterOrNumber() && !line[i].isSpace())
                    line.remove(i,1);

            int i;
            for(i=0; i<line.size() && line[i].isSpace(); ++i);
            line.remove(0,i);
            for(i=line.size()-1; i>=0 && line[i].isSpace(); --i);
            line.remove(i+1);
        }

    }





    /* 	Olyan sort keres, amely tartalmazza az "összesen"/"bankkártya" szót,
        ugyanakkor tartalmaz egy X ft szerű kifejezést.
    */
    /*
    void searchLineForSum(string line){

        //"Összesen" szó keresése
        stringstream ss(line);
        string word;
        vector<QString> recoms;

        bool sum_found = false;

        while(!ss.eof()){

            ss>>word;

            if(word.length()>3){

                //cout<<word<<' '<<word.length()<<'\n';
                //Az adott szó spellcheckelése
                recoms.clear();
                spellchecker.getRecommendations(word,recoms);

                //Javaslatok között az "összesen" keresése
                for(size_t i = 0;i<recoms.size();++i){
                    if(recoms[i]=="összesen"){
                        sum_found = true;
                        cout<<"Sum found! \n";
                        break;
                    }
                }
            }
        }

        //Ha nincs "összesen" a sorban, akkor lépjünk ki
        if(!sum_found) return;

        //Keressük meg az X ft/fT/Ft/FT részt, nyerjük ki az összeg értékét
        regex ft_regex("((F|f)\\w)|(\\w(T|t))");
        smatch m;


        if(regex_search(line,m,ft_regex)){

            string prefix = m.prefix();
            size_t i = prefix.length()-1;
            string number_string;

           //A találat prefixében végétől előre haladva olvasunk ki számokat,
           // amíg lehet, majd ezeket számmá alakítjuk
            while(i>=0 && (prefix[i]==' ' || isdigit(prefix[i]))){
                if(prefix[i]!=' ')
                    number_string.insert(0,1,prefix[i]);
                --i;
            }
            cout<<"Extracted sum value: "<<std::stoi(number_string)<<'\n';
        }


    }
*/
/*
    bool isListDelimiter(string line){

        stringstream ss(line);

        string word;

        while(!ss.eof()){
            ss>>word;
            if(word == "NYUGTA"){
                return true;
            }
        }

        return false;
    }
*/

};
#endif
