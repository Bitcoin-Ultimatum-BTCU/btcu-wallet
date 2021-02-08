//
// Created by ek-ut on 19.11.2020.
//

#include "cbtokenrow.h"
#include "qt/btcu/forms/ui_cbtokenrow.h"
#include "qt/btcu/qtutils.h"

CBTokenRow::CBTokenRow(QWidget *parent) :
QWidget(parent),
ui(new Ui::CBTokenRow)
{
   ui->setupUi(this);

   setCssSubtitleScreen(ui->labelToken);
   setCssSubtitleScreen(ui->labelAmount);
   //ui->labelIc->setProperty("cssClass","ic-cb-token");
   ui->labelIc->setPixmap(QPixmap("://img-tokens"));

   setCssProperty(ui->lblDivisory,"container-divider");
   ui->lblDivisory->setVisible(false);
}

void CBTokenRow::setText(QString Token, QString Available)
{
   ui->labelToken->setText(Token);
   ui->labelAmount->setText(Available);
}
 QString CBTokenRow::getText() const
{
   return ui->labelToken->text();
}

QString CBTokenRow::getAvailable() const
{
   return ui->labelAmount->text();
}

 QPixmap CBTokenRow::getPixmap() const
{
   QPixmap pixReturn(*ui->labelIc->pixmap()) ;
   return pixReturn;//ui->labelIc->pixmap();
}
/*QWidget* CBTokenRow::getWig() const
{
   return this;
}*/

void CBTokenRow::setVisibleDivisory(bool visible)
{
   ui->lblDivisory->setVisible(visible);
}
CBTokenRow::~CBTokenRow(){
   delete ui;
}
