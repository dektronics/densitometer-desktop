#ifndef SETTINGSEXPORTER_H
#define SETTINGSEXPORTER_H

#include <QObject>
#include <QJsonObject>

#include "densinterface.h"

class QTimer;

class SettingsExporter : public QObject
{
    Q_OBJECT
public:
    explicit SettingsExporter(DensInterface *densInterface, QObject *parent = nullptr);

    void prepareExport();
    bool saveExport(const QString &filename);

signals:
    void exportReady();
    void exportFailed();

private slots:
    void onPrepareTimeout();
    void onConnectionClosed();
    void onSystemVersionResponse();
    void onSystemBuildResponse();
    void onSystemUniqueId();
    void onCalGainResponse();
    void onCalSlopeResponse();
    void onCalReflectionResponse();
    void onCalTransmissionResponse();
    void onCalUvTransmissionResponse();
    void onCalVisTemperatureResponse();
    void onCalUvTemperatureResponse();

private:
    QJsonObject createJsonCalTarget(const DensCalTarget &calTarget);
    void checkResponses();

    DensInterface *densInterface_;
    QTimer *timer_ = nullptr;
    bool hasSystemVersion_ = false;
    bool hasSystemBuild_ = false;
    bool hasSystemUid_ = false;
    bool hasCalGain_ = false;
    bool hasCalSlope_ = false;
    bool hasCalReflection_ = false;
    bool hasCalTransmission_ = false;
    bool hasCalUvTransmission_ = false;
    bool hasCalVisTemperature_ = false;
    bool hasCalUvTemperature_ = false;
    bool hasAllData_ = false;
    bool prepareFailed_ = false;
};

#endif // SETTINGSEXPORTER_H
