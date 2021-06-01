// Copyright (c) 2019 The PIVX developers
// Copyright (c) 2020 The BTCU developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "qt/btcu/qtutils.h"

#include "qt/btcu/snackbar.h"
#include "qrencode.h"
#include "guiconstants.h"

#include <QFile>
#include <QStyle>
#include <QListView>
#include <QGraphicsDropShadowEffect>

Qt::Modifier SHORT_KEY
#ifdef Q_OS_MAC
        = Qt::CTRL;
#else
        = Qt::ALT;
#endif

// Open dialog at the bottom
bool openDialog(QDialog *widget, QWidget *gui){
    widget->setWindowFlags(Qt::CustomizeWindowHint);
    widget->setAttribute(Qt::WA_TranslucentBackground, true);
    QPropertyAnimation* animation = new QPropertyAnimation(widget, "pos");
    animation->setDuration(300);
    animation->setStartValue(QPoint(0, gui->height()));
    animation->setEndValue(QPoint(0, gui->height() - widget->height()));
    animation->setEasingCurve(QEasingCurve::OutQuad);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
    widget->activateWindow();
    widget->raise();
    return widget->exec();
}
bool openDialogDropRight(QDialog *widget, QWidget *gui)
{
   widget->setWindowFlags(Qt::CustomizeWindowHint);
   widget->setAttribute(Qt::WA_TranslucentBackground, true);
   QPropertyAnimation* animation = new QPropertyAnimation(widget, "pos");
   animation->setDuration(300);
   animation->setStartValue(QPoint(gui->width(), widget->height()+15));
   animation->setEndValue(QPoint(gui->width() - widget->width() - 10, widget->height()+15));
   animation->setEasingCurve(QEasingCurve::OutQuad);
   animation->start(QAbstractAnimation::DeleteWhenStopped);
   widget->activateWindow();
   widget->raise();
   return widget->exec();
}

void closeDialog(QDialog *widget, BTCUGUI *gui){
    widget->setWindowFlags(Qt::CustomizeWindowHint);
    widget->setAttribute(Qt::WA_TranslucentBackground, true);
    QPropertyAnimation* animation = new QPropertyAnimation(widget, "pos");
    animation->setDuration(300);
    animation->setStartValue(widget->pos());
    animation->setEndValue(QPoint(0, gui->height() + 100));
    animation->setEasingCurve(QEasingCurve::OutQuad);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void closeDialogDropRight(QDialog *widget, BTCUGUI *gui)
{
   widget->setWindowFlags(Qt::CustomizeWindowHint);
   widget->setAttribute(Qt::WA_TranslucentBackground, true);
   QPropertyAnimation* animation = new QPropertyAnimation(widget, "pos");
   animation->setDuration(300);
   animation->setStartValue(widget->pos());
   animation->setEndValue(QPoint(gui->width(), widget->pos().y()));
   animation->setEasingCurve(QEasingCurve::OutQuad);
   animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void openDialogFullScreen(QWidget *parent, QWidget * dialog){
    dialog->setWindowFlags(Qt::CustomizeWindowHint);
    dialog->move(0, 0);
    dialog->show();
    dialog->activateWindow();
    dialog->resize(parent->width(),parent->height());
}

bool openDialogWithOpaqueBackgroundY(QDialog *widget, BTCUGUI *gui, double posX, double posY){
    widget->setWindowFlags(Qt::CustomizeWindowHint);
    widget->setAttribute(Qt::WA_TranslucentBackground, true);
    QPropertyAnimation* animation = new QPropertyAnimation(widget, "pos");
    animation->setDuration(300);
    int xPos = gui->width() / posX ;
    animation->setStartValue(QPoint(xPos, gui->height()));
    animation->setEndValue(QPoint(xPos, gui->height() / posY));//- (gui->height()) / posY  ));
    animation->setEasingCurve(QEasingCurve::OutQuad);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
    widget->activateWindow();
    bool res = widget->exec();
    gui->showHide(false);
    return res;
}

bool openDialogWithOpaqueBackground(QDialog *widget, BTCUGUI *gui, double posX){
    return openDialogWithOpaqueBackgroundY(widget, gui, posX, 5);
}

bool openDialogWithOpaqueBackground(QDialog *widget, BTCUGUI *gui)
{
   widget->setWindowFlags(Qt::CustomizeWindowHint);
   widget->setAttribute(Qt::WA_TranslucentBackground, true);
   QPropertyAnimation* animation = new QPropertyAnimation(widget, "pos");
   animation->setDuration(300);
   int xPos = (gui->width()/2) - (widget->width()/2) ;
   animation->setStartValue(QPoint(xPos, gui->height()));
   animation->setEndValue(QPoint(xPos, (gui->height() / 2)- (widget->height()/2)));
   animation->setEasingCurve(QEasingCurve::OutQuad);
   animation->start(QAbstractAnimation::DeleteWhenStopped);
   widget->activateWindow();
   bool res = widget->exec();
   gui->showHide(false);
   return res;
}

bool openDialogWithOpaqueBackgroundFullScreen(QDialog *widget, BTCUGUI *gui){
    widget->setWindowFlags(Qt::CustomizeWindowHint);
    widget->setAttribute(Qt::WA_TranslucentBackground, true);

    widget->activateWindow();
    widget->resize(gui->width(),gui->height());

    QPropertyAnimation* animation = new QPropertyAnimation(widget, "pos");
    animation->setDuration(300);
    int xPos = 0;
    animation->setStartValue(QPoint(xPos, gui->height()));
    animation->setEndValue(QPoint(xPos, 0));
    animation->setEasingCurve(QEasingCurve::OutQuad);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
    widget->activateWindow();
    bool res = widget->exec();
    gui->showHide(false);
    return res;
}

QPixmap encodeToQr(QString str, QString &errorStr, QColor qrColor){
    if (!str.isEmpty()) {
        // limit URI length
        if (str.length() > MAX_URI_LENGTH) {
            errorStr = "Resulting URI too long, try to reduce the text for label / message.";
            return QPixmap();
        } else {
            QRcode* code = QRcode_encodeString(str.toUtf8().constData(), 0, QR_ECLEVEL_L, QR_MODE_8, 1);
            if (!code) {
                errorStr = "Error encoding URI into QR Code.";
                return QPixmap();
            }
            QImage myImage = QImage(code->width + 8, code->width + 8, QImage::Format_RGB32);
            myImage.fill(0xffffff);
            unsigned char* p = code->data;
            for (int y = 0; y < code->width; y++) {
                for (int x = 0; x < code->width; x++) {
                    myImage.setPixel(x + 4, y + 4, ((*p & 1) ? qrColor.rgb() : 0xffffff));
                    p++;
                }
            }
            QRcode_free(code);
            return QPixmap::fromImage(myImage);
        }
    }
    return QPixmap();
}

void setupSettings(QSettings *settings){
    if(!settings->contains("lightTheme")){
        settings->setValue("lightTheme", true);
    }
}

QSettings *settings = nullptr;

QSettings* getSettings(){
    if(!settings){
        settings = new QSettings();
        // Setup initial values if them are not there
        setupSettings(settings);
    }

    return settings;
}

bool isLightTheme(){
    return getSettings()->value("lightTheme", true).toBool();
}

void setTheme(bool isLight){
    QSettings* settings =  getSettings();
    settings->setValue("theme", isLight ? "default" : "default-dark");
    settings->setValue("lightTheme", isLight);
}


// Style

void updateStyle(QWidget* widget){
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    widget->update();
}


QColor getRowColor(bool isLightTheme, bool isHovered, bool isSelected){
    if(isLightTheme){
       return QColor("#ffffff");
        /*if (isSelected) {
            return QColor("#EDF0F8ff");
        }else if(isHovered){
            return QColor("#25bababa");
        } else{
            return QColor("#ffffff");
        }*/
    }else{
       return QColor("#222241");
        /*if (isSelected) {
            return QColor("#25b088ff");
        }else if(isHovered){
            return QColor("#25bababa");
        } else{
            return QColor("#0f0b16");
        }*/
    }
}


QIcon getIconComboBox(bool isLightTheme, bool isUncapped)
{
   if(isLightTheme){
      if(isUncapped)
      {
         return QIcon("://ic-contact-arrow-up");
      }
      else
      {
        return QIcon("://ic-contact-arrow-down");
      }
   }
   else
   {
      if(isUncapped)
      {
         return QIcon("://ic-contact-arrow-up-white");
      }
      else
      {
         return QIcon("://ic-contact-arrow-down-white");
      }
   }

}

void initComboBox(QComboBox* combo, QLineEdit* lineEdit){
    setCssProperty(combo, "btn-combo");
    combo->setEditable(true);
    if (lineEdit) {
        lineEdit->setReadOnly(true);
        lineEdit->setAlignment(Qt::AlignRight);
        combo->setLineEdit(lineEdit);
    }
    combo->setStyleSheet("selection-background-color:transparent; selection-color:transparent;");
    combo->setView(new QListView());
}

void initCssEditLine(QLineEdit *edit, bool isDialog){
    if (isDialog) setCssEditLineDialog(edit, true, false);
    else setCssEditLine(edit, true, false);
    setShadow(edit);
    edit->setAttribute(Qt::WA_MacShowFocusRect, 0);
}

void setCssEditLine(QLineEdit *edit, bool isValid, bool forceUpdate){
       setCssProperty(edit, isValid ? "edit-primary" : "edit-primary-error", forceUpdate);
}

void setCssEditLineDialog(QLineEdit *edit, bool isValid, bool forceUpdate){
    setCssProperty(edit, isValid ? "edit-primary-dialog" : "edit-primary-dialog-error", forceUpdate);
}

void initCssEditLine(QWidget *edit, bool isDialog){
   if (isDialog) setCssEditLineDialog(edit, true, false);
   else setCssEditLine(edit, true, false);
   setShadow(edit);
   edit->setAttribute(Qt::WA_MacShowFocusRect, 0);
}

void setCssEditLine(QWidget *edit, bool isValid, bool forceUpdate){
   setCssProperty(edit, isValid ? "edit-primary" : "edit-primary-error", false);
}

void setCssEditLineDialog(QWidget *edit, bool isValid, bool forceUpdate){
   setCssProperty(edit, isValid ? "edit-primary-dialog" : "edit-primary-dialog-error", false);
}

void setShadow(QWidget *edit){
    QGraphicsDropShadowEffect* shadowEffect = new QGraphicsDropShadowEffect();
    shadowEffect->setColor(QColor(0, 0, 0, 22));
    shadowEffect->setXOffset(0);
    shadowEffect->setYOffset(2);
    shadowEffect->setBlurRadius(6);
    //edit->setGraphicsEffect(shadowEffect);
}

void setCssBtnPrimary(QPushButton *btn, bool forceUpdate){
    setCssProperty(btn, "btn-primary", forceUpdate);
}

void setCssBtnSecondary(QPushButton *btn, bool forceUpdate){
    setCssProperty(btn, "btn-secundary", forceUpdate);
}

void setCssTextBodyDialog(std::initializer_list<QWidget*> args){
    Q_FOREACH (QWidget* w, args) { setCssTextBodyDialog(w); }
}

void setCssTextBodyDialog(QWidget* widget) {
    setCssProperty(widget, "text-body1-dialog", false);
}

void setCssTitleScreen(QLabel* label) {
    setCssProperty(label, "text-title-screen", false);
}

void setCssSubtitleScreen(QWidget* wid) {
    setCssProperty(wid, "text-subtitle", false);
}

void setCssProperty(std::initializer_list<QWidget*> args, QString value){
    Q_FOREACH (QWidget* w, args) { setCssProperty(w, value); }
}

void setCssProperty(QWidget *wid, QString value, bool forceUpdate){
    wid->setProperty("cssClass", value);
    forceUpdateStyle(wid, forceUpdate);
}

void forceUpdateStyle(QWidget *widget, bool forceUpdate){
    if(forceUpdate)
        updateStyle(widget);
}

void forceUpdateStyle(std::initializer_list<QWidget*> args){
    Q_FOREACH (QWidget* w, args) { forceUpdateStyle(w, true); }
}
