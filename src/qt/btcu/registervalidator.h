// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2016-2018 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef REGISTERVALIDATOR_H
#define REGISTERVALIDATOR_H

#include <QDialog>
#include <QComboBox>
#include <QListView>
#include "qt/btcu/snackbar.h"
#include "qt/btcu/pwidget.h"
#include "walletmodel.h"
#include "addresstablemodel.h"
#include "qt/btcu/addressfilterproxymodel.h"

struct masterNode
{
    std::string name;
    std::string address;
    std::string hash;
};

namespace Ui {
class RegisterValidator;
}

class RegisterValidator : public QDialog
{
    Q_OBJECT

public:
    explicit RegisterValidator(QWidget *parent = nullptr);
    ~RegisterValidator();

    void load(WalletModel* walletModel, PWidget* widget);

private Q_SLOTS:
    void onComboBox(const QModelIndex &index);
    void onBoxClicked();
    void onRegister();

Q_SIGNALS:
    void registered(std::string MNName, std::string address, std::string hash);

private:
    void fillComboBox();
    void inform(QString text);

private:
    Ui::RegisterValidator *ui;
    QWidget* window;
    QVector<masterNode> MNs;

    SnackBar *snackBar = nullptr;
    QAction *btnBox = nullptr;
    QListView *listViewComboBox = nullptr;
    QWidget * widgetBox = nullptr;
    AddressTableModel* addressTableModel = nullptr;
    AddressFilterProxyModel *filter = nullptr;

    bool isConcatMasternodeSelected;
    int contactsSize;
};

#endif // REGISTERVALIDATOR_H
