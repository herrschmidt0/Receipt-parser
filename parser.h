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

using namespace std;

class Parser
{

private:

    vector<string> products;

    void readInput(vector<string>&input){

        //QString path = QCoreApplication::applicationFilePath();
        //path.append("/in.txt");
       /* QFile file("in.txt");

        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
          qDebug()<<file.errorString();
          return;
        }

        while (!file.atEnd()) {
          string line = file.readLine().data();
          input.push_back(line);
        }*/

        ifstream ifs("in.txt", std::ifstream::in);

        string s;

        while(!ifs.eof()){
            std::getline(ifs, s);
            input.push_back(s);
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


    /*
        Felismeri, hogy egy adott sor tartalmaz-e termék-ár párt,
        illetve melyik oldalon van az ÁFA kód.
        0 - nem terméksor
        1 - terméksor, ÁFA kód bal oldalon
        2 - terméksor, ÁFA kód jobb oldalon
    */
    int isProductLine(string line){


        std::regex product_line_regex1("\\w{3} .+ \\d+"),
                   product_line_regex2(".+ \\d+ \\w{3}");

        std::smatch match;


        if(std::regex_match(line,match,product_line_regex1)){
            //qDebug()<<"Product line, with tax code on left.\n";
            return 1;
        }

        if(std::regex_match(line,match,product_line_regex2)){
            //qDebug()<<"Product line, with tax code on right.\n";
            return 2;
        }


        /*
        QRegExp product_line_regex1("\\w{3} .+ \\d+"),
                product_line_regex2(".+ \\d+ \\w{3}");

        if(product_line_regex1.exactMatch(QString::fromStdString(line)))
        {
            qDebug()<<"Product line, with tax code on left.\n";
            return 1;
        }

        if(product_line_regex2.exactMatch(QString::fromStdString(line)))
        {
            qDebug()<<"Product line, with tax code on right.\n";
            return 2;
        }

        qDebug()<<product_line_regex1.matchedLength();*/

        return 0;
    }


    /*
        Kinyeri egy terméksorból a termék nevét és árát.
    */
    void extractProductInfo(int tc_pos, string line, string &product){

        // X00 TERMEKNEV AR - formátumú
        if(tc_pos == 1){

            size_t first_space_pos = line.find_first_of(' ');
            line = line.substr(first_space_pos);

            size_t price_pos = line.find_last_of(' ');

            //qDebug()<<"Product: "<<QString::fromStdString(line.substr(0,price_pos))<<' ';
            product = line.substr(0,price_pos);

            string number_str = line.substr(price_pos);
            int price = std::stoi(number_str);

            //cout<<"Price: "<<price<<'\n';
        }
        // TERMEKNEV AR X00 - formátumú
        else{
            size_t last_space_pos = line.find_last_of(' ');
            line = line.substr(0,last_space_pos);
            cout<<line<<'\n';
        }


    }

    /* 	Olyan sort keres, amely tartalmazza az "összesen" szót,
        ugyanakkor tartalmaz egy X ft szerű kifejezést.
    */
    void searchLineForSum(string line){

        //"Összesen" szó keresése
        stringstream ss(line);
        string word;
        vector<string> recoms;
        SpellChecker sc;

        bool sum_found = false;

        while(!ss.eof()){

            ss>>word;

            //Mindenféle jelek, nem alfanumerikus karekterek törlése a szóból
            word.erase(std::remove_if(word.begin(), word.end(), [](char c){return !bool(std::isalpha(c));} ), word.end());

            if(word.length()>3){

                //cout<<word<<' '<<word.length()<<'\n';
                //Az adott szó spellcheckelése
                recoms.clear();
                sc.getRecommendations(word,recoms);

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


public:
    void executeParser(vector<string> &pr)
    {
        std::vector<string> input;

        readInput(input);
        ///////////////////

        size_t line_nr = 1;
        size_t possible_lines_for_sum;

        //std::cout<<"Üzlet(lánc) címe: \n\n";

        while(!isListDelimiter(input[line_nr-1])){

            cout<<input[line_nr-1]<<'\n';
            ++line_nr;
        }

        string product;

        for(size_t i=line_nr;i<input.size();++i){

            if(input[i].length()>3){

                int tc_pos = isProductLine(input[i]);

                if(tc_pos){

                    extractProductInfo(tc_pos, input[i], product);
                    pr.push_back(product);
                    possible_lines_for_sum = i+1;
                }

            }
        }

        for(size_t i=line_nr; i<input.size();++i){

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
};
#endif
