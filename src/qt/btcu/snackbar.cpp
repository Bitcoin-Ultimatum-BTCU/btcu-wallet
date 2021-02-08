// Copyright (c) 2019 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "qt/btcu/snackbar.h"
#include "qt/btcu/forms/ui_snackbar.h"
#include "qt/btcu/qtutils.h"
#include <QTimer>


SnackBar::SnackBar(BTCUGUI* _window, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SnackBar),
    window(_window)
{
    ui->setupUi(this);

    this->setStyleSheet(parent->styleSheet());
    ui->snackContainer->setProperty("cssClass", "container-snackbar");
    ui->label->setProperty("cssClass", "text-snackbar");
    ui->label->setAlignment(Qt::AlignVCenter);
    ui->pushButton->setProperty("cssClass", "ic-close-snackbar");

    connect(ui->pushButton, SIGNAL(clicked()), this, SLOT(close()));
    if (window)
        connect(window, SIGNAL(windowResizeEvent(QResizeEvent*)), this, SLOT(windowResizeEvent(QResizeEvent*)));
    else {
        //ui->horizontalLayout->setContentsMargins(0,0,0,0);
        //ui->label->setStyleSheet("font-size: 15px; color:white;");
    }
    ui->pbnBorder->setMinimumSize(8, this->height());
    ui->pbnBorder->setProperty("cssClass", "pbn-snackbar");
}

void SnackBar::windowResizeEvent(QResizeEvent* event){
    //this->resize(qobject_cast<QWidget*>(parent())->width(), this->height());
    this->move(QPoint(window->width() - this->width(), this->height() ));
}

void SnackBar::showEvent(QShowEvent *event){
    QTimer::singleShot(3000, this, SLOT(hideAnim()));
}

void SnackBar::hideAnim(){
    if (window) closeDialogDropRight(this, window);
    QTimer::singleShot(310, this, SLOT(hide()));
}

void SnackBar::setType(int Type)
{
   switch (Type)
   {
      case CClientUIInterface::MSG_WARNING_SNACK:
      {
         ui->label->setProperty("cssClass", "text-snackbar-green");
         ui->pbnBorder->setProperty("cssClass", "pbn-snackbar-green");
         break;
      }
      case CClientUIInterface::MSG_ERROR_SNACK:
      {
         ui->label->setProperty("cssClass", "text-snackbar-red");
         ui->pbnBorder->setProperty("cssClass", "pbn-snackbar-red");
         break;
      }
      default:
      {
         ui->label->setProperty("cssClass", "text-snackbar");
         ui->pbnBorder->setProperty("cssClass", "pbn-snackbar");
         break;
      }
   }
   updateStyle(ui->label);
   updateStyle(ui->pbnBorder);
}

void SnackBar::sizeTo(QWidget* widget){

}

void SnackBar::setText(QString text){
    ui->label->setText(text);
}

SnackBar::~SnackBar(){
    delete ui;
}
