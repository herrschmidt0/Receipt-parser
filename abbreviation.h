#ifndef ABBREVIATION_H
#define ABBREVIATION_H

#include <vector>

class Abbreviation
{
public:
    Abbreviation() : dictionaryFile("roviditesek.txt")
    {
        string s, l;
        dictElem elem;

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
    void resolveAbbrevs(string& input)
    {
        size_t i_pos, pos = 0;

        /** Szó és pont közti space-ek törlése **/
        int j;
        for(size_t i=0; i< input.length(); ++i)
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
                //qDebug() << QString::fromStdString( input.substr(i_pos, pos-i_pos) );
                input.insert(pos+1, " ");

                for(size_t i=0; i<dictionary.size(); ++i)
                {
                    /*qDebug()<<QString::fromStdString(dictionary[i].Short)
                    << QString::fromStdString(input.substr(i_pos, pos-i_pos));*/

                   if(dictionary[i].Short == input.substr(i_pos, pos-i_pos))
                   {
                      input.erase(i_pos, pos-i_pos+1);
                      input.insert(i_pos, dictionary[i].Long);
                      pos = i_pos + dictionary[i].Long.length() + 1;
                   }
                }

            }

            ++pos;
        }

       //qDebug() <<QString::fromStdString( input);
    }

private:
    std::ifstream dictionaryFile;

    struct dictElem{
        string Short;
        string Long;
    };

    std::vector<dictElem> dictionary;

};

#endif // ABBREVIATION_H
