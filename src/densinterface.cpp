#include "densinterface.h"

#include <QDebug>

#include "denscommand.h"
#include "util.h"

DensInterface::DensInterface(QObject *parent)
    : QObject(parent)
    , serialPort_(nullptr)
    , multilinePending_(false)
    , connecting_(false)
    , connected_(false)
    , deviceUnrecognized_(false)
    , remoteControlEnabled_(false)
    , deviceType_(DeviceType::DeviceUnknown)
    , buildChecksum_(0)
    , freeRtosHeapSize_(0)
    , freeRtosHeapWatermark_(0)
    , freeRtosTaskCount_(0)
    , diagLightMax_(128)
{
}

DensInterface::DeviceType DensInterface::portDeviceType(const QSerialPortInfo &info)
{
    if (info.vendorIdentifier() == 0x16D0 && info.productIdentifier() == 0x10EB) {
        return DeviceBaseline;
    } else if (info.vendorIdentifier() == 0x16D0 && info.productIdentifier() == 0x13E7) {
        return DeviceUvVis;
    } else {
        return DeviceUnknown;
    }
}

bool DensInterface::connectToDevice(QSerialPort *serialPort, DeviceType deviceType)
{
    if (serialPort_) { return false; }
    if (!serialPort || !serialPort->isOpen()) {
        return false;
    }
    if (connected_ || connecting_) {
        return false;
    }

    // Set to connecting state
    connecting_ = true;
    deviceUnrecognized_ = false;
    remoteControlEnabled_ = false;

    // Connect to signals for non-blocking command use
    if (serialPort->parent() == nullptr) {
        serialPort->setParent(this);
    }
    serialPort_ = serialPort;
    deviceType_ = deviceType;
    connect(serialPort_, &QSerialPort::errorOccurred, this, &DensInterface::handleError);
    connect(serialPort_, &QSerialPort::readyRead, this, &DensInterface::readData);

    // Send command to get system version, to verify connected device
    DensCommand command(DensCommand::TypeGet, DensCommand::CategorySystem, "V");
    return sendCommand(command);
}

void DensInterface::disconnectFromDevice()
{
    bool notify = connected_ || connecting_;
    if (serialPort_) {
        disconnect(serialPort_, &QSerialPort::errorOccurred, this, &DensInterface::handleError);
        disconnect(serialPort_, &QSerialPort::readyRead, this, &DensInterface::readData);
        if (serialPort_->parent() == this) {
            if (serialPort_->isOpen()) {
                serialPort_->close();
            }
			serialPort_->deleteLater();
        }
		serialPort_ = nullptr;
    }
    multilineResponse_ = DensCommand();
    multilineBuffer_.clear();
    multilinePending_ = false;
    connecting_ = false;
    connected_ = false;
    remoteControlEnabled_ = false;
    if (notify) {
        emit connectionClosed();
    }
}

void DensInterface::sendGetSystemVersion()
{
    DensCommand command(DensCommand::TypeGet, DensCommand::CategorySystem, "V");
    sendCommand(command);
}

void DensInterface::sendGetSystemBuild()
{
    DensCommand command(DensCommand::TypeGet, DensCommand::CategorySystem, "B");
    sendCommand(command);
}

void DensInterface::sendGetSystemDeviceInfo()
{
    DensCommand command(DensCommand::TypeGet, DensCommand::CategorySystem, "DEV");
    sendCommand(command);
}

void DensInterface::sendGetSystemRtosInfo()
{
    DensCommand command(DensCommand::TypeGet, DensCommand::CategorySystem, "RTOS");
    sendCommand(command);
}

void DensInterface::sendGetSystemUID()
{
    DensCommand command(DensCommand::TypeGet, DensCommand::CategorySystem, "UID");
    sendCommand(command);
}

void DensInterface::sendGetSystemInternalSensors()
{
    DensCommand command(DensCommand::TypeGet, DensCommand::CategorySystem, "ISEN");
    sendCommand(command);
}

void DensInterface::sendInvokeSystemRemoteControl(bool enabled)
{
    QStringList args;
    args.append(enabled ? "1" : "0");

    DensCommand command(DensCommand::TypeInvoke, DensCommand::CategorySystem, "REMOTE", args);
    sendCommand(command);
}

void DensInterface::sendSetSystemDisplayText(const QString &text)
{
    QString sendText = text;
    sendText.replace(QChar('\\'), QLatin1String("\\\\"));
    sendText.replace(QChar('\n'), QLatin1String("\\n"));

    if (deviceType_ == DeviceType::DeviceUvVis) {
        sendText = QChar('"') + sendText + QChar('"');
    }

    QStringList args;
    args.append(sendText);

    DensCommand command(DensCommand::TypeSet, DensCommand::CategorySystem, "DISP", args);
    sendCommand(command);
}

void DensInterface::sendSetSystemDisplayEnable(bool enabled)
{
    if (deviceType_ != DeviceType::DeviceUvVis) { return; }

    QStringList args;
    args.append(enabled ? "1" : "0");

    DensCommand command(DensCommand::TypeSet, DensCommand::CategorySystem, "DISP", args);
    sendCommand(command);
}

void DensInterface::sendSetMeasurementFormat(DensInterface::DensityFormat format)
{
    QStringList args;
    if (format == FormatBasic) {
        args.append("BASIC");
    } else if (format == FormatExtended) {
        args.append("EXT");
    } else {
        qWarning() << "Unsupported format:" << format;
        return;
    }

    DensCommand command(DensCommand::TypeSet, DensCommand::CategoryMeasurement, "FORMAT", args);
    sendCommand(command);
}

void DensInterface::sendSetAllowUncalibratedMeasurements(bool allow)
{
    QStringList args;
    if (allow) {
        args.append("1");
    } else {
        args.append("0");
    }

    DensCommand command(DensCommand::TypeSet, DensCommand::CategoryMeasurement, "UNCAL", args);
    sendCommand(command);
}

void DensInterface::sendGetDiagDisplayScreenshot()
{
    DensCommand command(DensCommand::TypeGet, DensCommand::CategoryDiagnostics, "DISP");
    sendCommand(command);
}

void DensInterface::sendGetDiagLightMax()
{
    if (deviceType_ != DeviceType::DeviceUvVis) { return; }

    DensCommand command(DensCommand::TypeGet, DensCommand::CategoryDiagnostics, "LMAX");
    sendCommand(command);
}

void DensInterface::sendSetDiagLightRefl(int value)
{
    if (value < 0) { value = 0; }
    else if (value > std::numeric_limits<uint16_t>::max()) { value = std::numeric_limits<uint16_t>::max(); }

    QStringList args;
    args.append(QString::number(value));

    DensCommand command(DensCommand::TypeSet, DensCommand::CategoryDiagnostics, "LR", args);
    sendCommand(command);
}

void DensInterface::sendSetDiagLightTran(int value)
{
    if (value < 0) { value = 0; }
    else if (value > std::numeric_limits<uint16_t>::max()) { value = std::numeric_limits<uint16_t>::max(); }

    QStringList args;
    args.append(QString::number(value));

    DensCommand command(DensCommand::TypeSet, DensCommand::CategoryDiagnostics, "LT", args);
    sendCommand(command);
}

void DensInterface::sendSetDiagLightTranUv(int value)
{
    if (deviceType_ != DeviceType::DeviceUvVis) { return; }

    if (value < 0) { value = 0; }
    else if (value > std::numeric_limits<uint16_t>::max()) { value = std::numeric_limits<uint16_t>::max(); }

    QStringList args;
    args.append(QString::number(value));

    DensCommand command(DensCommand::TypeSet, DensCommand::CategoryDiagnostics, "LTU", args);
    sendCommand(command);
}

void DensInterface::sendInvokeDiagSensorStart()
{
    DensCommand command(DensCommand::TypeInvoke, DensCommand::CategoryDiagnostics, "S",
                        QStringList() << "START");
    sendCommand(command);
}

void DensInterface::sendInvokeDiagSensorStop()
{
    DensCommand command(DensCommand::TypeInvoke, DensCommand::CategoryDiagnostics, "S",
                        QStringList() << "STOP");
    sendCommand(command);
}

void DensInterface::sendSetUvDiagSensorMode(int mode)
{
    if (deviceType_ != DeviceType::DeviceUvVis) { return; }
    if (mode < 0) { mode = 0; }
    else if (mode > 2) { mode = 2; }

    QStringList args;
    args.append("MODE");
    args.append(QString::number(mode));

    DensCommand command(DensCommand::TypeSet, DensCommand::CategoryDiagnostics, "S", args);
    sendCommand(command);
}

void DensInterface::sendSetBaselineDiagSensorConfig(int gain, int integration)
{
    if (deviceType_ != DeviceBaseline) { return; }

    if (gain < 0) { gain = 0; }
    else if (gain > 3) { gain = 3; }
    if (integration < 0) { integration = 0; }
    else if (integration > 5) { integration = 5; }

    QStringList args;
    args.append("CFG");
    args.append(QString::number(gain));
    args.append(QString::number(integration));

    DensCommand command(DensCommand::TypeSet, DensCommand::CategoryDiagnostics, "S", args);
    sendCommand(command);
}

void DensInterface::sendSetUvDiagSensorConfig(int gain, int sampleTime, int sampleCount)
{
    if (deviceType_ != DeviceUvVis) { return; }

    if (gain < 0) { gain = 0; }
    else if (gain > 9) { gain = 9; }
    if (sampleTime < 0) { sampleTime = 0; }
    else if (sampleTime > 2047) { sampleTime = 2047; }
    if (sampleCount < 0) { sampleCount = 0; }
    else if (sampleCount > 2047) { sampleCount = 2047; }

    QStringList args;
    args.append("CFG");
    args.append(QString::number(gain));
    args.append(QString::number(sampleTime));
    args.append(QString::number(sampleCount));

    DensCommand command(DensCommand::TypeSet, DensCommand::CategoryDiagnostics, "S", args);
    sendCommand(command);
}


void DensInterface::sendSetUvDiagSensorAgcEnable(int sampleCount)
{
    if (deviceType_ != DeviceUvVis) { return; }

    QStringList args;
    args.append("AGCEN");
    args.append(QString::number(sampleCount));

    DensCommand command(DensCommand::TypeSet, DensCommand::CategoryDiagnostics, "S", args);
    sendCommand(command);
}

void DensInterface::sendSetUvDiagSensorAgcDisable()
{
    if (deviceType_ != DeviceUvVis) { return; }

    QStringList args;
    args.append("AGCDIS");

    DensCommand command(DensCommand::TypeSet, DensCommand::CategoryDiagnostics, "S", args);
    sendCommand(command);
}

void DensInterface::sendInvokeBaselineDiagRead(DensInterface::SensorLight light, int gain, int integration)
{
    if (deviceType_ != DeviceBaseline) { return; }

    QStringList args;
    if (light == SensorLight::SensorLightReflection) {
        args.append("R");
    } else if (light == SensorLight::SensorLightTransmission) {
        args.append("T");
    } else {
        args.append("0");
    }
    args.append(QString::number(gain));
    args.append(QString::number(integration));

    DensCommand command(DensCommand::TypeInvoke, DensCommand::CategoryDiagnostics, "READ", args);
    sendCommand(command);
}

void DensInterface::sendInvokeUvDiagRead(DensInterface::SensorLight light, int lightValue, int mode, int gain, int sampleTime, int sampleCount)
{
    if (deviceType_ != DeviceUvVis) { return; }

    QStringList args;
    if (light == SensorLight::SensorLightReflection) {
        args.append("R");
    } else if (light == SensorLight::SensorLightTransmission) {
        args.append("T");
    } else if (light == SensorLight::SensorLightUvTransmission) {
        args.append("U");
    } else {
        args.append("0");
    }

    if (lightValue < 0) { lightValue = 0; }
    else if (lightValue > std::numeric_limits<uint16_t>::max()) { lightValue = std::numeric_limits<uint16_t>::max(); }
    args.append(QString::number(lightValue));

    args.append(QString::number(mode));
    args.append(QString::number(gain));
    args.append(QString::number(sampleTime));
    args.append(QString::number(sampleCount));

    DensCommand command(DensCommand::TypeInvoke, DensCommand::CategoryDiagnostics, "READ", args);
    sendCommand(command);
}

void DensInterface::sendInvokeUvDiagMeasure(DensInterface::SensorLight light, int lightValue)
{
    if (deviceType_ != DeviceUvVis) { return; }

    QStringList args;
    if (light == SensorLight::SensorLightReflection) {
        args.append("R");
    } else if (light == SensorLight::SensorLightTransmission) {
        args.append("T");
    } else if (light == SensorLight::SensorLightUvTransmission) {
        args.append("U");
    } else {
        return;
    }

    if (lightValue < 0) { lightValue = 0; }
    else if (lightValue > std::numeric_limits<uint16_t>::max()) { lightValue = std::numeric_limits<uint16_t>::max(); }
    args.append(QString::number(lightValue));

    DensCommand command(DensCommand::TypeInvoke, DensCommand::CategoryDiagnostics, "MEAS", args);
    sendCommand(command);
}

void DensInterface::sendSetDiagLoggingModeUsb()
{
    DensCommand command(DensCommand::TypeSet, DensCommand::CategoryDiagnostics,
                        "LOG", QStringList() << "U");
    sendCommand(command);
}

void DensInterface::sendSetDiagLoggingModeDebug()
{
    DensCommand command(DensCommand::TypeSet, DensCommand::CategoryDiagnostics,
                        "LOG", QStringList() << "D");
    sendCommand(command);
}

void DensInterface::sendInvokeCalGain()
{
    DensCommand command(DensCommand::TypeInvoke, DensCommand::CategoryCalibration, "GAIN");
    sendCommand(command);
}

void DensInterface::sendGetCalLight()
{
    DensCommand command(DensCommand::TypeGet, DensCommand::CategoryCalibration, "LIGHT");
    sendCommand(command);
}

void DensInterface::sendSetCalLight(const DensCalLight &calLight)
{
    QStringList args;
    args.append(QString::number(calLight.reflectionValue()));
    args.append(QString::number(calLight.transmissionValue()));

    DensCommand command(DensCommand::TypeSet, DensCommand::CategoryCalibration, "LIGHT", args);
    sendCommand(command);
}

void DensInterface::sendGetCalGain()
{
    DensCommand command(DensCommand::TypeGet, DensCommand::CategoryCalibration, "GAIN");
    sendCommand(command);
}

void DensInterface::sendSetCalGain(const DensCalGain &calGain)
{
    if (deviceType_ != DeviceBaseline) { return; }

    QStringList args;
    args.append(util::encode_f32(calGain.med0()));
    args.append(util::encode_f32(calGain.med1()));
    args.append(util::encode_f32(calGain.high0()));
    args.append(util::encode_f32(calGain.high1()));
    args.append(util::encode_f32(calGain.max0()));
    args.append(util::encode_f32(calGain.max1()));

    DensCommand command(DensCommand::TypeSet, DensCommand::CategoryCalibration, "GAIN", args);
    sendCommand(command);
}

void DensInterface::sendSetUvVisCalGain(const DensUvVisCalGain &calGain)
{
    if (deviceType_ != DeviceUvVis) { return; }

    QStringList args;
    args.append(util::encode_f32(calGain.gainValue(DensUvVisCalGain::Gain0_5X)));
    args.append(util::encode_f32(calGain.gainValue(DensUvVisCalGain::Gain1X)));
    args.append(util::encode_f32(calGain.gainValue(DensUvVisCalGain::Gain2X)));
    args.append(util::encode_f32(calGain.gainValue(DensUvVisCalGain::Gain4X)));
    args.append(util::encode_f32(calGain.gainValue(DensUvVisCalGain::Gain8X)));
    args.append(util::encode_f32(calGain.gainValue(DensUvVisCalGain::Gain16X)));
    args.append(util::encode_f32(calGain.gainValue(DensUvVisCalGain::Gain32X)));
    args.append(util::encode_f32(calGain.gainValue(DensUvVisCalGain::Gain64X)));
    args.append(util::encode_f32(calGain.gainValue(DensUvVisCalGain::Gain128X)));
    args.append(util::encode_f32(calGain.gainValue(DensUvVisCalGain::Gain256X)));

    DensCommand command(DensCommand::TypeSet, DensCommand::CategoryCalibration, "GAIN", args);
    sendCommand(command);
}

void DensInterface::sendGetCalSlope()
{
    if (deviceType_ != DeviceBaseline) { return; }

    DensCommand command(DensCommand::TypeGet, DensCommand::CategoryCalibration, "SLOPE");
    sendCommand(command);
}

void DensInterface::sendSetCalSlope(const DensCalSlope &calSlope)
{
    if (deviceType_ != DeviceBaseline) { return; }

    QStringList args;
    args.append(util::encode_f32(calSlope.b0()));
    args.append(util::encode_f32(calSlope.b1()));
    args.append(util::encode_f32(calSlope.b2()));

    DensCommand command(DensCommand::TypeSet, DensCommand::CategoryCalibration, "SLOPE", args);
    sendCommand(command);
}

void DensInterface::sendGetCalVisTemperature()
{
    if (deviceType_ != DeviceUvVis) { return; }

    DensCommand command(DensCommand::TypeGet, DensCommand::CategoryCalibration, "VTEMP");
    sendCommand(command);
}

void DensInterface::sendSetCalVisTemperature(const DensCalTemperature &calTemperature)
{
    if (deviceType_ != DeviceUvVis) { return; }

    QStringList args;
    args.append(util::encode_f32(calTemperature.b0()));
    args.append(util::encode_f32(calTemperature.b1()));
    args.append(util::encode_f32(calTemperature.b2()));

    DensCommand command(DensCommand::TypeSet, DensCommand::CategoryCalibration, "VTEMP", args);
    sendCommand(command);
}

void DensInterface::sendGetCalUvTemperature()
{
    if (deviceType_ != DeviceUvVis) { return; }

    DensCommand command(DensCommand::TypeGet, DensCommand::CategoryCalibration, "UTEMP");
    sendCommand(command);
}

void DensInterface::sendSetCalUvTemperature(const DensCalTemperature &calTemperature)
{
    if (deviceType_ != DeviceUvVis) { return; }

    QStringList args;
    args.append(util::encode_f32(calTemperature.b0()));
    args.append(util::encode_f32(calTemperature.b1()));
    args.append(util::encode_f32(calTemperature.b2()));

    DensCommand command(DensCommand::TypeSet, DensCommand::CategoryCalibration, "UTEMP", args);
    sendCommand(command);
}

void DensInterface::sendGetCalReflection()
{
    DensCommand command(DensCommand::TypeGet, DensCommand::CategoryCalibration, "REFL");
    sendCommand(command);
}

void DensInterface::sendSetCalReflection(const DensCalTarget &calTarget)
{
    QStringList args;
    args.append(util::encode_f32(calTarget.loDensity()));
    args.append(util::encode_f32(calTarget.loReading()));
    args.append(util::encode_f32(calTarget.hiDensity()));
    args.append(util::encode_f32(calTarget.hiReading()));

    DensCommand command(DensCommand::TypeSet, DensCommand::CategoryCalibration, "REFL", args);
    sendCommand(command);
}

void DensInterface::sendGetCalTransmission()
{
    DensCommand command(DensCommand::TypeGet, DensCommand::CategoryCalibration, "TRAN");
    sendCommand(command);
}

void DensInterface::sendSetCalTransmission(const DensCalTarget &calTarget)
{
    QStringList args;
    args.append(util::encode_f32(calTarget.loDensity()));
    args.append(util::encode_f32(calTarget.loReading()));
    args.append(util::encode_f32(calTarget.hiDensity()));
    args.append(util::encode_f32(calTarget.hiReading()));

    DensCommand command(DensCommand::TypeSet, DensCommand::CategoryCalibration, "TRAN", args);
    sendCommand(command);
}

void DensInterface::sendGetCalUvTransmission()
{
    if (deviceType_ != DeviceUvVis) { return; }

    DensCommand command(DensCommand::TypeGet, DensCommand::CategoryCalibration, "UVTR");
    sendCommand(command);
}

void DensInterface::sendSetCalUvTransmission(const DensCalTarget &calTarget)
{
    if (deviceType_ != DeviceUvVis) { return; }

    QStringList args;
    args.append(util::encode_f32(calTarget.loDensity()));
    args.append(util::encode_f32(calTarget.loReading()));
    args.append(util::encode_f32(calTarget.hiDensity()));
    args.append(util::encode_f32(calTarget.hiReading()));

    DensCommand command(DensCommand::TypeSet, DensCommand::CategoryCalibration, "UVTR", args);
    sendCommand(command);
}

bool DensInterface::connected() const { return connected_; }
bool DensInterface::deviceUnrecognized() const { return deviceUnrecognized_; }
bool DensInterface::remoteControlEnabled() const { return remoteControlEnabled_; }

DensInterface::DeviceType DensInterface::deviceType() const { return deviceType_; }
QString DensInterface::projectName() const { return projectName_; }
QString DensInterface::version() const { return version_; }
QDateTime DensInterface::buildDate() const { return buildDate_; }
QString DensInterface::buildDescribe() const { return buildDescribe_; }
uint32_t DensInterface::buildChecksum() const { return buildChecksum_; }

QString DensInterface::halVersion() const { return halVersion_; }
QString DensInterface::mcuDeviceId() const { return mcuDeviceId_; }
QString DensInterface::mcuRevisionId() const { return mcuRevisionId_; }
QString DensInterface::mcuSysClock() const { return mcuSysClock_; }

QString DensInterface::freeRtosVersion() const { return freeRtosVersion_; }
uint32_t DensInterface::freeRtosHeapSize() const { return freeRtosHeapSize_; }
uint32_t DensInterface::freeRtosHeapWatermark() const { return freeRtosHeapWatermark_; }
uint32_t DensInterface::freeRtosTaskCount() const { return freeRtosTaskCount_; }

QString DensInterface::uniqueId() const { return uniqueId_; }

QString DensInterface::mcuVdda() const { return mcuVdda_; }
QString DensInterface::mcuTemp() const { return mcuTemp_; }
QString DensInterface::sensorTemp() const { return sensorTemp_; }

uint16_t DensInterface::diagLightMax() const { return diagLightMax_; }

DensCalLight DensInterface::calLight() const { return calLight_; }
DensCalGain DensInterface::calGain() const { return calGain_; }
DensUvVisCalGain DensInterface::calUvVisGain() const { return calUvVisGain_; }
DensCalSlope DensInterface::calSlope() const { return calSlope_; }
DensCalTemperature DensInterface::calVisTemperature() const { return calVisTemperature_; }
DensCalTemperature DensInterface::calUvTemperature() const { return calUvTemperature_; }

DensCalTarget DensInterface::calReflection() const { return calReflection_; }
DensCalTarget DensInterface::calTransmission() const { return calTransmission_; }
DensCalTarget DensInterface::calUvTransmission() const { return calUvTransmission_; }

void DensInterface::readData()
{
    while (serialPort_->canReadLine()) {
        const QByteArray line = serialPort_->readLine();
        if (connecting_) {
            // In connecting mode we expect to only receive very specific
            // information from the device. Anything else will cause the
            // connection check to fail.
            DensCommand response = DensCommand::parse(line);
            if (response.isDensity()) {
                // Density responses are the only thing the device can send
                // without first receiving a command or mode change request.
                // Therefore, we need to ignore those here.
                continue;
            } else if (response.isValid()
                        && response.category() == DensCommand::CategorySystem
                        && response.type() == DensCommand::TypeGet
                        && response.action() == QLatin1String("V")) {
                // System version responses are the only thing expected and
                // handled in this state, so pre-validate the response type
                // before calling the normal parsing code.

                // Save and clear old values, in case the parsing failed
                QString oldProjectName = projectName_;
                QString oldVersion = version_;
                projectName_ = QString();
                version_ = QString();

                // Call the normal command parser
                readCommandResponse(response);

                if (!projectName_.isEmpty() && !version_.isEmpty()) {
                    connecting_ = false;
                    connected_ = true;
                    if (deviceType_ != DeviceUvVis) {
                        diagLightMax_ = 128;
                    }
                    emit connectionOpened();
                    emit systemVersionResponse();
                } else {
                    // Failing to cleanly parse the version response should
                    // be treated like a connection failure
                    projectName_ = oldProjectName;
                    version_ = oldVersion;
                    qWarning() << "Unexpected version response:" << line;
                    deviceUnrecognized_ = true;
                    deviceType_ = DeviceType::DeviceUnknown;
                    disconnectFromDevice();
                }
            } else {
                // Any response other than what is explicitly expected should
                // be treated as a connection failure
                qWarning() << "Unexpected response:" << line;
                deviceUnrecognized_ = true;
                deviceType_ = DeviceType::DeviceUnknown;
                disconnectFromDevice();
            }
            return;
        }

        if (multilinePending_) {
            if (line == "]]\r\n") {
                multilineResponse_.setBuffer(multilineBuffer_);
                multilineBuffer_.clear();
                multilinePending_ = false;

                if (multilineResponse_.isValid()) {
                    readCommandResponse(multilineResponse_);
                } else {
                    qWarning() << "Unrecognized line:" << line;
                }
                multilineResponse_ = DensCommand();
                continue;
            } else {
                multilineBuffer_.append(line);
                continue;
            }
        }

        if (isLogLine(line)) {
            emit diagLogLine(line);
        } else {
            DensCommand response = DensCommand::parse(line);

            if (response.args().size() == 1 && response.args().at(0) == QLatin1String("NAK")) {
                qWarning() << "Invalid command:" << response.toString();
            } else if (response.args().size() == 1 && response.args().at(0) == QLatin1String("[[")) {
                multilineResponse_ = response;
                multilineBuffer_.clear();
                multilinePending_ = true;
            } else {
                if (response.isDensity()) {
                    readDensityResponse(response);
                } else if (response.isValid()) {
                    readCommandResponse(response);
                } else {
                    qWarning() << "Unrecognized line:" << line;
                }
            }
        }
    }
}

void DensInterface::handleError(QSerialPort::SerialPortError error)
{
    qDebug() << "Serial port error:" << error;
    emit connectionError();
}

bool DensInterface::isLogLine(const QByteArray &line)
{
    return line.size() > 2 && line[1] == '/'
            && (line[0] == 'A' || line[0] == 'E' || line[0] == 'W'
            || line[0] == 'I' || line[0] == 'D' || line[0] == 'V');
}

void DensInterface::readDensityResponse(const DensCommand &response)
{
    qDebug() << "Read:" << response.toString();
    if (!response.args().isEmpty() && response.args().at(0).endsWith(QLatin1Char('D'))) {
        DensityType densityType;
        float dValue;
        float dZero = qSNaN();
        float rawValue = qSNaN();
        float corrValue = qSNaN();

        if (response.type() == DensCommand::TypeDensityReflection) {
            densityType = DensityReflection;
        } else if (response.type() == DensCommand::TypeDensityTransmission) {
            densityType = DensityTransmission;
        } else if (response.type() == DensCommand::TypeDensityUvTransmission) {
            densityType = DensityUvTransmission;
        } else {
            return;
        }

        if (response.args().size() > 1) {
            dValue = util::decode_f32(response.args().at(1));
            if (response.args().size() > 2) {
                dZero = util::decode_f32(response.args().at(2));
            }
            if (response.args().size() > 3) {
                rawValue = util::decode_f32(response.args().at(3));
            }
            if (response.args().size() > 4) {
                corrValue = util::decode_f32(response.args().at(4));
            }
        } else {
            QString readingStr = response.args().at(0);
            readingStr.chop(1);
            bool ok;
            dValue = readingStr.toFloat(&ok);
            if (!ok) {
                qWarning() << "Bad reading value:" << response.args().at(0);
                return;
            }
        }

        emit densityReading(densityType, dValue, dZero, rawValue, corrValue);
    }
}

void DensInterface::readCommandResponse(const DensCommand &response)
{
    switch (response.category()) {
    case DensCommand::CategorySystem:
        readSystemResponse(response);
        break;
    case DensCommand::CategoryMeasurement:
        readMeasurementResponse(response);
        break;
    case DensCommand::CategoryCalibration:
        readCalibrationResponse(response);
        break;
    case DensCommand::CategoryDiagnostics:
        readDiagnosticsResponse(response);
        break;
    default:
        break;
    }
}

void DensInterface::readSystemResponse(const DensCommand &response)
{
    if (response.type() == DensCommand::TypeGet) {
        const QStringList args = response.args();
        if (response.action() == QLatin1String("V")) {
            if (args.length() > 0) {
                projectName_ = args.at(0);
            }
            if (args.length() > 1) {
                version_ = args.at(1);
            }
            if (!connecting_) {
                emit systemVersionResponse();
            }
        } else if (response.action() == QLatin1String("B")) {
            if (args.length() > 0) {
                buildDate_ = QDateTime::fromString(args.at(0), "yyyy-MM-dd hh:mm");
            }
            if (args.length() > 1) {
                buildDescribe_ = args.at(1);
            }
            if (args.length() > 2) {
                bool ok;
                buildChecksum_ = args.at(2).toUInt(&ok, 16);
                if (!ok) { buildChecksum_ = 0; }
            }
            emit systemBuildResponse();
        } else if (response.action() == QLatin1String("DEV")) {
            if (args.length() > 0) {
                halVersion_ = args.at(0).trimmed();
            }
            if (args.length() > 1) {
                mcuDeviceId_ = args.at(1);
            }
            if (args.length() > 2) {
                mcuRevisionId_ = args.at(2);
            }
            if (args.length() > 3) {
                mcuSysClock_ = args.at(3);
            }
            emit systemDeviceResponse();
        } else if (response.action() == QLatin1String("RTOS")) {
            if (args.length() > 0) {
                freeRtosVersion_ = args.at(0).trimmed();
            }
            if (args.length() > 1) {
                freeRtosHeapSize_ = args.at(1).toUInt();
            }
            if (args.length() > 2) {
                freeRtosHeapWatermark_ = args.at(2).toUInt();
            }
            if (args.length() > 3) {
                freeRtosTaskCount_ = args.at(3).toUInt();
            }
            emit systemRtosResponse();
        } else if (response.action() == QLatin1String("UID")) {
            if (args.length() > 0) {
                uniqueId_ = args.at(0);
            }
            emit systemUniqueId();
        } else if (response.action() == QLatin1String("ISEN")) {
            if (args.length() > 0) {
                mcuVdda_ = args.at(0);
            }
            if (args.length() > 1) {
                mcuTemp_ = args.at(1);
            }
            if (deviceType_ == DeviceUvVis && args.length() > 2) {
                sensorTemp_ = args.at(2);
            }
            emit systemInternalSensors();
        }
    } else if (response.type() == DensCommand::TypeSet) {
        if (isResponseSetOk(response, QLatin1String("DISP"))) {
            emit systemDisplaySetComplete();
        }
    } else if (response.type() == DensCommand::TypeInvoke) {
        const QStringList args = response.args();
        if (response.action() == QLatin1String("REMOTE") && args.length() > 0) {
            remoteControlEnabled_ = (args.at(0) == QLatin1String("1"));
            emit systemRemoteControl(remoteControlEnabled_);
        }
    }
}

void DensInterface::readMeasurementResponse(const DensCommand &response)
{
    if (response.type() == DensCommand::TypeSet
            && response.action() == QLatin1String("FORMAT")
            && !response.args().isEmpty()
            && response.args().at(0) == QLatin1String("OK")) {
        emit measurementFormatChanged();
    } else if (response.type() == DensCommand::TypeSet
               && response.action() == QLatin1String("UNCAL")
               && !response.args().isEmpty()
               && response.args().at(0) == QLatin1String("OK")) {
        emit allowUncalibratedMeasurementsChanged();
    }
}

void DensInterface::readCalibrationResponse(const DensCommand &response)
{
    if (response.type() == DensCommand::TypeInvoke
            && response.action() == QLatin1String("GAIN")) {
        if (response.args().size() > 0 && response.args().at(0) == QLatin1String("OK")) {
            emit calGainCalFinished();
        } else if (response.args().size() > 0 && response.args().at(0) == QLatin1String("ERR")) {
            emit calGainCalError();
        } else if (response.args().size() > 1 && response.args().at(0) == QLatin1String("STATUS")) {
            bool ok;
            int status = response.args().at(1).toInt(&ok);
            if (!ok) { status = -1; }
            int param = response.args().size() > 2 ? response.args().at(2).toInt(&ok) : -1;
            if (!ok) { param = -1; }
            emit calGainCalStatus(status, param);
        }
    } else if (response.type() == DensCommand::TypeGet
               && response.action() == QLatin1String("LIGHT")
               && response.args().length() == 2) {
        calLight_.setReflectionValue(response.args().at(0).toInt());
        calLight_.setTransmissionValue(response.args().at(1).toInt());
        emit calLightResponse();
    } else if (isResponseSetOk(response, QLatin1String("LIGHT"))) {
        emit calLightSetComplete();
    } else if (response.type() == DensCommand::TypeGet
               && response.action() == QLatin1String("GAIN")) {
        if (deviceType_ == DeviceBaseline && response.args().size() >= 8) {
            calGain_.setLow0(util::decode_f32(response.args().at(0)));
            calGain_.setLow1(util::decode_f32(response.args().at(1)));
            calGain_.setMed0(util::decode_f32(response.args().at(2)));
            calGain_.setMed1(util::decode_f32(response.args().at(3)));
            calGain_.setHigh0(util::decode_f32(response.args().at(4)));
            calGain_.setHigh1(util::decode_f32(response.args().at(5)));
            calGain_.setMax0(util::decode_f32(response.args().at(6)));
            calGain_.setMax1(util::decode_f32(response.args().at(7)));
            emit calGainResponse();
        } else if (deviceType_ == DeviceUvVis && response.args().size() >= 10) {
            calUvVisGain_.setGainValue(DensUvVisCalGain::Gain0_5X, util::decode_f32(response.args().at(0)));
            calUvVisGain_.setGainValue(DensUvVisCalGain::Gain1X, util::decode_f32(response.args().at(1)));
            calUvVisGain_.setGainValue(DensUvVisCalGain::Gain2X, util::decode_f32(response.args().at(2)));
            calUvVisGain_.setGainValue(DensUvVisCalGain::Gain4X, util::decode_f32(response.args().at(3)));
            calUvVisGain_.setGainValue(DensUvVisCalGain::Gain8X, util::decode_f32(response.args().at(4)));
            calUvVisGain_.setGainValue(DensUvVisCalGain::Gain16X, util::decode_f32(response.args().at(5)));
            calUvVisGain_.setGainValue(DensUvVisCalGain::Gain32X, util::decode_f32(response.args().at(6)));
            calUvVisGain_.setGainValue(DensUvVisCalGain::Gain64X, util::decode_f32(response.args().at(7)));
            calUvVisGain_.setGainValue(DensUvVisCalGain::Gain128X, util::decode_f32(response.args().at(8)));
            calUvVisGain_.setGainValue(DensUvVisCalGain::Gain256X, util::decode_f32(response.args().at(9)));
            emit calGainResponse();
        } else {
            qDebug() << response.toString();
        }
    } else if (isResponseSetOk(response, QLatin1String("GAIN"))) {
        emit calGainSetComplete();
    } else if (response.type() == DensCommand::TypeGet
             && response.action() == QLatin1String("SLOPE")
             && response.args().length() >= 3) {
        calSlope_.setB0(util::decode_f32(response.args().at(0)));
        calSlope_.setB1(util::decode_f32(response.args().at(1)));
        calSlope_.setB2(util::decode_f32(response.args().at(2)));
        emit calSlopeResponse();
    } else if (isResponseSetOk(response, QLatin1String("SLOPE"))) {
        emit calSlopeSetComplete();
    } else if (response.type() == DensCommand::TypeGet
               && response.action() == QLatin1String("VTEMP")
               && response.args().length() >= 3) {
        calVisTemperature_.setB0(util::decode_f32(response.args().at(0)));
        calVisTemperature_.setB1(util::decode_f32(response.args().at(1)));
        calVisTemperature_.setB2(util::decode_f32(response.args().at(2)));
        emit calVisTemperatureResponse();
    } else if (isResponseSetOk(response, QLatin1String("VTEMP"))) {
        emit calVisTemperatureSetComplete();
    } else if (response.type() == DensCommand::TypeGet
               && response.action() == QLatin1String("UTEMP")
               && response.args().length() >= 3) {
        calUvTemperature_.setB0(util::decode_f32(response.args().at(0)));
        calUvTemperature_.setB1(util::decode_f32(response.args().at(1)));
        calUvTemperature_.setB2(util::decode_f32(response.args().at(2)));
        emit calUvTemperatureResponse();
    } else if (isResponseSetOk(response, QLatin1String("UTEMP"))) {
        emit calUvTemperatureSetComplete();
    } else if (response.type() == DensCommand::TypeGet
               && response.action() == QLatin1String("REFL")
               && response.args().length() == 4) {
        calReflection_.setLoDensity(util::decode_f32(response.args().at(0)));
        calReflection_.setLoReading(util::decode_f32(response.args().at(1)));
        calReflection_.setHiDensity(util::decode_f32(response.args().at(2)));
        calReflection_.setHiReading(util::decode_f32(response.args().at(3)));
        emit calReflectionResponse();
    } else if (isResponseSetOk(response, QLatin1String("REFL"))) {
        emit calReflectionSetComplete();
    } else if (response.type() == DensCommand::TypeGet
               && response.action() == QLatin1String("TRAN")
               && response.args().length() == 4) {
        calTransmission_.setLoDensity(util::decode_f32(response.args().at(0)));
        calTransmission_.setLoReading(util::decode_f32(response.args().at(1)));
        calTransmission_.setHiDensity(util::decode_f32(response.args().at(2)));
        calTransmission_.setHiReading(util::decode_f32(response.args().at(3)));
        emit calTransmissionResponse();
    } else if (isResponseSetOk(response, QLatin1String("TRAN"))) {
        emit calTransmissionSetComplete();
    } else if (response.type() == DensCommand::TypeGet
               && response.action() == QLatin1String("UVTR")
               && response.args().length() == 4) {
        calUvTransmission_.setLoDensity(util::decode_f32(response.args().at(0)));
        calUvTransmission_.setLoReading(util::decode_f32(response.args().at(1)));
        calUvTransmission_.setHiDensity(util::decode_f32(response.args().at(2)));
        calUvTransmission_.setHiReading(util::decode_f32(response.args().at(3)));
        emit calUvTransmissionResponse();
    } else if (isResponseSetOk(response, QLatin1String("UVTR"))) {
        emit calUvTransmissionSetComplete();
    }
}

bool DensInterface::isResponseSetOk(const DensCommand &response, QLatin1String action)
{
    if (response.type() == DensCommand::TypeSet
            && response.action() == action
            && response.args().length() == 1
            && response.args().at(0) == QLatin1String("OK")) {
        return true;
    } else {
        return false;
    }
}

void DensInterface::readDiagnosticsResponse(const DensCommand &response)
{
    if (response.type() == DensCommand::TypeGet
            && response.action() == QLatin1String("DISP")
            && !response.buffer().isEmpty()) {
        emit diagDisplayScreenshot(response.buffer());
    } else if (response.type() == DensCommand::TypeGet
               && response.action() == QLatin1String("LMAX")
               && deviceType_ == DeviceUvVis
               && response.args().size() >= 1) {
        diagLightMax_ = response.args().at(0).toUInt();
        emit diagLightMaxChanged();
    } else if (response.type() == DensCommand::TypeSet
               && response.action() == QLatin1String("LR")
               && response.args().size() == 1
               && response.args().at(0) == QLatin1String("OK")) {
        emit diagLightReflChanged();
    } else if (response.type() == DensCommand::TypeSet
               && response.action() == QLatin1String("LT")
               && response.args().size() == 1
               && response.args().at(0) == QLatin1String("OK")) {
        emit diagLightTranChanged();
    } else if (response.type() == DensCommand::TypeSet
             && response.action() == QLatin1String("LTU")
             && response.args().size() == 1
             && response.args().at(0) == QLatin1String("OK")) {
        emit diagLightTranUvChanged();
    } else if (response.type() == DensCommand::TypeInvoke
               && response.action() == QLatin1String("S")
               && response.args().size() == 1
               && response.args().at(0) == QLatin1String("OK")) {
        emit diagSensorInvoked();
    } else if (response.type() == DensCommand::TypeSet
               && response.action() == QLatin1String("S")
               && response.args().size() == 1
               && response.args().at(0) == QLatin1String("OK")) {
        emit diagSensorChanged();
    } else if (response.type() == DensCommand::TypeGet
               && response.action() == QLatin1String("S")) {
        if (deviceType_ == DeviceBaseline && response.args().size() >= 2) {
            emit diagSensorBaselineGetReading(
                response.args().at(0).toInt(),
                response.args().at(1).toInt());
        } else if (deviceType_ == DeviceUvVis && response.args().size() >= 4) {
            emit diagSensorUvGetReading(
                response.args().at(0).toUInt(),
                response.args().at(1).toInt(),
                response.args().at(2).toInt(),
                response.args().at(3).toInt());
        } else {
            qDebug() << response.toString();
        }
    } else if (response.type() == DensCommand::TypeInvoke
               && response.action() == QLatin1String("READ")) {
        if (response.isError()) {
            emit diagSensorInvokeReadingError();
        } else if (deviceType_ == DeviceBaseline && response.args().size() >= 2) {
            emit diagSensorBaselineInvokeReading(
                response.args().at(0).toInt(),
                response.args().at(1).toInt());
        } else if (deviceType_ == DeviceUvVis && response.args().size() >= 1) {
            emit diagSensorUvInvokeReading(
                response.args().at(0).toUInt());
        } else {
            qDebug() << response.toString();
        }
    } else if (response.type() == DensCommand::TypeInvoke
               && response.action() == QLatin1String("MEAS")) {
        if (response.isError()) {
            emit diagSensorInvokeMeasurementError();
        } else if (deviceType_ == DeviceUvVis && response.args().size() >= 1) {
            emit diagSensorUvInvokeMeasurement(
                util::decode_f32(response.args().at(0)));
        } else {
            qDebug() << response.toString();
        }
    } else if (response.type() == DensCommand::TypeGet
            && response.action() == QLatin1String("LOG")
            && response.args().size() == 1
            && response.args().at(0) == QLatin1String("OK")) {
        qDebug() << "Logging mode changed";
    } else {
        qDebug() << response.toString();
    }
}

bool DensInterface::sendCommand(const DensCommand &command)
{
    if (!serialPort_ || !serialPort_->isOpen() || !command.isValid()) {
        return false;
    }

    QByteArray commandBytes = command.toString().toLatin1();
    commandBytes.append("\r\n");
    return serialPort_->write(commandBytes) != -1;
}
