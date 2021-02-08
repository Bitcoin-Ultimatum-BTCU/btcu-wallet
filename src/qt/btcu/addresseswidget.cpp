// Copyright (c) 2019-2020 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "qt/btcu/addresseswidget.h"
#include "qt/btcu/forms/ui_addresseswidget.h"
#include "qt/btcu/addnewaddressdialog.h"
#include "qt/btcu/tooltipmenu.h"

#include "qt/btcu/addnewcontactdialog.h"
#include "qt/btcu/btcugui.h"
#include "guiutil.h"
#include "qt/btcu/qtutils.h"
#include "walletmodel.h"

#include <QModelIndex>
#include <QRegExpValidator>


#define DECORATION_SIZE 60
#define NUM_ITEMS 3

class ContactsHolder : public FurListRow<QWidget*>
{
public:
    ContactsHolder();

    explicit ContactsHolder(bool _isLightTheme) : FurListRow(), isLightTheme(_isLightTheme){}

    AddressLabelRow* createHolder(int pos) override{
        if (!cachedRow) cachedRow = new AddressLabelRow();
        cachedRow->init(isLightTheme, false);
        return cachedRow;
    }

    void init(QWidget* holder,const QModelIndex &index, bool isHovered, bool isSelected) const override{
        AddressLabelRow* row = static_cast<AddressLabelRow*>(holder);

        row->updateState(isLightTheme, isHovered, isSelected);

        QString address = index.data(Qt::DisplayRole).toString();
        QModelIndex sibling = index.sibling(index.row(), AddressTableModel::Label);
        QString label = sibling.data(Qt::DisplayRole).toString();

        row->updateView(address, label);
    }

    QColor rectColor(bool isHovered, bool isSelected) override{
       return QColor("#FFFFFF");
        /*return getRowColor(isLightTheme, isHovered, isSelected);*/
    }

   AddressLabelRow* getWidget() override{
      return cachedRow;
   }

    ~ContactsHolder() override{}

    bool isLightTheme;
    AddressLabelRow* cachedRow = nullptr;
};


AddressesWidget::AddressesWidget(BTCUGUI* parent) :
    PWidget(parent),
    ui(new Ui::AddressesWidget)
{
    ui->setupUi(this);

    this->setStyleSheet(parent->styleSheet());

    delegate = new FurAbstractListItemDelegate(
                DECORATION_SIZE,
                new ContactsHolder(isLightTheme()),
                this
    );

    /* Containers */
    setCssProperty(ui->left, "container-border");
    ui->left->setContentsMargins(0,20,0,20);
    setCssProperty(ui->right, "container-border");
    ui->right->setContentsMargins(20,10,20,20);
    //setCssProperty(ui->listAddresses, "container");
    setCssProperty(ui->scrollAddresses, "container");
   //ui->listAddresses->setContentsMargins(9,9,9,9);
   ui->layoutNewContact->setVisible(true);

    // Title
    ui->labelTitle->setText(tr("Address list"));
    ui->labelSubtitle1->setText(tr("You can add a new one in the options menu to the side."));
   ui->labelSubtitle1->setVisible(false);
    setCssTitleScreen(ui->labelTitle);
    setCssSubtitleScreen(ui->labelSubtitle1);
   setCssTitleScreen(ui->labelAddAddess);
   setCssSubtitleScreen(ui->labelAddressTitle);

    // Change eddress option
   ui->btnAddContact->setVisible(false);
    ui->btnAddContact->setTitleClassAndText("btn-title-grey", "Add new contact");
    ui->btnAddContact->setSubTitleClassAndText("text-subtitle", "Generate a new address to receive tokens.");
    ui->btnAddContact->setRightIconClass("ic-arrow-down");

    setCssProperty(ui->labelListAddress, "text-body2-grey");
    setCssProperty(ui->labelListName, "text-body2-grey");

    // List Addresses
    ui->listAddresses->setItemDelegate(delegate);
    ui->listAddresses->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    ui->listAddresses->setMinimumHeight(NUM_ITEMS * (DECORATION_SIZE + 2));
    ui->listAddresses->setAttribute(Qt::WA_MacShowFocusRect, false);
    ui->listAddresses->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->listAddresses->setUniformItemSizes(true);
    ui->listAddresses->setVisible(false);


    //Empty List
    ui->emptyContainer->setVisible(false);
    setCssProperty(ui->pushImgEmpty, "img-empty-contacts");

    ui->labelEmpty->setText(tr("No contacts yet"));
    setCssProperty(ui->labelEmpty, "text-empty");

    // Add Contact
    setCssProperty(ui->layoutNewContact, "container");

    // Name
   ui->labelName->setVisible(false);
    ui->labelName->setText(tr("Contact name"));
    setCssProperty(ui->labelName, "text-title");
    ui->lineEditName->setPlaceholderText(tr("e.g. John Doe"));
    setCssEditLine(ui->lineEditName, true);

    // Address
   ui->labelAddress->setVisible(false);
    ui->labelAddress->setText(tr("Enter BTCU address"));
    setCssProperty(ui->labelAddress, "text-title");
    ui->lineEditAddress->setPlaceholderText("e.g. D7VFR83SQbiezrW72hjc…");
    setCssEditLine(ui->lineEditAddress, true);
    ui->lineEditAddress->setValidator(new QRegExpValidator(QRegExp("^[A-Za-z0-9]+"), ui->lineEditName));

    // Buttons
    ui->btnSave->setText(tr("SAVE"));
   ui->btnSave->setProperty("cssClass","btn-secundary");
    //setCssBtnPrimary(ui->btnSave);

    connect(ui->listAddresses, SIGNAL(clicked(QModelIndex)), this, SLOT(handleAddressClicked(QModelIndex)));
    connect(ui->btnSave, SIGNAL(clicked()), this, SLOT(onStoreContactClicked()));
    //connect(ui->btnAddContact, SIGNAL(clicked()), this, SLOT(onAddContactShowHideClicked()));
}

void AddressesWidget::handleAddressClicked(const QModelIndex &index){
    ui->listAddresses->setCurrentIndex(index);
    AddressLabelRow* row= qobject_cast<AddressLabelRow*>(delegate->getRowFactory()->getWidget());
    //if(!row->getButonActive()) return;
    QRect rect = ui->listAddresses->visualRect(index);
    QPoint pos = rect.topRight();
    pos.setX(pos.x() - (DECORATION_SIZE * 2));
    pos.setY(pos.y() + (DECORATION_SIZE)*2);

    QModelIndex rIndex = filter->mapToSource(index);

    if(!this->menu){
        this->menu = new TooltipMenu(window, this);
        this->menu->setCopyBtnText(tr("Copy"));
        this->menu->setEditBtnText(tr("Edit"));
        this->menu->setDeleteBtnText(tr("Delete"));
        connect(this->menu, &TooltipMenu::message, this, &AddressesWidget::message);
        connect(this->menu, SIGNAL(onEditClicked()), this, SLOT(onEditClicked()));
        connect(this->menu, SIGNAL(onDeleteClicked()), this, SLOT(onDeleteClicked()));
        connect(this->menu, SIGNAL(onCopyClicked()), this, SLOT(onCopyClicked()));
    }else {
        this->menu->hide();
    }
    this->index = rIndex;
    menu->move(pos);
    menu->show();
}

void AddressesWidget::loadWalletModel(){
    if(walletModel) {
        addressTablemodel = walletModel->getAddressTableModel();
        this->filter = new AddressFilterProxyModel(QStringList({AddressTableModel::Send, AddressTableModel::ColdStakingSend}), this);
        this->filter->setSourceModel(addressTablemodel);
        ui->listAddresses->setModel(this->filter);
        ui->listAddresses->setModelColumn(AddressTableModel::Address);

        updateListView();
       addRows();
    }
}

void AddressesWidget::updateListView(){
    bool empty = addressTablemodel->sizeSend() == 0;
    if(empty)
    {
       ui->verticalSpacer_7->changeSize(0, 0, QSizePolicy::Fixed,QSizePolicy::Fixed);
    }else{
       ui->verticalSpacer_7->changeSize(0, 0, QSizePolicy::Fixed,QSizePolicy::Expanding);
    }
    ui->emptyContainer->setVisible(empty);
    //ui->listAddresses->setVisible(!empty);
    ui->scrollAddresses->setVisible(!empty);
   ui->labelListName->setVisible(!empty);
   ui->labelListAddress->setVisible(!empty);
}

void AddressesWidget::onStoreContactClicked(){
    if (walletModel) {
        QString label = ui->lineEditName->text();
        QString address = ui->lineEditAddress->text();

        if(label.length() > 30)
        {
           informError(tr("Name exceeds 30 characters"));
           return;
        }

        if (!walletModel->validateAddress(address)) {
            setCssEditLine(ui->lineEditAddress, false, true);
            informError(tr("Invalid Contact Address"));
            return;
        }

        CBTCUAddress btcuAdd = CBTCUAddress(address.toUtf8().constData());
        if (walletModel->isMine(btcuAdd)) {
            setCssEditLine(ui->lineEditAddress, false, true);
            inform(tr("Cannot store your own address as contact"));
            return;
        }

        QString storedLabel = walletModel->getAddressTableModel()->labelForAddress(address);

        if(!storedLabel.isEmpty()){
            informError(tr("Address already stored, label: %1").arg("\'"+storedLabel+"\'"));
            return;
        }

        if (walletModel->updateAddressBookLabels(btcuAdd.Get(), label.toUtf8().constData(),
                btcuAdd.IsStakingAddress() ? AddressBook::AddressBookPurpose::COLD_STAKING_SEND : AddressBook::AddressBookPurpose::SEND)
                ) {
            ui->lineEditAddress->setText("");
            ui->lineEditName->setText("");
            setCssEditLine(ui->lineEditAddress, true, true);
            setCssEditLine(ui->lineEditName, true, true);

            if (ui->emptyContainer->isVisible()) {
                ui->emptyContainer->setVisible(false);
                ui->scrollAddresses->setVisible(true);
               ui->labelListName->setVisible(true);
               ui->labelListAddress->setVisible(true);
            }
           updateAddresses();
            informWarning(tr("New Contact Stored"));
        } else {
            informError(tr("Error Storing Contact"));
        }
    }
}

void AddressesWidget::onEditClicked(){
    QString address = index.data(Qt::DisplayRole).toString();
    QString currentLabel = index.sibling(index.row(), AddressTableModel::Label).data(Qt::DisplayRole).toString();
    showHideOp(true);
    AddNewContactDialog *dialog = new AddNewContactDialog(window);
    dialog->setTexts(tr("Edit Address Label"));
    dialog->setData(address, currentLabel);
    if(openDialogWithOpaqueBackground(dialog, window)){
        if(walletModel->updateAddressBookLabels(
                CBTCUAddress(address.toStdString()).Get(), dialog->getLabel().toStdString(), addressTablemodel->purposeForAddress(address.toStdString()))){
           updateAddresses();
           informWarning(tr("Contact edited"));
        }else{
            informError(tr("Contact edit failed"));
        }
    }
    dialog->deleteLater();
}

void AddressesWidget::onDeleteClicked(){
    if(walletModel) {
        if (ask(tr("Delete Contact"), tr("You are just about to remove the contact:\n%1\nAre you sure?\n").arg(index.data(Qt::DisplayRole).toString().toUtf8().constData()))
        ) {
            if (this->walletModel->getAddressTableModel()->removeRows(index.row(), 1, index)) {

               //addressTablemodel = walletModel->getAddressTableModel();
               updateAddresses();
               updateListView();
                informWarning(tr("Contact Deleted"));
            } else {
                informError(tr("Error deleting a contact"));
            }
        }
    }
}

void AddressesWidget::onCopyClicked(){
    GUIUtil::setClipboard(index.data(Qt::DisplayRole).toString());
    informWarning(tr("Address copied"));
}

void AddressesWidget::onAddContactShowHideClicked(){
    if(!ui->layoutNewContact->isVisible()){
        ui->btnAddContact->setRightIconClass("btn-dropdown", true);
        ui->layoutNewContact->setVisible(true);
    }else {
        ui->btnAddContact->setRightIconClass("ic-arrow", true);
        ui->layoutNewContact->setVisible(false);
    }
}

void AddressesWidget::changeTheme(bool isLightTheme, QString& theme){
    static_cast<ContactsHolder*>(this->delegate->getRowFactory())->isLightTheme = isLightTheme;
}

AddressesWidget::~AddressesWidget(){
    delete ui;
}


void AddressesWidget::addRows()
{
   int Count = this->filter->rowCount();
   if(Count > 0)
   {
      QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect();
      shadowEffect->setColor(QColor(0, 0, 0, 22));
      shadowEffect->setXOffset(0);
      shadowEffect->setYOffset(2);
      shadowEffect->setBlurRadius(6);
      if(SpacerAddresses)
      {
         ui->scrollAreaWidgetContents->layout()->removeItem(SpacerAddresses);
         delete SpacerAddresses;
      }
      for (int i = 0; i < Count; i++)
      {
         SpacerAddresses = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);
         AddressLabelRow* row = new AddressLabelRow(ui->scrollAddresses);
         QModelIndex rowIndex = this->filter->index(i, AddressTableModel::Address);
         QModelIndex sibling = rowIndex.sibling(i, AddressTableModel::Address);
         QString address = sibling.data(Qt::DisplayRole).toString();
         sibling = rowIndex.sibling(i, AddressTableModel::Label);
         QString label = sibling.data(Qt::DisplayRole).toString();
         row->updateView(address, label);
         row->setIndex(rowIndex);
         row->setGraphicsEffect(shadowEffect);
         connect(row, SIGNAL(onMenuClicked()), this, SLOT(onpbnMenuClicked()));
         ui->scrollAreaWidgetContents->layout()->addWidget(row);
      }
      ui->scrollAreaWidgetContents->layout()->addItem(SpacerAddresses);
   }
}

void AddressesWidget::onpbnMenuClicked()
{
   QPoint pos;
   QPushButton* btnMenu = (QPushButton*) sender();
   AddressLabelRow* row = (AddressLabelRow*) sender();
   pos = btnMenu->rect().bottomRight();
   pos = btnMenu->mapToParent(pos);
   pos = btnMenu->parentWidget()->mapToParent(pos);
   pos = btnMenu->parentWidget()->parentWidget()->mapToParent(pos);
   pos.setX(pos.x() - (DECORATION_SIZE * 2));

   QModelIndex rIndex = filter->mapToSource(row->getIndex());

   if(!this->menu){
      this->menu = new TooltipMenu(window, ui->scrollAddresses);
      this->menu->setCopyBtnText(tr("Copy"));
      this->menu->setEditBtnText(tr("Edit"));
      this->menu->setDeleteBtnText(tr("Delete"));
      connect(this->menu, &TooltipMenu::message, this, &AddressesWidget::message);
      connect(this->menu, SIGNAL(onEditClicked()), this, SLOT(onEditClicked()));
      connect(this->menu, SIGNAL(onDeleteClicked()), this, SLOT(onDeleteClicked()));
      connect(this->menu, SIGNAL(onCopyClicked()), this, SLOT(onCopyClicked()));
   }else {
      if(this->menu->isVisible() && this->index == rIndex)
      {
         this->menu->hide();
         delete this->menu;
         this->menu = nullptr;
         return;
      }
      else
      {
         this->menu->hide();
      }
   }
   this->index = rIndex;
   menu->move(pos);
   menu->show();

}


void AddressesWidget::updateAddresses()
{
   addressTablemodel->refreshAddressTable();
   this->filter->setSourceModel(addressTablemodel);
   QList<AddressLabelRow *> listRow = ui->scrollAreaWidgetContents->findChildren<AddressLabelRow*> ();
   int size = listRow.length();
   AddressLabelRow * row;
   for(int i = 0; i < size; i++)
   {
      row = listRow.at(i);
      ui->scrollAreaWidgetContents->layout()->removeWidget(row);
      delete row;
   }
   addRows();
}