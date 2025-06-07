#ifndef CALIBRATIONUVVISTAB_H
#define CALIBRATIONUVVISTAB_H

#include <QWidget>

#include "calibrationtab.h"
#include "densinterface.h"

class QLineEdit;
class QTableWidget;
class QTableWidgetItem;

namespace Ui {
class CalibrationUvVisTab;
}

class CalibrationUvVisTab : public CalibrationTab
{
    Q_OBJECT

public:
    explicit CalibrationUvVisTab(DensInterface *densInterface, QWidget *parent = nullptr);
    ~CalibrationUvVisTab();

    virtual DensInterface::DeviceType deviceType() const override { return DensInterface::DeviceUvVis; }

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
    void onCalGainAutoCalClicked();
    void onGainAutoCalibrationFinished(int result);
    void onCalGainFilterCalClicked();
    void onGainFilterCalibrationFinished(int result);
    void onCalGainSetClicked();
    void onCalTempSetClicked();
    void onCalReflectionSetClicked();
    void onCalTransmissionSetClicked();
    void onCalUvTransmissionSetClicked();

    void onCalGainItemChanged(QTableWidgetItem *item);
    void onCalTempItemChanged(QTableWidgetItem *item);
    void onCalReflectionTextChanged();
    void onCalTransmissionTextChanged();
    void onCalUvTransmissionTextChanged();

    void onCalGainResponse();
    void onCalVisTempResponse();
    void onCalVisTempSetComplete();
    void onCalUvTempResponse();
    void onCalUvTempSetComplete();
    void onCalReflectionResponse();
    void onCalTransmissionResponse();
    void onCalUvTransmissionResponse();

    void onTempCalibrationTool();
    void onTempCalibrationToolFinished(int result);

private:
    void refreshButtonState();
    void calTemperatureCheckDirtyColumn(QTableWidget *table, int col, const DensCalTemperature &sourceValues);
    void calTemperatureAssignColumn(QTableWidget *table, int col, const DensCalTemperature &sourceValues);
    DensCalTemperature calTemperatureCollectColumn(QTableWidget *table, int col);

    bool editable_ = false;
    int densPrecision_ = 2;
    Ui::CalibrationUvVisTab *ui;
};

#endif // CALIBRATIONUVVISTAB_H
