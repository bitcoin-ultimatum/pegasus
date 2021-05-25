// Copyright (c) 2019-2020 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "qt/btcu/masternodeswidget.h"
#include "qt/btcu/forms/ui_masternodeswidget.h"
#include "qt/btcu/qtutils.h"
#include "qt/btcu/createmasternodewidget.h"
#include "qt/btcu/mninfodialog.h"

#include "clientmodel.h"
#include "guiutil.h"
#include "init.h"
#include "wallet/wallet.h"
#include "walletmodel.h"
#include "util.h"
#include "activemasternode.h"
#include "masternode-sync.h"
#include "masternodeconfig.h"
#include "masternodeman.h"
#include "sync.h"
#include "askpassphrasedialog.h"
#include "qt/btcu/optionbutton.h"
#include <boost/filesystem.hpp>
#include <iostream>
#include <fstream>
#include "qt/btcu/createmasternodewidget.h"
#include "qt/btcu/createvalidatorwidget.h"
#include "qt/btcu/masternodewizarddialog.h"
#include "univalue.h"
#include "../../rpc/server.h"
#include "../../init.h"

#include "main.h"
#include <QGraphicsDropShadowEffect>

#define DECORATION_SIZE 65
#define NUM_ITEMS 3
#define REQUEST_START_ALL 1
#define REQUEST_START_MISSING 2

enum class LeaserType: int {
    ValidatorNode = 1,
    MasterNode = 2
};

class CLeasingManager: public CValidationInterface {
public:
    void GetAllAmountsLeasedTo(CPubKey &pubKey, CAmount &amount) const;
    void CalcLeasingReward(CPubKey &pubKey, CAmount &amount) const;
};

class MNHolder : public FurListRow<QWidget*>
{
public:
    MNHolder();

    explicit MNHolder(bool _isLightTheme) : FurListRow(), isLightTheme(_isLightTheme){}

    MNRow* createHolder(int pos) override{
        if(!cachedRow) cachedRow = new MNRow();
        return cachedRow;
    }

    void init(QWidget* holder,const QModelIndex &index, bool isHovered, bool isSelected) const override{
        MNRow* row = static_cast<MNRow*>(holder);
        QString label = index.data(Qt::DisplayRole).toString();
        QString address = index.sibling(index.row(), MNModel::ADDRESS).data(Qt::DisplayRole).toString();
        QString status = index.sibling(index.row(), MNModel::STATUS).data(Qt::DisplayRole).toString();
        bool wasCollateralAccepted = index.sibling(index.row(), MNModel::WAS_COLLATERAL_ACCEPTED).data(Qt::DisplayRole).toBool();
        //row->updateView("Address: " + address, label, status, wasCollateralAccepted);
    }

    QColor rectColor(bool isHovered, bool isSelected) override{
        return getRowColor(isLightTheme, isHovered, isSelected);
    }

    ~MNHolder() override{}

    bool isLightTheme;
    MNRow* cachedRow = nullptr;
};

MasterNodesWidget::MasterNodesWidget(BTCUGUI *parent) :
    PWidget(parent),
    ui(new Ui::MasterNodesWidget),
    isLoading(false)
{
    ui->setupUi(this);

    delegate = new FurAbstractListItemDelegate(
            DECORATION_SIZE,
            new MNHolder(isLightTheme()),
            this
    );
    mnModel = new MNModel(this);

    this->setStyleSheet(parent->styleSheet());

    /* Containers */
    //setCssProperty(ui->left, "container");
    //ui->left->setContentsMargins(0,20,0,20);
    //setCssProperty(ui->right, "container-right");
    //ui->right->setContentsMargins(20,20,20,20);
   //ui->right->setVisible(false);
   setCssProperty(ui->scrollArea, "container");
   setCssProperty(ui->scrollAreaMy, "container");
    ui->scrollAreaMy->resize(50, ui->scrollAreaMy->height());
   this->setGraphicsEffect(0);
   //ui->scrollAreaMy->setGraphicsEffect(0);
    /* Light Font */
    QFont fontLight;
    fontLight.setWeight(QFont::Light);

    /* Title */
    ui->labelTitle->setText(tr("Nodes rank"));
    setCssTitleScreen(ui->labelTitle);
    ui->labelTitle->setFont(fontLight);

   setCssProperty(ui->labelName, "text-body2-grey");
   setCssProperty(ui->labelAddress, "text-body2-grey");
   setCssProperty(ui->labelLeasing, "text-body2-grey");
   setCssProperty(ui->labelBlockHeight, "text-body2-grey");
   setCssProperty(ui->labelType, "text-body2-grey");
   setCssProperty(ui->labelProfit, "text-body2-grey");

   setCssProperty(ui->pushImgEmpty, "img-empty-master");
   ui->labelEmpty->setText(tr("No active Masternode yet"));
   setCssProperty(ui->labelEmpty, "text-empty");

    //ui->labelSubtitle1->setText(tr("Full nodes that incentivize node operators to perform the core consensus functions \n and vote on trasury system receiving a periodic reward."));
    //setCssSubtitleScreen(ui->labelSubtitle1);

    /* Buttons */
   //ui->pbnGlobalMasternodes->setProperty("cssClass","btn-secundary-small");
   ui->pbnGlobalMasternodes->setProperty("cssClass","btn-check-left");
   ui->pbnGlobalMasternodes->setChecked(true);
   //ui->pbnMyMasternode->setProperty("cssClass","btn-primary-small");
   ui->pbnMyMasternodes->setProperty("cssClass","btn-check-left");
   ui->pbnMyMasternodes->setChecked(false);
   ui->pbnMasternode->setProperty("cssClass","btn-secundary-small");

   //setCssBtnSecondary(ui->pbnTempAdd);
   ui->pbnTempAdd->hide();

   //connect(ui->pbnTempAdd, SIGNAL(clicked()), this, SLOT(onTempADD()));
   connect(ui->pbnMasternode, SIGNAL(clicked()), this, SLOT(onpbnMasternodeClicked()));
   connect(ui->pbnMyMasternodes, SIGNAL(clicked()), this, SLOT(onpbnMyMasternodesClicked()));
   connect(ui->pbnGlobalMasternodes, SIGNAL(clicked()), this, SLOT(onpbnGlobalMasternodesClicked()));
   showHistory();

   /*ui->pushButtonSave->setText(tr("Create Masternode Controller"));
    setCssBtnPrimary(ui->pushButtonSave);
    setCssBtnPrimary(ui->pushButtonStartAll);
    setCssBtnPrimary(ui->pushButtonStartMissing);
   ui->pushButtonSave->setVisible(false);
   ui->pushButtonStartAll->setVisible(false);
   ui->pushButtonStartMissing->setVisible(false);*/

    /* Options */
    /*ui->btnAbout->setTitleClassAndText("btn-title-grey", "What is a Masternode?");
    ui->btnAbout->setSubTitleClassAndText("text-subtitle", "FAQ explaining what Masternodes are");
    ui->btnAboutController->setTitleClassAndText("btn-title-grey", "What is a Controller?");
    ui->btnAboutController->setSubTitleClassAndText("text-subtitle", "FAQ explaining what is a Masternode Controller");

    setCssProperty(ui->listMn, "container");
    ui->listMn->setItemDelegate(delegate);
    ui->listMn->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    ui->listMn->setMinimumHeight(NUM_ITEMS * (DECORATION_SIZE + 2));
    ui->listMn->setAttribute(Qt::WA_MacShowFocusRect, false);
    ui->listMn->setSelectionBehavior(QAbstractItemView::SelectRows);

    ui->emptyContainer->setVisible(false);
    setCssProperty(ui->pushImgEmpty, "img-empty-master");
    ui->labelEmpty->setText(tr("No active Masternode yet"));
    setCssProperty(ui->labelEmpty, "text-empty");

    connect(ui->pushButtonSave, SIGNAL(clicked()), this, SLOT(onCreateMNClicked()));
    connect(ui->pushButtonStartAll, &QPushButton::clicked, [this]() {
        onStartAllClicked(REQUEST_START_ALL);
    });
    connect(ui->pushButtonStartMissing, &QPushButton::clicked, [this]() {
        onStartAllClicked(REQUEST_START_MISSING);
    });
    connect(ui->listMn, SIGNAL(clicked(QModelIndex)), this, SLOT(onMNClicked(QModelIndex)));
    connect(ui->btnAbout, &OptionButton::clicked, [this](){window->openFAQ(9);});
    connect(ui->btnAboutController, &OptionButton::clicked, [this](){window->openFAQ(10);});
*/
    timer = new QTimer(this);
    day = QDateTime::currentDateTime().toString("dd.MM.yy");
}

void MasterNodesWidget::onTempADD()
{
   showHistory();
}

void MasterNodesWidget::showHistory()
{
   ui->verticalSpacer_2->changeSize(0,0,QSizePolicy::Fixed,QSizePolicy::Fixed);
   ui->verticalSpacer_3->changeSize(0,0,QSizePolicy::Fixed,QSizePolicy::Fixed);
   if(ui->pbnMyMasternodes->isChecked())
   {
      ui->scrollArea->setVisible(false);
      ui->scrollAreaMy->setVisible(true);
      if(bShowHistoryMy)
      {
         ui->NodesContainer->setVisible(true);
         ui->emptyContainer->setVisible(false);
      }
      else
      {
         ui->verticalSpacer_2->changeSize(0,0,QSizePolicy::Fixed,QSizePolicy::Expanding);
         ui->verticalSpacer_3->changeSize(0,0,QSizePolicy::Fixed,QSizePolicy::Expanding);
         ui->NodesContainer->setVisible(false);
         ui->emptyContainer->setVisible(true);
      }
   }
   else{
      ui->scrollArea->setVisible(true);
      ui->scrollAreaMy->setVisible(false);
      if(bShowHistory)
      {
         ui->NodesContainer->setVisible(true);
         ui->emptyContainer->setVisible(false);
      }
      else
      {
         ui->verticalSpacer_2->changeSize(0,0,QSizePolicy::Fixed,QSizePolicy::Expanding);
         ui->verticalSpacer_3->changeSize(0,0,QSizePolicy::Fixed,QSizePolicy::Expanding);
         ui->NodesContainer->setVisible(false);
         ui->emptyContainer->setVisible(true);
      }
   }

}

void MasterNodesWidget::onpbnMenuClicked(QModelIndex index)
{
   bool My = ui->pbnMyMasternodes->isChecked();
   this->index = index;
   QPoint pos;
   QPushButton* btnMenu = (QPushButton*) sender();
   pos = btnMenu->rect().bottomRight();
   pos = btnMenu->mapToParent(pos);
   pos = btnMenu->parentWidget()->mapToParent(pos);
   pos = btnMenu->parentWidget()->parentWidget()->mapToParent(pos);

   if(My)
   {
      if(!this->menuMy)
      {
         this->menuMy = new TooltipMenu(window, ui->scrollAreaMy);
         this->menuMy->setCopyBtnText(tr("More information"));
         this->menuMy->setEditBtnText(tr("Upgrade to validator"));
         this->menuMy->setDeleteBtnText(tr("Stop Masternode"));

         this->menuMy->adjustSize();
         this->menuMy->setMinimumWidth(this->menuMy->width() + 10);

         if(pos.y()+ this->menuMy->height() > ui->scrollAreaMy->height())
         {
             pos.setY(pos.y() - (this->menuMy->height() + btnMenu->height()));
         }

          pos.setX(pos.x() - (DECORATION_SIZE * 2.6));

          menuMy->move(pos);
          menuMy->show();

         //connect(this->menuMy, &TooltipMenu::message, this, &AddressesWidget::message);
         //connect(this->menuMy, SIGNAL(onEditClicked()), this, SLOT(onUpgradeMNClicked()), Qt::UniqueConnection);
         //connect(this->menuMy, SIGNAL(onDeleteClicked()), this, SLOT(onDeleteMNClicked()));
         connect(this->menuMy, SIGNAL(onCopyClicked()), this, SLOT(onInfoMNClicked()));
      }else {
         this->menuMy->hide();
         delete this->menuMy;
         menuMy = nullptr;
      }
   }
   else{
      if(!this->menu)
      {
         this->menu = new TooltipMenu(window, ui->scrollArea);
         this->menu->setCopyBtnText(tr("More information"));
         this->menu->setEditBtnText(tr("Upgrade to validator"));
         this->menu->setDeleteBtnText(tr("Stop Masternode")); //delete masternode?
         this->menu->adjustSize();
         //this->menu->setMinimumWidth(this->menu->width() - 10);
         this->menu->setFixedHeight(this->menu->height() - 50);

         if(pos.y()+ this->menu->height() > ui->scrollArea->height())
         {
             pos.setY(pos.y() - (this->menu->height() + btnMenu->height()));
         }

          pos.setX(pos.x() - (DECORATION_SIZE * 2.6));

          menu->move(pos);
          menu->show();

         //connect(this->menu, &TooltipMenu::message, this, &AddressesWidget::message);
         //connect(this->menu, SIGNAL(onEditClicked()), this, SLOT(onUpgradeMNClicked()));
         //connect(this->menu, SIGNAL(onDeleteClicked()), this, SLOT(onDeleteMNClicked()));
         //connect(this->menu, SIGNAL(onCopyClicked()), this, SLOT(onInfoMNClicked()));
      }else {
         if(this->menu->isVisible())
         {
            this->menu->hide();
            delete this->menu;
            this->menu = nullptr;
         }
      }
   }
}
void MasterNodesWidget::onpbnMasternodeClicked()
{
    if(pwalletMain->GetBalance()/100000000 < CREATE_MN_AMOUNT)
    {
        std::string error = "Not enough coins to create masternode, min = " +
              std::to_string(CREATE_MN_AMOUNT) +
              ", current = " + std::to_string(pwalletMain->GetBalance()/100000000);
        informError(error.c_str());
        return;
    }

    MasterNodeWizardDialog* wizardDialog = new MasterNodeWizardDialog(walletModel, window);
    showHideOp(true);
    openDialogWithOpaqueBackground(wizardDialog, window);
    /*
   CreateMasterNodeWidget* newMacterNode = new CreateMasterNodeWidget(window);
   showHideOp(true);
   openDialogWithOpaqueBackground(newMacterNode, window);*/
   //window->goToCreateMasternode();
   //Q_EMIT CreateMasternode();
}

void MasterNodesWidget::onpbnValidatorClicked()
{
   CreateValidatorWidget* newValidator = new CreateValidatorWidget(window);
   showHideOp(true);
   openDialogWithOpaqueBackground(newValidator, window);
   //window->goToCreateValidator();
}


void MasterNodesWidget::onpbnMyMasternodesClicked()
{
    clearScrollWidget();
    ui->pbnGlobalMasternodes->setChecked(false);
    ui->pbnMyMasternodes->setChecked(true);

    QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect();
    shadowEffect->setColor(QColor(0, 0, 0, 22));
    shadowEffect->setXOffset(0);
    shadowEffect->setYOffset(2);
    shadowEffect->setBlurRadius(6);

    bShowHistoryMy = true;

    std::string strConfFile = "masternode.conf";
    std::string strDataDir = GetDataDir().string();
    if (strConfFile != boost::filesystem::basename(strConfFile) + boost::filesystem::extension(strConfFile)){
        throw std::runtime_error(strprintf(_("masternode.conf %s resides outside data directory %s"), strConfFile, strDataDir));
    }

    boost::filesystem::path pathBootstrap = GetDataDir() / strConfFile;
    if (boost::filesystem::exists(pathBootstrap)) {
        boost::filesystem::path pathMasternodeConfigFile = GetMasternodeConfigFile();
        boost::filesystem::ifstream streamConfig(pathMasternodeConfigFile);

        if (streamConfig.good()) {
            int linenumber = 1;
            for (std::string line; std::getline(streamConfig, line); linenumber++) {
                if (line.empty()) continue;
                if (line.at(0) == '#') continue;

                std::string name = "";
                std::string hash = "";

                std::string buffLine = "";
                int count = 0;
                for (int i = 0; i < line.size(); ++i) {
                    if (line.at(i) == ' ') {
                        if (count == 3) break;
                        count++;
                    } else if (count == 0) name += line.at(i);
                    else if (count == 3) hash += line.at(i);
                }

                std::string address = "";
                QModelIndex index;
                int rowCount = filter->rowCount();
                for (int addressNumber = 0; addressNumber < rowCount; addressNumber++) {
                    QModelIndex rowIndex = filter->index(addressNumber, AddressTableModel::Address);
                    QModelIndex sibling = rowIndex.sibling(addressNumber, AddressTableModel::Label);
                    QString label = sibling.data(Qt::DisplayRole).toString();
                    if (label.toStdString() == name) {
                        sibling = rowIndex.sibling(addressNumber, AddressTableModel::Address);
                        address = sibling.data(Qt::DisplayRole).toString().toStdString();
                        index = rowIndex;
                        break;
                    }
                }

                if (address == "") continue;

                uint256 uHash = uint256(hash);
                uint256 uBlock;
                CTransaction tr;
                int blockHeight = -1;

                GetTransaction(uHash, tr, uBlock, true);
                IsTransactionInChain(uHash, blockHeight, tr);

                CKeyID key;
                walletModel->getKeyId(CBTCUAddress(address), key);
                CPubKey pubKey;
                walletModel->getPubKey(key, pubKey);

                QString type = "-";
                //LeaserType type;
                for (auto i : tr.GetOutPoints()) {
                    //type = static_cast<LeaserType>(i.n);
                    LeaserType t = static_cast<LeaserType>(i.n);
                    if (t == LeaserType::MasterNode)
                        type = "Validator";
                    else if (t == LeaserType::ValidatorNode)
                        type = "Masternode";
                }

                CAmount leasingAmount;
                double roundAmount = 0.0;
#ifdef ENABLE_LEASING_MANAGER
                assert(pwalletMain != NULL);
                LOCK2(cs_main, pwalletMain->cs_wallet);

                if (pwalletMain->pLeasingManager)
                    pwalletMain->pLeasingManager->GetAllAmountsLeasedTo(pubKey, leasingAmount);
                roundAmount = round(leasingAmount) / 100000000.0;
#endif

                double reward = calculateDailyReward(QString::fromStdString(address));

                if (SpacerNodeMy) {
                    ui->scrollAreaWidgetContentsMy->layout()->removeItem(SpacerNodeMy);
                    delete SpacerNodeMy;
                }

                QSharedPointer<MNRow> mnrow = QSharedPointer<MNRow>(new MNRow(ui->scrollAreaMy));
                mnrow->setIndex(index);
                mnrow->setGraphicsEffect(shadowEffect);
                connect(mnrow.get(), SIGNAL(onMenuClicked(QModelIndex)), this, SLOT(onpbnMenuClicked(QModelIndex)));
                mnrow->setView(name, address, roundAmount, blockHeight, type, reward);
                ui->scrollAreaWidgetContentsMy->layout()->addWidget(mnrow.get());
                MNRows.push_back(mnrow);

                SpacerNodeMy = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);
                ui->scrollAreaWidgetContentsMy->layout()->addItem(SpacerNodeMy);
            }
        }
    }

    showHistory();
}

void MasterNodesWidget::onpbnGlobalMasternodesClicked()
{
    clearScrollWidget();
   ui->pbnGlobalMasternodes->setChecked(true);
   ui->pbnMyMasternodes->setChecked(false);

    QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect();
    shadowEffect->setColor(QColor(0, 0, 0, 22));
    shadowEffect->setXOffset(0);
    shadowEffect->setYOffset(2);
    shadowEffect->setBlurRadius(6);

    bShowHistory = true;
    if(SpacerNode)
    {
        ui->scrollAreaWidgetContents->layout()->removeItem(SpacerNode);
        delete SpacerNode;
    }
    /*SpacerNode = new QSpacerItem(20,20,QSizePolicy::Minimum,QSizePolicy::Expanding);
    //MNRow * mnrow = new MNRow(ui->scrollArea);
    QSharedPointer<MNRow> mnrow = QSharedPointer<MNRow>(new MNRow(ui->scrollArea));
    mnrow->setGraphicsEffect(shadowEffect);
    connect(mnrow.data(), SIGNAL(onMenuClicked()), this, SLOT(onpbnMenuClicked()));
    ui->scrollAreaWidgetContents->layout()->addWidget(mnrow.data());
    ui->scrollAreaWidgetContents->layout()->addItem(SpacerNode);
    MNRows.push_back(mnrow);*/

   showHistory();
}

double MasterNodesWidget::calculateDailyReward(QString address)
{
    double reward = 0.0;
    for(int txNumber = 0; txNumber < txFilter->rowCount(); ++txNumber)
    {
        QModelIndex rowIndex = txFilter->index(txNumber, TransactionTableModel::Date);
        QDateTime date = rowIndex.data(TransactionTableModel::DateRole).toDateTime();
        if(date.toString("dd.MM.yyyy") == QDateTime::currentDateTime().toString("dd.MM.yyyy"))
            continue;
        else if(date.toString("dd.MM.yyyy") == QDateTime::currentDateTime().addDays(-1).toString("dd.MM.yyyy"))
        {
            QModelIndex rowIndexType = txFilter->index(txNumber, TransactionTableModel::Type);
            int type = rowIndexType.data(TransactionTableModel::TypeRole).toInt();
            if(type == TransactionRecord::LeasingReward)
            {
                QModelIndex rowIndexAddress = txFilter->index(txNumber, TransactionTableModel::ToAddress);
                QString txAddress = rowIndexAddress.data(TransactionTableModel::AddressRole).toString();
                std::cout << txAddress.toStdString() << " " << address.toStdString() << std::endl;
                if(txAddress == address)
                    reward += rowIndex.data(TransactionTableModel::AmountRole).toDouble() / 100000000;
            }
        }
        else
            break;
    }
    return reward;
}

void MasterNodesWidget::onMasternodesUpdate()
{
    if(mnModel->getCount() != MNRows.size())
    {
        clearScrollWidget();

        if (ui->pbnMyMasternodes->isChecked())
            onpbnMyMasternodesClicked();
        else
            onpbnGlobalMasternodesClicked();
        return;
    }

    for(auto rowsPtr : MNRows)
    {
        CKeyID key;
        walletModel->getKeyId(CBTCUAddress(rowsPtr->getAddress().toStdString()), key);
        CPubKey pubKey;
        walletModel->getPubKey(key, pubKey);

        CAmount leasingAmount;
        double roundAmount = 0.0;
#ifdef ENABLE_LEASING_MANAGER
        assert(pwalletMain != NULL);
        LOCK2(cs_main, pwalletMain->cs_wallet);

        if (pwalletMain->pLeasingManager)
            pwalletMain->pLeasingManager->GetAllAmountsLeasedTo(pubKey, leasingAmount);
        roundAmount = round(leasingAmount) / 100000000.0;
#endif

        double reward = 0.0;
        if(day != QDateTime::currentDateTime().toString("dd.MM.yy"))
        {
            reward = calculateDailyReward(rowsPtr->getAddress());
            day = QDateTime::currentDateTime().toString("dd.MM.yy");
        }

        rowsPtr->updateView(roundAmount, reward);
    }
}

void MasterNodesWidget::clearScrollWidget()
{
    if (ui->pbnMyMasternodes->isChecked())
    {
        for (auto i : MNRows) {
            i->hide();
            ui->scrollAreaWidgetContentsMy->layout()->removeWidget(i.data());
        }
        MNRows.clear();
    }
    else
    {
        for (auto i : MNRows) {
            i->hide();
            ui->scrollAreaWidgetContents->layout()->removeWidget(i.data());
        }
        MNRows.clear();
    }
}

void MasterNodesWidget::showEvent(QShowEvent *event)
{
    if(ui->pbnMyMasternodes->isChecked())
        onpbnMyMasternodesClicked();
    else if(ui->pbnGlobalMasternodes->isChecked())
        onpbnGlobalMasternodesClicked();

    connect(timer, SIGNAL(timeout()), this, SLOT(onMasternodesUpdate()));
    timer->start(10000);
}

void MasterNodesWidget::hideEvent(QHideEvent *event)
{
    clearScrollWidget();

    disconnect(timer, SIGNAL(timeout()), this, SLOT(onMasternodesUpdate()));
    timer->stop();
}

void MasterNodesWidget::loadWalletModel(){
    if(walletModel)
    {
        addressTableModel = walletModel->getAddressTableModel();
        filter = new AddressFilterProxyModel(QString(AddressTableModel::Leasing), this);
        filter->setSourceModel(addressTableModel);

        txModel = walletModel->getTransactionTableModel();
        txFilter = new TransactionFilterProxy();
        txFilter->setDynamicSortFilter(true);
        txFilter->setSortCaseSensitivity(Qt::CaseInsensitive);
        txFilter->setFilterCaseSensitivity(Qt::CaseInsensitive);
        txFilter->setSortRole(Qt::EditRole);
        txFilter->setSourceModel(txModel);
        txFilter->sort(TransactionTableModel::Date, Qt::DescendingOrder);
        txFilter->setTypeFilter(TransactionFilterProxy::TYPE(TransactionRecord::LeasingReward));
    }
   /* if(walletModel) {
        ui->listMn->setModel(mnModel);
        ui->listMn->setModelColumn(AddressTableModel::Label);
        updateListState();
    }*/
}

void MasterNodesWidget::updateListState() {
    /*bool show = mnModel->rowCount() > 0;
    ui->listMn->setVisible(show);
    ui->emptyContainer->setVisible(!show);
    //ui->pushButtonStartAll->setVisible(show);*/
}

bool MasterNodesWidget::checkMNsNetwork() {
   bool isTierTwoSync = mnModel->isMNsNetworkSynced();
   if (!isTierTwoSync) inform(tr("Please wait until the node is fully synced"));
   return isTierTwoSync;
}

void MasterNodesWidget::onUpgradeMNClicked(){
    //dont work? not like "mnregvalidator"
    if(walletModel) {
        if (!checkMNsNetwork()) return;
        QString name = index.sibling(index.row(), AddressTableModel::Label).data(Qt::DisplayRole).toString();
        if (mnModel->isCollateralAccepted(name)) {
            QString strAlias = this->index.data(Qt::DisplayRole).toString();//address
            std::cout << strAlias.toStdString() << std::endl;
            if (ask(tr("Start Masternode"), tr("Are you sure you want to start masternode %1?\n").arg(strAlias))) {
                if (!verifyWalletUnlocked()) return;
                startAlias(strAlias);
            }
        }else {
            inform(tr("Cannot start masternode, the collateral transaction has not been accepted by the network.\nPlease wait few more minutes."));
        }
    }
}

void MasterNodesWidget::startAlias(QString strAlias) {
    QString strStatusHtml;
    strStatusHtml += "Alias: " + strAlias + " ";

    for (CMasternodeConfig::CMasternodeEntry mne : masternodeConfig.getEntries()) {
        if (mne.getAlias() == strAlias.toStdString()) {
            std::string strError;
            strStatusHtml += (!startMN(mne, strError)) ? ("failed to start.\nError: " + QString::fromStdString(strError)) : "successfully started.";
            break;
        }
    }
    // update UI and notify
    updateModelAndInform(strStatusHtml);
}

void MasterNodesWidget::updateModelAndInform(QString informText) {
    mnModel->updateMNList();
    inform(informText);
}

bool MasterNodesWidget::startMN(CMasternodeConfig::CMasternodeEntry mne, std::string& strError) {
    CMasternodeBroadcast mnb;
    if (!CMasternodeBroadcast::Create(mne.getIp(), mne.getPrivKey(), mne.getTxHash(), mne.getOutputIndex(), strError, mnb))
        return false;

    mnodeman.UpdateMasternodeList(mnb);
    mnb.Relay();
    return true;
}

void MasterNodesWidget::onStartAllClicked(int type) {
    /*if (!verifyWalletUnlocked()) return;
    if (!checkMNsNetwork()) return;
    if (isLoading) {
        inform(tr("Background task is being executed, please wait"));
    } else {
        isLoading = true;
        if (!execute(type)) {
            isLoading = false;
            inform(tr("Cannot perform Mastenodes start"));
        }
    }*/
}

bool MasterNodesWidget::startAll(QString& failText, bool onlyMissing) {
    /*int amountOfMnFailed = 0;
    int amountOfMnStarted = 0;
    for (CMasternodeConfig::CMasternodeEntry mne : masternodeConfig.getEntries()) {
        // Check for missing only
        QString mnAlias = QString::fromStdString(mne.getAlias());
        if (onlyMissing && !mnModel->isMNInactive(mnAlias)) {
            if (!mnModel->isMNActive(mnAlias))
                amountOfMnFailed++;
            continue;
        }

        std::string strError;
        if (!startMN(mne, strError)) {
            amountOfMnFailed++;
        } else {
            amountOfMnStarted++;
        }
    }
    if (amountOfMnFailed > 0) {
        failText = tr("%1 Masternodes failed to start, %2 started").arg(amountOfMnFailed).arg(amountOfMnStarted);
        return false;
    }*/
    return true;
}

void MasterNodesWidget::run(int type) {
    /*bool isStartMissing = type == REQUEST_START_MISSING;
    if (type == REQUEST_START_ALL || isStartMissing) {
        QString failText;
        QString inform = startAll(failText, isStartMissing) ? tr("All Masternodes started!") : failText;
        QMetaObject::invokeMethod(this, "updateModelAndInform", Qt::QueuedConnection,
                                  Q_ARG(QString, inform));
    }

    isLoading = false;*/
}

void MasterNodesWidget::onError(QString error, int type) {
    /*if (type == REQUEST_START_ALL) {
        QMetaObject::invokeMethod(this, "inform", Qt::QueuedConnection,
                                  Q_ARG(QString, "Error starting all Masternodes"));
    }*/
}

void MasterNodesWidget::onInfoMNClicked() {
    if(!verifyWalletUnlocked()) return;
    showHideOp(true);

    MnInfoDialog* dialog = new MnInfoDialog(window);

    QString name = index.sibling(index.row(), AddressTableModel::Label).data(Qt::DisplayRole).toString();
    QString address = index.sibling(index.row(), AddressTableModel::Address).data(Qt::DisplayRole).toString();
    QString pubKey = mnModel->getPubKey(name);
    QString ip = mnModel->getIP(name);
    QString txId = mnModel->getTxId(name);
    QString outIndex = mnModel->getOutputIndex(name);
    QString status = mnModel->getStatus(name);

    dialog->setData(name, address, pubKey, ip, txId, outIndex, status);
    dialog->adjustSize();
    showDialog(dialog, 3, 17);

    if (dialog->exportMN){
        if (ask(tr("Remote Masternode Data"),
                tr("You are just about to export the required data to run a Masternode\non a remote server to your clipboard.\n\n\n"
                   "You will only have to paste the data in the btcu.conf file\nof your remote server and start it, "
                   "then start the Masternode using\nthis controller wallet (select the Masternode in the list and press \"start\").\n"
                ))) {
            // export data
            QString exportedMN = "masternode=1\n"
                                 "externalip=" + ip.left(ip.lastIndexOf(":")) + "\n" +
                                 "masternodeaddr=" + address + + "\n" +
                                 "masternodeprivkey=" + mnModel->getPrivKey(name) + "\n";
            GUIUtil::setClipboard(exportedMN);
            inform(tr("Masternode exported!, check your clipboard"));
        }
    }

    dialog->deleteLater();
}

void MasterNodesWidget::onDeleteMNClicked(){
    //delete only MN info in masternode.conf, masternode still can be used
    QString name = index.sibling(index.row(), AddressTableModel::Label).data(Qt::DisplayRole).toString();
    QString MN = name + " - " + index.data(Qt::DisplayRole).toString();
    std::string aliasToRemove = name.toStdString();

    if (!ask(tr("Delete Masternode"), tr("You are just about to delete Masternode:\n%1\n\nAre you sure?").arg(MN)))
        return;

    std::string strConfFile = "masternode.conf";
    std::string strDataDir = GetDataDir().string();
    if (strConfFile != boost::filesystem::basename(strConfFile) + boost::filesystem::extension(strConfFile)){
        throw std::runtime_error(strprintf(_("masternode.conf %s resides outside data directory %s"), strConfFile, strDataDir));
    }

    boost::filesystem::path pathBootstrap = GetDataDir() / strConfFile;
    if (boost::filesystem::exists(pathBootstrap)) {
        boost::filesystem::path pathMasternodeConfigFile = GetMasternodeConfigFile();
        boost::filesystem::ifstream streamConfig(pathMasternodeConfigFile);

        if (!streamConfig.good()) {
            inform(tr("Invalid masternode.conf file"));
            return;
        }

        int lineNumToRemove = -1;
        int linenumber = 1;
        std::string lineCopy = "";
        for (std::string line; std::getline(streamConfig, line); linenumber++) {
            if (line.empty()) continue;

            std::istringstream iss(line);
            std::string comment, alias, ip, privKey, txHash, outputIndex;

            if (iss >> comment) {
                if (comment.at(0) == '#') continue;
                iss.str(line);
                iss.clear();
            }

            if (!(iss >> alias >> ip >> privKey >> txHash >> outputIndex)) {
                iss.str(line);
                iss.clear();
                if (!(iss >> alias >> ip >> privKey >> txHash >> outputIndex)) {
                    streamConfig.close();
                    inform(tr("Error parsing masternode.conf file"));
                    return;
                }
            }

            if (aliasToRemove == alias) {
                lineNumToRemove = linenumber;
            } else
                lineCopy += line + "\n";

        }
        streamConfig.close();

        if (lineNumToRemove != -1) {
            boost::filesystem::path pathConfigFile("masternode_temp.conf");
            if (!pathConfigFile.is_complete()) pathConfigFile = GetDataDir() / pathConfigFile;
            FILE* configFile = fopen(pathConfigFile.string().c_str(), "w");
            fwrite(lineCopy.c_str(), std::strlen(lineCopy.c_str()), 1, configFile);
            fclose(configFile);

            boost::filesystem::path pathOldConfFile("old_masternode.conf");
            if (!pathOldConfFile.is_complete()) pathOldConfFile = GetDataDir() / pathOldConfFile;
            if (boost::filesystem::exists(pathOldConfFile)) {
                boost::filesystem::remove(pathOldConfFile);
            }
            rename(pathMasternodeConfigFile, pathOldConfFile);

            boost::filesystem::path pathNewConfFile("masternode.conf");
            if (!pathNewConfFile.is_complete()) pathNewConfFile = GetDataDir() / pathNewConfFile;
            rename(pathConfigFile, pathNewConfFile);

            // Remove alias
            masternodeConfig.remove(aliasToRemove);
            // Update list
            mnModel->removeMn(index);
            updateListState();

            inform("Masternode deleted successfully!");
            removeMNLine();
        }
        else
        {
            inform("There is nothing to delete!");
        }
        //removeMNLine();
    }
    else
        inform(tr("masternode.conf file doesn't exists"));
}

void MasterNodesWidget::removeMNLine() {
    for (int i = 0; i < MNRows.size(); i++)
    {
        if(this->index == MNRows.at(i)->getIndex()) {
            MNRows.remove(i);
            break;
        }
    }
}

void MasterNodesWidget::onCreateMNClicked(){
    /*if(verifyWalletUnlocked()) {
        if(walletModel->getBalance() <= (COIN * 10000)){
            inform(tr("Not enough balance to create a masternode, 10,000 BTCU required."));
            return;
        }
        showHideOp(true);
        MasterNodeWizardDialog *dialog = new MasterNodeWizardDialog(walletModel, window);
        if(openDialogWithOpaqueBackgroundY(dialog, window, 5, 7)) {
            if (dialog->isOk) {
                // Update list
                mnModel->addMn(dialog->mnEntry);
                updateListState();
                // add mn
                inform(dialog->returnStr);
            } else {
                warn(tr("Error creating masternode"), dialog->returnStr);
            }
        }
        dialog->deleteLater();
    }*/
}

void MasterNodesWidget::changeTheme(bool isLightTheme, QString& theme){
    //static_cast<MNHolder*>(this->delegate->getRowFactory())->isLightTheme = isLightTheme;
}

MasterNodesWidget::~MasterNodesWidget()
{
    delete ui;
}
