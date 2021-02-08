// Copyright (c) 2019 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "qt/btcu/addresslabelrow.h"
#include "qt/btcu/forms/ui_addresslabelrow.h"
#include "qt/btcu/qtutils.h"

AddressLabelRow::AddressLabelRow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AddressLabelRow)
{
   shadowEffect = new QGraphicsDropShadowEffect();
   shadowEffect->setColor(QColor(0, 0, 0, 22));
   shadowEffect->setXOffset(0);
   shadowEffect->setYOffset(2);
   shadowEffect->setBlurRadius(6);

    ui->setupUi(this);
    //this->setStyleSheet("border-radius:20px");
    ui->rowContainer->setProperty("cssClass", "container-border");
    ui->lblAddress->setProperty("cssClass", "text-list-amount-send");
    setCssSubtitleScreen(ui->lblLabel);
    ui->btnMenu->installEventFilter(this);
   ui->lblDivisory->setVisible(false);
   connect(ui->btnMenu, &QPushButton::clicked, [this](){
      //ui->lblLabel->setText("Cliked!!!!");
      Q_EMIT onMenuClicked();
   });
    //ui->lblLabel->setProperty("cssClass", "text-subtitle");
   //ui->lblDivisory->setStyleSheet("background-color:rgba(0, 0, 0, 0)");
}

void AddressLabelRow::init(bool isLightTheme, bool isHover) {
    updateState(isLightTheme, isHover, false);
}

void AddressLabelRow::updateView(QString address, QString label){
    ui->lblAddress->setText(address);
    ui->lblLabel->setText(label);
}

void AddressLabelRow::updateState(bool isLightTheme, bool isHovered, bool isSelected){
    if(isLightTheme)
        ui->lblDivisory->setStyleSheet("background-color:rgba(0, 0, 0, 0)/*#bababa*/");
    else
        ui->lblDivisory->setStyleSheet("background-color:rgba(0, 0, 0, 0)/*#40ffffff*/");

     /*ui->btnMenu->setVisible(isHovered);*/
   this->setGraphicsEffect(0);
   ui->rowContainer->setGraphicsEffect(shadowEffect);

}
bool AddressLabelRow::getButonActive()
{
   return ui->btnMenu->underMouse();
}

void AddressLabelRow::enterEvent(QEvent *) {
   /*ui->btnMenu->setVisible(true);
   update();*/
}

void AddressLabelRow::leaveEvent(QEvent *) {
    /*ui->btnMenu->setVisible(false);
    update();*/
}

bool AddressLabelRow::eventFilter(QObject *obj, QEvent *event)
{
   QEvent::Type type = event->type();
   if (obj ==  ui->btnMenu) {
      if  (type == QEvent::HoverLeave) {
         btn = false;
      } else if (type == QEvent::HoverEnter)
      {
         btn = true;
      }
   }

   return QWidget::eventFilter(obj, event);
}
void AddressLabelRow::setIndex(QModelIndex pIndex)
{
   index = pIndex;
}
QModelIndex AddressLabelRow::getIndex()
{
   return index;
}

AddressLabelRow::~AddressLabelRow()
{
    delete ui;
}
