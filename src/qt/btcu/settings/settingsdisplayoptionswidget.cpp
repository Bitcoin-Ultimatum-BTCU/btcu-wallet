// Copyright (c) 2019 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "qt/btcu/settings/settingsdisplayoptionswidget.h"
#include "qt/btcu/settings/forms/ui_settingsdisplayoptionswidget.h"
#include <QListView>
#include <QSettings>
#include <QDir>
#include "guiutil.h"
#include "clientmodel.h"
#include "optionsmodel.h"
#include "bitcoinunits.h"
#include "qt/btcu/qtutils.h"

#include <QGraphicsDropShadowEffect>

SettingsDisplayOptionsWidget::SettingsDisplayOptionsWidget(BTCUGUI* _window, QWidget *parent) :
    PWidget(_window,parent),
    ui(new Ui::SettingsDisplayOptionsWidget)
{
    ui->setupUi(this);

   QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect();
   shadowEffect->setColor(QColor(0, 0, 0, 22));
   shadowEffect->setXOffset(0);
   shadowEffect->setYOffset(2);
   shadowEffect->setBlurRadius(6);


    this->setStyleSheet(parent->styleSheet());
    pw=new QWidget(this);
    QVBoxLayout* Layout = new QVBoxLayout(pw);
    lw = new QListView();
   Layout->addWidget(lw);
   pw->setGraphicsEffect(0);
   this->setGraphicsEffect(0);
   lw->setGraphicsEffect(shadowEffect);
   lw->setProperty("cssClass", "container-border-light");
   btnBoxUnit = ui->lineEditBoxUnit->addAction(QIcon("://ic-contact-arrow-down"), QLineEdit::TrailingPosition);
   btnUpBoxUnit = ui->lineEditBoxUnit->addAction(QIcon("://ic-contact-arrow-up"), QLineEdit::TrailingPosition);
   ui->lineEditBoxUnit->removeAction(btnUpBoxUnit);
   pwLanguage =new QWidget(this);
   QVBoxLayout* LayoutLanguage = new QVBoxLayout(pwLanguage);
   lwLanguage = new QListView();
   LayoutLanguage->addWidget(lwLanguage);
   pwLanguage->setGraphicsEffect(0);
   lwLanguage->setGraphicsEffect(shadowEffect);
   lwLanguage->setProperty("cssClass", "container-border-light");
   btnBoxLanguage = ui->lineEditLanguage->addAction(QIcon("://ic-contact-arrow-down"), QLineEdit::TrailingPosition);
   btnUpBoxLanguage = ui->lineEditLanguage->addAction(QIcon("://ic-contact-arrow-up"), QLineEdit::TrailingPosition);
   ui->lineEditLanguage->removeAction(btnUpBoxLanguage);
   pwDigits=new QWidget(this);
   QVBoxLayout* LayoutDigits = new QVBoxLayout(pwDigits);
   lwDigits = new QListView();
   LayoutDigits->addWidget(lwDigits);
   pwDigits->setGraphicsEffect(0);
   lwDigits->setGraphicsEffect(shadowEffect);
   lwDigits->setProperty("cssClass", "container-border-light");
   btnBoxDigits = ui->lineEditDigits->addAction(QIcon("://ic-contact-arrow-down"), QLineEdit::TrailingPosition);
   btnUpBoxDigits = ui->lineEditDigits->addAction(QIcon("://ic-contact-arrow-up"), QLineEdit::TrailingPosition);
   ui->lineEditDigits->removeAction(btnUpBoxDigits);
   // Containers
    ui->left->setProperty("cssClass", "container-border");
    ui->left->setContentsMargins(10,10,10,10);

    // Title
    ui->labelTitle->setText(tr("Display"));
    setCssTitleScreen(ui->labelTitle);

    // Subtitle
    ui->labelSubtitle1->setText(tr("Customize the display view options"));
    setCssSubtitleScreen(ui->labelSubtitle1);

    ui->labelTitleLanguage->setText(tr("Language"));
   setCssSubtitleScreen(ui->labelTitleLanguage);//->setProperty("cssClass", "text-main-settings");

    ui->labelTitleUnit->setText(tr("Unit to show amount"));
   setCssSubtitleScreen(ui->labelTitleUnit);//->setProperty("cssClass", "text-main-settings");

    ui->labelTitleDigits->setText(tr("Decimal digits"));
   setCssSubtitleScreen(ui->labelTitleDigits);//->setProperty("cssClass", "text-main-settings");

    ui->labelTitleUrl->setText(tr("Third party transactions URLs"));
   setCssSubtitleScreen(ui->labelTitleUrl);//->setProperty("cssClass", "text-main-settings");
    // TODO: Reconnect this option to an action. Hide it for now
    ui->labelTitleUrl->hide();

    // Switch
    ui->pushButtonSwitchBalance->setText(tr("Hide empty balances"));
    ui->pushButtonSwitchBalance->setProperty("cssClass", "btn-switch");
    ui->pushButtonSwitchBalance->setVisible(false);

    // Combobox
    setCssProperty(ui->lineEditBoxUnit, "edit-primary-multi-book");
   setCssProperty(ui->lineEditLanguage, "edit-primary-multi-book");
   setCssProperty(ui->lineEditDigits, "edit-primary-multi-book");
    ui->comboBoxLanguage->setProperty("cssClass", "btn-combo");
    ui->comboBoxLanguage->setStyleSheet("border: 1px solid rgba(38, 38, 66, 0.07);");
    ui->comboBoxLanguage->setView(new QListView());
    ui->comboBoxLanguage->setEditable(true);
    QLineEdit* LanguageEdit = new QLineEdit(ui->comboBoxLanguage);
    LanguageEdit->setReadOnly(true);
    LanguageEdit->setAlignment(Qt::AlignRight);
    ui->comboBoxLanguage->setLineEdit(LanguageEdit);

   ui->comboBoxLanguage->setVisible(false);

    ui->comboBoxUnit->setProperty("cssClass", "btn-combo");
    ui->comboBoxUnit->setStyleSheet("border: 1px solid rgba(38, 38, 66, 0.07);");
    ui->comboBoxUnit->setView(new QListView());
    ui->comboBoxUnit->setModel(new BitcoinUnits(this));
    ui->comboBoxUnit->setModelColumn(Qt::DisplayRole);
    ui->comboBoxUnit->setEditable(true);
    ui->comboBoxUnit->setVisible(false);
   lw->setModel(new BitcoinUnits(this));
    QLineEdit* UnitEdit = new QLineEdit(ui->comboBoxUnit);
    UnitEdit->setReadOnly(true);
    UnitEdit->setAlignment(Qt::AlignRight);
    ui->comboBoxUnit->setLineEdit(UnitEdit);

    ui->comboBoxDigits->setProperty("cssClass", "btn-combo-options");

    ui->comboBoxDigits->setVisible(false);
    ui->comboBoxDigits->setView(new QListView());
    ui->comboBoxDigits->setEditable(true);
    QLineEdit* DigitsEdit = new QLineEdit(ui->comboBoxDigits);
    DigitsEdit->setReadOnly(true);
    DigitsEdit->setAlignment(Qt::AlignRight);
    ui->comboBoxDigits->setLineEdit(DigitsEdit);
    ui->comboBoxDigits->setStyleSheet("border: 1px solid rgba(38, 38, 66, 0.07);");

    /* Number of displayed decimal digits selector */
    QString digits;
    for (int index = 2; index <= 8; index++) {
        digits.setNum(index);
        ui->comboBoxDigits->addItem(digits, digits);

    }
   lwDigits->setModel(ui->comboBoxDigits->model());

    // Urls
    ui->lineEditUrl->setPlaceholderText("e.g. https://example.com/tx/%s");
    initCssEditLine(ui->lineEditUrl);
    // TODO: Reconnect this option to an action. Hide it for now
    ui->lineEditUrl->hide();

    // Buttons
    ui->pushButtonSave->setText(tr("SAVE"));
    ui->pushButtonReset->setText(tr("Reset to default"));
   setCssBtnSecondary(ui->pushButtonSave);
   setCssBtnPrimary(ui->pushButtonReset);
   setCssBtnPrimary(ui->pushButtonClean);

    initLanguages();
    connect(ui->pushButtonSave, SIGNAL(clicked()), parent, SLOT(onSaveOptionsClicked()));
    connect(ui->pushButtonReset, SIGNAL(clicked()), this, SLOT(onResetClicked()));
    connect(ui->pushButtonClean, SIGNAL(clicked()), parent, SLOT(onDiscardChanges()));
    connect(lw, SIGNAL(clicked(QModelIndex)), this, SLOT(handleClick(QModelIndex)));
   connect(lwLanguage, SIGNAL(clicked(QModelIndex)), this, SLOT(handleLanguageClick(QModelIndex)));
   connect(lwDigits, SIGNAL(clicked(QModelIndex)), this, SLOT(handleDigitsClick(QModelIndex)));
    connect(btnBoxUnit, &QAction::triggered, [this](){ onBoxUnitClicked(); });
   connect(btnBoxLanguage, &QAction::triggered, [this](){ onBoxLanguageClicked(); });
   connect(btnBoxDigits, &QAction::triggered, [this](){ onBoxDigitsClicked(); });
   connect(btnUpBoxUnit, &QAction::triggered, [this](){ onBoxUnitClicked(); });
   connect(btnUpBoxLanguage, &QAction::triggered, [this](){ onBoxLanguageClicked(); });
   connect(btnUpBoxDigits, &QAction::triggered, [this](){ onBoxDigitsClicked(); });
   pw->hide();
   pwLanguage->hide();
   pwDigits->hide();
}

void SettingsDisplayOptionsWidget::initLanguages(){
    /* Language selector */
    QDir translations(":translations");
    QString defaultStr = QString("(") + tr("default") + QString(")");
    ui->comboBoxLanguage->addItem(defaultStr, QVariant(""));
    Q_FOREACH (const QString& langStr, translations.entryList()) {
        QLocale locale(langStr);

        /** check if the locale name consists of 2 parts (language_country) */
        if(langStr.contains("_")){
            /** display language strings as "native language - native country (locale name)", e.g. "Deutsch - Deutschland (de)" */
            ui->comboBoxLanguage->addItem(locale.nativeLanguageName() + QString(" - ") + locale.nativeCountryName() + QString(" (") + langStr + QString(")"), QVariant(langStr));
        }
        else{
            /** display language strings as "native language (locale name)", e.g. "Deutsch (de)" */
            ui->comboBoxLanguage->addItem(locale.nativeLanguageName() + QString(" (") + langStr + QString(")"), QVariant(langStr));
        }
    }
   lwLanguage->setModel(ui->comboBoxLanguage->model());
}

void SettingsDisplayOptionsWidget::onResetClicked() {
    if (clientModel) {
        OptionsModel *optionsModel = clientModel->getOptionsModel();
        QSettings settings;
        optionsModel->setDisplayDefaultOptions(settings, true);
        informWarning(tr("Options reset succeed"));
    }
}

void SettingsDisplayOptionsWidget::setMapper(QDataWidgetMapper *mapper){
    mapper->addMapping(ui->comboBoxDigits, OptionsModel::Digits);
    mapper->addMapping(ui->comboBoxLanguage, OptionsModel::Language);
    mapper->addMapping(ui->comboBoxUnit, OptionsModel::DisplayUnit);
    mapper->addMapping(ui->pushButtonSwitchBalance, OptionsModel::HideZeroBalances);
    /*mapper->addMapping(lw, OptionsModel::DisplayUnit);
    mapper->addMapping(lwLanguage, OptionsModel::Language);
    mapper->addMapping(lwDigits, OptionsModel::Digits);*/
}

void SettingsDisplayOptionsWidget::loadClientModel(){
    if(clientModel) {
        ui->comboBoxUnit->setCurrentIndex(this->clientModel->getOptionsModel()->getDisplayUnit());
       int index = this->clientModel->getOptionsModel()->getDisplayUnit();
        ui->lineEditBoxUnit->setText(lw->model()->index(index,0).data(0).toString());
        ui->lineEditLanguage->setText(this->clientModel->getOptionsModel()->getLanguage());
        ui->lineEditDigits->setText(this->clientModel->getOptionsModel()->getDigits());
       /*lw->selectionModel()->setCurrentIndex(lw->model()->index(index,0), QItemSelectionModel::Select);
       lwLanguage->selectionModel()->setCurrentIndex(lwLanguage->model()->index(ui->comboBoxLanguage->currentIndex(),0), QItemSelectionModel::Select);
       lwDigits->selectionModel()->setCurrentIndex(lwDigits->model()->index(ui->comboBoxDigits->currentIndex(),0), QItemSelectionModel::Select);*/

    }
}

SettingsDisplayOptionsWidget::~SettingsDisplayOptionsWidget(){
    delete ui;
}

void SettingsDisplayOptionsWidget::handleClick(const QModelIndex &index)
{
   ui->lineEditBoxUnit->setText(index.data(0).toString());
   ui->comboBoxUnit->setCurrentIndex(index.row());
   pw->hide();
   ui->lineEditBoxUnit->addAction(btnBoxUnit, QLineEdit::TrailingPosition);
   ui->lineEditBoxUnit->removeAction(btnUpBoxUnit);
}


void SettingsDisplayOptionsWidget::handleLanguageClick(const QModelIndex &index)
{
   ui->lineEditLanguage->setText(index.data(0).toString());
   ui->comboBoxLanguage->setCurrentText(index.data(0).toString());
   pwLanguage->hide();
   ui->lineEditLanguage->addAction(btnBoxLanguage, QLineEdit::TrailingPosition);
   ui->lineEditLanguage->removeAction(btnUpBoxLanguage);
}
void SettingsDisplayOptionsWidget::handleDigitsClick(const QModelIndex &index)
{
   ui->lineEditDigits->setText(index.data(0).toString());
   ui->comboBoxDigits->setCurrentText(index.data(0).toString());
   pwDigits->hide();
   ui->lineEditDigits->addAction(btnBoxDigits, QLineEdit::TrailingPosition);
   ui->lineEditDigits->removeAction(btnUpBoxDigits);
}

void SettingsDisplayOptionsWidget::onBoxUnitClicked()
{
   if(pwLanguage->isVisible()){
      pwLanguage->hide();
      ui->lineEditLanguage->addAction(btnBoxLanguage, QLineEdit::TrailingPosition);
      ui->lineEditLanguage->removeAction(btnUpBoxLanguage);
   }
   if(pwDigits->isVisible()){
      pwDigits->hide();
      ui->lineEditDigits->addAction(btnBoxDigits, QLineEdit::TrailingPosition);
      ui->lineEditDigits->removeAction(btnUpBoxDigits);
   }
   if(pw->isVisible()){
      pw->hide();
      ui->lineEditBoxUnit->addAction(btnBoxUnit, QLineEdit::TrailingPosition);
      ui->lineEditBoxUnit->removeAction(btnUpBoxUnit);
      return;
   }
   ui->lineEditBoxUnit->addAction(btnUpBoxUnit, QLineEdit::TrailingPosition);
   ui->lineEditBoxUnit->removeAction(btnBoxUnit);
   QPoint pos = ui->lineEditBoxUnit->pos();
   QPoint point = ui->lineEditBoxUnit->rect().bottomRight();
   pw->setFixedSize(ui->lineEditBoxUnit->width() + 20,140);
   pos.setY(pos.y() + point.y() - 7);
   pos.setX(pos.x() - 10);
   pw->move(pos);
   pw->show();
}


void SettingsDisplayOptionsWidget::onBoxLanguageClicked()
{
   if(pw->isVisible()){
      pw->hide();
      ui->lineEditBoxUnit->addAction(btnBoxUnit, QLineEdit::TrailingPosition);
      ui->lineEditBoxUnit->removeAction(btnUpBoxUnit);
   }
   if(pwDigits->isVisible()){
      pwDigits->hide();
      ui->lineEditDigits->addAction(btnBoxDigits, QLineEdit::TrailingPosition);
      ui->lineEditDigits->removeAction(btnUpBoxDigits);
   }
   if(pwLanguage->isVisible()){
      pwLanguage->hide();
      ui->lineEditLanguage->addAction(btnBoxLanguage, QLineEdit::TrailingPosition);
      ui->lineEditLanguage->removeAction(btnUpBoxLanguage);

      return;
   }
   ui->lineEditLanguage->addAction(btnUpBoxLanguage, QLineEdit::TrailingPosition);
   ui->lineEditLanguage->removeAction(btnBoxLanguage);
   QPoint pos = ui->lineEditLanguage->pos();
   QPoint point = ui->lineEditLanguage->rect().bottomRight();
   pwLanguage->setFixedSize(ui->lineEditLanguage->width() + 20,400);
   pos.setY(pos.y() + point.y() - 7);
   pos.setX(pos.x() - 10);
   pwLanguage->move(pos);
   pwLanguage->show();
}
void SettingsDisplayOptionsWidget::onBoxDigitsClicked()
{
   if(pw->isVisible()){
      pw->hide();ui->lineEditBoxUnit->addAction(btnBoxUnit, QLineEdit::TrailingPosition);
      ui->lineEditBoxUnit->removeAction(btnUpBoxUnit);
   }
   if(pwLanguage->isVisible()){
      pwLanguage->hide();
      ui->lineEditLanguage->addAction(btnBoxLanguage, QLineEdit::TrailingPosition);
      ui->lineEditLanguage->removeAction(btnUpBoxLanguage);
   }
   if(pwDigits->isVisible()){
      pwDigits->hide();
      ui->lineEditDigits->addAction(btnBoxDigits, QLineEdit::TrailingPosition);
      ui->lineEditDigits->removeAction(btnUpBoxDigits);
      return;
   }
   ui->lineEditDigits->addAction(btnUpBoxDigits, QLineEdit::TrailingPosition);
   ui->lineEditDigits->removeAction(btnBoxDigits);
   QPoint pos = ui->lineEditDigits->pos();
   QPoint point = ui->lineEditDigits->rect().bottomRight();
   pwDigits->setFixedSize(ui->lineEditDigits->width() + 20,245);
   pos.setY(pos.y() + point.y() - 7);
   pos.setX(pos.x() - 10);
   pwDigits->move(pos);
   pwDigits->show();
}
