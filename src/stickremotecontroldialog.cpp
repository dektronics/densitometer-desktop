#include "stickremotecontroldialog.h"
#include "stickinterface.h"
#include "ui_stickremotecontroldialog.h"

#include <QStyleHints>
#include <QDebug>
#include <cmath>

namespace
{
static const uint16_t SAMPLE_TIME = 719;
}

StickRemoteControlDialog::StickRemoteControlDialog(StickInterface *stickInterface, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::StickRemoteControlDialog),
    stickInterface_(stickInterface),
    sensorStarted_(false),
    sensorConfigOnStart_(true)
{
    ui->setupUi(this);

    connect(ui->reflOffPushButton, &QPushButton::clicked, this, &StickRemoteControlDialog::onReflOffClicked);
    connect(ui->reflOnPushButton, &QPushButton::clicked, this, &StickRemoteControlDialog::onReflOnClicked);
    connect(ui->reflSetPushButton, &QPushButton::clicked, this, &StickRemoteControlDialog::onReflSetClicked);
    connect(ui->reflSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &StickRemoteControlDialog::onReflSpinBoxValueChanged);

    connect(ui->sensorStartPushButton, &QPushButton::clicked, this, &StickRemoteControlDialog::onSensorStartClicked);
    connect(ui->sensorStopPushButton, &QPushButton::clicked, this, &StickRemoteControlDialog::onSensorStopClicked);
    connect(ui->gainComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StickRemoteControlDialog::onSensorGainIndexChanged);
    connect(ui->intComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &StickRemoteControlDialog::onSensorIntIndexChanged);
    connect(ui->agcCheckBox, &QCheckBox::stateChanged, this, &StickRemoteControlDialog::onAgcCheckBoxStateChanged);
    connect(ui->reflReadPushButton, &QPushButton::clicked, this, &StickRemoteControlDialog::onReflReadClicked);

    connect(stickInterface_, &StickInterface::sensorReading, this, &StickRemoteControlDialog::onSensorReading);
    connect(stickInterface_, &QObject::destroyed, this, &StickRemoteControlDialog::onStickInterfaceDestroyed);

    ledControlState(true);
    sensorControlState(true);
    onStickLightChanged();
}

StickRemoteControlDialog::~StickRemoteControlDialog()
{
    delete ui;
}

void StickRemoteControlDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
}

void StickRemoteControlDialog::closeEvent(QCloseEvent *event)
{
    if (stickInterface_) {
        stickInterface_->sensorStop();
        stickInterface_->setLightEnable(false);
    }
    QDialog::closeEvent(event);
}

void StickRemoteControlDialog::onStickInterfaceDestroyed(QObject *obj)
{
    stickInterface_ = nullptr;
    close();
}

void StickRemoteControlDialog::onStickLightChanged()
{
    if (!stickInterface_) { return; }
    ui->reflSpinBox->setValue(stickInterface_->lightBrightness());
    ui->reflCurrentLineEdit->setText(tr("%1 mA").arg(std::round(stickInterface_->lightCurrent() * 1000.0F)));

    if (QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark) {
        ui->reflSpinBox->setStyleSheet("QSpinBox { background-color: darkgreen; }");
    } else {
        ui->reflSpinBox->setStyleSheet("QSpinBox { background-color: lightgreen; }");
    }
    ledControlState(true);
}

void StickRemoteControlDialog::onReflOffClicked()
{
    if (!stickInterface_) { return; }
    ledControlState(false);
    stickInterface_->setLightEnable(false);
    onStickLightChanged();
}

void StickRemoteControlDialog::onReflOnClicked()
{
    if (!stickInterface_) { return; }
    ledControlState(false);
    stickInterface_->setLightEnable(true);
    onStickLightChanged();
}

void StickRemoteControlDialog::onReflSetClicked()
{
    if (!stickInterface_) { return; }
    ledControlState(false);
    stickInterface_->setLightBrightness(ui->reflSpinBox->value());
    onStickLightChanged();
}

void StickRemoteControlDialog::onReflSpinBoxValueChanged(int value)
{
    Q_UNUSED(value)
    ui->reflSpinBox->setStyleSheet(styleSheet());
}

void StickRemoteControlDialog::ledControlState(bool enabled)
{
    if (!stickInterface_) { return; }
    ui->reflOffPushButton->setEnabled(enabled && stickInterface_->lightEnabled());
    ui->reflOnPushButton->setEnabled(enabled && !stickInterface_->lightEnabled());
    ui->reflSetPushButton->setEnabled(enabled);
    ui->reflSpinBox->setEnabled(enabled);
}

void StickRemoteControlDialog::onSensorStartClicked()
{
    if (!stickInterface_) { return; }
    sensorControlState(false);
    if (sensorConfigOnStart_) {
        sendSetSensorConfig();
        sendSetSensorAgc();
    }
    if (stickInterface_->sensorStart()) {
        sensorStarted_ = true;
    }
    sensorControlState(true);
}

void StickRemoteControlDialog::sendSetSensorConfig()
{
    if (!stickInterface_) { return; }
    stickInterface_->setSensorConfig(
        ui->gainComboBox->currentIndex(),
        SAMPLE_TIME, ((ui->intComboBox->currentIndex() + 1) * 100) - 1);
}

void StickRemoteControlDialog::sendSetSensorAgc()
{
    if (!stickInterface_) { return; }
    if (ui->agcCheckBox->isChecked()) {
        // Currently using a value that's a quarter of the normal count
        const int count = (ui->intComboBox->currentIndex() + 1) * 100;
        stickInterface_->setSensorAgcEnable((count / 4) - 1);
    } else {
        stickInterface_->setSensorAgcDisable();
    }
}

void StickRemoteControlDialog::onSensorStopClicked()
{
    if (!stickInterface_) { return; }
    stickInterface_->sensorStop();
    sensorStarted_ = false;
    sensorControlState(true);
}

void StickRemoteControlDialog::onSensorGainIndexChanged(int index)
{
    Q_UNUSED(index)
    if (sensorStarted_) {
        sendSetSensorConfig();
    } else {
        sensorConfigOnStart_ = true;
    }
    sensorControlState(true);
}

void StickRemoteControlDialog::onSensorIntIndexChanged(int index)
{
    Q_UNUSED(index)
    if (sensorStarted_) {
        sendSetSensorConfig();
    } else {
        sensorConfigOnStart_ = true;
    }
    sensorControlState(true);
}

void StickRemoteControlDialog::onAgcCheckBoxStateChanged(int state)
{
    Q_UNUSED(state)
    if (sensorStarted_) {
        sendSetSensorAgc();
    } else {
        sensorConfigOnStart_ = true;
    }
    sensorControlState(true);
}

void StickRemoteControlDialog::onReflReadClicked()
{
    if (!stickInterface_) { return; }
    ledControlState(false);
    sensorControlState(false);
    ui->reflSpinBox->setValue(128);
    ui->rawReadingLineEdit->setEnabled(false);
    ui->basicReadingLineEdit->setEnabled(false);
    //TODO sendInvokeDiagRead(DensInterface::SensorLightReflection);
}

// void StickRemoteControlDialog::sendInvokeDiagRead(DensInterface::SensorLight light)
// {
//     if (densInterface_->deviceType() == DensInterface::DeviceUvVis) {
//         densInterface_->sendInvokeUvDiagRead(light,
//                                              ui->modeComboBox->currentIndex(),
//                                              ui->gainComboBox->currentIndex(),
//                                              SAMPLE_TIME, ((ui->intComboBox->currentIndex() + 1) * 100) - 1);
//     } else {
//         densInterface_->sendInvokeBaselineDiagRead(light,
//                                                    ui->gainComboBox->currentIndex(),
//                                                    ui->intComboBox->currentIndex());
//     }
// }

void StickRemoteControlDialog::sensorControlState(bool enabled)
{
    ui->sensorStartPushButton->setEnabled(enabled ? !sensorStarted_ : false);
    ui->sensorStopPushButton->setEnabled(enabled ? sensorStarted_ : false);

    if (ui->agcCheckBox->isChecked()) {
        ui->gainComboBox->setEnabled(!sensorStarted_);
    } else {
        ui->gainComboBox->setEnabled(enabled);
    }

    ui->intComboBox->setEnabled(enabled);
    ui->reflReadPushButton->setEnabled(enabled ? !sensorStarted_ : false);
}

// void StickRemoteControlDialog::onDiagSensorBaselineInvokeReading(int ch0, int ch1)
// {
//     updateSensorReading(ch0, ch1);
//     ui->reflSpinBox->setValue(0);
//     ui->rawReadingLineEdit->setEnabled(true);
//     ui->basicReadingLineEdit->setEnabled(true);
//     sensorControlState(true);
//     ledControlState(true);
// }

void StickRemoteControlDialog::onSensorReading(const StickReading& reading)
{
    if (!stickInterface_->running()) { return; }


    if (reading.status() == StickReading::ResultOverflow) {
        ui->rawReadingLineEdit->setText(tr("Overflow"));
        ui->basicReadingLineEdit->setText(QString());
    } else if (reading.status() == StickReading::ResultSaturated) {
        ui->rawReadingLineEdit->setText(tr("ASAT"));
        ui->basicReadingLineEdit->setText(QString());
    } else if (reading.status() == StickReading::ResultValid) {
        const float timeMs = TSL2585::integrationTimeMs(
            SAMPLE_TIME,
            ((ui->intComboBox->currentIndex() + 1) * 100) - 1);
        const float gainValue = TSL2585::gainValue(reading.gain());
        float alsReading = (float)reading.reading() / 16.0F;
        float basicReading = alsReading / (timeMs * gainValue);

        ui->rawReadingLineEdit->setText(QString::number(reading.reading()));
        ui->basicReadingLineEdit->setText(QString::number(basicReading));

        const int gainIndex = static_cast<int>(reading.gain());
        if (ui->agcCheckBox->isChecked()
            && ui->gainComboBox->currentIndex() != gainIndex
            && gainIndex < ui->gainComboBox->maxCount()) {
            ui->gainComboBox->setCurrentIndex(gainIndex);
        }
    }
}
