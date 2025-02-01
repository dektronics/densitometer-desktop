#include "calibrationsticktab.h"
#include "ui_calibrationsticktab.h"

#include <QtWidgets/QMessageBox>
#include <QDebug>

#include "stickgaincalibrationdialog.h"
#include "slopecalibrationdialog.h"
#include "densisticksettings.h"
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
    connect(ui->slopeSetPushButton, &QPushButton::clicked, this, &CalibrationStickTab::onCalSlopeSetClicked);
    connect(ui->reflSetPushButton, &QPushButton::clicked, this, &CalibrationStickTab::onCalReflectionSetClicked);
    connect(ui->slopeCalPushButton, &QPushButton::clicked, this, &CalibrationStickTab::onSlopeCalibrationTool);

    // Calibration (gain) field validation
    for (int i = 0; i < ui->gainTableWidget->rowCount(); i++) {
        QLineEdit *lineEdit = new QLineEdit();
        lineEdit->setValidator(util::createFloatValidator(0.0, 512.0, 6, this));
        ui->gainTableWidget->setCellWidget(i, 0, lineEdit);
        connect(lineEdit, &QLineEdit::textChanged, this, &CalibrationStickTab::onCalGainTextChanged);
    }

    // Calibration (slope) field validation
    ui->b0LineEdit->setValidator(util::createFloatValidator(-100.0, 100.0, 6, this));
    ui->b1LineEdit->setValidator(util::createFloatValidator(-100.0, 100.0, 6, this));
    ui->b2LineEdit->setValidator(util::createFloatValidator(-100.0, 100.0, 6, this));
    connect(ui->b0LineEdit, &QLineEdit::textChanged, this, &CalibrationStickTab::onCalSlopeTextChanged);
    connect(ui->b1LineEdit, &QLineEdit::textChanged, this, &CalibrationStickTab::onCalSlopeTextChanged);
    connect(ui->b2LineEdit, &QLineEdit::textChanged, this, &CalibrationStickTab::onCalSlopeTextChanged);

    // Calibration (reflection density) field validation
    ui->reflLoDensityLineEdit->setValidator(util::createFloatValidator(0.0, 2.5, 2, this));
    ui->reflLoReadingLineEdit->setValidator(util::createFloatValidator(0.0, 500.0, 6, this));
    ui->reflHiDensityLineEdit->setValidator(util::createFloatValidator(0.0, 2.5, 2, this));
    ui->reflHiReadingLineEdit->setValidator(util::createFloatValidator(0.0, 500.0, 6, this));
    connect(ui->reflLoDensityLineEdit, &QLineEdit::textChanged, this, &CalibrationStickTab::onCalReflectionTextChanged);
    connect(ui->reflLoReadingLineEdit, &QLineEdit::textChanged, this, &CalibrationStickTab::onCalReflectionTextChanged);
    connect(ui->reflHiDensityLineEdit, &QLineEdit::textChanged, this, &CalibrationStickTab::onCalReflectionTextChanged);
    connect(ui->reflHiReadingLineEdit, &QLineEdit::textChanged, this, &CalibrationStickTab::onCalReflectionTextChanged);

    // It has been determined that slope calibration is not beneficial
    // for a reflection-only device, and can even make errors worse.
    // This is likely because the range of readings is not as large
    // as with transmission, and thus does not cover the region where
    // these corrections are most helpful.
    // As such, slope calibration features are disabled and may be
    // later removed.
    ui->slopeGroupBox->setEnabled(false);

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
    onCalSlopeTextChanged();
}

void CalibrationStickTab::clear()
{
    for (int i = 0; i < ui->gainTableWidget->rowCount(); i++) {
        QLineEdit *lineEdit = qobject_cast<QLineEdit *>(ui->gainTableWidget->cellWidget(i, 0));
        if (lineEdit) {
            lineEdit->clear();
        }
    }

    ui->b0LineEdit->clear();
    ui->b1LineEdit->clear();
    ui->b2LineEdit->clear();

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

    ui->b0LineEdit->setReadOnly(!connected || !editable_);
    ui->b1LineEdit->setReadOnly(!connected || !editable_);
    ui->b2LineEdit->setReadOnly(!connected || !editable_);

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

    calData_ = stickRunner_->stickInterface()->settings()->readCalTsl2585();
    updateCalGain();
    updateCalSlope();
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

    Tsl2585Calibration updatedCal = calData_;

    for (int i = 0; i < ui->gainTableWidget->rowCount(); i++) {
        QLineEdit *lineEdit = qobject_cast<QLineEdit *>(ui->gainTableWidget->cellWidget(i, 0));
        if (lineEdit) {
            float gainValue = lineEdit->text().toFloat(&ok);
            if (!ok) { return; }

            updatedCal.setGainCalibration(static_cast<tsl2585_gain_t>(i), gainValue);
        }
    }

    if (stickRunner_->stickInterface()->settings()->writeCalTsl2585(updatedCal)) {
        calData_ = updatedCal;
        stickRunner_->reloadCalibration();
        updateCalGain();
    }
    emit calibrationSaved();
}

void CalibrationStickTab::onCalSlopeSetClicked()
{
    Tsl2585CalSlope calSlope;
    bool ok;

    calSlope.setB0(ui->b0LineEdit->text().toFloat(&ok));
    if (!ok) { return; }

    calSlope.setB1(ui->b1LineEdit->text().toFloat(&ok));
    if (!ok) { return; }

    calSlope.setB2(ui->b2LineEdit->text().toFloat(&ok));
    if (!ok) { return; }

    calData_.setSlopeCalibration(calSlope);
    //densInterface_->sendSetCalSlope(calSlope);
    emit calibrationSaved();
}

void CalibrationStickTab::onCalReflectionSetClicked()
{
    Tsl2585CalTarget calTarget;
    bool ok;

    calTarget.setLoDensity(ui->reflLoDensityLineEdit->text().toFloat(&ok));
    if (!ok) { return; }

    calTarget.setLoReading(ui->reflLoReadingLineEdit->text().toFloat(&ok));
    if (!ok) { return; }

    calTarget.setHiDensity(ui->reflHiDensityLineEdit->text().toFloat(&ok));
    if (!ok) { return; }

    calTarget.setHiReading(ui->reflHiReadingLineEdit->text().toFloat(&ok));
    if (!ok) { return; }

    Tsl2585Calibration updatedCal = calData_;
    updatedCal.setTargetCalibration(calTarget);

    if (stickRunner_->stickInterface()->settings()->writeCalTsl2585(updatedCal)) {
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
        float gainValue = calData_.gainCalibration(static_cast<tsl2585_gain_t>(i));
        if (lineEdit) {
            updateLineEditDirtyState(lineEdit, gainValue, 6);
        }
    }
}

void CalibrationStickTab::onCalSlopeTextChanged()
{
    if (stickRunner_ && stickRunner_->stickInterface()->connected() && stickRunner_->stickInterface()->hasSettings()
        && ui->b0LineEdit->hasAcceptableInput()
        && ui->b1LineEdit->hasAcceptableInput()
        && ui->b2LineEdit->hasAcceptableInput()
        && editable_) {
        ui->slopeSetPushButton->setEnabled(true);
    } else {
        ui->slopeSetPushButton->setEnabled(false);
    }

    const Tsl2585CalSlope calSlope = calData_.slopeCalibration();
    updateLineEditDirtyState(ui->b0LineEdit, calSlope.b0(), 6);
    updateLineEditDirtyState(ui->b1LineEdit, calSlope.b1(), 6);
    updateLineEditDirtyState(ui->b2LineEdit, calSlope.b2(), 6);
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

    const Tsl2585CalTarget calTarget = calData_.targetCalibration();
    updateLineEditDirtyState(ui->reflLoDensityLineEdit, calTarget.loDensity(), 2);
    updateLineEditDirtyState(ui->reflLoReadingLineEdit, calTarget.loReading(), 6);
    updateLineEditDirtyState(ui->reflHiDensityLineEdit, calTarget.hiDensity(), 2);
    updateLineEditDirtyState(ui->reflHiReadingLineEdit, calTarget.hiReading(), 6);
}

void CalibrationStickTab::updateCalGain()
{
    for (int i = 0; i < ui->gainTableWidget->rowCount(); i++) {
        QLineEdit *lineEdit = qobject_cast<QLineEdit *>(ui->gainTableWidget->cellWidget(i, 0));
        float gainValue = calData_.gainCalibration(static_cast<tsl2585_gain_t>(i));
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

void CalibrationStickTab::updateCalSlope()
{
    const Tsl2585CalSlope calSlope = calData_.slopeCalibration();

    ui->b0LineEdit->setText(QString::number(calSlope.b0(), 'f'));
    ui->b1LineEdit->setText(QString::number(calSlope.b1(), 'f'));
    ui->b2LineEdit->setText(QString::number(calSlope.b2(), 'f'));

    onCalSlopeTextChanged();
}

void CalibrationStickTab::updateCalTarget()
{
    const Tsl2585CalTarget calReflection = calData_.targetCalibration();

    ui->reflLoDensityLineEdit->setText(QString::number(calReflection.loDensity(), 'f', 2));
    ui->reflLoReadingLineEdit->setText(QString::number(calReflection.loReading(), 'f', 6));
    ui->reflHiDensityLineEdit->setText(QString::number(calReflection.hiDensity(), 'f', 2));
    ui->reflHiReadingLineEdit->setText(QString::number(calReflection.hiReading(), 'f', 6));

    onCalReflectionTextChanged();
}

void CalibrationStickTab::onSlopeCalibrationTool()
{
    SlopeCalibrationDialog *dialog = new SlopeCalibrationDialog(stickRunner_, this);
    connect(dialog, &QDialog::finished, this, &CalibrationStickTab::onSlopeCalibrationToolFinished);
    dialog->setCalculateZeroAdjustment(true);
    dialog->show();
}

void CalibrationStickTab::onSlopeCalibrationToolFinished(int result)
{
    SlopeCalibrationDialog *dialog = dynamic_cast<SlopeCalibrationDialog *>(sender());
    dialog->deleteLater();

    if (result == QDialog::Accepted) {
        auto result = dialog->calValues();
        ui->b0LineEdit->setText(QString::number(std::get<0>(result), 'f'));
        ui->b1LineEdit->setText(QString::number(std::get<1>(result), 'f'));
        ui->b2LineEdit->setText(QString::number(std::get<2>(result), 'f'));
    }
}
