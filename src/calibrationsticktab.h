#ifndef CALIBRATIONSTICKTAB_H
#define CALIBRATIONSTICKTAB_H

#include <QWidget>

#include "calibrationtab.h"
#include "densistick/densistickrunner.h"
#include "densistick/peripheralcalvalues.h"

class QLineEdit;

namespace Ui {
class CalibrationStickTab;
}

class CalibrationStickTab : public CalibrationTab
{
    Q_OBJECT

public:
    explicit CalibrationStickTab(DensiStickRunner *stickRunner, QWidget *parent = nullptr);
    ~CalibrationStickTab();

    virtual DensInterface::DeviceType deviceType() const override { return DensInterface::DeviceUvVis; }

    virtual void setAdvancedCalibrationEditable(bool editable) override;
    virtual void setDensityPrecision(int precision) override;

public slots:
    virtual void clear() override;
    virtual void reloadAll() override;

private slots:
    void onStickInterfaceDestroyed(QObject *obj);

    void onConnectionOpened();
    void onConnectionClosed();

    void onTargetMeasurement(float basicReading);

    void onCalGetAllValues();
    void onCalGainCalClicked();
    void onCalGainSetClicked();
    void onCalReflectionSetClicked();

    void onCalGainTextChanged();
    void onCalReflectionTextChanged();

private:
    void refreshButtonState();
    void updateCalGain();
    void updateCalTarget();

    bool editable_ = false;
    int densPrecision_ = 2;
    Ui::CalibrationStickTab *ui;
    DensiStickRunner *stickRunner_ = nullptr;
    DensiStickCalibration calData_;
};

#endif // CALIBRATIONSTICKTAB_H
