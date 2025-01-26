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

    virtual DensInterface::DeviceType deviceType() const { return DensInterface::DeviceUvVis; }

    virtual void setAdvancedCalibrationEditable(bool editable);

public slots:
    virtual void clear();
    virtual void reloadAll();

private slots:
    void onConnectionOpened();
    void onConnectionClosed();

    void onDensityReading(DensInterface::DensityType type, float dValue, float dZero, float rawValue, float corrValue);

    void onCalGetAllValues();
    void onCalGainCalClicked();
    void onCalGainSetClicked();
    void onCalSlopeSetClicked();
    void onCalTempSetClicked();
    void onCalReflectionSetClicked();
    void onCalTransmissionSetClicked();
    void onCalUvTransmissionSetClicked();

    void onCalGainItemChanged(QTableWidgetItem *item);
    void onCalSlopeTextChanged();
    void onCalTempItemChanged(QTableWidgetItem *item);
    void onCalReflectionTextChanged();
    void onCalTransmissionTextChanged();
    void onCalUvTransmissionTextChanged();

    void onCalGainResponse();
    void onCalSlopeResponse();
    void onCalVisTempResponse();
    void onCalVisTempSetComplete();
    void onCalUvTempResponse();
    void onCalUvTempSetComplete();
    void onCalReflectionResponse();
    void onCalTransmissionResponse();
    void onCalUvTransmissionResponse();

    void onSlopeCalibrationTool();
    void onSlopeCalibrationToolFinished(int result);

    void onTempCalibrationTool();
    void onTempCalibrationToolFinished(int result);

private:
    void refreshButtonState();
    bool tableHasEmptyCells(QTableWidget *table);
    QTableWidgetItem *tableWidgetItem(QTableWidget *table, int row, int column);
    void coefficientSetCheckDirtyRow(QTableWidget *table, int row, const CoefficientSet &sourceValues);
    void coefficientSetAssignRow(QTableWidget *table, int row, const CoefficientSet &sourceValues);
    CoefficientSet coefficientSetCollectRow(QTableWidget *table, int row);

    bool editable_ = false;
    Ui::CalibrationUvVisTab *ui;
};

#endif // CALIBRATIONUVVISTAB_H
