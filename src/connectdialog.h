#ifndef CONNECTDIALOG_H
#define CONNECTDIALOG_H

#include <QDialog>
#include <QSerialPort>
#include "ft260deviceinfo.h"

QT_BEGIN_NAMESPACE

namespace Ui {
class ConnectDialog;
}

QT_END_NAMESPACE

class ConnectDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConnectDialog(QWidget *parent = nullptr);
    ~ConnectDialog();

    QVariant portInfo() const;

private slots:
    void showPortInfo(int idx);
    virtual void accept();

private:
    void fillPortsInfo();

    Ui::ConnectDialog *ui;
    QVariant portInfo_;
    QMap<int, QSerialPortInfo> serialInfoList_;
    QMap<int, Ft260DeviceInfo> ft260InfoList_;
};

#endif // CONNECTDIALOG_H
