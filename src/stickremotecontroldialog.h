#ifndef STICKREMOTECONTROLDIALOG_H
#define STICKREMOTECONTROLDIALOG_H

#include <QDialog>
#include "densistickinterface.h"

namespace Ui {
class StickRemoteControlDialog;
}

class StickRemoteControlDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StickRemoteControlDialog(DensiStickInterface *stickInterface, QWidget *parent = nullptr);
    ~StickRemoteControlDialog();

protected:
    void showEvent(QShowEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onStickInterfaceDestroyed(QObject *obj);
    void onStickLightChanged();

    void onReflOffClicked();
    void onReflOnClicked();
    void onReflSetClicked();
    void onReflSpinBoxValueChanged(int value);

    void onSensorStartClicked();
    void onSensorStopClicked();
    void onSensorGainIndexChanged(int index);
    void onSensorIntIndexChanged(int index);
    void onAgcCheckBoxStateChanged(Qt::CheckState state);
    void onReflReadClicked();

    void onSensorReading(const DensiStickReading& reading);

private:
    void sendSetSensorConfig();
    void sendSetSensorAgc();
    //void sendInvokeDiagRead(DensInterface::SensorLight light);
    void ledControlState(bool enabled);
    void sensorControlState(bool enabled);

    Ui::StickRemoteControlDialog *ui;
    DensiStickInterface *stickInterface_;
    bool sensorStarted_;
    bool sensorConfigOnStart_;
};

#endif // STICKREMOTECONTROLDIALOG_H
