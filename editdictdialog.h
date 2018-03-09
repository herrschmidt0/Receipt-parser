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
    EditDictDialog(QWidget *parent) : QDialog(parent)
    {
        setWindowTitle("Szótár szerkesztése");

        QVBoxLayout *layout = new QVBoxLayout();

        areaLabel = new QLabel();
        areaLabel->setText("Szótár tartalma");

        area = new QTextEdit();

        dict = new QFile("termekek.txt");
        dict->open(QIODevice::ReadOnly);
        QTextStream in(dict);
        QString data = in.readAll();
        area->setText(data);

        saveButton = new QPushButton();
        saveButton->setText("Mentés");
        connect(saveButton, SIGNAL(clicked(bool)), this, SLOT(save(bool)));

        layout->addWidget(areaLabel);
        layout->addWidget(area);
        layout->addWidget(saveButton);

        this->setLayout(layout);

        dict->close();
    }

private:
    QLabel *areaLabel;
    QTextEdit *area;
    QPushButton *saveButton;

    QFile *dict;

private slots:
    void save(bool)
    {
        dict->open(QIODevice::ReadWrite);
        dict->resize(0);

        QString newData = area->toPlainText();
        QTextStream out(dict);
        out << newData;

        dict->close();
        close();
    }
};


#endif // EDITDICTDIALOG_H
