#include "settingsexporter.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QTimer>
#include <QDebug>

SettingsExporter::SettingsExporter(DensInterface *densInterface, QObject *parent)
    : QObject{parent},
      densInterface_(densInterface)
{
    timer_ = new QTimer(this);
    timer_->setSingleShot(true);
    connect(timer_, &QTimer::timeout, this, QOverload<>::of(&SettingsExporter::onPrepareTimeout));

    connect(densInterface_, &DensInterface::connectionClosed, this, &SettingsExporter::onConnectionClosed);
    connect(densInterface_, &DensInterface::systemVersionResponse, this, &SettingsExporter::onSystemVersionResponse);
    connect(densInterface_, &DensInterface::systemBuildResponse, this, &SettingsExporter::onSystemBuildResponse);
    connect(densInterface_, &DensInterface::systemUniqueId, this, &SettingsExporter::onSystemUniqueId);

    connect(densInterface_, &DensInterface::calGainResponse, this, &SettingsExporter::onCalGainResponse);
    connect(densInterface_, &DensInterface::calSlopeResponse, this, &SettingsExporter::onCalSlopeResponse);
    connect(densInterface_, &DensInterface::calReflectionResponse, this, &SettingsExporter::onCalReflectionResponse);
    connect(densInterface_, &DensInterface::calTransmissionResponse, this, &SettingsExporter::onCalTransmissionResponse);
    connect(densInterface_, &DensInterface::calUvTransmissionResponse, this, &SettingsExporter::onCalUvTransmissionResponse);
    connect(densInterface_, &DensInterface::calVisTemperatureResponse, this, &SettingsExporter::onCalVisTemperatureResponse);
    connect(densInterface_, &DensInterface::calUvTemperatureResponse, this, &SettingsExporter::onCalUvTemperatureResponse);
}

void SettingsExporter::prepareExport()
{
    qDebug() << "Getting all settings for export";
    densInterface_->sendGetSystemVersion();
    densInterface_->sendGetSystemBuild();
    densInterface_->sendGetSystemUID();
    densInterface_->sendGetCalGain();
    densInterface_->sendGetCalReflection();
    densInterface_->sendGetCalTransmission();

    if (densInterface_->deviceType() == DensInterface::DeviceBaseline) {
        densInterface_->sendGetCalSlope();
    } else if (densInterface_->deviceType() == DensInterface::DeviceUvVis) {
        densInterface_->sendGetCalUvTransmission();
        densInterface_->sendGetCalVisTemperature();
        densInterface_->sendGetCalUvTemperature();
    }

    timer_->start(5000);
}

bool SettingsExporter::saveExport(const QString &filename)
{
    if (prepareFailed_ || !hasAllData_ || filename.isEmpty()) { return false; }
    qDebug() << "Saving data to file:" << filename;

    // File header information
    QJsonObject jsonHeader;
    jsonHeader["version"] = QString::number(1);
    jsonHeader["date"] = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm");

    // General system properties, for reference
    QJsonObject jsonSystem;
    jsonSystem["name"] = densInterface_->projectName();
    jsonSystem["version"] = densInterface_->version();
    jsonSystem["buildDate"] = densInterface_->buildDate().toString("yyyy-MM-dd hh:mm");
    jsonSystem["buildDescribe"] = densInterface_->buildDescribe();
    jsonSystem["checksum"] = QString::number(densInterface_->buildChecksum(), 16);
    jsonSystem["uid"] = densInterface_->uniqueId();
    if (densInterface_->deviceType() == DensInterface::DeviceBaseline) {
        jsonSystem["partNumber"] = "DPD-100";
    } else if (densInterface_->deviceType() == DensInterface::DeviceUvVis) {
        jsonSystem["partNumber"] = "DPD-105";
    }

    // Calibration data
    QJsonObject jsonCal;

    // Sensor calibration
    QJsonObject jsonCalSensor;

    if (densInterface_->deviceType() == DensInterface::DeviceBaseline) {
        const DensCalGain calGain = densInterface_->calGain();
        jsonCalSensor["gain"] = calGain.toJson();
    } else if (densInterface_->deviceType() == DensInterface::DeviceUvVis) {
        const DensUvVisCalGain calGain = densInterface_->calUvVisGain();
        jsonCalSensor["gain"] = calGain.toJson();
    }

    if (densInterface_->deviceType() == DensInterface::DeviceBaseline) {
        const DensCalSlope calSlope = densInterface_->calSlope();
        jsonCalSensor["slope"] = calSlope.toJson();
    }

    jsonCal["sensor"] = jsonCalSensor;

    // Target calibration
    QJsonObject jsonCalTarget;

    if (densInterface_->deviceType() == DensInterface::DeviceBaseline) {
        const DensCalTarget calReflection = densInterface_->calReflection();
        const DensCalTarget calTransmission = densInterface_->calTransmission();
        jsonCalTarget["reflection"] = calReflection.toJson();
        jsonCalTarget["transmission"] = calTransmission.toJson();
    } else if (densInterface_->deviceType() == DensInterface::DeviceUvVis) {
        const DensCalTarget calVisReflection = densInterface_->calReflection();
        const DensCalTarget calVisTransmission = densInterface_->calTransmission();
        const DensCalTarget calUvTransmission = densInterface_->calUvTransmission();
        jsonCalTarget["visReflection"] = calVisReflection.toJson();
        jsonCalTarget["visTransmission"] = calVisTransmission.toJson();
        jsonCalTarget["uvTransmission"] = calUvTransmission.toJson();
    }
    jsonCal["target"] = jsonCalTarget;

    // Temperature calibration
    if (densInterface_->deviceType() == DensInterface::DeviceUvVis) {
        const DensCalTemperature calVisTemperature = densInterface_->calVisTemperature();
        const DensCalTemperature calUvTemperature = densInterface_->calUvTemperature();

        QJsonObject jsonCalTemperature;
        jsonCalTemperature["vis"] = calVisTemperature.toJson();
        jsonCalTemperature["uv"] = calUvTemperature.toJson();
        jsonCal["temperature"] = jsonCalTemperature;
    }

    // Top level JSON object
    QJsonObject jsonExport;
    jsonExport["header"] = jsonHeader;
    jsonExport["system"] = jsonSystem;
    jsonExport["calibration"] = jsonCal;

    QFile exportFile(filename);

    if (!exportFile.open(QIODevice::WriteOnly)) {
        qWarning() << "Couldn't open export file.";
        return false;
    }

    exportFile.write(QJsonDocument(jsonExport).toJson(QJsonDocument::Indented));
    exportFile.close();
    return true;
}

void SettingsExporter::onPrepareTimeout()
{
    if (!hasAllData_) {
        timer_->stop();
        prepareFailed_ = true;
        emit exportFailed();
    }
}

void SettingsExporter::onConnectionClosed()
{
    if (!hasAllData_) {
        timer_->stop();
        prepareFailed_ = true;
        emit exportFailed();
    }
}

void SettingsExporter::onSystemVersionResponse()
{
    hasSystemVersion_ = true;
    checkResponses();
}

void SettingsExporter::onSystemBuildResponse()
{
    hasSystemBuild_ = true;
    checkResponses();
}

void SettingsExporter::onSystemUniqueId()
{
    hasSystemUid_ = true;
    checkResponses();
}

void SettingsExporter::onCalGainResponse()
{
    hasCalGain_ = true;
    checkResponses();
}

void SettingsExporter::onCalSlopeResponse()
{
    hasCalSlope_ = true;
    checkResponses();
}

void SettingsExporter::onCalReflectionResponse()
{
    hasCalReflection_ = true;
    checkResponses();
}

void SettingsExporter::onCalTransmissionResponse()
{
    hasCalTransmission_ = true;
    checkResponses();
}

void SettingsExporter::onCalUvTransmissionResponse()
{
    hasCalUvTransmission_ = true;
    checkResponses();
}

void SettingsExporter::onCalVisTemperatureResponse()
{
    hasCalVisTemperature_ = true;
    checkResponses();
}

void SettingsExporter::onCalUvTemperatureResponse()
{
    hasCalUvTemperature_ = true;
    checkResponses();
}

void SettingsExporter::checkResponses()
{
    if (!hasSystemVersion_ || !hasSystemBuild_ || !hasSystemUid_
        || !hasCalGain_
        || !hasCalReflection_ || !hasCalTransmission_) {
        return;
    }

    if (densInterface_->deviceType() == DensInterface::DeviceBaseline) {
        if (!hasCalSlope_) {
            return;
        }
    } else if (densInterface_->deviceType() == DensInterface::DeviceUvVis) {
        if (!hasCalUvTransmission_ || !hasCalVisTemperature_ || !hasCalUvTemperature_) {
            return;
        }
    }

    hasAllData_ = true;
    timer_->stop();
    emit exportReady();
}
