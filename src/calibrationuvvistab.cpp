#include "calibrationuvvistab.h"
#include "ui_calibrationuvvistab.h"

#include <QtWidgets/QMessageBox>
#include <QDebug>

#include "gaincalibrationdialog.h"
#include "slopecalibrationdialog.h"
#include "util.h"

CalibrationUvVisTab::CalibrationUvVisTab(DensInterface *densInterface, QWidget *parent)
    : CalibrationTab(densInterface, parent)
    , ui(new Ui::CalibrationUvVisTab)
{
    ui->setupUi(this);

    // Densitometer interface update signals
    connect(densInterface_, &DensInterface::connectionOpened, this, &CalibrationUvVisTab::onConnectionOpened);
    connect(densInterface_, &DensInterface::connectionClosed, this, &CalibrationUvVisTab::onConnectionClosed);
    connect(densInterface_, &DensInterface::densityReading, this, &CalibrationUvVisTab::onDensityReading);
    connect(densInterface_, &DensInterface::calGainResponse, this, &CalibrationUvVisTab::onCalGainResponse);
    connect(densInterface_, &DensInterface::calSlopeResponse, this, &CalibrationUvVisTab::onCalSlopeResponse);
    connect(densInterface_, &DensInterface::calVisTemperatureResponse, this, &CalibrationUvVisTab::onCalVisTempResponse);
    connect(densInterface_, &DensInterface::calVisTemperatureSetComplete, this, &CalibrationUvVisTab::onCalVisTempSetComplete);
    connect(densInterface_, &DensInterface::calUvTemperatureResponse, this, &CalibrationUvVisTab::onCalUvTempResponse);
    connect(densInterface_, &DensInterface::calUvTemperatureSetComplete, this, &CalibrationUvVisTab::onCalUvTempSetComplete);
    connect(densInterface_, &DensInterface::calReflectionResponse, this, &CalibrationUvVisTab::onCalReflectionResponse);
    connect(densInterface_, &DensInterface::calTransmissionResponse, this, &CalibrationUvVisTab::onCalTransmissionResponse);
    connect(densInterface_, &DensInterface::calUvTransmissionResponse, this, &CalibrationUvVisTab::onCalUvTransmissionResponse);

    // Calibration UI signals
    connect(ui->calGetAllPushButton, &QPushButton::clicked, this, &CalibrationUvVisTab::onCalGetAllValues);
    connect(ui->gainCalPushButton, &QPushButton::clicked, this, &CalibrationUvVisTab::onCalGainCalClicked);
    connect(ui->gainGetPushButton, &QPushButton::clicked, densInterface_, &DensInterface::sendGetCalGain);
    connect(ui->gainSetPushButton, &QPushButton::clicked, this, &CalibrationUvVisTab::onCalGainSetClicked);
    connect(ui->slopeGetPushButton, &QPushButton::clicked, densInterface_, &DensInterface::sendGetCalSlope);
    connect(ui->slopeSetPushButton, &QPushButton::clicked, this, &CalibrationUvVisTab::onCalSlopeSetClicked);
    connect(ui->visTempGetPushButton, &QPushButton::clicked, densInterface_, &DensInterface::sendGetCalVisTemperature);
    connect(ui->visTempSetPushButton, &QPushButton::clicked, this, &CalibrationUvVisTab::onCalVisTempSetClicked);
    connect(ui->uvTempGetPushButton, &QPushButton::clicked, densInterface_, &DensInterface::sendGetCalUvTemperature);
    connect(ui->uvTempSetPushButton, &QPushButton::clicked, this, &CalibrationUvVisTab::onCalUvTempSetClicked);
    connect(ui->reflGetPushButton, &QPushButton::clicked, densInterface_, &DensInterface::sendGetCalReflection);
    connect(ui->reflSetPushButton, &QPushButton::clicked, this, &CalibrationUvVisTab::onCalReflectionSetClicked);
    connect(ui->tranGetPushButton, &QPushButton::clicked, densInterface_, &DensInterface::sendGetCalTransmission);
    connect(ui->tranSetPushButton, &QPushButton::clicked, this, &CalibrationUvVisTab::onCalTransmissionSetClicked);
    connect(ui->tranUvGetPushButton, &QPushButton::clicked, densInterface_, &DensInterface::sendGetCalUvTransmission);
    connect(ui->tranUvSetPushButton, &QPushButton::clicked, this, &CalibrationUvVisTab::onCalUvTransmissionSetClicked);
    connect(ui->slopeCalPushButton, &QPushButton::clicked, this, &CalibrationUvVisTab::onSlopeCalibrationTool);

    // Calibration (gain) field validation
    for (int i = 0; i < ui->gainTableWidget->rowCount(); i++) {
        QLineEdit *lineEdit = new QLineEdit();
        lineEdit->setValidator(util::createFloatValidator(0.0, 512.0, 6, this));
        ui->gainTableWidget->setCellWidget(i, 0, lineEdit);
        connect(lineEdit, &QLineEdit::textChanged, this, &CalibrationUvVisTab::onCalGainTextChanged);
    }

    // Calibration (slope) field validation
    ui->zLineEdit->setValidator(util::createFloatValidator(-100.0, 100.0, 6, this));
    ui->b0LineEdit->setValidator(util::createFloatValidator(-100.0, 100.0, 6, this));
    ui->b1LineEdit->setValidator(util::createFloatValidator(-100.0, 100.0, 6, this));
    ui->b2LineEdit->setValidator(util::createFloatValidator(-100.0, 100.0, 6, this));
    connect(ui->zLineEdit, &QLineEdit::textChanged, this, &CalibrationUvVisTab::onCalSlopeTextChanged);
    connect(ui->b0LineEdit, &QLineEdit::textChanged, this, &CalibrationUvVisTab::onCalSlopeTextChanged);
    connect(ui->b1LineEdit, &QLineEdit::textChanged, this, &CalibrationUvVisTab::onCalSlopeTextChanged);
    connect(ui->b2LineEdit, &QLineEdit::textChanged, this, &CalibrationUvVisTab::onCalSlopeTextChanged);

    // Calibration (VIS temperature) field validation
    for (int i = 0; i < ui->visTempTableWidget->rowCount(); i++) {
        for (int j = 0; j < ui->visTempTableWidget->columnCount(); j++) {
            QLineEdit *lineEdit = new QLineEdit();
            QDoubleValidator *validator = new QDoubleValidator(this);
            validator->setNotation(QDoubleValidator::ScientificNotation);
            lineEdit->setValidator(validator);
            ui->visTempTableWidget->setCellWidget(i, j, lineEdit);
            connect(lineEdit, &QLineEdit::textChanged, this, &CalibrationUvVisTab::onCalVisTempTextChanged);
        }
    }

    // Calibration (UV temperature) field validation
    for (int i = 0; i < ui->uvTempTableWidget->rowCount(); i++) {
        for (int j = 0; j < ui->visTempTableWidget->columnCount(); j++) {
            QLineEdit *lineEdit = new QLineEdit();
            QDoubleValidator *validator = new QDoubleValidator(this);
            validator->setNotation(QDoubleValidator::ScientificNotation);
            lineEdit->setValidator(validator);
            ui->uvTempTableWidget->setCellWidget(i, j, lineEdit);
            connect(lineEdit, &QLineEdit::textChanged, this, &CalibrationUvVisTab::onCalUvTempTextChanged);
        }
    }

    // Calibration (VIS reflection density) field validation
    ui->reflLoDensityLineEdit->setValidator(util::createFloatValidator(0.0, 2.5, 2, this));
    ui->reflLoReadingLineEdit->setValidator(util::createFloatValidator(0.0, 500.0, 6, this));
    ui->reflHiDensityLineEdit->setValidator(util::createFloatValidator(0.0, 2.5, 2, this));
    ui->reflHiReadingLineEdit->setValidator(util::createFloatValidator(0.0, 500.0, 6, this));
    connect(ui->reflLoDensityLineEdit, &QLineEdit::textChanged, this, &CalibrationUvVisTab::onCalReflectionTextChanged);
    connect(ui->reflLoReadingLineEdit, &QLineEdit::textChanged, this, &CalibrationUvVisTab::onCalReflectionTextChanged);
    connect(ui->reflHiDensityLineEdit, &QLineEdit::textChanged, this, &CalibrationUvVisTab::onCalReflectionTextChanged);
    connect(ui->reflHiReadingLineEdit, &QLineEdit::textChanged, this, &CalibrationUvVisTab::onCalReflectionTextChanged);

    // Calibration (VIS transmission density) field validation
    ui->tranLoReadingLineEdit->setValidator(util::createFloatValidator(0.0, 500.0, 6, this));
    ui->tranHiDensityLineEdit->setValidator(util::createFloatValidator(0.0, 5.0, 2, this));
    ui->tranHiReadingLineEdit->setValidator(util::createFloatValidator(0.0, 500.0, 6, this));
    connect(ui->tranLoReadingLineEdit, &QLineEdit::textChanged, this, &CalibrationUvVisTab::onCalTransmissionTextChanged);
    connect(ui->tranHiDensityLineEdit, &QLineEdit::textChanged, this, &CalibrationUvVisTab::onCalTransmissionTextChanged);
    connect(ui->tranHiReadingLineEdit, &QLineEdit::textChanged, this, &CalibrationUvVisTab::onCalTransmissionTextChanged);

    // Calibration (UV transmission density) field validation
    ui->tranUvLoReadingLineEdit->setValidator(util::createFloatValidator(0.0, 500.0, 6, this));
    ui->tranUvHiDensityLineEdit->setValidator(util::createFloatValidator(0.0, 5.0, 2, this));
    ui->tranUvHiReadingLineEdit->setValidator(util::createFloatValidator(0.0, 500.0, 6, this));
    connect(ui->tranUvLoReadingLineEdit, &QLineEdit::textChanged, this, &CalibrationUvVisTab::onCalUvTransmissionTextChanged);
    connect(ui->tranUvHiDensityLineEdit, &QLineEdit::textChanged, this, &CalibrationUvVisTab::onCalUvTransmissionTextChanged);
    connect(ui->tranUvHiReadingLineEdit, &QLineEdit::textChanged, this, &CalibrationUvVisTab::onCalUvTransmissionTextChanged);

    refreshButtonState();
}

CalibrationUvVisTab::~CalibrationUvVisTab()
{
    delete ui;
}

void CalibrationUvVisTab::clear()
{
    for (int i = 0; i < ui->gainTableWidget->rowCount(); i++) {
        QLineEdit *lineEdit = qobject_cast<QLineEdit *>(ui->gainTableWidget->cellWidget(i, 0));
        if (lineEdit) {
            lineEdit->clear();
        }
    }

    ui->zLineEdit->clear();
    ui->b0LineEdit->clear();
    ui->b1LineEdit->clear();
    ui->b2LineEdit->clear();

    ui->reflLoDensityLineEdit->clear();
    ui->reflLoReadingLineEdit->clear();
    ui->reflHiDensityLineEdit->clear();
    ui->reflHiReadingLineEdit->clear();

    ui->tranLoDensityLineEdit->clear();
    ui->tranLoReadingLineEdit->clear();
    ui->tranHiDensityLineEdit->clear();
    ui->tranHiReadingLineEdit->clear();

    ui->tranUvLoDensityLineEdit->clear();
    ui->tranUvLoReadingLineEdit->clear();
    ui->tranUvHiDensityLineEdit->clear();
    ui->tranUvHiReadingLineEdit->clear();

    refreshButtonState();
}

void CalibrationUvVisTab::reloadAll()
{
    onCalGetAllValues();
}

void CalibrationUvVisTab::onConnectionOpened()
{
    // Clear the calibration page since values could have changed
    clear();
}

void CalibrationUvVisTab::onConnectionClosed()
{
    refreshButtonState();
}

void CalibrationUvVisTab::refreshButtonState()
{
    const bool connected = densInterface_->connected();
    if (connected) {
        ui->calGetAllPushButton->setEnabled(true);
        ui->gainCalPushButton->setEnabled(true);
        ui->gainGetPushButton->setEnabled(true);
        ui->slopeGetPushButton->setEnabled(true);
        ui->visTempGetPushButton->setEnabled(true);
        ui->uvTempGetPushButton->setEnabled(true);
        ui->reflGetPushButton->setEnabled(true);
        ui->tranGetPushButton->setEnabled(true);

        // Populate read-only edit fields that are only set
        // via the protocol for consistency of the data formats
        if (ui->tranLoDensityLineEdit->text().isEmpty()) {
            ui->tranLoDensityLineEdit->setText("0.00");
        }
        if (ui->tranUvLoDensityLineEdit->text().isEmpty()) {
            ui->tranUvLoDensityLineEdit->setText("0.00");
        }

    } else {
        ui->calGetAllPushButton->setEnabled(false);
        ui->gainCalPushButton->setEnabled(false);
        ui->gainGetPushButton->setEnabled(false);
        ui->slopeGetPushButton->setEnabled(false);
        ui->visTempGetPushButton->setEnabled(false);
        ui->uvTempGetPushButton->setEnabled(false);
        ui->reflGetPushButton->setEnabled(false);
        ui->tranGetPushButton->setEnabled(false);
    }

    // Make calibration values editable only if connected
    for (int i = 0; i < ui->gainTableWidget->rowCount(); i++) {
        QLineEdit *lineEdit = qobject_cast<QLineEdit *>(ui->gainTableWidget->cellWidget(i, 0));
        if (lineEdit) {
            lineEdit->setReadOnly(!connected);
        }
    }

    for (int i = 0; i < ui->visTempTableWidget->rowCount(); i++) {
        for (int j = 0; j < ui->visTempTableWidget->columnCount(); j++) {
            QLineEdit *lineEdit = qobject_cast<QLineEdit *>(ui->visTempTableWidget->cellWidget(i, j));
            if (lineEdit) {
                lineEdit->setReadOnly(!connected);
            }
        }
    }

    for (int i = 0; i < ui->uvTempTableWidget->rowCount(); i++) {
        for (int j = 0; j < ui->uvTempTableWidget->columnCount(); j++) {
            QLineEdit *lineEdit = qobject_cast<QLineEdit *>(ui->uvTempTableWidget->cellWidget(i, j));
            if (lineEdit) {
                lineEdit->setReadOnly(!connected);
            }
        }
    }

    ui->zLineEdit->setReadOnly(!connected);
    ui->b0LineEdit->setReadOnly(!connected);
    ui->b1LineEdit->setReadOnly(!connected);
    ui->b2LineEdit->setReadOnly(!connected);

    ui->reflLoDensityLineEdit->setReadOnly(!connected);
    ui->reflLoReadingLineEdit->setReadOnly(!connected);
    ui->reflHiDensityLineEdit->setReadOnly(!connected);
    ui->reflHiReadingLineEdit->setReadOnly(!connected);

    ui->tranLoReadingLineEdit->setReadOnly(!connected);
    ui->tranHiDensityLineEdit->setReadOnly(!connected);
    ui->tranHiReadingLineEdit->setReadOnly(!connected);

    ui->tranUvLoReadingLineEdit->setReadOnly(!connected);
    ui->tranUvHiDensityLineEdit->setReadOnly(!connected);
    ui->tranUvHiReadingLineEdit->setReadOnly(!connected);
}

void CalibrationUvVisTab::onDensityReading(DensInterface::DensityType type, float dValue, float dZero, float rawValue, float corrValue)
{
    Q_UNUSED(dValue)
    Q_UNUSED(dZero)
    Q_UNUSED(rawValue)

    // Update calibration tab fields, if focused
    if (type == DensInterface::DensityReflection) {
        if (ui->reflLoReadingLineEdit->hasFocus()) {
            ui->reflLoReadingLineEdit->setText(QString::number(corrValue, 'f'));
        } else if (ui->reflHiReadingLineEdit->hasFocus()) {
            ui->reflHiReadingLineEdit->setText(QString::number(corrValue, 'f'));
        }
    } else if (type == DensInterface::DensityTransmission) {
        if (ui->tranLoReadingLineEdit->hasFocus()) {
            ui->tranLoReadingLineEdit->setText(QString::number(corrValue, 'f'));
        } else if (ui->tranHiReadingLineEdit->hasFocus()) {
            ui->tranHiReadingLineEdit->setText(QString::number(corrValue, 'f'));
        }
    } else if (type == DensInterface::DensityUvTransmission) {
        if (ui->tranUvLoReadingLineEdit->hasFocus()) {
            ui->tranUvLoReadingLineEdit->setText(QString::number(corrValue, 'f'));
        } else if (ui->tranUvHiReadingLineEdit->hasFocus()) {
            ui->tranUvHiReadingLineEdit->setText(QString::number(corrValue, 'f'));
        }
    }
}

void CalibrationUvVisTab::onCalGetAllValues()
{
    densInterface_->sendGetCalGain();
    densInterface_->sendGetCalSlope();
    densInterface_->sendGetCalVisTemperature();
    densInterface_->sendGetCalUvTemperature();
    densInterface_->sendGetCalReflection();
    densInterface_->sendGetCalTransmission();
    densInterface_->sendGetCalUvTransmission();
}

void CalibrationUvVisTab::onCalGainCalClicked()
{
    if (densInterface_->remoteControlEnabled()) {
        qWarning() << "Cannot start gain calibration while in remote mode";
        return;
    }
    ui->gainCalPushButton->setEnabled(false);

    QMessageBox messageBox;
    messageBox.setWindowTitle(tr("Sensor Gain Calibration"));
    messageBox.setText(tr("Hold the device firmly closed with no film in the optical path."));
    messageBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    messageBox.setDefaultButton(QMessageBox::Ok);

    if (messageBox.exec() == QMessageBox::Ok) {
        GainCalibrationDialog dialog(densInterface_, this);
        dialog.exec();
        if (dialog.success()) {
            densInterface_->sendGetCalLight();
            densInterface_->sendGetCalGain();
        }
    }

    ui->gainCalPushButton->setEnabled(true);
}

void CalibrationUvVisTab::onCalGainSetClicked()
{
    DensUvVisCalGain calGain;
    bool ok;

    for (int i = 0; i < ui->gainTableWidget->rowCount(); i++) {
        QLineEdit *lineEdit = qobject_cast<QLineEdit *>(ui->gainTableWidget->cellWidget(i, 0));
        if (lineEdit) {
            DensUvVisCalGain::GainLevel gainLevel = static_cast<DensUvVisCalGain::GainLevel>(i);

            float gainValue = lineEdit->text().toFloat(&ok);
            if (!ok) { return; }

            calGain.setGainValue(gainLevel, gainValue);
        }
    }

    if (!calGain.isValid()) {
        QMessageBox::warning(this, tr("Invalid Values"), tr("Cannot set invalid gain values!"));
        return;
    }

    densInterface_->sendSetUvVisCalGain(calGain);
}

void CalibrationUvVisTab::onCalSlopeSetClicked()
{
    DensCalSlope calSlope;
    bool ok;

    calSlope.setZ(ui->zLineEdit->text().toFloat(&ok));
    if (!ok) { return; }

    calSlope.setB0(ui->b0LineEdit->text().toFloat(&ok));
    if (!ok) { return; }

    calSlope.setB1(ui->b1LineEdit->text().toFloat(&ok));
    if (!ok) { return; }

    calSlope.setB2(ui->b2LineEdit->text().toFloat(&ok));
    if (!ok) { return; }

    if (!calSlope.isValid()) {
        QMessageBox::warning(this, tr("Invalid Values"), tr("Cannot set invalid slope correction values!"));
        return;
    }

    densInterface_->sendSetCalSlope(calSlope);
}

void CalibrationUvVisTab::onCalVisTempSetClicked()
{
    DensCalTemperature calTemperature;

    calTemperature.setB0(coefficientSetCollectRow(ui->visTempTableWidget, 0));
    calTemperature.setB1(coefficientSetCollectRow(ui->visTempTableWidget, 1));
    calTemperature.setB2(coefficientSetCollectRow(ui->visTempTableWidget, 2));

    if (!calTemperature.isValid()) {
        QMessageBox::warning(this, tr("Invalid Values"), tr("Cannot set invalid temperature correction values!"));
        return;
    }

    densInterface_->sendSetCalVisTemperature(calTemperature);
}

void CalibrationUvVisTab::onCalUvTempSetClicked()
{
    DensCalTemperature calTemperature;

    calTemperature.setB0(coefficientSetCollectRow(ui->uvTempTableWidget, 0));
    calTemperature.setB1(coefficientSetCollectRow(ui->uvTempTableWidget, 1));
    calTemperature.setB2(coefficientSetCollectRow(ui->uvTempTableWidget, 2));

    if (!calTemperature.isValid()) {
        QMessageBox::warning(this, tr("Invalid Values"), tr("Cannot set invalid temperature correction values!"));
        return;
    }

    densInterface_->sendSetCalUvTemperature(calTemperature);
}

void CalibrationUvVisTab::onCalReflectionSetClicked()
{
    DensCalTarget calTarget;
    bool ok;

    calTarget.setLoDensity(ui->reflLoDensityLineEdit->text().toFloat(&ok));
    if (!ok) { return; }

    calTarget.setLoReading(ui->reflLoReadingLineEdit->text().toFloat(&ok));
    if (!ok) { return; }

    calTarget.setHiDensity(ui->reflHiDensityLineEdit->text().toFloat(&ok));
    if (!ok) { return; }

    calTarget.setHiReading(ui->reflHiReadingLineEdit->text().toFloat(&ok));
    if (!ok) { return; }

    if (!calTarget.isValid()) {
        QMessageBox::warning(this, tr("Invalid Values"), tr("Cannot set invalid reflection calibration values!"));
        return;
    }

    densInterface_->sendSetCalReflection(calTarget);
}

void CalibrationUvVisTab::onCalTransmissionSetClicked()
{
    DensCalTarget calTarget;
    bool ok;

    calTarget.setLoDensity(0.0F);

    calTarget.setLoReading(ui->tranLoReadingLineEdit->text().toFloat(&ok));
    if (!ok) { return; }

    calTarget.setHiDensity(ui->tranHiDensityLineEdit->text().toFloat(&ok));
    if (!ok) { return; }

    calTarget.setHiReading(ui->tranHiReadingLineEdit->text().toFloat(&ok));
    if (!ok) { return; }

    if (!calTarget.isValid()) {
        QMessageBox::warning(this, tr("Invalid Values"), tr("Cannot set invalid transmission calibration values!"));
        return;
    }

    densInterface_->sendSetCalTransmission(calTarget);
}

void CalibrationUvVisTab::onCalUvTransmissionSetClicked()
{
    DensCalTarget calTarget;
    bool ok;

    calTarget.setLoDensity(0.0F);

    calTarget.setLoReading(ui->tranLoReadingLineEdit->text().toFloat(&ok));
    if (!ok) { return; }

    calTarget.setHiDensity(ui->tranHiDensityLineEdit->text().toFloat(&ok));
    if (!ok) { return; }

    calTarget.setHiReading(ui->tranHiReadingLineEdit->text().toFloat(&ok));
    if (!ok) { return; }

    if (!calTarget.isValid()) {
        QMessageBox::warning(this, tr("Invalid Values"), tr("Cannot set invalid transmission calibration values!"));
        return;
    }

    densInterface_->sendSetCalUvTransmission(calTarget);
}

void CalibrationUvVisTab::onCalGainTextChanged()
{
    bool enableSet = true;
    if (densInterface_->connected()) {
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

    const DensUvVisCalGain calGain = densInterface_->calUvVisGain();

    for (int i = 0; i < ui->gainTableWidget->rowCount(); i++) {
        QLineEdit *lineEdit = qobject_cast<QLineEdit *>(ui->gainTableWidget->cellWidget(i, 0));
        float gainValue = calGain.gainValue(static_cast<DensUvVisCalGain::GainLevel>(i));
        if (lineEdit) {
            updateLineEditDirtyState(lineEdit, gainValue, 6);
        }
    }
}

void CalibrationUvVisTab::onCalSlopeTextChanged()
{
    const bool hasZ = (densInterface_->deviceType() == DensInterface::DeviceUvVis);
    if (densInterface_->connected()
        && (ui->zLineEdit->hasAcceptableInput() || !hasZ)
        && ui->b0LineEdit->hasAcceptableInput()
        && ui->b1LineEdit->hasAcceptableInput()
        && ui->b2LineEdit->hasAcceptableInput()) {
        ui->slopeSetPushButton->setEnabled(true);
    } else {
        ui->slopeSetPushButton->setEnabled(false);
    }

    const DensCalSlope calSlope = densInterface_->calSlope();
    updateLineEditDirtyState(ui->zLineEdit, calSlope.z(), 6);
    updateLineEditDirtyState(ui->b0LineEdit, calSlope.b0(), 6);
    updateLineEditDirtyState(ui->b1LineEdit, calSlope.b1(), 6);
    updateLineEditDirtyState(ui->b2LineEdit, calSlope.b2(), 6);
}

void CalibrationUvVisTab::onCalVisTempTextChanged()
{
    bool enableSet = true;
    if (densInterface_->connected()) {
        for (int i = 0; i < ui->visTempTableWidget->rowCount(); i++) {
            for (int j = 0; j < ui->visTempTableWidget->columnCount(); j++) {
                QLineEdit *lineEdit = qobject_cast<QLineEdit *>(ui->visTempTableWidget->cellWidget(i, j));
                if (lineEdit && (lineEdit->text().isEmpty() || !lineEdit->hasAcceptableInput())) {
                    enableSet = false;
                    break;
                }
            }
        }
    } else {
        enableSet = false;
    }
    ui->visTempSetPushButton->setEnabled(enableSet);

    const DensCalTemperature calTemperature = densInterface_->calVisTemperature();
    coefficientSetCheckDirtyRow(ui->visTempTableWidget, 0, calTemperature.b0());
    coefficientSetCheckDirtyRow(ui->visTempTableWidget, 1, calTemperature.b1());
    coefficientSetCheckDirtyRow(ui->visTempTableWidget, 2, calTemperature.b2());
}

void CalibrationUvVisTab::onCalUvTempTextChanged()
{
    bool enableSet = true;
    if (densInterface_->connected()) {
        for (int i = 0; i < ui->uvTempTableWidget->rowCount(); i++) {
            for (int j = 0; j < ui->uvTempTableWidget->columnCount(); j++) {
                QLineEdit *lineEdit = qobject_cast<QLineEdit *>(ui->uvTempTableWidget->cellWidget(i, j));
                if (lineEdit && (lineEdit->text().isEmpty() || !lineEdit->hasAcceptableInput())) {
                    enableSet = false;
                    break;
                }
            }
        }
    } else {
        enableSet = false;
    }
    ui->uvTempSetPushButton->setEnabled(enableSet);

    const DensCalTemperature calTemperature = densInterface_->calUvTemperature();
    coefficientSetCheckDirtyRow(ui->uvTempTableWidget, 0, calTemperature.b0());
    coefficientSetCheckDirtyRow(ui->uvTempTableWidget, 1, calTemperature.b1());
    coefficientSetCheckDirtyRow(ui->uvTempTableWidget, 2, calTemperature.b2());
}

void CalibrationUvVisTab::onCalReflectionTextChanged()
{
    if (densInterface_->connected()
        && ui->reflLoDensityLineEdit->hasAcceptableInput()
        && ui->reflLoReadingLineEdit->hasAcceptableInput()
        && ui->reflHiDensityLineEdit->hasAcceptableInput()
        && ui->reflHiReadingLineEdit->hasAcceptableInput()) {
        ui->reflSetPushButton->setEnabled(true);
    } else {
        ui->reflSetPushButton->setEnabled(false);
    }

    const DensCalTarget calTarget = densInterface_->calReflection();
    updateLineEditDirtyState(ui->reflLoDensityLineEdit, calTarget.loDensity(), 2);
    updateLineEditDirtyState(ui->reflLoReadingLineEdit, calTarget.loReading(), 6);
    updateLineEditDirtyState(ui->reflHiDensityLineEdit, calTarget.hiDensity(), 2);
    updateLineEditDirtyState(ui->reflHiReadingLineEdit, calTarget.hiReading(), 6);
}

void CalibrationUvVisTab::onCalTransmissionTextChanged()
{
    if (densInterface_->connected()
        && !ui->tranLoDensityLineEdit->text().isEmpty()
        && ui->tranLoReadingLineEdit->hasAcceptableInput()
        && ui->tranHiDensityLineEdit->hasAcceptableInput()
        && ui->tranHiReadingLineEdit->hasAcceptableInput()) {
        ui->tranSetPushButton->setEnabled(true);
    } else {
        ui->tranSetPushButton->setEnabled(false);
    }

    const DensCalTarget calTarget = densInterface_->calTransmission();
    updateLineEditDirtyState(ui->tranLoReadingLineEdit, calTarget.loReading(), 6);
    updateLineEditDirtyState(ui->tranHiDensityLineEdit, calTarget.hiDensity(), 2);
    updateLineEditDirtyState(ui->tranHiReadingLineEdit, calTarget.hiReading(), 6);
}

void CalibrationUvVisTab::onCalUvTransmissionTextChanged()
{
    if (densInterface_->connected()
        && !ui->tranUvLoDensityLineEdit->text().isEmpty()
        && ui->tranUvLoReadingLineEdit->hasAcceptableInput()
        && ui->tranUvHiDensityLineEdit->hasAcceptableInput()
        && ui->tranUvHiReadingLineEdit->hasAcceptableInput()) {
        ui->tranUvSetPushButton->setEnabled(true);
    } else {
        ui->tranUvSetPushButton->setEnabled(false);
    }

    const DensCalTarget calTarget = densInterface_->calUvTransmission();
    updateLineEditDirtyState(ui->tranUvLoReadingLineEdit, calTarget.loReading(), 6);
    updateLineEditDirtyState(ui->tranUvHiDensityLineEdit, calTarget.hiDensity(), 2);
    updateLineEditDirtyState(ui->tranUvHiReadingLineEdit, calTarget.hiReading(), 6);
}

void CalibrationUvVisTab::onCalGainResponse()
{
    const DensUvVisCalGain calGain = densInterface_->calUvVisGain();

    for (int i = 0; i < ui->gainTableWidget->rowCount(); i++) {
        QLineEdit *lineEdit = qobject_cast<QLineEdit *>(ui->gainTableWidget->cellWidget(i, 0));
        float gainValue = calGain.gainValue(static_cast<DensUvVisCalGain::GainLevel>(i));
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

void CalibrationUvVisTab::onCalSlopeResponse()
{
    const DensCalSlope calSlope = densInterface_->calSlope();

    ui->zLineEdit->setText(QString::number(calSlope.z(), 'f'));
    ui->b0LineEdit->setText(QString::number(calSlope.b0(), 'f'));
    ui->b1LineEdit->setText(QString::number(calSlope.b1(), 'f'));
    ui->b2LineEdit->setText(QString::number(calSlope.b2(), 'f'));

    onCalSlopeTextChanged();
}

void CalibrationUvVisTab::onCalVisTempResponse()
{
    const DensCalTemperature calTemperature = densInterface_->calVisTemperature();

    coefficientSetAssignRow(ui->visTempTableWidget, 0, calTemperature.b0());
    coefficientSetAssignRow(ui->visTempTableWidget, 1, calTemperature.b1());
    coefficientSetAssignRow(ui->visTempTableWidget, 2, calTemperature.b2());

    onCalVisTempTextChanged();
}

void CalibrationUvVisTab::onCalVisTempSetComplete()
{
    densInterface_->sendGetCalVisTemperature();
}

void CalibrationUvVisTab::onCalUvTempResponse()
{
    const DensCalTemperature calTemperature = densInterface_->calUvTemperature();

    coefficientSetAssignRow(ui->uvTempTableWidget, 0, calTemperature.b0());
    coefficientSetAssignRow(ui->uvTempTableWidget, 1, calTemperature.b1());
    coefficientSetAssignRow(ui->uvTempTableWidget, 2, calTemperature.b2());

    onCalUvTempTextChanged();
}

void CalibrationUvVisTab::onCalUvTempSetComplete()
{
    densInterface_->sendGetCalUvTemperature();
}

void CalibrationUvVisTab::onCalReflectionResponse()
{
    const DensCalTarget calReflection = densInterface_->calReflection();

    ui->reflLoDensityLineEdit->setText(QString::number(calReflection.loDensity(), 'f', 2));
    ui->reflLoReadingLineEdit->setText(QString::number(calReflection.loReading(), 'f', 6));
    ui->reflHiDensityLineEdit->setText(QString::number(calReflection.hiDensity(), 'f', 2));
    ui->reflHiReadingLineEdit->setText(QString::number(calReflection.hiReading(), 'f', 6));

    onCalReflectionTextChanged();
}

void CalibrationUvVisTab::onCalTransmissionResponse()
{
    const DensCalTarget calTransmission = densInterface_->calTransmission();

    ui->tranLoDensityLineEdit->setText(QString::number(calTransmission.loDensity(), 'f', 2));
    ui->tranLoReadingLineEdit->setText(QString::number(calTransmission.loReading(), 'f', 6));
    ui->tranHiDensityLineEdit->setText(QString::number(calTransmission.hiDensity(), 'f', 2));
    ui->tranHiReadingLineEdit->setText(QString::number(calTransmission.hiReading(), 'f', 6));

    onCalTransmissionTextChanged();
}

void CalibrationUvVisTab::onCalUvTransmissionResponse()
{
    const DensCalTarget calTransmission = densInterface_->calUvTransmission();

    ui->tranUvLoDensityLineEdit->setText(QString::number(calTransmission.loDensity(), 'f', 2));
    ui->tranUvLoReadingLineEdit->setText(QString::number(calTransmission.loReading(), 'f', 6));
    ui->tranUvHiDensityLineEdit->setText(QString::number(calTransmission.hiDensity(), 'f', 2));
    ui->tranUvHiReadingLineEdit->setText(QString::number(calTransmission.hiReading(), 'f', 6));

    onCalUvTransmissionTextChanged();
}

void CalibrationUvVisTab::onSlopeCalibrationTool()
{
    SlopeCalibrationDialog *dialog = new SlopeCalibrationDialog(densInterface_, this);
    connect(dialog, &QDialog::finished, this, &CalibrationUvVisTab::onSlopeCalibrationToolFinished);
    dialog->setCalculateZeroAdjustment(true);
    dialog->show();
}

void CalibrationUvVisTab::onSlopeCalibrationToolFinished(int result)
{
    SlopeCalibrationDialog *dialog = dynamic_cast<SlopeCalibrationDialog *>(sender());
    dialog->deleteLater();

    if (result == QDialog::Accepted) {
        if (densInterface_->deviceType() == DensInterface::DeviceUvVis) {
            ui->zLineEdit->setText(QString::number(dialog->zeroAdjustment(), 'f'));
        } else {
            ui->zLineEdit->setText(QString());
        }

        auto result = dialog->calValues();
        ui->b0LineEdit->setText(QString::number(std::get<0>(result), 'f'));
        ui->b1LineEdit->setText(QString::number(std::get<1>(result), 'f'));
        ui->b2LineEdit->setText(QString::number(std::get<2>(result), 'f'));
    }
}

void CalibrationUvVisTab::coefficientSetCheckDirtyRow(QTableWidget *table, int row, const CoefficientSet &sourceValues)
{
    if (!table || table->rowCount() <= row || table->columnCount() < 3) { return; }

    QLineEdit *lineEdit;

    lineEdit = qobject_cast<QLineEdit *>(table->cellWidget(row, 0));
    updateLineEditDirtyState(lineEdit, std::get<0>(sourceValues));

    lineEdit = qobject_cast<QLineEdit *>(table->cellWidget(row, 1));
    updateLineEditDirtyState(lineEdit, std::get<1>(sourceValues));

    lineEdit = qobject_cast<QLineEdit *>(table->cellWidget(row, 2));
    updateLineEditDirtyState(lineEdit, std::get<2>(sourceValues));
}

void CalibrationUvVisTab::coefficientSetAssignRow(QTableWidget *table, int row, const CoefficientSet &sourceValues)
{
    if (!table || table->rowCount() <= row || table->columnCount() < 3) { return; }

    QLineEdit *lineEdit;

    lineEdit = qobject_cast<QLineEdit *>(table->cellWidget(row, 0));
    if (lineEdit) { lineEdit->setText(QString::number(std::get<0>(sourceValues))); }

    lineEdit = qobject_cast<QLineEdit *>(table->cellWidget(row, 1));
    if (lineEdit) { lineEdit->setText(QString::number(std::get<1>(sourceValues))); }

    lineEdit = qobject_cast<QLineEdit *>(table->cellWidget(row, 2));
    if (lineEdit) { lineEdit->setText(QString::number(std::get<2>(sourceValues))); }
}

CoefficientSet CalibrationUvVisTab::coefficientSetCollectRow(QTableWidget *table, int row)
{
    bool ok;
    float v0 = qSNaN();;
    float v1 = qSNaN();;
    float v2 = qSNaN();;

    if (!table || table->rowCount() <= row || table->columnCount() < 3) { return { v0, v1, v2 }; }

    QLineEdit *lineEdit;

    lineEdit = qobject_cast<QLineEdit *>(table->cellWidget(row, 0));
    if (lineEdit) {
        v0 = lineEdit->text().toFloat(&ok);
        if (!ok) { v0 = qSNaN(); }
    }

    lineEdit = qobject_cast<QLineEdit *>(table->cellWidget(row, 1));
    if (lineEdit) {
        v1 = lineEdit->text().toFloat(&ok);
        if (!ok) { v1 = qSNaN(); }
    }

    lineEdit = qobject_cast<QLineEdit *>(table->cellWidget(row, 2));
    if (lineEdit) {
        v2 = lineEdit->text().toFloat(&ok);
        if (!ok) { v2 = qSNaN(); }
    }


    return { v0, v1, v2 };
}
