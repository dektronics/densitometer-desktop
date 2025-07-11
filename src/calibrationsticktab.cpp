#include "calibrationsticktab.h"
#include "ui_calibrationsticktab.h"

#include <QtWidgets/QMessageBox>
#include <QDebug>

#include "stickgaincalibrationdialog.h"
#include "densistick/densisticksettings.h"
#include "util.h"

CalibrationStickTab::CalibrationStickTab(DensiStickRunner *stickRunner, QWidget *parent)
    : CalibrationTab(nullptr, parent)
    , ui(new Ui::CalibrationStickTab), stickRunner_(stickRunner)
{
    ui->setupUi(this);

    connect(stickRunner_, &QObject::destroyed, this, &CalibrationStickTab::onStickInterfaceDestroyed);
    connect(stickRunner_, &DensiStickRunner::targetMeasurement, this, &CalibrationStickTab::onTargetMeasurement);

    // Calibration UI signals
    connect(ui->calGetAllPushButton, &QPushButton::clicked, this, &CalibrationStickTab::onCalGetAllValues);
    connect(ui->gainCalPushButton, &QPushButton::clicked, this, &CalibrationStickTab::onCalGainCalClicked);
    connect(ui->gainSetPushButton, &QPushButton::clicked, this, &CalibrationStickTab::onCalGainSetClicked);
    connect(ui->reflSetPushButton, &QPushButton::clicked, this, &CalibrationStickTab::onCalReflectionSetClicked);

    // Calibration (gain) field validation
    for (int i = 0; i < ui->gainTableWidget->rowCount(); i++) {
        QLineEdit *lineEdit = new QLineEdit();
        lineEdit->setValidator(util::createFloatValidator(0.0, 512.0, 6, this));
        ui->gainTableWidget->setCellWidget(i, 0, lineEdit);
        connect(lineEdit, &QLineEdit::textChanged, this, &CalibrationStickTab::onCalGainTextChanged);
    }

    // Calibration (reflection density) field validation
    ui->reflLoDensityLineEdit->setValidator(util::createFloatValidator(0.0, 2.5, 2, this));
    ui->reflLoReadingLineEdit->setValidator(util::createFloatValidator(0.0, 500.0, 6, this));
    ui->reflHiDensityLineEdit->setValidator(util::createFloatValidator(0.0, 2.5, 2, this));
    ui->reflHiReadingLineEdit->setValidator(util::createFloatValidator(0.0, 500.0, 6, this));
    connect(ui->reflLoDensityLineEdit, &QLineEdit::textChanged, this, &CalibrationStickTab::onCalReflectionTextChanged);
    connect(ui->reflLoReadingLineEdit, &QLineEdit::textChanged, this, &CalibrationStickTab::onCalReflectionTextChanged);
    connect(ui->reflHiDensityLineEdit, &QLineEdit::textChanged, this, &CalibrationStickTab::onCalReflectionTextChanged);
    connect(ui->reflHiReadingLineEdit, &QLineEdit::textChanged, this, &CalibrationStickTab::onCalReflectionTextChanged);

    refreshButtonState();
}

CalibrationStickTab::~CalibrationStickTab()
{
    delete ui;
}

void CalibrationStickTab::setAdvancedCalibrationEditable(bool editable)
{
    editable_ = editable;
    refreshButtonState();
    onCalGainTextChanged();
}

void CalibrationStickTab::setDensityPrecision(int precision)
{
    util::changeLineEditDecimals(ui->reflLoDensityLineEdit, precision);
    util::changeLineEditDecimals(ui->reflHiDensityLineEdit, precision);
    densPrecision_ = precision;
}

void CalibrationStickTab::clear()
{
    for (int i = 0; i < ui->gainTableWidget->rowCount(); i++) {
        QLineEdit *lineEdit = qobject_cast<QLineEdit *>(ui->gainTableWidget->cellWidget(i, 0));
        if (lineEdit) {
            lineEdit->clear();
        }
    }

    ui->reflLoDensityLineEdit->clear();
    ui->reflLoReadingLineEdit->clear();
    ui->reflHiDensityLineEdit->clear();
    ui->reflHiReadingLineEdit->clear();

    refreshButtonState();
}

void CalibrationStickTab::reloadAll()
{
    onCalGetAllValues();
}

void CalibrationStickTab::onStickInterfaceDestroyed(QObject *obj)
{
    Q_UNUSED(obj)
    stickRunner_ = nullptr;
    refreshButtonState();
}

void CalibrationStickTab::onConnectionOpened()
{
    // Clear the calibration page since values could have changed
    clear();
}

void CalibrationStickTab::onConnectionClosed()
{
    refreshButtonState();
}

void CalibrationStickTab::refreshButtonState()
{
    const bool connected = stickRunner_ && stickRunner_->stickInterface()->connected()  && stickRunner_->stickInterface()->hasSettings();
    if (connected) {
        ui->calGetAllPushButton->setEnabled(true);
        ui->gainCalPushButton->setEnabled(editable_);
    } else {
        ui->calGetAllPushButton->setEnabled(false);
        ui->gainCalPushButton->setEnabled(false);
    }

    // Make calibration values editable only if connected
    for (int i = 0; i < ui->gainTableWidget->rowCount(); i++) {
        QLineEdit *lineEdit = qobject_cast<QLineEdit *>(ui->gainTableWidget->cellWidget(i, 0));
        if (lineEdit) {
            lineEdit->setReadOnly(!connected || !editable_);
        }
    }

    ui->reflLoDensityLineEdit->setReadOnly(!connected);
    ui->reflLoReadingLineEdit->setReadOnly(!connected);
    ui->reflHiDensityLineEdit->setReadOnly(!connected);
    ui->reflHiReadingLineEdit->setReadOnly(!connected);
}

void CalibrationStickTab::onTargetMeasurement(float basicReading)
{
    if (ui->reflLoReadingLineEdit->hasFocus()) {
        ui->reflLoReadingLineEdit->setText(QString::number(basicReading, 'f'));
    } else if (ui->reflHiReadingLineEdit->hasFocus()) {
        ui->reflHiReadingLineEdit->setText(QString::number(basicReading, 'f'));
    }
}

void CalibrationStickTab::onCalGetAllValues()
{
    if (!stickRunner_ || !stickRunner_->stickInterface()->connected() || !stickRunner_->stickInterface()->hasSettings()) { return; }

    calData_ = stickRunner_->stickInterface()->settings()->readCalibration();
    updateCalGain();
    updateCalTarget();
}

void CalibrationStickTab::onCalGainCalClicked()
{
    ui->gainCalPushButton->setEnabled(false);

    QMessageBox messageBox;
    messageBox.setWindowTitle(tr("Sensor Gain Calibration"));
    messageBox.setText(tr("Place the device face-down on a reflective white surface "
                          "and do not touch it for the duration of the gain "
                          "calibration cycle."));
    messageBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    messageBox.setDefaultButton(QMessageBox::Ok);

    if (messageBox.exec() == QMessageBox::Ok) {
        stickRunner_->setEnabled(false);
        StickGainCalibrationDialog dialog(stickRunner_->stickInterface(), this);
        dialog.exec();
        if (dialog.success()) {
            const QMap<int, float> gainMeasurements = dialog.gainMeasurements();

            for (int i = 0; i < ui->gainTableWidget->rowCount(); i++) {
                QLineEdit *lineEdit = qobject_cast<QLineEdit *>(ui->gainTableWidget->cellWidget(i, 0));
                const float gainValue = gainMeasurements.value(i);
                if (lineEdit) {
                    if (qIsNaN(gainValue) || !qIsFinite(gainValue) || gainValue <= 0.0F) {
                        lineEdit->setText(QString());
                    } else {
                        lineEdit->setText(QString::number(gainValue, 'f'));
                    }
                }
            }
        }
        stickRunner_->setEnabled(true);
    }

    ui->gainCalPushButton->setEnabled(true);
}

void CalibrationStickTab::onCalGainSetClicked()
{
    bool ok;

    DensiStickCalibration updatedCal = calData_;

    PeripheralCalGain calGain;
    for (int i = 0; i < ui->gainTableWidget->rowCount(); i++) {
        QLineEdit *lineEdit = qobject_cast<QLineEdit *>(ui->gainTableWidget->cellWidget(i, 0));
        if (lineEdit) {
            float gainValue = lineEdit->text().toFloat(&ok);
            if (!ok) { return; }

            calGain.setGainValue(static_cast<PeripheralCalGain::GainLevel>(i), gainValue);
        }
    }
    updatedCal.setGainCalibration(calGain);

    if (stickRunner_->stickInterface()->settings()->writeCalibration(updatedCal)) {
        calData_ = updatedCal;
        stickRunner_->reloadCalibration();
        updateCalGain();
    }
    emit calibrationSaved();
}

void CalibrationStickTab::onCalReflectionSetClicked()
{
    PeripheralCalDensityTarget calTarget;
    bool ok;

    calTarget.setLoDensity(ui->reflLoDensityLineEdit->text().toFloat(&ok));
    if (!ok) { return; }

    calTarget.setLoReading(ui->reflLoReadingLineEdit->text().toFloat(&ok));
    if (!ok) { return; }

    calTarget.setHiDensity(ui->reflHiDensityLineEdit->text().toFloat(&ok));
    if (!ok) { return; }

    calTarget.setHiReading(ui->reflHiReadingLineEdit->text().toFloat(&ok));
    if (!ok) { return; }

    DensiStickCalibration updatedCal = calData_;
    updatedCal.setTargetCalibration(calTarget);

    if (stickRunner_->stickInterface()->settings()->writeCalibration(updatedCal)) {
        calData_ = updatedCal;
        stickRunner_->reloadCalibration();
        updateCalTarget();
    }
    emit calibrationSaved();
}

void CalibrationStickTab::onCalGainTextChanged()
{
    bool enableSet = true;
    if (stickRunner_ && stickRunner_->stickInterface()->connected() && stickRunner_->stickInterface()->hasSettings() && editable_) {
        for (int i = 0; i < ui->gainTableWidget->rowCount(); i++) {
            QLineEdit *lineEdit = qobject_cast<QLineEdit *>(ui->gainTableWidget->cellWidget(i, 0));
            if (lineEdit && (lineEdit->text().isEmpty() || !lineEdit->hasAcceptableInput())) {
                enableSet = false;
                break;
            }
        }
    } else {
        enableSet = false;
    }
    ui->gainSetPushButton->setEnabled(enableSet);

    for (int i = 0; i < ui->gainTableWidget->rowCount(); i++) {
        QLineEdit *lineEdit = qobject_cast<QLineEdit *>(ui->gainTableWidget->cellWidget(i, 0));
        float gainValue = calData_.gainCalibration().gainValue(static_cast<PeripheralCalGain::GainLevel>(i));
        if (lineEdit) {
            updateLineEditDirtyState(lineEdit, gainValue, 6);
        }
    }
}

void CalibrationStickTab::onCalReflectionTextChanged()
{
    if (stickRunner_ && stickRunner_->stickInterface()->connected() && stickRunner_->stickInterface()->hasSettings()
        && ui->reflLoDensityLineEdit->hasAcceptableInput()
        && ui->reflLoReadingLineEdit->hasAcceptableInput()
        && ui->reflHiDensityLineEdit->hasAcceptableInput()
        && ui->reflHiReadingLineEdit->hasAcceptableInput()) {
        ui->reflSetPushButton->setEnabled(true);
    } else {
        ui->reflSetPushButton->setEnabled(false);
    }

    const PeripheralCalDensityTarget calTarget = calData_.targetCalibration();
    updateLineEditDirtyState(ui->reflLoDensityLineEdit, calTarget.loDensity(), densPrecision_);
    updateLineEditDirtyState(ui->reflLoReadingLineEdit, calTarget.loReading(), 6);
    updateLineEditDirtyState(ui->reflHiDensityLineEdit, calTarget.hiDensity(), densPrecision_);
    updateLineEditDirtyState(ui->reflHiReadingLineEdit, calTarget.hiReading(), 6);
}

void CalibrationStickTab::updateCalGain()
{
    for (int i = 0; i < ui->gainTableWidget->rowCount(); i++) {
        QLineEdit *lineEdit = qobject_cast<QLineEdit *>(ui->gainTableWidget->cellWidget(i, 0));
        float gainValue = calData_.gainCalibration().gainValue(static_cast<PeripheralCalGain::GainLevel>(i));
        if (lineEdit) {
            if (qIsNaN(gainValue) || !qIsFinite(gainValue) || gainValue <= 0.0F) {
                lineEdit->setText(QString());
            } else {
                lineEdit->setText(QString::number(gainValue, 'f'));
            }
        }
    }

    onCalGainTextChanged();
}

void CalibrationStickTab::updateCalTarget()
{
    const PeripheralCalDensityTarget calReflection = calData_.targetCalibration();

    ui->reflLoDensityLineEdit->setText(QString::number(calReflection.loDensity(), 'f', densPrecision_));
    ui->reflLoReadingLineEdit->setText(QString::number(calReflection.loReading(), 'f', 6));
    ui->reflHiDensityLineEdit->setText(QString::number(calReflection.hiDensity(), 'f', densPrecision_));
    ui->reflHiReadingLineEdit->setText(QString::number(calReflection.hiReading(), 'f', 6));

    onCalReflectionTextChanged();
}
