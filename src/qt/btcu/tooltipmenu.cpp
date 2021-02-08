// Copyright (c) 2019 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "qt/btcu/tooltipmenu.h"
#include "qt/btcu/forms/ui_tooltipmenu.h"

#include "qt/btcu/btcugui.h"
#include "qt/btcu/qtutils.h"
#include <QTimer>

TooltipMenu::TooltipMenu(BTCUGUI *_window, QWidget *parent) :
    PWidget(_window, parent),
    ui(new Ui::TooltipMenu)
{
    ui->setupUi(this);
    ui->btnCopy->setVisible(false);
    ui->btnDelete->setVisible(false);
    ui->btnEdit->setVisible(false);
    ui->btnLast->setVisible(false);

    ui->label->setVisible(false);
    ui->label_2->setVisible(false);
    ui->label_3->setVisible(false);
    this->setGraphicsEffect(0);
    setCssProperty(ui->container, "container-list-menu");
    setCssProperty({ui->btnCopy, ui->btnDelete, ui->btnEdit, ui->btnLast}, "btn-list-menu");
    setCssProperty({ui->label, ui->label_2, ui->label_3}, "line-list-menu");
    //ui->btnEdit
    connect(ui->btnCopy, SIGNAL(clicked()), this, SLOT(copyClicked()));
    connect(ui->btnDelete, SIGNAL(clicked()), this, SLOT(deleteClicked()));
    connect(ui->btnEdit, SIGNAL(clicked()), this, SLOT(editClicked()));
    connect(ui->btnLast, SIGNAL(clicked()), this, SLOT(lastClicked()));
}
void TooltipMenu::setCopyBtnText(QString btnText){
    ui->btnCopy->setText(btnText);
    ui->btnCopy->setVisible(true);
}

void TooltipMenu::setEditBtnText(QString btnText){
    ui->btnEdit->setText(btnText);
    ui->btnEdit->setVisible(true);
    ui->label->setVisible(true);
    /*setCssProperty(ui->btnCopy,"btn-list-menu-border");
    updateStyle(ui->btnCopy);*/
}

void TooltipMenu::setDeleteBtnText(QString btnText){
    ui->btnDelete->setText(btnText);
    ui->btnDelete->setVisible(true);
    ui->label_2->setVisible(true);
    /*setCssProperty(ui->btnEdit,"btn-list-menu-border");
    updateStyle(ui->btnEdit);*/
}

void TooltipMenu::setLastBtnText(QString btnText, int minHeight){
    ui->btnLast->setText(btnText);
    ui->btnLast->setMinimumHeight(minHeight);
    ui->btnLast->setVisible(true);
    ui->label_3->setVisible(true);
        /*setCssProperty(ui->btnDelete,"btn-list-menu-border");
    updateStyle(ui->btnDelete);*/
}

void TooltipMenu::setCopyBtnVisible(bool visible){
    ui->btnCopy->setVisible(visible);
}

void TooltipMenu::setDeleteBtnVisible(bool visible){
    ui->btnDelete->setVisible(visible);
}

void TooltipMenu::setEditBtnVisible(bool visible) {
    ui->btnEdit->setVisible(visible);
}

void TooltipMenu::setLastBtnVisible(bool visible) {
    ui->btnLast->setVisible(visible);
}

void TooltipMenu::deleteClicked(){
    hide();
    Q_EMIT onDeleteClicked();
}

void TooltipMenu::copyClicked(){
    hide();
    Q_EMIT onCopyClicked();
}

void TooltipMenu::editClicked(){
    hide();
    Q_EMIT onEditClicked();
}

void TooltipMenu::lastClicked() {
    hide();
    Q_EMIT onLastClicked();
}

void TooltipMenu::showEvent(QShowEvent *event){
    QTimer::singleShot(5000, this, SLOT(hide()));
}

TooltipMenu::~TooltipMenu()
{
    delete ui;
}
