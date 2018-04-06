#ifndef SPELLCH_H
#define SPELLCH_H

/*
#include <iostream>
#include <fstream>
#include <locale>
#include <codecvt>
#include <string>
#include <vector>
#include <algorithm>*/

#include <QTextStream>
#include <QFile>

#define productsDictionaryFile "termekek.txt"

using namespace std;

class SpellChecker{

private:

    struct node{
        QString word;
        int distFromParent;
        std::vector<node*> children;

        node() = default;
        node(QString arg, int arg2) : word(arg), distFromParent(arg2) {}
    } *root;

    struct comp{
        SpellChecker &parent;
        QString word;

        comp(QString arg, SpellChecker&p): word(arg), parent(p) {}
        bool operator()(QString a, QString b)
        {
            return parent.LevenshteinDistance(word, a) < parent.LevenshteinDistance(word,b) ||
                   ( parent.LevenshteinDistance(word, a) == parent.LevenshteinDistance(word,b) &&
                       std::abs(word.length()-a.length()) < std::abs(word.length()-b.length()) );
        }    
    };

    int maxDistance;

    /*  Kiszámítja két szó Levenshtein távolságát,
        dinamikus programozás elveket használva.
	*/
    int LevenshteinDistance(QString s, QString t)
    {
        // create two work vectors of integer distances
        const int n = t.length();
        const int m = s.length();
        int substitutionCost;

        int v0[n+1];
        int v1[n+1];

        // initialize v0 (the previous row of distances)
        // this row is A[0][i]: edit distance for an empty s
        // the distance is just the number of characters to delete from t
        for(int i = 0 ; i<=n; ++i)
            v0[i] = i;

        for (int i = 0; i<=m-1; ++i)
        {
            // calculate v1 (current row distances) from the previous row v0

            // first element of v1 is A[i+1][0]
            //   edit distance is delete (i+1) chars from s to match empty t
            v1[0] = i + 1;

            // use formula to fill in the rest of the row
            for(int j = 0; j<=n-1;++j)
            {
                if (s[i] == t[j])
                    substitutionCost = 0;
                else
                    substitutionCost = 1;

                v1[j + 1] = std::min(v1[j] + 1, std::min(v0[j + 1] + 1, v0[j] + substitutionCost));
            }

            // copy v1 (current row) to v0 (previous row) for next iteration
            for(int j=0; j<n+1; ++j)
                std::swap(v0[j],v1[j]);        
         }
        // after the last swap, the results of v1 are now in v0
        return v0[n];

    }


    /*	Hozzáad egy levelet a fához.
    	Ha a szülőnek ő az első gyereke, akkor egyből hozzáadja.
    	Különben leellenőrzi, hogy van-e olyan gyereke a szülőnek, amely 
    		ugyanakkora távolságra van mint az új levél, ha nincs
    		akkor beteszi a többi gyerek mellé, ha van
    		akkor rekurzívan mélyebbre megy a konfliktusos csomóponton keresztül.
    */
    void addNode(node* parent, QString word){

        if(parent->children.empty()){
            parent->children.push_back(new node(word,LevenshteinDistance(parent->word,word)));
        }
        else{

            int dist = LevenshteinDistance(parent->word,word);
            bool conflict = false;
            node* conflicted_node;

            for(node * n : parent->children){
                if(n->distFromParent == dist){
                    conflict = true;
                    conflicted_node = n;
                    break;
                }
            }

            if(!conflict){
                parent->children.push_back(new node(word,dist));
            }
            else{
                addNode(conflicted_node,word);
            }
        }

    }

    /*  BK-fa felépítése
		Ellenőrzi, hogy van-e már gyökér, ha nincs készít egyet,
		különben megkeresi hova tegye az új levelet a fában.
	*/
    void createBKtree()
    {
        QString w;
        QFile in(productsDictionaryFile);
        QTextStream ts(&in);
        in.open(QIODevice::ReadOnly);
        

        while (!ts.atEnd()) {

          ts >> w;

          if(!root)
          {
              root = new node(w, 0);
          }
          else
              addNode(root,w);

        }

    }

    /*	Találatokat keres egy bizonyos szóhoz, rekurzív részleges fabejárással.
    */
    void searchMatches(node * parent, QString word, std::vector<QString> &results){

        int dist = LevenshteinDistance(parent->word, word);

        if(dist<=maxDistance){
            results.push_back(parent->word);
        }

        for(node*n : parent->children){
            if(dist-1 <= n->distFromParent && n->distFromParent <= dist+1){
                searchMatches(n,word,results);
            }
        }

    }



public:

    SpellChecker() : root(0) {

        //Build the BK tree
        createBKtree();
    }


    void getRecommendations(QString input, std::vector<QString>& results)
    {    

        /** Kisbetűkké alakít minden betűt **/
        input.toLower();

        /** A szó hosszától függően állapítja meg a maximális távolságot **/
        if(input.length() <= 4)
            maxDistance = 1;
        else
            maxDistance = 2;

        searchMatches(root, input, results);

        /** Rendezi távolság szerint nővekvő sorrendbe **/
        sort(results.begin(), results.end(), comp(input, *this));
    }


    ~SpellChecker(){
        deleteTree(root);
    }

    void deleteTree(node *x)
    {
        if(x!=NULL)
        {
            for(node *l: x->children)
                deleteTree(l);

            delete x;
        }
    }

};

#endif
