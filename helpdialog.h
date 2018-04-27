#ifndef HELPDIALOG_H
#define HELPDIALOG_H

#include <QObject>
#include <QDialog>
#include <QVBoxLayout>
#include <QVector>
#include <QLabel>

class HelpDialog : public QDialog
{
    Q_OBJECT

private:

    QVBoxLayout  mainLayout;

    const QVector<QString> helpStrings =
    {
        "Nyugta beolvasásához a Fájl->Nyugta megnyitása menügombra kell kattintani, és ezután egy nyugtát ábrázoló képet kiválasztani.",
        "Az eredmények a jobb-felső ablakban jelennek meg.",
        "Az Eszközök->Parszolás mentése gomb által lehetőség van a parszolást eredményét kimenteni fájlba.",
        "Az Eszközök->Online keresés gombra kattintva előugrik egy ablak, ahova a keresési stringet beírva, online keresést tudunk végezni.",
        "A keresési találatok a jobb-alsó ablakban listázódnak.",
        "Az Eszközök->Online keresés mentése gombokkal a keresés eredményeit lehet fájlba kimenteni.",
        "Hasonlóan, az Eszközök->Kép mentése gombra kattintva a keresési találat képét lehet elmenteni.",
        "Az Eszközök->Szótárak szerkesztése gombon keresztül a helyességellenőrzés és rövidítéskibontás szótárai lehet szerkeszteni.",
        "'Ez a javaslat helyes! Hozzáadás a szótárhoz' gomb a kiválasztott, rövidítéskibontás javaslataként kapott szót megtanulja, \n lementi a szótárba."
    };

    QVector<QLabel*> labels;

public:

    HelpDialog(QWidget *parent) : QDialog(parent)
    {
        setWindowTitle("Súgó");


        for(int i=0; i<helpStrings.size(); ++i)
        {
            QLabel *l = new QLabel(helpStrings.at(i));

            mainLayout.addWidget(l);
            labels.push_back(l);
        }

        setLayout(&mainLayout);
    }

    ~HelpDialog()
    {
        for(int i=0; i<labels.size(); ++i)
            delete labels[i];
    }

};

#endif // HELPDIALOG_H
