#ifndef CALIBRATIONSTICKTAB_H
#define CALIBRATIONSTICKTAB_H

#include <QWidget>

#include "calibrationtab.h"
#include "densistickrunner.h"
#include "tsl2585calibration.h"

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

    virtual DensInterface::DeviceType deviceType() const { return DensInterface::DeviceUvVis; }

    virtual void setAdvancedCalibrationEditable(bool editable);

public slots:
    virtual void clear();
    virtual void reloadAll();

private slots:
    void onStickInterfaceDestroyed(QObject *obj);

    void onConnectionOpened();
    void onConnectionClosed();

    void onTargetMeasurement(float basicReading);

    void onCalGetAllValues();
    void onCalGainCalClicked();
    void onCalGainSetClicked();
    void onCalSlopeSetClicked();
    void onCalReflectionSetClicked();

    void onCalGainTextChanged();
    void onCalSlopeTextChanged();
    void onCalReflectionTextChanged();

    void onSlopeCalibrationTool();
    void onSlopeCalibrationToolFinished(int result);

private:
    void refreshButtonState();
    void updateCalGain();
    void updateCalSlope();
    void updateCalTarget();

    bool editable_ = false;
    Ui::CalibrationStickTab *ui;
    DensiStickRunner *stickRunner_ = nullptr;
    Tsl2585Calibration calData_;
};

#endif // CALIBRATIONSTICKTAB_H
