#ifndef DENSINTERFACE_H
#define DENSINTERFACE_H

#include <stdint.h>
#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QDateTime>
#include "denscommand.h"
#include "denscalvalues.h"

class DensInterface : public QObject
{
    Q_OBJECT
public:
    enum DeviceType {
        DeviceUnknown,
        DeviceBaseline,
        DeviceUvVis
    };

    enum DensityType {
        DensityReflection,
        DensityTransmission,
        DensityUvTransmission,
        DensityUnknown = -1
    };
    Q_ENUM(DensityType)

    enum DensityFormat {
        FormatBasic,
        FormatExtended
    };
    Q_ENUM(DensityFormat)

    enum SensorLight {
        SensorLightOff,
        SensorLightReflection,
        SensorLightTransmission,
        SensorLightUvTransmission
    };
    Q_ENUM(SensorLight)

    explicit DensInterface(QObject *parent = nullptr);

    static DeviceType portDeviceType(const QSerialPortInfo &info);

    bool connectToDevice(QSerialPort *serialPort, DeviceType deviceType);
    void disconnectFromDevice();

public slots:
    void sendGetSystemVersion();
    void sendGetSystemBuild();
    void sendGetSystemDeviceInfo();
    void sendGetSystemRtosInfo();
    void sendGetSystemUID();
    void sendGetSystemInternalSensors();
    void sendInvokeSystemRemoteControl(bool enabled);
    void sendSetSystemDisplayText(const QString &text);
    void sendSetSystemDisplayEnable(bool enabled);

    void sendSetMeasurementFormat(DensInterface::DensityFormat format);
    void sendSetAllowUncalibratedMeasurements(bool allow);

    void sendGetDiagDisplayScreenshot();
    void sendGetDiagLightMax();
    void sendSetDiagLightRefl(int value);
    void sendSetDiagLightTran(int value);
    void sendSetDiagLightTranUv(int value);
    void sendInvokeDiagSensorStart();
    void sendInvokeDiagSensorStop();
    void sendSetUvDiagSensorMode(int mode);
    void sendSetBaselineDiagSensorConfig(int gain, int integration);
    void sendSetUvDiagSensorConfig(int gain, int sampleTime, int sampleCount);
    void sendSetUvDiagSensorAgcEnable(int sampleCount);
    void sendSetUvDiagSensorAgcDisable();
    void sendInvokeBaselineDiagRead(DensInterface::SensorLight light, int gain, int integration);
    void sendInvokeUvDiagRead(DensInterface::SensorLight light, int lightValue, int mode, int gain, int sampleTime, int sampleCount);
    void sendInvokeUvDiagMeasure(DensInterface::SensorLight light, int lightValue);
    void sendSetDiagLoggingModeUsb();
    void sendSetDiagLoggingModeDebug();

    void sendInvokeCalGain();
    void sendGetCalLight();
    void sendSetCalLight(const DensCalLight &calLight);
    void sendGetCalGain();
    void sendSetCalGain(const DensCalGain &calGain);
    void sendSetUvVisCalGain(const DensUvVisCalGain &calGain);
    void sendGetCalSlope();
    void sendSetCalSlope(const DensCalSlope &calSlope);
    void sendGetCalVisTemperature();
    void sendSetCalVisTemperature(const DensCalTemperature &calTemperature);
    void sendGetCalUvTemperature();
    void sendSetCalUvTemperature(const DensCalTemperature &calTemperature);
    void sendGetCalReflection();
    void sendSetCalReflection(const DensCalTarget &calTarget);
    void sendGetCalTransmission();
    void sendSetCalTransmission(const DensCalTarget &calTarget);
    void sendGetCalUvTransmission();
    void sendSetCalUvTransmission(const DensCalTarget &calTarget);

public:
    bool connected() const;
    bool deviceUnrecognized() const;
    bool remoteControlEnabled() const;

    DeviceType deviceType() const;
    QString projectName() const;
    QString version() const;

    QDateTime buildDate() const;
    QString buildDescribe() const;
    uint32_t buildChecksum() const;

    QString halVersion() const;
    QString mcuDeviceId() const;
    QString mcuRevisionId() const;
    QString mcuSysClock() const;

    QString freeRtosVersion() const;
    uint32_t freeRtosHeapSize() const;
    uint32_t freeRtosHeapWatermark() const;
    uint32_t freeRtosTaskCount() const;

    QString uniqueId() const;

    QString mcuVdda() const;
    QString mcuTemp() const;
    QString sensorTemp() const;

    uint16_t diagLightMax() const;

    DensCalLight calLight() const;
    DensCalGain calGain() const;
    DensUvVisCalGain calUvVisGain() const;
    DensCalSlope calSlope() const;
    DensCalTemperature calVisTemperature() const;
    DensCalTemperature calUvTemperature() const;

    DensCalTarget calReflection() const;
    DensCalTarget calTransmission() const;
    DensCalTarget calUvTransmission() const;

signals:
    void connectionOpened();
    void connectionClosed();
    void connectionError();

    void densityReading(DensInterface::DensityType type, float dValue, float dZero, float rawValue, float corrValue);
    void measurementFormatChanged();
    void allowUncalibratedMeasurementsChanged();

    void systemVersionResponse();
    void systemBuildResponse();
    void systemDeviceResponse();
    void systemRtosResponse();
    void systemUniqueId();
    void systemInternalSensors();
    void systemRemoteControl(bool enabled);
    void systemDisplaySetComplete();

    void diagDisplayScreenshot(const QByteArray &data);
    void diagLightMaxChanged();
    void diagLightReflChanged();
    void diagLightTranChanged();
    void diagLightTranUvChanged();
    void diagSensorInvoked();
    void diagSensorChanged();
    void diagSensorBaselineGetReading(int ch0, int ch1);
    void diagSensorUvGetReading(unsigned int ch0, int gain, int sampleTime, int sampleCount);
    void diagSensorBaselineInvokeReading(int ch0, int ch1);
    void diagSensorUvInvokeReading(unsigned int ch0);
    void diagSensorInvokeReadingError();
    void diagSensorUvInvokeMeasurement(float ch0Basic);
    void diagSensorInvokeMeasurementError();
    void diagLogLine(const QByteArray &data);

    void calLightResponse();
    void calLightSetComplete();
    void calGainCalStatus(int status, int param);
    void calGainCalFinished();
    void calGainCalError();
    void calGainResponse();
    void calGainSetComplete();
    void calSlopeResponse();
    void calSlopeSetComplete();
    void calVisTemperatureResponse();
    void calVisTemperatureSetComplete();
    void calUvTemperatureResponse();
    void calUvTemperatureSetComplete();
    void calReflectionResponse();
    void calReflectionSetComplete();
    void calTransmissionResponse();
    void calTransmissionSetComplete();
    void calUvTransmissionResponse();
    void calUvTransmissionSetComplete();

private slots:
    void readData();
    void handleError(QSerialPort::SerialPortError error);

private:
    static bool isLogLine(const QByteArray &line);
    void readDensityResponse(const DensCommand &response);
    void readCommandResponse(const DensCommand &response);
    void readSystemResponse(const DensCommand &response);
    void readMeasurementResponse(const DensCommand &response);
    void readCalibrationResponse(const DensCommand &response);
    void readDiagnosticsResponse(const DensCommand &response);
    static bool isResponseSetOk(const DensCommand &response, QLatin1String action);

    bool sendCommand(const DensCommand &command);

    QSerialPort *serialPort_;
    bool multilinePending_;
    DensCommand multilineResponse_;
    QByteArray multilineBuffer_;
    bool connecting_;
    bool connected_;
    bool deviceUnrecognized_;
    bool remoteControlEnabled_;

    DeviceType deviceType_;
    QString projectName_;
    QString version_;
    QDateTime buildDate_;
    QString buildDescribe_;
    uint32_t buildChecksum_;
    QString halVersion_;
    QString mcuRevisionId_;
    QString mcuDeviceId_;
    QString mcuSysClock_;
    QString freeRtosVersion_;
    uint32_t freeRtosHeapSize_;
    uint32_t freeRtosHeapWatermark_;
    uint32_t freeRtosTaskCount_;
    QString uniqueId_;
    QString mcuVdda_;
    QString mcuTemp_;
    QString sensorTemp_;
    uint16_t diagLightMax_;
    DensCalLight calLight_;
    DensCalGain calGain_;
    DensUvVisCalGain calUvVisGain_;
    DensCalSlope calSlope_;
    DensCalTemperature calVisTemperature_;
    DensCalTemperature calUvTemperature_;
    DensCalTarget calReflection_;
    DensCalTarget calTransmission_;
    DensCalTarget calUvTransmission_;
};

#endif // DENSINTERFACE_H
