// Copyright (c) 2019 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "qt/btcu/settings/settingsfaqwidget.h"
#include "qt/btcu/settings/forms/ui_settingsfaqwidget.h"
#include <QScrollBar>
#include <QMetaObject>
#include "qt/btcu/qtutils.h"

SettingsFaqWidget::SettingsFaqWidget(BTCUGUI *parent) :
    QDialog(parent),
    ui(new Ui::SettingsFaqWidget),
    window(parent)
{
    ui->setupUi(this);

    this->setStyleSheet(parent->styleSheet());

    /*ui->labelTitle->setText(tr("Frequently Asked Questions"));
    ui->labelWebLink->setText(tr("You can read more here"));*/
#ifdef Q_OS_MAC
    ui->container->load("://bg-welcome");
    setCssProperty(ui->container, "container-welcome-no-image");
#else
    setCssProperty(ui->container, "container-faq");
#endif
    setCssProperty(ui->pbnIco, "img-faq-logo");
    setCssProperty(ui->labelFAQ, "text-title-faq");

   setCssProperty({
                  ui->pushButtonFaq1,
                  ui->pushButtonFaq2,
                  ui->pushButtonFaq3,
                  ui->pushButtonFaq4,
                  ui->pushButtonFaq5,
                  ui->pushButtonFaq6,
                  ui->pushButtonFaq7,
                  ui->pushButtonFaq8,
                  ui->pushButtonFaq9,
                  ui->pushButtonFaq10
                  }, "btn-faq-check");
   setCssProperty(ui->label, "btn-faq-line");

   // Content
   setCssProperty({
                  ui->scrollAreaWidgetContents1,
                  ui->scrollAreaWidgetContents2,
                  ui->scrollAreaWidgetContents3,
                  ui->scrollAreaWidgetContents4,
                  ui->scrollAreaWidgetContents5,
                  ui->scrollAreaWidgetContents6,
                  ui->scrollAreaWidgetContents7,
                  ui->scrollAreaWidgetContents8,
                  ui->scrollAreaWidgetContents9,
                  ui->scrollAreaWidgetContents10
                  }, "container-faq-scroll");

   setCssProperty({
                  ui->scrollArea1,
                  ui->scrollArea2,
                  ui->scrollArea3,
                  ui->scrollArea4,
                  ui->scrollArea5,
                  ui->scrollArea6,
                  ui->scrollArea7,
                  ui->scrollArea8,
                  ui->scrollArea9,
                  ui->scrollArea10
                  }, "container-faq-scroll");

   setCssProperty({
                  ui->labelContent1,
                  ui->labelContent2,
                  ui->labelContent3,
                  ui->labelContent4,
                  ui->labelContent5,
                  ui->labelContent6,
                  ui->labelContent7_BTCU,
                  ui->labelContent7_zBTCU,
                  ui->labelContent8,
                  ui->labelContent9,
                  ui->labelContent9_These,
                  ui->labelContent9_2,
                  ui->labelContent9_Master,
                  ui->labelContent9_Req,
                  ui->labelContent10
                  }, "text-content-faq");

   setCssProperty({ui->labelContent7,
                   ui->labelContent7_2
                  }, "text-content-title-faq");

   setCssProperty({ui->labelContent9_These_1,
                   ui->labelContent9_These_2,
                   ui->labelContent9_These_3,
                   ui->labelContent9_These_4,
                   ui->labelContent9_These_5,
                   ui->labelContent9_Master_1,
                   ui->labelContent9_Master_2,
                   ui->labelContent9_Master_3,
                   ui->labelContent9_Master_4,
                   ui->labelContent9_Req_1,
                   ui->labelContent9_Req_2,
                   ui->labelContent9_Req_3,
                   ui->labelContent9_Req_4
                  }, "text-content-ic-faq");

   ui->labelContent1->setText("BTCU is a form of digital online money using blockchain technology that can be easily transferred \n"
                              "globally, instantly, and with near zero fees. BTCU incorporates market leading security & privacy t\n"
                              "and is also the firs PoS (Proof of Stake) Cryptocurrency to implement ZeroCoin(zBTCU) and Zerocoin\n"
                              "staking. BTCU utilizes a Proof of Stake (PoS) consensus system algorithm, allowing all owners of \n"
                              "BTCU to participate in earning block rewards while securing the network with full node wallets, as\n"
                              "well as to run Masternodes to create and vote on proposals.");

   ui->labelContent2->setText("Newly received BTCU requires 6 confirmations on the network to become eligible for spending \n"
                              "which can take ~6 minutes. Your BTCU wallet also needs to be completely synchronized to see \n"
                              "and spend balances on the network.");

   ui->labelContent3->setText("zBTCU is an optional privacy-centric method of coin mixing on the BTCU blockchain. Basically all \n"
                              "your transactions cannot be tracked on to any block explorer. You can read more about the technicals\n"
                              "in the \"BTCU Zerocoin (zBTCU) Technical Paper\".");

   ui->labelContent4->setText("After minting, zBTCU will require 20 confirmations as well as 1 additional mint of the same \n"
                              "denomination on the network to become eligible for spending.");

   ui->labelContent5->setText("By default the BTCU wallet will convert 10% of your entire BTCU balance to zBTCU to assist the \n"
                              "network. If you do not wish to stake zBTCU or take advantage of the privacy benefit it brings, you\n"
                              "can disable the automatic minting in your BTCU wallet by going to Settings->Options and deselecting\n"
                              "“Enable zBTCU Automint”. If you are not making use of the BTCU-QT or GUI you can simply open your \n"
                              "btcu.conf file and add enablezeromint=0 Without the quotation marks and restart your wallet to \n"
                              "disable automint.\n"
                              "You can read more about zBTCU in the \"BTCU Zerocoin (zBTCU) Technical Paper\". If you would like to \n"
                              "keep and stake your zBTCU, please read the \"How do I stake\" section of the FAQ below.");


   ui->labelContent6->setText("zBTCU can be spent and sent to any BTCU address. The receiver will receive standard BTCU but the \n"
                              "origin of the BTCU is anonymized by the zBTCU Protocol.\n"
                              "If you want more zBTCU you will need to mint your balance in the “Privacy” tab.");

   ui->labelContent7->setText("To Stake BTCU:");
   ui->labelContent7_2->setText("To Stake zBTCU:");
   ui->labelContent7_BTCU->setText("1. Make sure your wallet is completely synchronized and you are using the latest release.\n"
                                   "2. You must have a balance of BTCU with a minimum of 600 confirmations.\n"
                                   "3. Your wallet must stay online and be unlocked for staking purposes.\n"
                                   "4. Once all those steps are followed staking should be enabled.\n"
                                   "5. You can see the status of staking in the wallet by mousing over the package icon in the row on \n"
                                   "the top left of the wallet interface. There package will be lit up and will state \"Staking Enabled\"\n"
                                   "to indicate it is staking. Using the command line interface (btcu-cli); the command getstakingstatus\n"
                                   "will confirm that staking is active.");
   ui->labelContent7_zBTCU->setText("1. Make sure your wallet is completely synchronized and you are using the latest release.\n"
                                    "2. Your newly minted or existing zBTCU balance must have a minimum of 200 confirmations.\n"
                                    "3. Your wallet must stay online and be unlocked for anonymization and staking purposes. Staking \n"
                                    "should now be enabled.");

   ui->labelContent8->setText("We have support channels in most of our official chat groups, for example #support in our Discord. \n"
                              "If you prefer to submit a ticket, One can be our Freshdesk support site.");

   ui->labelContent9->setText("A masternode is a computer running a full node BTCU core wallet with a requirement of 10,000 BTCU \n"
                              "secured collateral to provide extra services to the network and in return, receive a portion of the \n"
                              "block reward regularly.");
   ui->labelContent9_These->setText("These services include:");
   ui->labelContent9_These_1->setText("Instant transactions (SwiftX)");
   ui->labelContent9_These_2->setText("A decentralized governance (Proposal Voting)");
   ui->labelContent9_These_3->setText("A decentralized budgeting system (Treasury)");
   ui->labelContent9_These_4->setText("Validation of transactions within each block");
   ui->labelContent9_These_5->setText("Act as an additional full node in the network");
   ui->labelContent9_2->setText("For providing such services, masternodes are also paid a certain portion of reward for each block. \n"
                                "This can serve as a passive income to the masternode owners minus their running cost.");
   ui->labelContent9_Master->setText("Masternode Perks:");
   ui->labelContent9_Master_1->setText("Participate in BTCU Governance");
   ui->labelContent9_Master_2->setText("Earn Masternode Rewards");
   ui->labelContent9_Master_3->setText("Commodity option for future sale");
   ui->labelContent9_Master_4->setText("Help secure the BTCU network");
   ui->labelContent9_Req->setText("Requirements:");
   ui->labelContent9_Req_1->setText("10,000 BTCU per single Masternode instance");
   ui->labelContent9_Req_2->setText("Must be stored in a core wallet");
   ui->labelContent9_Req_3->setText("Need dedicated IP address");
   ui->labelContent9_Req_4->setText("Masternode wallet to remain online");


   ui->labelContent10->setText("A Masternode Controller wallet is where the 10,000 BTCU collateral can reside during a \n"
                               "Controller-Remote  masternode setup. It is a wallet that can activate the remote masternode  \n"
                               "wallet(s) and allows you to keep your collateral coins offline while the remote masternode \n"
                               "remains online.");
    /*setCssProperty(ui->labelTitle, "text-title-faq");
    setCssProperty(ui->labelWebLink, "text-content-white");

    // Content
    setCssProperty({
           ui->labelNumber1,
           ui->labelNumber2,
           ui->labelNumber3,
           ui->labelNumber4,
           ui->labelNumber5,
           ui->labelNumber6,
           ui->labelNumber7,
           ui->labelNumber8,
           ui->labelNumber9,
           ui->labelNumber10
        }, "container-number-faq");

    setCssProperty({
              ui->labelSubtitle1,
              ui->labelSubtitle2,
              ui->labelSubtitle3,
              ui->labelSubtitle4,
              ui->labelSubtitle5,
              ui->labelSubtitle6,
              ui->labelSubtitle7,
              ui->labelSubtitle8,
              ui->labelSubtitle9,
              ui->labelSubtitle10
            }, "text-subtitle-faq");


    setCssProperty({
              ui->labelContent1,
              ui->labelContent2,
              ui->labelContent3,
              ui->labelContent4,
              ui->labelContent5,
              ui->labelContent6,
              ui->labelContent7,
              ui->labelContent8,
              ui->labelContent9,
              ui->labelContent10
            }, "text-content-faq");


    setCssProperty({
              ui->pushButtonFaq1,
              ui->pushButtonFaq2,
              ui->pushButtonFaq3,
              ui->pushButtonFaq4,
              ui->pushButtonFaq5,
              ui->pushButtonFaq6,
              ui->pushButtonFaq7,
              ui->pushButtonFaq8,
              ui->pushButtonFaq9,
              ui->pushButtonFaq10
            }, "btn-faq-options");

    ui->labelContent3->setOpenExternalLinks(true);
    ui->labelContent5->setOpenExternalLinks(true);
    ui->labelContent8->setOpenExternalLinks(true);

    */// Exit button
    ui->pushButtonExit->setText(tr("Exit"));
    setCssProperty(ui->pushButtonExit, "btn-faq-exit");
    connect(ui->pushButtonExit, SIGNAL(clicked()), this, SLOT(onClose()));
/*
    // Web Link
    ui->pushButtonWebLink->setText("https://btcu.io/");
    setCssProperty(ui->pushButtonWebLink, "btn-faq-web");
    setCssProperty(ui->containerButtons, "container-faq-buttons");

    // Buttons

    */connect(ui->pushButtonFaq1, SIGNAL(clicked()), this, SLOT(onFaq1Clicked()));
    connect(ui->pushButtonFaq2, SIGNAL(clicked()), this, SLOT(onFaq2Clicked()));
    connect(ui->pushButtonFaq3, SIGNAL(clicked()), this, SLOT(onFaq3Clicked()));
    connect(ui->pushButtonFaq4, SIGNAL(clicked()), this, SLOT(onFaq4Clicked()));
    connect(ui->pushButtonFaq5, SIGNAL(clicked()), this, SLOT(onFaq5Clicked()));
    connect(ui->pushButtonFaq6, SIGNAL(clicked()), this, SLOT(onFaq6Clicked()));
    connect(ui->pushButtonFaq7, SIGNAL(clicked()), this, SLOT(onFaq7Clicked()));
    connect(ui->pushButtonFaq8, SIGNAL(clicked()), this, SLOT(onFaq8Clicked()));
    connect(ui->pushButtonFaq9, SIGNAL(clicked()), this, SLOT(onFaq9Clicked()));
    connect(ui->pushButtonFaq10, SIGNAL(clicked()), this, SLOT(onFaq10Clicked()));

   FullOffChecked();
    if (parent)
        connect(parent, SIGNAL(windowResizeEvent(QResizeEvent*)), this, SLOT(windowResizeEvent(QResizeEvent*)));

}

void SettingsFaqWidget::showEvent(QShowEvent *event){
    if(pos != 0){
        QPushButton* btn = getButtons()[pos - 1];
        QMetaObject::invokeMethod(btn, "setChecked", Qt::QueuedConnection, Q_ARG(bool, true));
        QMetaObject::invokeMethod(btn, "clicked", Qt::QueuedConnection);
    }
}

void SettingsFaqWidget::setSection(int num){
    if (num < 1 || num > 10)
        return;
    pos = num;
}

void SettingsFaqWidget::onFaq1Clicked(){
    if(!ui->pushButtonFaq1->isChecked())
    {
       FullOffChecked();
    }
    else{
       FullOffChecked();
       ui->pushButtonFaq1->setChecked(true);
       ui->scrollArea1->setVisible(true);
    }
}

void SettingsFaqWidget::onFaq2Clicked(){
   if(!ui->pushButtonFaq2->isChecked())
   {
      FullOffChecked();
   }
   else{
      FullOffChecked();
      ui->pushButtonFaq2->setChecked(true);
      ui->scrollArea2->setVisible(true);
   }
   //ui->scrollAreaFaq->verticalScrollBar()->setValue(ui->widgetFaq2->y());
}

void SettingsFaqWidget::onFaq3Clicked(){
   if(!ui->pushButtonFaq3->isChecked())
   {
      FullOffChecked();
   }
   else{
      FullOffChecked();
      ui->pushButtonFaq3->setChecked(true);
      ui->scrollArea3->setVisible(true);
   }
   //ui->scrollAreaFaq->verticalScrollBar()->setValue(ui->widgetFaq3->y());
}

void SettingsFaqWidget::onFaq4Clicked(){
   if(!ui->pushButtonFaq4->isChecked())
   {
      FullOffChecked();
   }
   else{
      FullOffChecked();
      ui->pushButtonFaq4->setChecked(true);
      ui->scrollArea4->setVisible(true);
   }
   //ui->scrollAreaFaq->verticalScrollBar()->setValue(ui->widgetFaq4->y());
}

void SettingsFaqWidget::onFaq5Clicked(){
   if(!ui->pushButtonFaq5->isChecked())
   {
      FullOffChecked();
   }
   else{
      FullOffChecked();
      ui->pushButtonFaq5->setChecked(true);
      ui->scrollArea5->setVisible(true);
   }
   //ui->scrollAreaFaq->verticalScrollBar()->setValue(ui->widgetFaq5->y());
}

void SettingsFaqWidget::onFaq6Clicked(){
   if(!ui->pushButtonFaq6->isChecked())
   {
      FullOffChecked();
   }
   else{
      FullOffChecked();
      ui->pushButtonFaq6->setChecked(true);
      ui->scrollArea6->setVisible(true);
   }
   //ui->scrollAreaFaq->verticalScrollBar()->setValue(ui->widgetFaq6->y());
}

void SettingsFaqWidget::onFaq7Clicked(){
   if(!ui->pushButtonFaq7->isChecked())
   {
      FullOffChecked();
   }
   else{
      FullOffChecked();
      ui->pushButtonFaq7->setChecked(true);
      ui->scrollArea7->setVisible(true);
   }
   //ui->scrollAreaFaq->verticalScrollBar()->setValue(ui->widgetFaq7->y());
}

void SettingsFaqWidget::onFaq8Clicked(){
   if(!ui->pushButtonFaq8->isChecked())
   {
      FullOffChecked();
   }
   else{
      FullOffChecked();
      ui->pushButtonFaq8->setChecked(true);
      ui->scrollArea8->setVisible(true);
   }
   //ui->scrollAreaFaq->verticalScrollBar()->setValue(ui->widgetFaq8->y());
}

void SettingsFaqWidget::onFaq9Clicked(){
   if(!ui->pushButtonFaq9->isChecked())
   {
      FullOffChecked();
   }
   else{
      FullOffChecked();
      ui->pushButtonFaq9->setChecked(true);
      ui->scrollArea9->setVisible(true);
   }
   //ui->scrollAreaFaq->verticalScrollBar()->setValue(ui->widgetFaq9->y());
}

void SettingsFaqWidget::onFaq10Clicked(){
   if(!ui->pushButtonFaq10->isChecked())
   {
      FullOffChecked();
   }
   else{
      FullOffChecked();
      ui->pushButtonFaq10->setChecked(true);
      ui->scrollArea10->setVisible(true);
   }
   //ui->scrollAreaFaq->verticalScrollBar()->setValue(ui->widgetFaq10->y());
}

void SettingsFaqWidget::windowResizeEvent(QResizeEvent* event){
    QWidget* w = qobject_cast<QWidget*>(parent());
    this->resize(w->width(), w->height());
    this->move(QPoint(0, 0));
}

std::vector<QPushButton*> SettingsFaqWidget::getButtons(){
    return {
            ui->pushButtonFaq1,
            ui->pushButtonFaq2,
            ui->pushButtonFaq3,
            ui->pushButtonFaq4,
            ui->pushButtonFaq5,
            ui->pushButtonFaq6,
            ui->pushButtonFaq7,
            ui->pushButtonFaq8,
            ui->pushButtonFaq9,
            ui->pushButtonFaq10
    };
}

void SettingsFaqWidget::FullOffChecked()
{
   ui->pushButtonFaq1->setChecked(false);
   ui->pushButtonFaq2->setChecked(false);
   ui->pushButtonFaq3->setChecked(false);
   ui->pushButtonFaq4->setChecked(false);
   ui->pushButtonFaq5->setChecked(false);
   ui->pushButtonFaq6->setChecked(false);
   ui->pushButtonFaq7->setChecked(false);
   ui->pushButtonFaq8->setChecked(false);
   ui->pushButtonFaq9->setChecked(false);
   ui->pushButtonFaq10->setChecked(false);

   ui->scrollArea1->setVisible(false);
   ui->scrollArea2->setVisible(false);
   ui->scrollArea3->setVisible(false);
   ui->scrollArea4->setVisible(false);
   ui->scrollArea5->setVisible(false);
   ui->scrollArea6->setVisible(false);
   ui->scrollArea7->setVisible(false);
   ui->scrollArea8->setVisible(false);
   ui->scrollArea9->setVisible(false);
   ui->scrollArea10->setVisible(false);
}

void SettingsFaqWidget::onClose()
{
   if (window)
   {
      closeDialog(this, window);
      QTimer::singleShot(310, this, SLOT(close()));
   }else close();
}

SettingsFaqWidget::~SettingsFaqWidget(){
    delete ui;
}


