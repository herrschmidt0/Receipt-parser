#ifndef PARSER_H
#define PARSER_H

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <regex>
#include <cctype>
#include <QRegExp>

#include "spellcheck.h"
#include "abbreviation.h"
#include "defines.h"


using namespace std;

class Parser
{

public:

    Parser() :
                taxcodeRight(std::regex("^.*[A,B,C,c][0,O]{2}$")),
                taxcodeLeft(std::regex("^[A,B,C,c][0,O]{2}.*$")),
                productType1Confidence2(std::regex("\\w{3} .+ \\d{2,5}")),
                productType1Confidence3(std::regex("\\w{3,4} .+ \\d{2,5}")),
                productType2Confidence2(std::regex(".+ \\d{2,5} \\w{3,4}")),
                productType2Confidence3(std::regex(".+ \\d{2,5} \\w{3}")),
                abrevSolver()
    { }

    void executeParser(vector<string> &input, vector<Product> &output)
    {

        //Mindenféle jelek, nem alfanumerikus karekterek törlése a szövegből
        for(size_t i=0; i<input.size(); ++i){
            removeUnnecessaryCharacters(input[i]);
        }
        /*
        size_t line_nr = 1;
        size_t possible_lines_for_sum;

        while(line_nr < input.size() && !isListDelimiter(input[line_nr-1])){
            cout<<input[line_nr-1]<<'\n';
            ++line_nr;
        }*/


        for(size_t i=0;i<input.size();++i){

            if(input[i].length()>3){

                Product product;

                qDebug()<<QString::fromStdString( input[i] );

                int tc_pos = isProductLine(input[i], product);

                if(tc_pos){

                    product.originalLine = input[i];

                    std::transform(input[i].begin(), input[i].end(), input[i].begin(), ::tolower);
                    extractProductInfo(tc_pos, input[i], product);

                    output.push_back(product);
                }

            }
        }

        for(size_t i=0; i<input.size();++i){
            searchLineForSum(input[i]);
        }


        /*
        //Spell Checker próba
        std::vector<string> res;

        SpellChecker sc;

        sc.getRecommendations("isszesen",res);

        for(unsigned int i=0;i<res.size();++i){
            cout<<res[i]<<'\n';
        }
        cout<<"Size: "<<res.size();
        */
    }

private:

     std::regex taxcodeRight,
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
    int isProductLine(string line, Product& product){

        std::smatch match;

        /** Egyre szigorúbb minták illesztése (1. típus) **/
        if(std::regex_match(line,match,productType1Confidence3)){

            product.confidence = alpha*product.confidence + (1-alpha)*CONFIDENCE3;

            if(std::regex_match(line,match,productType1Confidence2)){
                product.confidence = alpha*product.confidence + (1-alpha)*CONFIDENCE2;

                if(std::regex_match(line,match,taxcodeLeft)){
                   product.confidence = alpha*product.confidence + (1-alpha)*CONFIDENCE1;
                }
            }

            return 1;
        }

         /** Egyre szigorúbb minták illesztése (2. típus) **/
        if(std::regex_match(line,match,productType2Confidence3)){

            product.confidence = alpha*product.confidence + (1-alpha)*CONFIDENCE3;

            if(std::regex_match(line,match,productType2Confidence2)){
                product.confidence = alpha*product.confidence + (1-alpha)*CONFIDENCE2;

                if(std::regex_match(line,match,taxcodeRight)){
                   product.confidence = alpha*product.confidence + (1-alpha)*CONFIDENCE1;
                }
            }

            return 2;
        }

        /** Csak az áfa kód talál **/
        if(std::regex_match(line,match,taxcodeLeft)){
           product.confidence = alpha*product.confidence + (1-alpha)*CONFIDENCE2;
           return 1;
        }

        if(std::regex_match(line,match,taxcodeRight)){
           product.confidence = alpha*product.confidence + (1-alpha)*CONFIDENCE1;
           return 2;
        }

        return 0;
    }


    /*
        Kinyeri egy terméksorból a termék nevét és árát.
    */
    void extractProductInfo(int tc_pos, string line, Product &product){

        // X00 TERMEKNEV AR - formátumú
        if(tc_pos == 1){

            int first_space_pos = line.find_first_of(' ');
            line = line.substr(first_space_pos+1);
        }
        // TERMEKNEV AR X00 - formátumú
        else{

            int last_space_pos = line.find_last_of(' ');
            line = line.substr(0, last_space_pos);
        }

        int price_pos = line.length() - 1;
        bool space_found = false;

        if(price_pos > 0)
        {

            /** Felismeri a több, mint 3 jegyű számokat,
             * ahol esetenként a számjegyek 1 space-el vannak elválasztva **/
            while(price_pos > 0 && (isspace(line[price_pos]) || isdigit(line[price_pos])))
            {
                if(isspace(line[price_pos]))
                {
                    if(!space_found)
                        space_found = true;
                    else
                        break;
                }
                else
                    space_found = false;

                --price_pos;
            }

            //Terméknév
            string productString = line.substr(0, price_pos);
            abrevSolver.resolveAbbrevs(productString);
            product.name = runSpellcheckOnProduct(productString);

            /** Termékár
            *  Kitörli a space-eket a stringből,
            *  aztán stoi-val a stringet int-té konvertálja  **/
            string number_str = line.substr(price_pos);
            for(size_t i=0; i<number_str.length(); ++i)
                if(isspace(number_str[i]))
                    number_str.erase(i,1);
            try{
                product.price = std::stoi(number_str);
                product.confidence =  alpha*product.confidence + (1-alpha)*PRICE_CONF_FOUND;
            }
            catch(std::invalid_argument& e){
                qDebug() << "Invalid argument! at number parsing";
                product.confidence =  alpha*product.confidence + (1-alpha)*PRICE_CONF_ERROR;
            }
        }
        else{
            product.confidence =  alpha*product.confidence + (1-alpha)*PRICE_CONF_ERROR;
        }


/*
            int price_pos = line.find_last_of(' ');

            if(price_pos > 0)
            {
                //Name
                std::string productString = line.substr(0,price_pos);
                abrevSolver.resolveAbbrevs(productString);
                product.name = runSpellcheckOnProduct(productString);

                //Price
                string number_str = line.substr(price_pos);
                try{
                    product.price = std::stoi(number_str);
                    product.confidence =  alpha*product.confidence + (1-alpha)*PRICE_CONF_FOUND;
                }
                catch(std::invalid_argument& e){
                    qDebug() << "Invalid argument! at number parsing";
                    product.confidence =  alpha*product.confidence + (1-alpha)*PRICE_CONF_ERROR;
                }
            }
            else{
                product.confidence =  alpha*product.confidence + (1-alpha)*PRICE_CONF_ERROR;
            }*/

    }

    /*
     * Spellchecker futtatása az adott terméksoron, megpróbál minden szót kijavítani
     */
    string runSpellcheckOnProduct(string input)
    {
        stringstream ss(input);
        string word;
        ostringstream oss;

        vector<string> results;

        while(!ss.eof())
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
                    oss << results[0] << " ";
                }
                else{
                    oss << word << " ";
                }

            }
            else{
                oss << word << " ";
            }

            word.clear();
        }

        return oss.str();
    }


    /*
     * Mindenféle jelek, nem alfanumerikus karekterek,
     * valamint sor eleji/végi szóközök törlése
    */
    void removeUnnecessaryCharacters(string& line)
    {
        //std::cout<<line<<'\n';

        if(line.size()>0)
        {
            line.erase(std::remove_if(line.begin(), line.end(),
                [](char c){return c!='.' && c!='-'
                        && c!='\u2014' && !isspace(c) && !std::isalnum(c);} ), line.end());

            size_t i;
            for(i=0; i<line.size() && isspace(line[i]); ++i);
            line.erase(0,i);
            for(i=line.size()-1; i>=0 && isspace(line[i]); --i);
            line.erase(i+1);
        }

        //std::cout<<line<<'\n';
    }


    /* 	Olyan sort keres, amely tartalmazza az "összesen"/"bankkártya" szót,
        ugyanakkor tartalmaz egy X ft szerű kifejezést.
    */
    void searchLineForSum(string line){

        //"Összesen" szó keresése
        stringstream ss(line);
        string word;
        vector<string> recoms;

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

            /*A találat prefixében végétől előre haladva olvasunk ki számokat,
            amíg lehet, majd ezeket számmá alakítjuk */
            while(i>=0 && (prefix[i]==' ' || isdigit(prefix[i]))){
                if(prefix[i]!=' ')
                    number_string.insert(0,1,prefix[i]);
                --i;
            }
            cout<<"Extracted sum value: "<<std::stoi(number_string)<<'\n';
        }


    }


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


};
#endif
