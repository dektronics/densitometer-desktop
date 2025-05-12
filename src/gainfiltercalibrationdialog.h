#ifndef GAINFILTERCALIBRATIONDIALOG_H
#define GAINFILTERCALIBRATIONDIALOG_H

#include <QDialog>

#include "densinterface.h"

namespace Ui {
class GainFilterCalibrationDialog;
}

class GainFilterCalibrationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GainFilterCalibrationDialog(DensInterface *densInterface, QWidget *parent = nullptr);
    ~GainFilterCalibrationDialog();
    QList<float> gainValues() const;

protected:
    void showEvent(QShowEvent *event) override;

public slots:
    void done(int r) override;

private slots:
    void onSystemRemoteControl(bool enabled);
    void onDiagLightMaxChanged();
    void onDiagSensorUvInvokeReading(unsigned int reading);

    void onActionCut();
    void onActionCopy();
    void onActionPaste();
    void onActionDelete();

    void onScanPushButtonClicked();
    void onClearMeasTable();
    void onMeasTableWidgetDataChanged();
    void onMeasTableCurrentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);
    void onGainRatioTableWidgetDataChanged();
    void onGainValueTableWidgetDataChanged();
    void onCalcPushButtonClicked();
    void onCalcValuesPushButtonClicked();

private:
    void refreshButtonState();
    void measureNextGain();

    Ui::GainFilterCalibrationDialog *ui;
    DensInterface *densInterface_;
    bool started_ = false;
    bool remoteMode_ = false;
    bool offline_ = false;
    bool running_ = false;
    int selectedMeasRow_ = -1;
    int currentGain_ = 0;
};

#endif // GAINFILTERCALIBRATIONDIALOG_H
