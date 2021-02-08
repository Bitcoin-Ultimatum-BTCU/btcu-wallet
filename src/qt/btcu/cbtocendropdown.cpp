//
// Created by ek-ut on 27.11.2020.
//

#include "cbtocendropdown.h"
#include "qt/btcu/furlistrow.h"
#include "qt/btcu/qtutils.h"
#define DECORATION_SIZE 30
#define NUM_ITEMS 6

class TocenViewHolder : public FurListRow<QWidget*>
{
public:
   TocenViewHolder();

   explicit TocenViewHolder(bool _isLightTheme) : FurListRow(), isLightTheme(_isLightTheme){}

   CBTokenRow* createHolder(int pos) override{
      if (!row) row = new CBTokenRow();
      //row->init(true, false);
      return row;
   }

   void init(QWidget* holder,const QModelIndex &index, bool isHovered, bool isSelected) const override{
      CBTokenRow* row = static_cast<CBTokenRow*>(holder);
      QString Tocen = index.data(Qt::DisplayRole).toString();
      row->setText(Tocen, "0.32313");
      int rowCount =index.model()->rowCount();
      int indexrow = index.row();
      row->setVisibleDivisory(true);
      if(rowCount-1 == indexrow)
      {
         row->setVisibleDivisory(false);
      }
   }

   QColor rectColor(bool isHovered, bool isSelected) override{
      return getRowColor(isLightTheme, isHovered, isSelected);
   }

   ~TocenViewHolder() override{}

   bool isLightTheme;
   CBTokenRow* row = nullptr;
};


CbTocenDropdown::CbTocenDropdown(int minWidth, int minHeight, BTCUGUI *parent):
PWidget(parent)
{
   this->setStyleSheet(parent->styleSheet());
   delegate = new FurAbstractListItemDelegate(
   DECORATION_SIZE,
   new TocenViewHolder(true),
   this
   );
   setMinimumWidth(minWidth);
   setMinimumHeight(minHeight);
   setContentsMargins(0,0,0,0);
   setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

   frameList = new QFrame(this);
   //frameList->setProperty("cssClass", "container-border-light");
   //frameList->setContentsMargins(10,10,10,10);
   frameList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
   list = new QListView(frameList);
   list->setMinimumWidth(minWidth);
   list->setProperty("cssClass", "container-border-light");
   list->setItemDelegate(delegate);
   list->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
   list->setMinimumHeight(NUM_ITEMS * (DECORATION_SIZE + 2));
   list->setAttribute(Qt::WA_MacShowFocusRect, false);
   list->setSelectionBehavior(QAbstractItemView::SelectRows);

   connect(list, SIGNAL(clicked(QModelIndex)), this, SLOT(handleClick(QModelIndex)));

}

void CbTocenDropdown::resizeList(int minWidth, int mintHeight)
{
   list->setMinimumWidth(minWidth);
   setMinimumWidth(minWidth);
   setMinimumHeight(mintHeight);
   frameList->setMinimumHeight(mintHeight);
   frameList->setMinimumWidth(minWidth);
   list->setMinimumHeight(mintHeight);
   list->resize(minWidth,mintHeight);
   //list->adjustSize();
   frameList->resize(minWidth, mintHeight);
   resize(minWidth, mintHeight);
   //adjustSize();
   update();
}
void CbTocenDropdown::setTokenModel(CBTokenModel* _model, const QString& type)
{

        list->setModel(_model);
        list->setModelColumn(CBTokenModel::Date);
}
void CbTocenDropdown::handleClick(const QModelIndex &index)
{
   QModelIndex myindex = index.sibling(index.row(), CBTokenModel::Name);
   QString Tocen = myindex.data(CBTokenModel::Date).toString();
   myindex = index.sibling(index.row(), CBTokenModel::Label);
   QVariant variant = myindex.data(CBTokenModel::Date);
   QPixmap Pix = variant.value<QPixmap>();
   if(Tocen.isEmpty())
   {
      Tocen = "Empty tocen";
   }
   Q_EMIT contactSelected(Tocen, Pix);
   close();
}

void CbTocenDropdown::showEvent(QShowEvent *event)
{

   //Q_EMIT TimerHide();
}
void CbTocenDropdown::leaveEvent(QEvent *) {
   QTimer::singleShot(100, [this](){
      Q_EMIT TimerHide();
   });
   /*hide();
   Q_EMIT TimerHide();*/
}