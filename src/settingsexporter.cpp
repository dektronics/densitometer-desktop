#include "settingsexporter.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QTimer>
#include <QDebug>

using namespace Qt::StringLiterals;

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
    jsonHeader["version"_L1] = QString::number(1);
    jsonHeader["date"_L1] = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm"_L1);

    // General system properties, for reference
    QJsonObject jsonSystem;
    jsonSystem["name"_L1] = densInterface_->projectName();
    jsonSystem["version"_L1] = densInterface_->version();
    jsonSystem["buildDate"_L1] = densInterface_->buildDate().toString("yyyy-MM-dd hh:mm"_L1);
    jsonSystem["buildDescribe"_L1] = densInterface_->buildDescribe();
    jsonSystem["checksum"_L1] = QString::number(densInterface_->buildChecksum(), 16);
    jsonSystem["uid"_L1] = densInterface_->uniqueId();
    if (densInterface_->deviceType() == DensInterface::DeviceBaseline) {
        jsonSystem["partNumber"_L1] = "DPD-100"_L1;
    } else if (densInterface_->deviceType() == DensInterface::DeviceUvVis) {
        jsonSystem["partNumber"_L1] = "DPD-105"_L1;
    }

    // Calibration data
    QJsonObject jsonCal;

    // Sensor calibration
    QJsonObject jsonCalSensor;

    if (densInterface_->deviceType() == DensInterface::DeviceBaseline) {
        const DensCalGain calGain = densInterface_->calGain();
        jsonCalSensor["gain"_L1] = calGain.toJson();
    } else if (densInterface_->deviceType() == DensInterface::DeviceUvVis) {
        const DensUvVisCalGain calGain = densInterface_->calUvVisGain();
        jsonCalSensor["gain"_L1] = calGain.toJson();
    }

    if (densInterface_->deviceType() == DensInterface::DeviceBaseline) {
        const DensCalSlope calSlope = densInterface_->calSlope();
        jsonCalSensor["slope"_L1] = calSlope.toJson();
    }

    jsonCal["sensor"_L1] = jsonCalSensor;

    // Target calibration
    QJsonObject jsonCalTarget;

    if (densInterface_->deviceType() == DensInterface::DeviceBaseline) {
        const DensCalTarget calReflection = densInterface_->calReflection();
        const DensCalTarget calTransmission = densInterface_->calTransmission();
        jsonCalTarget["reflection"_L1] = calReflection.toJson();
        jsonCalTarget["transmission"_L1] = calTransmission.toJson();
    } else if (densInterface_->deviceType() == DensInterface::DeviceUvVis) {
        const DensCalTarget calVisReflection = densInterface_->calReflection();
        const DensCalTarget calVisTransmission = densInterface_->calTransmission();
        const DensCalTarget calUvTransmission = densInterface_->calUvTransmission();
        jsonCalTarget["visReflection"_L1] = calVisReflection.toJson();
        jsonCalTarget["visTransmission"_L1] = calVisTransmission.toJson();
        jsonCalTarget["uvTransmission"_L1] = calUvTransmission.toJson();
    }
    jsonCal["target"_L1] = jsonCalTarget;

    // Temperature calibration
    if (densInterface_->deviceType() == DensInterface::DeviceUvVis) {
        const DensCalTemperature calVisTemperature = densInterface_->calVisTemperature();
        const DensCalTemperature calUvTemperature = densInterface_->calUvTemperature();

        QJsonObject jsonCalTemperature;
        jsonCalTemperature["vis"_L1] = calVisTemperature.toJson();
        jsonCalTemperature["uv"_L1] = calUvTemperature.toJson();
        jsonCal["temperature"_L1] = jsonCalTemperature;
    }

    // Top level JSON object
    QJsonObject jsonExport;
    jsonExport["header"_L1] = jsonHeader;
    jsonExport["system"_L1] = jsonSystem;
    jsonExport["calibration"_L1] = jsonCal;

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
