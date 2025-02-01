#ifndef DIAGNOSTICSTAB_H
#define DIAGNOSTICSTAB_H

#include <QWidget>

#include "densinterface.h"

namespace Ui {
class DiagnosticsTab;
}

class DensiStickRunner;

class DiagnosticsTab : public QWidget
{
    Q_OBJECT

public:
    explicit DiagnosticsTab(DensInterface *densInterface, QWidget *parent = nullptr);
    ~DiagnosticsTab();

    void setStickRunner(DensiStickRunner *stickRunner);

    bool isRemoteOpen() const;

private slots:
    void onConnectionOpened();
    void onConnectionClosed();

    void onSystemVersionResponse();
    void onSystemBuildResponse();
    void onSystemDeviceResponse();
    void onSystemUniqueId();
    void onSystemInternalSensors();

    void onDiagDisplayScreenshot(const QByteArray &data);

    void onRemoteControl();
    void onRemoteControlFinished();

private:
    void configureForDeviceType();
    void refreshButtonState();
    QString formatDisplayTemp(const QString &tempStr);

    Ui::DiagnosticsTab *ui;
    DensInterface *densInterface_ = nullptr;
    DensInterface::DeviceType lastDeviceType_ = DensInterface::DeviceBaseline;
    QDialog *remoteDialog_ = nullptr;
    DensiStickRunner *stickRunner_ = nullptr;
};

#endif // DIAGNOSTICSTAB_H
