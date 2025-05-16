#include "gaincalibrationdialog.h"
#include "ui_gaincalibrationdialog.h"

#include <QScrollBar>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QDebug>

#include "tsl2585.h"

GainCalibrationDialog::GainCalibrationDialog(DensInterface *densInterface, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GainCalibrationDialog),
    densInterface_(densInterface),
    started_(false),
    running_(false),
    success_(false),
    lastStatus_(-1),
    lastParam_(-1)
{
    ui->setupUi(this);
    ui->plainTextEdit->document()->setMaximumBlockCount(100);
    ui->plainTextEdit->setReadOnly(true);

    connect(densInterface_, &DensInterface::systemRemoteControl, this, &GainCalibrationDialog::onSystemRemoteControl);
    connect(densInterface_, &DensInterface::calGainCalStatus, this, &GainCalibrationDialog::onCalGainCalStatus);
    connect(densInterface_, &DensInterface::calGainCalFinished, this, &GainCalibrationDialog::onCalGainCalFinished);
    connect(densInterface_, &DensInterface::calGainCalError, this, &GainCalibrationDialog::onCalGainCalError);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &GainCalibrationDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &GainCalibrationDialog::reject);
    ui->buttonBox->button(QDialogButtonBox::Close)->setEnabled(false);
}

GainCalibrationDialog::~GainCalibrationDialog()
{
    delete ui;
}

bool GainCalibrationDialog::success() const
{
    return success_;
}

void GainCalibrationDialog::accept()
{
    if (running_) { return; }

    if (densInterface_->connected()) {
        densInterface_->sendInvokeSystemRemoteControl(false);
    }
    QDialog::accept();
}

void GainCalibrationDialog::reject()
{
    if (running_) { return; }

    if (densInterface_->connected()) {
        densInterface_->sendInvokeSystemRemoteControl(false);
    }
    QDialog::reject();
}

void GainCalibrationDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    if (densInterface_->connected()) {
        densInterface_->sendInvokeSystemRemoteControl(true);
    }
}

void GainCalibrationDialog::onSystemRemoteControl(bool enabled)
{
    qDebug() << "Remote control:" << enabled;
    if (enabled && !started_) {
        started_ = true;
        running_ = true;
        densInterface_->sendInvokeCalGain();
    }
}

void GainCalibrationDialog::onCalGainCalStatus(int status, int param)
{
    if (status == lastStatus_ && param == lastParam_) {
        return;
    }

    if (densInterface_->deviceType() == DensInterface::DeviceBaseline) {
        updateBaselineStatus(status, param);
    } else if (densInterface_->deviceType() == DensInterface::DeviceUvVis) {
        updateUvVisCalStatus(status, param);
    }

    lastStatus_ = status;
    lastParam_ = param;
}

void GainCalibrationDialog::updateBaselineStatus(int status, int param)
{
    /*
     * typedef enum {
     *     SENSOR_GAIN_CALIBRATION_STATUS_INIT = 0,
     *     SENSOR_GAIN_CALIBRATION_STATUS_MEDIUM,
     *     SENSOR_GAIN_CALIBRATION_STATUS_HIGH,
     *     SENSOR_GAIN_CALIBRATION_STATUS_MAXIMUM,
     *     SENSOR_GAIN_CALIBRATION_STATUS_FAILED,
     *     SENSOR_GAIN_CALIBRATION_STATUS_LED,
     *     SENSOR_GAIN_CALIBRATION_STATUS_COOLDOWN,
     *     SENSOR_GAIN_CALIBRATION_STATUS_DONE
     * } sensor_gain_calibration_status_t;
     */

    switch (status) {
    case 0:
        if (param == 0) {
            addText(tr("Initializing..."));
        }
        break;
    case 1:
        addText(tr("Measuring medium gain... [%1]").arg(gainParamText(param)));
        break;
    case 2:
        addText(tr("Measuring high gain... [%1]").arg(gainParamText(param)));
        break;
    case 3:
        addText(tr("Measuring maximum gain... [%1]").arg(gainParamText(param)));
        break;
    case 5:
        addText(tr("Finding gain measurement brightness... [%1]").arg(lightParamText(param)));
        break;
    case 6:
        if (param == 0) {
            addText(tr("Waiting between measurements..."));
        }
        break;
    }
}

void GainCalibrationDialog::updateUvVisCalStatus(int status, int param)
{
    /*
     * typedef enum {
     *     SENSOR_GAIN_CALIBRATION_STATUS_INIT = 0,
     *     SENSOR_GAIN_CALIBRATION_STATUS_LED,
     *     SENSOR_GAIN_CALIBRATION_STATUS_WAITING,
     *     SENSOR_GAIN_CALIBRATION_STATUS_GAIN,
     *     SENSOR_GAIN_CALIBRATION_STATUS_FAILED,
     *     SENSOR_GAIN_CALIBRATION_STATUS_DONE
     * } sensor_gain_calibration_status_t;
     */

    switch (status) {
    case 0:
        if (param == 0) {
            addText(tr("Initializing..."));
        }
        break;
    case 1:
        addText(tr("Finding gain measurement brightness... [%1]").arg(TSL2585::gainString(static_cast<tsl2585_gain_t>(param))));
        break;
    case 2:
        if (param == 0) {
            addText(tr("Waiting before measurement..."));
        }
        break;
    case 3:
        addText(tr("Measuring gain level... [%1]").arg(TSL2585::gainString(static_cast<tsl2585_gain_t>(param))));
        break;
    }
}

QString GainCalibrationDialog::gainParamText(int param)
{
    if (param == 0) {
        return tr("lower");
    } else if (param == 1) {
        return tr("higher");
    } else {
        return QString::number(param);
    }
}

QString GainCalibrationDialog::lightParamText(int param)
{
    if (param == 0) {
        return tr("init");
    } else {
        return QString::number(param);
    }
}

void GainCalibrationDialog::onCalGainCalFinished()
{
    addText(tr("Gain calibration complete!"));
    running_ = false;
    success_ = true;
    ui->buttonBox->button(QDialogButtonBox::Close)->setEnabled(true);
}

void GainCalibrationDialog::onCalGainCalError()
{
    addText(tr("Gain calibration failed!"));
    running_ = false;
    success_ = false;
    ui->buttonBox->button(QDialogButtonBox::Close)->setEnabled(true);
}

void GainCalibrationDialog::addText(const QString &text)
{
    ui->plainTextEdit->insertPlainText(text);
    ui->plainTextEdit->insertPlainText("\n");
    QScrollBar *bar = ui->plainTextEdit->verticalScrollBar();
    bar->setValue(bar->maximum());
}
