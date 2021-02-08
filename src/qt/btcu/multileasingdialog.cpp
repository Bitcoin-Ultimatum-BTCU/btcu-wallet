//
// Created by ek-ut on 16.11.2020.
//

#include "multileasingdialog.h"
#include "qt/btcu/forms/ui_multileasingdialog.h"
#include "qt/btcu/defaultdialog.h"
#include "guiutil.h"
#include "qtutils.h"

MultiLeasingDialog::MultiLeasingDialog(BTCUGUI *parent) :
   QDialog(parent), ui(new Ui::MultiLeasingDialog)
   //ui(new Ui::MultiLeasingtDialog)
{

   ui->setupUi(this);

   pwParent = parent;
   // Stylesheet
   this->setStyleSheet(parent ? parent->styleSheet(): GUIUtil::loadStyleSheet());

   // Container
   ui->frame->setProperty("cssClass", "container-border");
   // Buttons
   ui->btnEsc->setText("");
   ui->btnEsc->setProperty("cssClass", "ic-close");
   ui->btnCancel->setProperty("cssClass", "btn-leasing-dialog-back");
   ui->btnSave->setProperty("cssClass", "btn-leasing-dialog-save");

   // Text
   ui->labelTitle->setProperty("cssClass", "text-title-leasing-dialog");
   ui->labelBalance->setProperty("cssClass", "text-leasing-dialog");
   ui->labelBTCU->setProperty("cssClass", "text-leasing-dialog-red");
   ui->labelTipe->setProperty("cssClass", "text-leasing-dialog-red");
   ui->labelMasternode->setProperty("cssClass", "text-leasing-dialog");
   ui->labelAmount->setProperty("cssClass", "text-leasing-dialog");
   ui->labelFee->setProperty("cssClass", "text-leasing-dialog-small");
   ui->lineEditMasternode->setPlaceholderText(tr("John Nicklson"));
   ui->lineEditMasternode->setProperty("cssClass", "edit-leasing-dialog");
   ui->lineEditAmount->setProperty("cssClass", "edit-leasing-dialog");

   connect(ui->btnEsc, SIGNAL(clicked()), this, SLOT(close()));
   connect(ui->btnSave, &QPushButton::clicked, [this](){this->isSave = true; accept();});
   connect(ui->btnCancel, SIGNAL(clicked()), this, SLOT(close()));
}


void MultiLeasingDialog::onbtnSave()
{
   /*DefaultDialog *dialog = new DefaultDialog();
   dialog->setText("", tr("Are you sure to lease\n\r 10 BTCU?\n"), tr("yes"),tr("no"));
   dialog->adjustSize();
   openDialogWithOpaqueBackground(dialog, pwParent);*/
   //Q_EMIT close();
}


void showHide(bool show)
{

}

MultiLeasingDialog::~MultiLeasingDialog()
{
   delete ui;
}
