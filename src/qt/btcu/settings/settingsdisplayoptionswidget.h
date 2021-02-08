// Copyright (c) 2019 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef SETTINGSDISPLAYOPTIONSWIDGET_H
#define SETTINGSDISPLAYOPTIONSWIDGET_H

#include <QWidget>
#include <QDataWidgetMapper>
#include "qt/btcu/pwidget.h"
#include <QListView>
#include <QAction>

namespace Ui {
class SettingsDisplayOptionsWidget;
}

class SettingsDisplayOptionsWidget : public PWidget
{
    Q_OBJECT

public:
    explicit SettingsDisplayOptionsWidget(BTCUGUI* _window = nullptr, QWidget *parent = nullptr);
    ~SettingsDisplayOptionsWidget();

    void setMapper(QDataWidgetMapper *mapper);
    void initLanguages();
    void loadClientModel() override;

public Q_SLOTS:
    void onResetClicked();
   void handleClick(const QModelIndex &index);
   void handleLanguageClick(const QModelIndex &index);
   void handleDigitsClick(const QModelIndex &index);
   void onBoxUnitClicked();
   void onBoxLanguageClicked();
   void onBoxDigitsClicked();

private:
    Ui::SettingsDisplayOptionsWidget *ui;
    QWidget * pw = nullptr;
    QListView *lw = nullptr;
   QWidget * pwLanguage = nullptr;
   QListView *lwLanguage = nullptr;
   QWidget * pwDigits = nullptr;
   QListView *lwDigits = nullptr;
   QAction *btnBoxLanguage = nullptr;
   QAction *btnBoxUnit = nullptr;
   QAction *btnBoxDigits = nullptr;
   QAction *btnUpBoxLanguage = nullptr;
   QAction *btnUpBoxUnit = nullptr;
   QAction *btnUpBoxDigits = nullptr;
};

#endif // SETTINGSDISPLAYOPTIONSWIDGET_H
