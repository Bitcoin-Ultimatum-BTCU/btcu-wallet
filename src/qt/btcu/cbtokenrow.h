//
// Created by ek-ut on 19.11.2020.
//

#ifndef CBTOKENROW_H
#define CBTOKENROW_H
#include <QWidget>

namespace Ui {
   class CBTokenRow;
}

class CBTokenRow : public QWidget
{

Q_OBJECT

public:
   explicit CBTokenRow(QWidget *parent = nullptr);
   ~CBTokenRow();
   void setText(QString Token, QString Available);
    QString getText() const ;
   QString getAvailable() const ;
   QPixmap getPixmap() const ;
   void setVisibleDivisory(bool visible);


private:
   Ui::CBTokenRow *ui;
};


#endif //CBTOKENROW_H
