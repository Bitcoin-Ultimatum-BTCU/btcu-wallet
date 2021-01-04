// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef LROW_H
#define LROW_H

#include <QWidget>

#include <memory>

namespace Ui {
class LRow;
}

class LRow : public QWidget
{
    Q_OBJECT

public:
    explicit LRow(QWidget *parent = nullptr);
    ~LRow();

    void updateView(const QString& address, const QString& label, bool isLeaser, const QString& amount, const QString& reward);
    void updateState(bool isLightTheme, bool isHovered, bool isSelected);
    void showMenuButton(bool show);
protected:
    virtual void enterEvent(QEvent *);
    virtual void leaveEvent(QEvent *);

private:
    std::unique_ptr<Ui::LRow> ui;

    bool fShowMenuButton = true;
};

#endif // LROW_H
