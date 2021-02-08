// Copyright (c) 2019 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ADDRESSLABELROW_H
#define ADDRESSLABELROW_H

#include <QWidget>
#include <QGraphicsDropShadowEffect>
#include <QtCore/QModelIndex>

namespace Ui {
class AddressLabelRow;
}

class AddressLabelRow : public QWidget
{
    Q_OBJECT

public:
    explicit AddressLabelRow(QWidget *parent = nullptr);
    ~AddressLabelRow();

    void init(bool isLightTheme, bool isHover);

    void updateState(bool isLightTheme, bool isHovered, bool isSelected);
    void updateView(QString address, QString label);
    void setIndex(QModelIndex pIndex);
    QModelIndex getIndex();
    bool getButonActive();
protected:
    virtual void enterEvent(QEvent *);
    virtual void leaveEvent(QEvent *);
    bool eventFilter(QObject *obj, QEvent *event);

Q_SIGNALS:
   void onMenuClicked();

private:
    Ui::AddressLabelRow *ui;
   QGraphicsDropShadowEffect* shadowEffect =nullptr;
   bool btn = false;
   QModelIndex index;
};

#endif // ADDRESSLABELROW_H
