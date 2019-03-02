#ifndef SEARCHDIALOG_H
#define SEARCHDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

class SearchDialog : public QDialog
{
    Q_OBJECT

public:
    SearchDialog(QWidget *parent, QString placeholder = "") : QDialog(parent)
    {
        setWindowTitle("Online keresés");

        QVBoxLayout *layout = new QVBoxLayout();

        queryLabel = new QLabel();
        queryLabel->setText("Keresendő szöveg:");

        query = new QLineEdit();
        query->setText(placeholder);

        sendButton = new QPushButton();
        sendButton->setText("Küldés");

        layout->addWidget(queryLabel);
        layout->addWidget(query);
        layout->addWidget(sendButton);

        this->setLayout(layout);

        connect(sendButton, SIGNAL(clicked(bool)), this, SLOT(sendClicked(bool)));
        connect(this, SIGNAL(executeSearch(QString)), parent, SLOT(onExecuteSearch(QString)));
    }

    ~SearchDialog()
    {
        delete query;
        delete queryLabel;
        delete sendButton;
    }

private:

    QLineEdit *query;
    QLabel *queryLabel;
    QPushButton *sendButton;

 private slots:
    void sendClicked(bool)
    {
        executeSearch(query->text());
        close();
    }

 signals:
    void executeSearch(QString query);


};

#endif // SEARCHDIALOG_H
