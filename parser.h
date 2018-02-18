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
                taxcodeRight(std::regex("^.*[B,C,c][0,O]{2}$")),
                taxcodeLeft(std::regex("^[B,C,c][0,O]{2}.*$")),
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

                int tc_pos = isProductLine(input[i], product);

                if(tc_pos){

                    product.originalLine = input[i];

                    extractProductInfo(tc_pos, input[i], product);
                    //std::cout<<"Product: "<<product<<'\n';
                    output.push_back(product);
                    //possible_lines_for_sum = i+1;
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

            size_t first_space_pos = line.find_first_of(' ');
            line = line.substr(first_space_pos+1);

            size_t price_pos = line.find_last_of(' ');

            //Name
            std::string productString = line.substr(0,price_pos);
            abrevSolver.resolveAbbrevs(productString);
            product.name = runSpellcheckOnProduct(productString);

            //Price
            string number_str = line.substr(price_pos);
            try{
                product.price = std::stoi(number_str);
            }
            catch(std::invalid_argument& e){
                qDebug() << "Invalid argument! at number parsing";
            }

        }
        // TERMEKNEV AR X00 - formátumú
        else{
            size_t last_space_pos = line.find_last_of(' ');
            line = line.substr(0,last_space_pos);
            //cout<<line<<'\n';
        }


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
            ss>>word;

            if(word.length()>=3 && word[word.length()-1]!='.')
            {
                results.clear();
                spellchecker.getRecommendations(word, results);

                /*    qDebug()<<QString::fromStdString(word)<<':';
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

            int i;
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
