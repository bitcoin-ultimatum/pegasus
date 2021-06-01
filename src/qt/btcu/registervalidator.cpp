// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2016-2018 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "qt/btcu/registervalidator.h"
#include "qt/btcu/ui_registervalidator.h"
#include "qt/btcu/qtutils.h"
#include <QRegExpValidator>
#include "../rpc/server.h"

RegisterValidator::RegisterValidator(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RegisterValidator)
{
    window = parent;

    ui->setupUi(this);
    this->setStyleSheet(parent->styleSheet());
    ui->frame->setProperty("cssClass", "container-border");

    //Buttons
    ui->pbnBack->setProperty("cssClass", "btn-leasing-dialog-back");
    ui->pbnRegister->setProperty("cssClass", "btn-leasing-dialog-save");
    connect(ui->pbnBack, SIGNAL(clicked()), this, SLOT(close()));
    connect(ui->pbnRegister, SIGNAL(clicked()), this, SLOT(onRegister()));

    //Label
    setCssTitleScreen(ui->labelValidator);
    setCssSubtitleScreen(ui->labelMasternode);
    ui->labelValidator->setProperty("cssClass", "text-title-screen");
    ui->labelMasternode->setProperty("cssClass", "text-leasing-dialog");

    //Line edit
    ui->lineEditMNName->setPlaceholderText("Masternode name");
    setCssProperty(ui->lineEditMNName, "edit-primary-multi-book");
    ui->lineEditMNName->setAttribute(Qt::WA_MacShowFocusRect, 0);
    ui->lineEditMNName->setReadOnly(true);

    //ComboBox
    btnBox = ui->lineEditMNName->addAction(getIconComboBox(isLightTheme(),false), QLineEdit::TrailingPosition);
    connect(btnBox, &QAction::triggered, [this](){ onBoxClicked(); });
    SortEdit* lineEdit = new SortEdit(ui->comboBox);
    initComboBox(ui->comboBox, lineEdit);
    connect(lineEdit, &SortEdit::Mouse_Pressed, [this](){ui->comboBox->showPopup();});
    ui->comboBox->setVisible(false);

    //Widget
    widgetBox = new QWidget(this);
    QVBoxLayout* LayoutBox = new QVBoxLayout(widgetBox);
    listViewComboBox = new QListView();
    LayoutBox->addWidget(listViewComboBox);
    widgetBox->setGraphicsEffect(0);
    listViewComboBox->setProperty("cssClass", "container-border-light");
    listViewComboBox->setModel(ui->comboBox->model());
    connect(listViewComboBox, SIGNAL(clicked(QModelIndex)), this, SLOT(onComboBox(QModelIndex)));
    widgetBox->hide();
}

RegisterValidator::~RegisterValidator()
{
    delete ui;
}

void RegisterValidator::load(WalletModel* walletModel, PWidget* widget)
{
    addressTableModel = walletModel->getAddressTableModel();
    filter = new AddressFilterProxyModel(QString(AddressTableModel::Leasing), this);
    filter->setSourceModel(addressTableModel);
    contactsSize = walletModel->getAddressTableModel()->sizeLeasing();

    fillComboBox();
}

void RegisterValidator::onBoxClicked() {
    if(widgetBox->isVisible()){
        widgetBox->hide();
        btnBox->setIcon(getIconComboBox(isLightTheme(),false));
        return;
    }
    btnBox->setIcon(getIconComboBox(isLightTheme(),true));

    QPoint pos = ui->lineEditMNName->pos();

    int height = contactsSize == 1 ? 70 : contactsSize == 2 ? 100 : 150;
    widgetBox->setFixedSize(ui->lineEditMNName->width() + 20, height);
    pos.setY(this->height()/2 + ui->lineEditMNName->height() + 1);
    pos.setX(this->width()/2 - ui->lineEditMNName->width()/2 - 13);
    widgetBox->move(pos);
    widgetBox->show();
}

void RegisterValidator::onComboBox(const QModelIndex &index) {
    QString value = index.data(0).toString();
    ui->lineEditMNName->setText(value);
    ui->comboBox->setCurrentIndex(index.row());
    widgetBox->hide();
}
void RegisterValidator::fillComboBox()
{
    int rowCount = filter->rowCount();
    for(int addressNumber = 0; addressNumber < rowCount; addressNumber++)
    {
        QModelIndex rowIndex = filter->index(addressNumber, AddressTableModel::Address);
        QModelIndex sibling = rowIndex.sibling(addressNumber, AddressTableModel::Label);
        QString label = sibling.data(Qt::DisplayRole).toString();
        sibling = rowIndex.sibling(addressNumber, AddressTableModel::Address);
        QString address = sibling.data(Qt::DisplayRole).toString();

        masterNode node;
        node.address = address.toStdString();
        node.name = label.toStdString();
        MNs.push_back(node);

        QString text = label + " : " + address;
        if(text.length() > 40)
            text = text.left(37) + "...";
        ui->comboBox->addItem(text);
    }
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

                auto ptr = std::find_if(MNs.begin(), MNs.end(), [&name](const masterNode& node){return node.name == name;});
                if(ptr != MNs.end())
                    ptr->hash = hash;
            }
        }
    }
}

void RegisterValidator::onRegister()
{
    if(ui->lineEditMNName->text().isEmpty())
    {
        setCssEditLine(ui->lineEditMNName, false, true);
        return;
    }

    this->hide();
    masterNode mn = MNs[ui->comboBox->currentIndex()];
    Q_EMIT registered(mn.name, mn.address, mn.hash);
    this->close();
}

void RegisterValidator::inform(QString text){
    if (!snackBar)
        snackBar = new SnackBar(nullptr, this);
    snackBar->setText(text);
    snackBar->resize(this->width(), snackBar->height());
    openDialog(snackBar, this);
}