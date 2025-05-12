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
    bool success() const;

public slots:
    virtual void accept() override;
    virtual void reject() override;

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void onSystemRemoteControl(bool enabled);
    void onDiagLightMaxChanged();
    void onDiagSensorUvInvokeReading(unsigned int reading);

private:
    void refreshButtonState();

    Ui::GainFilterCalibrationDialog *ui;
    DensInterface *densInterface_;
    bool started_ = false;
    bool offline_ = false;
    bool running_ = false;
    bool success_ = false;
};

#endif // GAINFILTERCALIBRATIONDIALOG_H
