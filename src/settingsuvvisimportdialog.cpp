#include "settingsuvvisimportdialog.h"
#include "ui_settingsuvvisimportdialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

#include "densinterface.h"
#include "util.h"

SettingsUvVisImportDialog::SettingsUvVisImportDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsUvVisImportDialog)
{
    ui->setupUi(this);

    // Clear out any designer default text from the UI fields
    ui->deviceNameLabel->setText(QString());
    ui->deviceVersionLabel->setText(tr("Version: %1").arg(QString()));
    ui->deviceUidLabel->setText(tr("Device UID: %1").arg(QString()));
    ui->exportDateLabel->setText(tr("Export Date: %1").arg(QString()));

    ui->gainTableWidget->clearContents();
    ui->tempTableWidget->clearContents();

    ui->reflCalLoDensityLabel->setText(QString());
    ui->reflCalLoReadingLabel->setText(QString());
    ui->reflCalHiDensityLabel->setText(QString());
    ui->reflCalHiReadingLabel->setText(QString());
    ui->tranCalLoDensityLabel->setText(QString());
    ui->tranCalLoReadingLabel->setText(QString());
    ui->tranCalHiDensityLabel->setText(QString());
    ui->tranCalHiReadingLabel->setText(QString());
    ui->tranUvCalLoDensityLabel->setText(QString());
    ui->tranUvCalLoReadingLabel->setText(QString());
    ui->tranUvCalHiDensityLabel->setText(QString());
    ui->tranUvCalHiReadingLabel->setText(QString());

    // Set all checkboxes to an initial disabled state
    ui->importReflCheckBox->setEnabled(false);
    ui->importTranCheckBox->setEnabled(false);
    ui->importUvTranCheckBox->setEnabled(false);
    ui->importGainCheckBox->setEnabled(false);
    ui->importTempCheckBox->setEnabled(false);

    connect(ui->importReflCheckBox, &QCheckBox::checkStateChanged, this, &SettingsUvVisImportDialog::onCheckBoxChanged);
    connect(ui->importTranCheckBox, &QCheckBox::checkStateChanged, this, &SettingsUvVisImportDialog::onCheckBoxChanged);
    connect(ui->importUvTranCheckBox, &QCheckBox::checkStateChanged, this, &SettingsUvVisImportDialog::onCheckBoxChanged);
    connect(ui->importGainCheckBox, &QCheckBox::checkStateChanged, this, &SettingsUvVisImportDialog::onCheckBoxChanged);
    connect(ui->importTempCheckBox, &QCheckBox::checkStateChanged, this, &SettingsUvVisImportDialog::onCheckBoxChanged);

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    onCheckBoxChanged(Qt::Unchecked);
}

SettingsUvVisImportDialog::~SettingsUvVisImportDialog()
{
    delete ui;
}

bool SettingsUvVisImportDialog::loadFile(const QString &filename)
{
    if (filename.isEmpty()) { return false; }

    QFile importFile(filename);

    if (!importFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Couldn't open import file.";
        return false;
    }

    QByteArray importData = importFile.readAll();
    importFile.close();

    QJsonDocument doc = QJsonDocument::fromJson(importData);
    if (doc.isEmpty()) {
        return false;
    }

    QJsonObject root = doc.object();
    if (!parseHeader(root)) {
        return false;
    }

    if (root.contains("calibration") && root["calibration"].isObject()) {
        const QJsonObject jsonCal = root["calibration"].toObject();
        if (jsonCal.contains("sensor") && jsonCal["sensor"].isObject()) {
            const QJsonObject jsonCalSensor = jsonCal["sensor"].toObject();
            parseCalSensor(jsonCalSensor);
        }
        if (jsonCal.contains("target") && jsonCal["target"].isObject()) {
            const QJsonObject jsonCalTarget = jsonCal["target"].toObject();
            parseCalTarget(jsonCalTarget);
        }
        if (jsonCal.contains("temperature") && jsonCal["temperature"].isObject()) {
            const QJsonObject jsonCalTemp = jsonCal["temperature"].toObject();
            parseCalTemperature(jsonCalTemp);
        }
    }

    onCheckBoxChanged(Qt::Unchecked);

    return true;
}

bool SettingsUvVisImportDialog::parseHeader(const QJsonObject &root)
{
    QString exportDate;
    QString deviceName;
    QString deviceVersion;
    QString deviceUid;
    QString devicePartNumber;
    if (root.contains("header") && root["header"].isObject()) {
        QJsonObject jsonHeader = root["header"].toObject();
        if (jsonHeader.contains("version")) {
            int version = util::parseJsonInt(jsonHeader["version"]);
            if (version != 1) {
                qWarning() << "Unexpected version:" << version;
                return false;
            }
        }
        if (jsonHeader.contains("date")) {
            exportDate = jsonHeader["date"].toString();
        }
    }
    if (root.contains("system") && root["system"].isObject()) {
        QJsonObject jsonSystem = root["system"].toObject();

        if (jsonSystem.contains("name")) {
            deviceName = jsonSystem["name"].toString();
        }
        if (jsonSystem.contains("version")) {
            deviceVersion = jsonSystem["version"].toString();
        }
        if (jsonSystem.contains("uid")) {
            deviceUid = jsonSystem["uid"].toString();
        }
        if (jsonSystem.contains("partNumber")) {
            devicePartNumber = jsonSystem["partNumber"].toString();
        }
    }

    // Validate matching device type
    if (devicePartNumber.isEmpty() && deviceName != QLatin1String("Printalyzer UV/VIS Densitometer")) {
        return false;
    }
    if (!devicePartNumber.isEmpty() && devicePartNumber != QLatin1String("DPD-105")) {
        return false;
    }

    ui->deviceNameLabel->setText(deviceName);
    ui->deviceVersionLabel->setText(tr("Version: %1").arg(deviceVersion));
    ui->deviceUidLabel->setText(tr("Device UID: %1").arg(deviceUid));
    ui->exportDateLabel->setText(tr("Export Date: %1").arg(exportDate));
    return true;
}

void SettingsUvVisImportDialog::parseCalSensor(const QJsonObject &jsonCalSensor)
{
    if (jsonCalSensor.contains("gain")) {
        calGain_ = DensUvVisCalGain::fromJson(jsonCalSensor["gain"]);

        // Assign UI labels
        for (int i = 0; i < ui->gainTableWidget->rowCount(); i++) {
            QTableWidgetItem *item = util::tableWidgetItem(ui->gainTableWidget, i, 0);
            float gainValue = calGain_.gainValue(static_cast<DensUvVisCalGain::GainLevel>(i));
            if (qIsNaN(gainValue) || !qIsFinite(gainValue) || gainValue <= 0.0F) {
                item->setText(QString());
            } else {
                item->setText(QString::number(gainValue, 'f'));
            }
        }

        // Do basic validation to enable the checkbox
        ui->importGainCheckBox->setEnabled(calGain_.isValid());
    }
}

void SettingsUvVisImportDialog::parseCalTarget(const QJsonObject &jsonCalTarget)
{
    if (jsonCalTarget.contains("visReflection")) {
        calReflection_ = DensCalTarget::fromJson(jsonCalTarget["visReflection"]);
    }

    // Assign UI labels
    ui->reflCalLoDensityLabel->setText(QString::number(calReflection_.loDensity(), 'f', 2));
    ui->reflCalLoReadingLabel->setText(QString::number(calReflection_.loReading(), 'f', 6));
    ui->reflCalHiDensityLabel->setText(QString::number(calReflection_.hiDensity(), 'f', 2));
    ui->reflCalHiReadingLabel->setText(QString::number(calReflection_.hiReading(), 'f', 6));

    // Do basic validation and enable the checkbox
    ui->importReflCheckBox->setEnabled(calReflection_.isValidReflection());

    if (jsonCalTarget.contains("visTransmission")) {
        calVisTransmission_ = DensCalTarget::fromJson(jsonCalTarget["visTransmission"]);
    }

    // Assign UI labels
    ui->tranCalLoDensityLabel->setText(QString::number(calVisTransmission_.loDensity(), 'f', 2));
    ui->tranCalLoReadingLabel->setText(QString::number(calVisTransmission_.loReading(), 'f', 6));
    ui->tranCalHiDensityLabel->setText(QString::number(calVisTransmission_.hiDensity(), 'f', 2));
    ui->tranCalHiReadingLabel->setText(QString::number(calVisTransmission_.hiReading(), 'f', 6));

    // Do basic validation and enable the checkbox
    ui->importTranCheckBox->setEnabled(calVisTransmission_.isValidTransmission());

    if (jsonCalTarget.contains("uvTransmission")) {
        calUvTransmission_ = DensCalTarget::fromJson(jsonCalTarget["uvTransmission"]);
    }

    // Assign UI labels
    ui->tranUvCalLoDensityLabel->setText(QString::number(calUvTransmission_.loDensity(), 'f', 2));
    ui->tranUvCalLoReadingLabel->setText(QString::number(calUvTransmission_.loReading(), 'f', 6));
    ui->tranUvCalHiDensityLabel->setText(QString::number(calUvTransmission_.hiDensity(), 'f', 2));
    ui->tranUvCalHiReadingLabel->setText(QString::number(calUvTransmission_.hiReading(), 'f', 6));

    // Do basic validation and enable the checkbox
    ui->importUvTranCheckBox->setEnabled(calUvTransmission_.isValidTransmission());
}

void SettingsUvVisImportDialog::parseCalTemperature(const QJsonObject &jsonCalTemp)
{
    if (jsonCalTemp.contains("vis")) {
        calVisTemp_ = DensCalTemperature::fromJson(jsonCalTemp["vis"]);
        calTemperatureAssignColumn(ui->tempTableWidget, 0, calVisTemp_);
    }

    if (jsonCalTemp.contains("uv")) {
        calUvTemp_ = DensCalTemperature::fromJson(jsonCalTemp["uv"]);
        calTemperatureAssignColumn(ui->tempTableWidget, 1, calUvTemp_);
    }

    // Do basic validation to enable the checkbox
    ui->importTempCheckBox->setEnabled(calVisTemp_.isValid() && calUvTemp_.isValid());
}

void SettingsUvVisImportDialog::onCheckBoxChanged(Qt::CheckState state)
{
    Q_UNUSED(state)
    if (ui->importGainCheckBox->isChecked() || ui->importTempCheckBox->isChecked()
            || ui->importReflCheckBox->isChecked() || ui->importTranCheckBox->isChecked()) {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    } else {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    }
}

void SettingsUvVisImportDialog::calTemperatureAssignColumn(QTableWidget *table, int col, const DensCalTemperature &sourceValues)
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

void SettingsUvVisImportDialog::sendSelectedSettings(DensInterface *densInterface)
{
    if (!densInterface) { return; }

    if (ui->importGainCheckBox->isChecked()) {
        densInterface->sendSetUvVisCalGain(calGain_);
    }
    if (ui->importTempCheckBox->isChecked()) {
        densInterface->sendSetCalVisTemperature(calVisTemp_);
        densInterface->sendSetCalUvTemperature(calUvTemp_);
    }
    if (ui->importReflCheckBox->isChecked()) {
        densInterface->sendSetCalReflection(calReflection_);
    }
    if (ui->importTranCheckBox->isChecked()) {
        densInterface->sendSetCalTransmission(calVisTransmission_);
    }
    if (ui->importUvTranCheckBox->isChecked()) {
        densInterface->sendSetCalUvTransmission(calUvTransmission_);
    }
}
