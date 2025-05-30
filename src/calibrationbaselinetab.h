#ifndef CALIBRATIONBASELINETAB_H
#define CALIBRATIONBASELINETAB_H

#include <QWidget>

#include "calibrationtab.h"
#include "densinterface.h"

class QLineEdit;

namespace Ui {
class CalibrationBaselineTab;
}

class CalibrationBaselineTab : public CalibrationTab
{
    Q_OBJECT

public:
    explicit CalibrationBaselineTab(DensInterface *densInterface, QWidget *parent = nullptr);
    ~CalibrationBaselineTab();

    virtual DensInterface::DeviceType deviceType() const override { return DensInterface::DeviceBaseline; }

    virtual void setAdvancedCalibrationEditable(bool editable) override;
    virtual void setDensityPrecision(int precision) override;

public slots:
    virtual void clear() override;
    virtual void reloadAll() override;

private slots:
    void onConnectionOpened();
    void onConnectionClosed();

    void onDensityReading(DensInterface::DensityType type, float dValue, float dZero, float rawValue, float corrValue);

    void onCalGetAllValues();
    void onCalLightSetClicked();
    void onCalGainCalClicked();
    void onCalGainSetClicked();
    void onCalSlopeSetClicked();
    void onCalReflectionSetClicked();
    void onCalTransmissionSetClicked();

    void onCalLightTextChanged();
    void onCalGainTextChanged();
    void onCalSlopeTextChanged();
    void onCalReflectionTextChanged();
    void onCalTransmissionTextChanged();

    void onCalLightResponse();
    void onCalGainResponse();
    void onCalSlopeResponse();
    void onCalReflectionResponse();
    void onCalTransmissionResponse();

    void onSlopeCalibrationTool();
    void onSlopeCalibrationToolFinished(int result);

private:
    void refreshButtonState();

    bool editable_ = false;
    int densPrecision_ = 2;
    Ui::CalibrationBaselineTab *ui;
};

#endif // CALIBRATIONBASELINETAB_H
