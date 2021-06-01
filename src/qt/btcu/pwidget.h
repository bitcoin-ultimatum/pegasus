// Copyright (c) 2019 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PWIDGET_H
#define PWIDGET_H

#include <QObject>
#include <QWidget>
#include <QString>
#include "qt/btcu/prunnable.h"

class BTCUGUI;
class ClientModel;
class WalletModel;
class WorkerTask;

namespace Ui {
class PWidget;
}

class Translator
{
public:
    virtual QString translate(const char *msg) = 0;
};

class PWidget : public QWidget, public Runnable, public Translator
{
    Q_OBJECT
public:
    explicit PWidget(BTCUGUI* _window = nullptr, QWidget *parent = nullptr);
    explicit PWidget(PWidget *parent = nullptr);

    void setClientModel(ClientModel* model);
    void setWalletModel(WalletModel* model);

    BTCUGUI* getWindow() { return this->window; }

    void run(int type) override;
    void onError(QString error, int type) override;

    void inform(const QString& message);
    void informWarning(const QString& message);
    void informError(const QString& message);
    void emitMessage(const QString& title, const QString& message, unsigned int style, bool* ret = nullptr);

    QString translate(const char *msg) override { return tr(msg); }

Q_SIGNALS:
    void message(const QString& title, const QString& body, unsigned int style, bool* ret = nullptr);
    void showHide(bool show);
    bool execDialog(QDialog *dialog, double xDiv = 3, double yDiv = 5);

protected Q_SLOTS:
    virtual void changeTheme(bool isLightTheme, QString &theme);
    void onChangeTheme(bool isLightTheme, QString &theme);

protected:
    BTCUGUI* window = nullptr;
    ClientModel* clientModel = nullptr;
    WalletModel* walletModel = nullptr;

    virtual void loadClientModel();
    virtual void loadWalletModel();

    void showHideOp(bool show);
    bool execute(int type);
    void warn(const QString& title, const QString& message);
    bool ask(const QString& title, const QString& message);
    void showDialog(QDialog *dialog, double xDiv = 3, double yDiv = 5);

    bool verifyWalletUnlocked();

private:
    QSharedPointer<WorkerTask> task;

    void init();
private Q_SLOTS:
    void errorString(QString, int);

};

#endif // PWIDGET_H
