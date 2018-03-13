#ifndef EDITDICTDIALOG_H
#define EDITDICTDIALOG_H

#include <QDialog>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFile>
#include <QTextStream>

class EditDictDialog : public QDialog
{
    Q_OBJECT

public:
    EditDictDialog(QWidget *parent) : QDialog(parent), dict1("termekek.txt"), dict2("roviditesek.txt")
    {
        setWindowTitle("Szótár szerkesztése");

        areaLabel.setText("Szótárak tartalma");

        dict1.open(QIODevice::ReadOnly);
        dict2.open(QIODevice::ReadOnly);
        QTextStream stream1(&dict1), stream2(&dict2);
        QString data1 = stream1.readAll(), data2 = stream2.readAll();

        area1.setText(data1);
        area2.setText(data2);

        saveButton.setText("Mentés");
        connect(&saveButton, SIGNAL(clicked(bool)), this, SLOT(save(bool)));

        mainLayout.addWidget(&areaLabel);
        areasLayout.addWidget(&area1);
        areasLayout.addWidget(&area2);
        mainLayout.addLayout(&areasLayout);
        mainLayout.addWidget(&saveButton);

        this->setLayout(&mainLayout);

        dict1.close();
        dict2.close();
    }

private:
    QVBoxLayout mainLayout;
    QHBoxLayout areasLayout;

    QLabel areaLabel;

    QTextEdit area1, area2;
    QPushButton saveButton;

    QFile dict1, dict2;

private slots:
    void save(bool)
    {
        dict1.open(QIODevice::ReadWrite);
        dict1.resize(0);

        dict2.open(QIODevice::ReadWrite);
        dict2.resize(0);

        QString newData1 = area1.toPlainText(),
                newData2 = area2.toPlainText();
        QTextStream stream1(&dict1), stream2(&dict2);
        stream1 << newData1;
        stream2 << newData2;

        dict1.close();
        dict2.close();
        close();
    }
};


#endif // EDITDICTDIALOG_H
