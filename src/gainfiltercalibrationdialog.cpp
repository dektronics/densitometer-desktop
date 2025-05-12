#include "gainfiltercalibrationdialog.h"
#include "ui_gainfiltercalibrationdialog.h"

GainFilterCalibrationDialog::GainFilterCalibrationDialog(DensInterface *densInterface, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::GainFilterCalibrationDialog)
    , densInterface_(densInterface)
{
    ui->setupUi(this);

    connect(densInterface_, &DensInterface::systemRemoteControl, this, &GainFilterCalibrationDialog::onSystemRemoteControl);
    connect(densInterface_, &DensInterface::diagLightMaxChanged, this, &GainFilterCalibrationDialog::onDiagLightMaxChanged);
    connect(densInterface_, &DensInterface::diagSensorUvInvokeReading, this, &GainFilterCalibrationDialog::onDiagSensorUvInvokeReading);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &GainFilterCalibrationDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &GainFilterCalibrationDialog::reject);
    ui->buttonBox->button(QDialogButtonBox::Close)->setEnabled(false);

    ui->measTableWidget->clearContents();
    ui->gainRatioTableWidget->clearContents();
    ui->gainValueTableWidget->clearContents();

    refreshButtonState();
}

GainFilterCalibrationDialog::~GainFilterCalibrationDialog()
{
    delete ui;
}

bool GainFilterCalibrationDialog::success() const
{
    return success_;
}

void GainFilterCalibrationDialog::accept()
{
    if (running_) { return; }

    if (densInterface_->connected()) {
        densInterface_->sendInvokeSystemRemoteControl(false);
    }
    QDialog::accept();
}

void GainFilterCalibrationDialog::reject()
{
    if (running_) { return; }

    if (densInterface_->connected()) {
        densInterface_->sendInvokeSystemRemoteControl(false);
    }
    QDialog::reject();
}

void GainFilterCalibrationDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    if (densInterface_->connected()) {
        densInterface_->sendInvokeSystemRemoteControl(true);
    } else {
        // Start in offline mode
        started_ = true;
        offline_ = true;
        refreshButtonState();
    }
}

void GainFilterCalibrationDialog::onSystemRemoteControl(bool enabled)
{
    qDebug() << "Remote control:" << enabled;
    if (enabled) {
        densInterface_->sendGetDiagLightMax();
    }
    // if (enabled && !started_) {
    //     started_ = true;
    //     running_ = true;
    //     //XXX densInterface_->sendInvokeCalGain();
    //     //XXX Doing test gain reading sweep
    //     curGain_ = 0;
    //     nextDiagReading();
    // }
}

void GainFilterCalibrationDialog::onDiagLightMaxChanged()
{
    densInterface_->sendSetSystemDisplayText("Gain\nCalibration");
    started_ = true;
    refreshButtonState();
}

void GainFilterCalibrationDialog::refreshButtonState()
{
    ui->scanPushButton->setEnabled(densInterface_->connected() && !offline_ && !running_);
}

void GainFilterCalibrationDialog::onDiagSensorUvInvokeReading(unsigned int reading)
{
    if (reading < std::numeric_limits<unsigned int>::max()) {
        //TODO Has valid reading
    } else {
        //TODO Error or saturation
    }
}
