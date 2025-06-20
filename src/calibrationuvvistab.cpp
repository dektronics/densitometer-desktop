#include "calibrationuvvistab.h"
#include "ui_calibrationuvvistab.h"

#include <QtWidgets/QMessageBox>
#include <QDebug>

#include "gaincalibrationdialog.h"
#include "gainfiltercalibrationdialog.h"
#include "tempcalibrationdialog.h"
#include "floatitemdelegate.h"
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
    connect(densInterface_, &DensInterface::calVisTemperatureResponse, this, &CalibrationUvVisTab::onCalVisTempResponse);
    connect(densInterface_, &DensInterface::calVisTemperatureSetComplete, this, &CalibrationUvVisTab::onCalVisTempSetComplete);
    connect(densInterface_, &DensInterface::calUvTemperatureResponse, this, &CalibrationUvVisTab::onCalUvTempResponse);
    connect(densInterface_, &DensInterface::calUvTemperatureSetComplete, this, &CalibrationUvVisTab::onCalUvTempSetComplete);
    connect(densInterface_, &DensInterface::calReflectionResponse, this, &CalibrationUvVisTab::onCalReflectionResponse);
    connect(densInterface_, &DensInterface::calTransmissionResponse, this, &CalibrationUvVisTab::onCalTransmissionResponse);
    connect(densInterface_, &DensInterface::calUvTransmissionResponse, this, &CalibrationUvVisTab::onCalUvTransmissionResponse);

    // Calibration UI signals
    connect(ui->calGetAllPushButton, &QPushButton::clicked, this, &CalibrationUvVisTab::onCalGetAllValues);
    connect(ui->gainAutoCalPushButton, &QPushButton::clicked, this, &CalibrationUvVisTab::onCalGainAutoCalClicked);
    connect(ui->gainFilterCalPushButton, &QPushButton::clicked, this, &CalibrationUvVisTab::onCalGainFilterCalClicked);
    connect(ui->gainGetPushButton, &QPushButton::clicked, densInterface_, &DensInterface::sendGetCalGain);
    connect(ui->gainSetPushButton, &QPushButton::clicked, this, &CalibrationUvVisTab::onCalGainSetClicked);
    connect(ui->tempGetPushButton, &QPushButton::clicked, densInterface_, &DensInterface::sendGetCalVisTemperature);
    connect(ui->tempGetPushButton, &QPushButton::clicked, densInterface_, &DensInterface::sendGetCalUvTemperature);
    connect(ui->tempSetPushButton, &QPushButton::clicked, this, &CalibrationUvVisTab::onCalTempSetClicked);
    connect(ui->reflGetPushButton, &QPushButton::clicked, densInterface_, &DensInterface::sendGetCalReflection);
    connect(ui->reflSetPushButton, &QPushButton::clicked, this, &CalibrationUvVisTab::onCalReflectionSetClicked);
    connect(ui->tranGetPushButton, &QPushButton::clicked, densInterface_, &DensInterface::sendGetCalTransmission);
    connect(ui->tranSetPushButton, &QPushButton::clicked, this, &CalibrationUvVisTab::onCalTransmissionSetClicked);
    connect(ui->tranUvGetPushButton, &QPushButton::clicked, densInterface_, &DensInterface::sendGetCalUvTransmission);
    connect(ui->tranUvSetPushButton, &QPushButton::clicked, this, &CalibrationUvVisTab::onCalUvTransmissionSetClicked);
    connect(ui->tempCalPushButton, &QPushButton::clicked, this, &CalibrationUvVisTab::onTempCalibrationTool);

    // Calibration (gain) field
    ui->gainTableWidget->setItemDelegate(new FloatItemDelegate(0.0, 512.0, 6));
    connect(ui->gainTableWidget, &QTableWidget::itemChanged, this, &CalibrationUvVisTab::onCalGainItemChanged);

    // Calibration (temperature) fields
    ui->tempTableWidget->setItemDelegate(new FloatItemDelegate());
    connect(ui->tempTableWidget, &QTableWidget::itemChanged, this, &CalibrationUvVisTab::onCalTempItemChanged);

    // Calibration (VIS reflection density) field validation
    ui->reflLoDensityLineEdit->setValidator(util::createFloatValidator(-0.5, 2.5, 2, this));
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

void CalibrationUvVisTab::setAdvancedCalibrationEditable(bool editable)
{
    editable_ = editable;
    refreshButtonState();
    onCalGainItemChanged(nullptr);
    ui->gainFilterCalPushButton->setEnabled(editable_);
    onCalTempItemChanged(nullptr);
    ui->tempCalPushButton->setEnabled(editable_);
}

void CalibrationUvVisTab::setDensityPrecision(int precision)
{
    util::changeLineEditDecimals(ui->reflLoDensityLineEdit, precision);
    util::changeLineEditDecimals(ui->reflHiDensityLineEdit, precision);
    util::changeLineEditDecimals(ui->tranLoDensityLineEdit, precision);
    util::changeLineEditDecimals(ui->tranHiDensityLineEdit, precision);
    util::changeLineEditDecimals(ui->tranUvLoDensityLineEdit, precision);
    util::changeLineEditDecimals(ui->tranUvHiDensityLineEdit, precision);
    densPrecision_ = precision;
}

void CalibrationUvVisTab::clear()
{
    ui->gainTableWidget->clearContents();
    ui->tempTableWidget->clearContents();

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
        ui->gainAutoCalPushButton->setEnabled(editable_);
        ui->gainGetPushButton->setEnabled(true);
        ui->tempGetPushButton->setEnabled(true);
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
        ui->gainAutoCalPushButton->setEnabled(false);
        ui->gainGetPushButton->setEnabled(false);
        ui->tempGetPushButton->setEnabled(false);
        ui->reflGetPushButton->setEnabled(false);
        ui->tranGetPushButton->setEnabled(false);
    }

    // Make calibration values editable only if connected
    if (connected && editable_) {
        ui->gainTableWidget->setEditTriggers(QAbstractItemView::DoubleClicked|QAbstractItemView::EditKeyPressed|QAbstractItemView::AnyKeyPressed);
        ui->tempTableWidget->setEditTriggers(QAbstractItemView::DoubleClicked|QAbstractItemView::EditKeyPressed|QAbstractItemView::AnyKeyPressed);
    } else {
        ui->gainTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
        ui->tempTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    }

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


void CalibrationUvVisTab::onCalGainAutoCalClicked()
{
    if (densInterface_->remoteControlEnabled()) {
        qWarning() << "Cannot start gain calibration while in remote mode";
        return;
    }
    ui->gainAutoCalPushButton->setEnabled(false);
    ui->gainFilterCalPushButton->setEnabled(false);

    QMessageBox messageBox;
    messageBox.setWindowTitle(tr("Sensor Gain Calibration"));
    messageBox.setText(tr("Hold the device firmly closed with no film in the optical path."));
    messageBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    messageBox.setDefaultButton(QMessageBox::Ok);

    if (messageBox.exec() == QMessageBox::Ok) {
        GainCalibrationDialog *dialog = new GainCalibrationDialog(densInterface_, this);
        connect(dialog, &QDialog::finished, this, &CalibrationUvVisTab::onGainAutoCalibrationFinished);
        dialog->show();
    }
}

void CalibrationUvVisTab::onGainAutoCalibrationFinished(int result)
{
    GainCalibrationDialog *dialog = dynamic_cast<GainCalibrationDialog *>(sender());
    dialog->deleteLater();

    if (dialog->success()) {
        densInterface_->sendGetCalGain();
    }

    ui->gainAutoCalPushButton->setEnabled(true);
    ui->gainFilterCalPushButton->setEnabled(true);
}

void CalibrationUvVisTab::onCalGainFilterCalClicked()
{
    if (densInterface_->remoteControlEnabled()) {
        qWarning() << "Cannot start gain calibration while in remote mode";
        return;
    }
    ui->gainAutoCalPushButton->setEnabled(false);
    ui->gainFilterCalPushButton->setEnabled(false);

    GainFilterCalibrationDialog *dialog = new GainFilterCalibrationDialog(densInterface_, this);
    connect(dialog, &QDialog::finished, this, &CalibrationUvVisTab::onGainFilterCalibrationFinished);
    dialog->show();
}

void CalibrationUvVisTab::onGainFilterCalibrationFinished(int result)
{
    GainFilterCalibrationDialog *dialog = dynamic_cast<GainFilterCalibrationDialog *>(sender());
    dialog->deleteLater();

    if (result == QDialog::Accepted) {
        const QList<float> gainValues = dialog->gainValues();

        disconnect(ui->gainTableWidget, &QTableWidget::itemChanged, this, &CalibrationUvVisTab::onCalGainItemChanged);

        for (int i = 0; i < qMin(ui->gainTableWidget->rowCount(), gainValues.size()); i++) {
            QTableWidgetItem *item = util::tableWidgetItem(ui->gainTableWidget, i, 0);
            item->setText(QString::number(gainValues[i], 'f', 6));
        }

        onCalGainItemChanged(nullptr);
        connect(ui->gainTableWidget, &QTableWidget::itemChanged, this, &CalibrationUvVisTab::onCalGainItemChanged);
    }

    ui->gainAutoCalPushButton->setEnabled(true);
    ui->gainFilterCalPushButton->setEnabled(true);
}

void CalibrationUvVisTab::onCalGainSetClicked()
{
    DensUvVisCalGain calGain;
    bool ok;

    for (int i = 0; i < ui->gainTableWidget->rowCount(); i++) {
        QTableWidgetItem *item = ui->gainTableWidget->item(i, 0);
        if (!item) { return; }

        DensUvVisCalGain::GainLevel gainLevel = static_cast<DensUvVisCalGain::GainLevel>(i);

        float gainValue = item->text().toFloat(&ok);
        if (!ok) { return; }

        calGain.setGainValue(gainLevel, gainValue);
    }

    if (!calGain.isValid()) {
        QMessageBox::warning(this, tr("Invalid Values"), tr("Cannot set invalid gain values!"));
        return;
    }

    densInterface_->sendSetUvVisCalGain(calGain);
}

void CalibrationUvVisTab::onCalTempSetClicked()
{
    //FIXME Make both of these do change detection, so both aren't needlessly updated

    const DensCalTemperature visCalTemperature = calTemperatureCollectColumn(ui->tempTableWidget, 0);
    const DensCalTemperature uvCalTemperature = calTemperatureCollectColumn(ui->tempTableWidget, 1);

    if (!visCalTemperature.isValid() || !uvCalTemperature.isValid()) {
        QMessageBox::warning(this, tr("Invalid Values"), tr("Cannot set invalid temperature correction values!"));
        return;
    }

    densInterface_->sendSetCalVisTemperature(visCalTemperature);
    densInterface_->sendSetCalUvTemperature(uvCalTemperature);
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

void CalibrationUvVisTab::onCalGainItemChanged(QTableWidgetItem *item)
{
    bool enableSet = true;
    if (densInterface_->connected() && editable_) {
        enableSet = !util::tableWidgetHasEmptyCells(ui->gainTableWidget);
    } else {
        enableSet = false;
    }
    ui->gainSetPushButton->setEnabled(enableSet);

    const DensUvVisCalGain calGain = densInterface_->calUvVisGain();

    for (int i = 0; i < ui->gainTableWidget->rowCount(); i++) {
        QTableWidgetItem *item = ui->gainTableWidget->item(i, 0);
        float gainValue = calGain.gainValue(static_cast<DensUvVisCalGain::GainLevel>(i));
        updateItemDirtyState(item, gainValue, 6);
    }
}

void CalibrationUvVisTab::onCalTempItemChanged(QTableWidgetItem *item)
{
    bool enableSet = true;
    if (densInterface_->connected()) {
        enableSet = !util::tableWidgetHasEmptyCells(ui->tempTableWidget);
    } else {
        enableSet = false;
    }

    ui->tempSetPushButton->setEnabled(editable_ && enableSet);

    const DensCalTemperature visCalTemperature = densInterface_->calVisTemperature();
    calTemperatureCheckDirtyColumn(ui->tempTableWidget, 0, visCalTemperature);

    const DensCalTemperature uvCalTemperature = densInterface_->calUvTemperature();
    calTemperatureCheckDirtyColumn(ui->tempTableWidget, 1, uvCalTemperature);
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
    updateLineEditDirtyState(ui->reflLoDensityLineEdit, calTarget.loDensity(), densPrecision_);
    updateLineEditDirtyState(ui->reflLoReadingLineEdit, calTarget.loReading(), 6);
    updateLineEditDirtyState(ui->reflHiDensityLineEdit, calTarget.hiDensity(), densPrecision_);
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
    updateLineEditDirtyState(ui->tranHiDensityLineEdit, calTarget.hiDensity(), densPrecision_);
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
    updateLineEditDirtyState(ui->tranUvHiDensityLineEdit, calTarget.hiDensity(), densPrecision_);
    updateLineEditDirtyState(ui->tranUvHiReadingLineEdit, calTarget.hiReading(), 6);
}

void CalibrationUvVisTab::onCalGainResponse()
{
    const DensUvVisCalGain calGain = densInterface_->calUvVisGain();

    disconnect(ui->gainTableWidget, &QTableWidget::itemChanged, this, &CalibrationUvVisTab::onCalGainItemChanged);

    for (int i = 0; i < ui->gainTableWidget->rowCount(); i++) {
        QTableWidgetItem *item = util::tableWidgetItem(ui->gainTableWidget, i, 0);
        float gainValue = calGain.gainValue(static_cast<DensUvVisCalGain::GainLevel>(i));
        if (qIsNaN(gainValue) || !qIsFinite(gainValue) || gainValue <= 0.0F) {
            item->setText(QString());
        } else {
            item->setText(QString::number(gainValue, 'f'));
        }
    }

    onCalGainItemChanged(nullptr);

    connect(ui->gainTableWidget, &QTableWidget::itemChanged, this, &CalibrationUvVisTab::onCalGainItemChanged);
}

void CalibrationUvVisTab::onCalVisTempResponse()
{
    const DensCalTemperature calTemperature = densInterface_->calVisTemperature();

    disconnect(ui->tempTableWidget, &QTableWidget::itemChanged, this, &CalibrationUvVisTab::onCalTempItemChanged);

    calTemperatureAssignColumn(ui->tempTableWidget, 0, calTemperature);

    onCalTempItemChanged(nullptr);

    connect(ui->tempTableWidget, &QTableWidget::itemChanged, this, &CalibrationUvVisTab::onCalTempItemChanged);
}

void CalibrationUvVisTab::onCalVisTempSetComplete()
{
    densInterface_->sendGetCalVisTemperature();
}

void CalibrationUvVisTab::onCalUvTempResponse()
{
    const DensCalTemperature calTemperature = densInterface_->calUvTemperature();

    disconnect(ui->tempTableWidget, &QTableWidget::itemChanged, this, &CalibrationUvVisTab::onCalTempItemChanged);

    calTemperatureAssignColumn(ui->tempTableWidget, 1, calTemperature);

    onCalTempItemChanged(nullptr);

    connect(ui->tempTableWidget, &QTableWidget::itemChanged, this, &CalibrationUvVisTab::onCalTempItemChanged);
}

void CalibrationUvVisTab::onCalUvTempSetComplete()
{
    densInterface_->sendGetCalUvTemperature();
}

void CalibrationUvVisTab::onCalReflectionResponse()
{
    const DensCalTarget calReflection = densInterface_->calReflection();

    ui->reflLoDensityLineEdit->setText(QString::number(calReflection.loDensity(), 'f', densPrecision_));
    ui->reflLoReadingLineEdit->setText(QString::number(calReflection.loReading(), 'f', 6));
    ui->reflHiDensityLineEdit->setText(QString::number(calReflection.hiDensity(), 'f', densPrecision_));
    ui->reflHiReadingLineEdit->setText(QString::number(calReflection.hiReading(), 'f', 6));

    onCalReflectionTextChanged();
}

void CalibrationUvVisTab::onCalTransmissionResponse()
{
    const DensCalTarget calTransmission = densInterface_->calTransmission();

    ui->tranLoDensityLineEdit->setText(QString::number(calTransmission.loDensity(), 'f', densPrecision_));
    ui->tranLoReadingLineEdit->setText(QString::number(calTransmission.loReading(), 'f', 6));
    ui->tranHiDensityLineEdit->setText(QString::number(calTransmission.hiDensity(), 'f', densPrecision_));
    ui->tranHiReadingLineEdit->setText(QString::number(calTransmission.hiReading(), 'f', 6));

    onCalTransmissionTextChanged();
}

void CalibrationUvVisTab::onCalUvTransmissionResponse()
{
    const DensCalTarget calTransmission = densInterface_->calUvTransmission();

    ui->tranUvLoDensityLineEdit->setText(QString::number(calTransmission.loDensity(), 'f', densPrecision_));
    ui->tranUvLoReadingLineEdit->setText(QString::number(calTransmission.loReading(), 'f', 6));
    ui->tranUvHiDensityLineEdit->setText(QString::number(calTransmission.hiDensity(), 'f', densPrecision_));
    ui->tranUvHiReadingLineEdit->setText(QString::number(calTransmission.hiReading(), 'f', 6));

    onCalUvTransmissionTextChanged();
}

void CalibrationUvVisTab::onTempCalibrationTool()
{
    TempCalibrationDialog *dialog = new TempCalibrationDialog(this);
    dialog->setUniqueId(densInterface_->uniqueId());

    connect(dialog, &QDialog::finished, this, &CalibrationUvVisTab::onTempCalibrationToolFinished);
    dialog->show();
}

void CalibrationUvVisTab::onTempCalibrationToolFinished(int result)
{
    TempCalibrationDialog *dialog = dynamic_cast<TempCalibrationDialog *>(sender());
    dialog->deleteLater();

    if (result == QDialog::Accepted) {
        disconnect(ui->tempTableWidget, &QTableWidget::itemChanged, this, &CalibrationUvVisTab::onCalTempItemChanged);
        if (dialog->hasVisValues()) {
            calTemperatureAssignColumn(ui->tempTableWidget, 0, dialog->visValues());
        }
        if (dialog->hasUvValues()) {
            calTemperatureAssignColumn(ui->tempTableWidget, 1, dialog->uvValues());
        }
        onCalTempItemChanged(nullptr);
        connect(ui->tempTableWidget, &QTableWidget::itemChanged, this, &CalibrationUvVisTab::onCalTempItemChanged);
    }
}

void CalibrationUvVisTab::calTemperatureCheckDirtyColumn(QTableWidget *table, int col, const DensCalTemperature &sourceValues)
{
    if (!table || table->columnCount() <= col || table->rowCount() < 3) { return; }

    QTableWidgetItem *item;

    item = util::tableWidgetItem(table, 0, col);
    updateItemDirtyState(item, sourceValues.b0());

    item = util::tableWidgetItem(table, 1, col);
    updateItemDirtyState(item, sourceValues.b1());

    item = util::tableWidgetItem(table, 2, col);
    updateItemDirtyState(item, sourceValues.b2());
}

void CalibrationUvVisTab::calTemperatureAssignColumn(QTableWidget *table, int col, const DensCalTemperature &sourceValues)
{
    if (!table || table->columnCount() <= col || table->rowCount() < 3) { return; }

    QTableWidgetItem *item;

    item = util::tableWidgetItem(table, 0, col);
    item->setText(QString::number(sourceValues.b0()));

    item = util::tableWidgetItem(table, 1, col);
    item->setText(QString::number(sourceValues.b1()));

    item = util::tableWidgetItem(table, 2, col);
    item->setText(QString::number(sourceValues.b2()));
}

DensCalTemperature CalibrationUvVisTab::calTemperatureCollectColumn(QTableWidget *table, int col)
{
    bool ok;
    float v0 = qSNaN();
    float v1 = qSNaN();
    float v2 = qSNaN();

    if (!table || table->columnCount() <= col || table->rowCount() < 3) { return DensCalTemperature(); }

    QTableWidgetItem *item;

    item = table->item(0, col);
    if (item) {
        v0 = item->text().toFloat(&ok);
        if (!ok) { v0 = qSNaN(); }
    }

    item = table->item(1, col);
    if (item) {
        v1 = item->text().toFloat(&ok);
        if (!ok) { v1 = qSNaN(); }
    }

    item = table->item(2, col);
    if (item) {
        v2 = item->text().toFloat(&ok);
        if (!ok) { v2 = qSNaN(); }
    }


    return DensCalTemperature(v0, v1, v2);
}
