#include "diagnosticstab.h"
#include "ui_diagnosticstab.h"

#include <QFileDialog>
#include <QDebug>

#include "densinterface.h"
#include "densistick/densistickrunner.h"
#include "densistick/densistickinterface.h"
#include "remotecontroldialog.h"
#include "stickremotecontroldialog.h"

DiagnosticsTab::DiagnosticsTab(DensInterface *densInterface, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::DiagnosticsTab)
    , densInterface_(densInterface)
{
    ui->setupUi(this);

    ui->refreshSensorsPushButton->setEnabled(false);
    ui->screenshotButton->setEnabled(false);

    // Diagnostics UI signals
    connect(ui->refreshSensorsPushButton, &QPushButton::clicked, densInterface_, &DensInterface::sendGetSystemInternalSensors);
    connect(ui->screenshotButton, &QPushButton::clicked, densInterface_, &DensInterface::sendGetDiagDisplayScreenshot);
    connect(ui->remotePushButton, &QPushButton::clicked, this, &DiagnosticsTab::onRemoteControl);

    // Densitometer interface update signals
    connect(densInterface_, &DensInterface::connectionOpened, this, &DiagnosticsTab::onConnectionOpened);
    connect(densInterface_, &DensInterface::connectionClosed, this, &DiagnosticsTab::onConnectionClosed);
    connect(densInterface_, &DensInterface::systemVersionResponse, this, &DiagnosticsTab::onSystemVersionResponse);
    connect(densInterface_, &DensInterface::systemBuildResponse, this, &DiagnosticsTab::onSystemBuildResponse);
    connect(densInterface_, &DensInterface::systemDeviceResponse, this, &DiagnosticsTab::onSystemDeviceResponse);
    connect(densInterface_, &DensInterface::systemUniqueId, this, &DiagnosticsTab::onSystemUniqueId);
    connect(densInterface_, &DensInterface::systemInternalSensors, this, &DiagnosticsTab::onSystemInternalSensors);
    connect(densInterface_, &DensInterface::diagDisplayScreenshot, this, &DiagnosticsTab::onDiagDisplayScreenshot);

    configureForDeviceType();

    // Initialize all fields with blank values
    onSystemVersionResponse();
    onSystemBuildResponse();
    onSystemDeviceResponse();
    onSystemUniqueId();
    onSystemInternalSensors();

    refreshButtonState();
}

DiagnosticsTab::~DiagnosticsTab()
{
    delete ui;
}

void DiagnosticsTab::setStickRunner(DensiStickRunner *stickRunner)
{
    if (stickRunner_ != stickRunner) {
        stickRunner_ = stickRunner;
        configureForDeviceType();
        refreshButtonState();
    }
}

bool DiagnosticsTab::isRemoteOpen() const
{
    return remoteDialog_ != nullptr;
}

void DiagnosticsTab::onConnectionOpened()
{
    configureForDeviceType();

    refreshButtonState();
}

void DiagnosticsTab::onConnectionClosed()
{
    refreshButtonState();

    if (remoteDialog_) {
        remoteDialog_->close();
    }
}

void DiagnosticsTab::onSystemVersionResponse()
{
    if (stickRunner_) {
        ui->nameLabel->setText("<b>Printalyzer DensiStick</b>");
        ui->versionLineEdit->clear();
        ui->halVersionLineEdit->setText(stickRunner_->stickInterface()->version());
        ui->mcuSysClockLineEdit->setText(stickRunner_->stickInterface()->systemClock());
        ui->uniqueIdLineEdit->setText(stickRunner_->stickInterface()->serialNumber());
    } else {
        if (densInterface_->projectName().isEmpty()) {
            ui->nameLabel->setText("Printalyzer Densitometer");
        } else {
            ui->nameLabel->setText(QString("<b>%1</b>").arg(densInterface_->projectName()));
        }
        ui->versionLineEdit->setText(densInterface_->version());
    }
}

void DiagnosticsTab::onSystemBuildResponse()
{
    ui->buildDateLineEdit->setText(densInterface_->buildDate().toString("yyyy-MM-dd hh:mm"));
    ui->buildCommitLineEdit->setText(densInterface_->buildDescribe());
    if (densInterface_->buildChecksum() == 0) {
        ui->buildChecksumLineEdit->clear();
    } else {
        ui->buildChecksumLineEdit->setText(QString("%1").arg(densInterface_->buildChecksum(), 0, 16));
    }
}

void DiagnosticsTab::onSystemDeviceResponse()
{
    ui->halVersionLineEdit->setText(densInterface_->halVersion());
    ui->mcuDevIdLineEdit->setText(densInterface_->mcuDeviceId());
    ui->mcuRevIdLineEdit->setText(densInterface_->mcuRevisionId());
    ui->mcuSysClockLineEdit->setText(densInterface_->mcuSysClock());
}

void DiagnosticsTab::onSystemUniqueId()
{
    ui->uniqueIdLineEdit->setText(densInterface_->uniqueId());
}

void DiagnosticsTab::onSystemInternalSensors()
{
    ui->mcuVddaLineEdit->setText(densInterface_->mcuVdda());
    ui->mcuTempLineEdit->setText(formatDisplayTemp(densInterface_->mcuTemp()));

    if (densInterface_->deviceType() == DensInterface::DeviceUvVis) {
        ui->sensorTempLineEdit->setText(formatDisplayTemp(densInterface_->sensorTemp()));
    } else {
        ui->sensorTempLineEdit->clear();
    }
}

void DiagnosticsTab::onDiagDisplayScreenshot(const QByteArray &data)
{
    qDebug() << "Got screenshot:" << data.size();
    QImage image = QImage::fromData(data, "XBM");
    if (!image.isNull()) {
        image = image.mirrored(true, true);
        image.invertPixels();

        QString fileName = QFileDialog::getSaveFileName(this, tr("Save Screenshot"),
                                                        "screenshot.png",
                                                        tr("Images (*.png *.jpg)"));
        if (!fileName.isEmpty()) {
            if (image.save(fileName)) {
                qDebug() << "Saved screenshot to:" << fileName;
            } else {
                qDebug() << "Error saving screenshot to:" << fileName;
            }
        }
    }
}

void DiagnosticsTab::onRemoteControl()
{
    if (!densInterface_->connected() && (!stickRunner_ || !stickRunner_->stickInterface()->connected())) {
        return;
    }

    if (remoteDialog_) {
        remoteDialog_->setFocus();
        return;
    }

    if (stickRunner_) {
        stickRunner_->setEnabled(false);
        remoteDialog_ = new StickRemoteControlDialog(stickRunner_->stickInterface(), this);
    } else {
        remoteDialog_ = new RemoteControlDialog(densInterface_, this);
    }
    connect(remoteDialog_, &QDialog::finished, this, &DiagnosticsTab::onRemoteControlFinished);
    remoteDialog_->show();
}

void DiagnosticsTab::onRemoteControlFinished()
{
    if (stickRunner_) {
        stickRunner_->setEnabled(true);
    }
    remoteDialog_->deleteLater();
    remoteDialog_ = nullptr;
}

void DiagnosticsTab::configureForDeviceType()
{
    // Clear all text fields
    ui->nameLabel->setText("Printalyzer Densitometer");
    ui->versionLineEdit->clear();
    ui->buildDateLineEdit->clear();
    ui->buildCommitLineEdit->clear();
    ui->buildChecksumLineEdit->clear();
    ui->halVersionLineEdit->clear();
    ui->mcuDevIdLineEdit->clear();
    ui->mcuRevIdLineEdit->clear();
    ui->mcuSysClockLineEdit->clear();
    ui->uniqueIdLineEdit->clear();
    ui->mcuVddaLineEdit->clear();
    ui->mcuTempLineEdit->clear();
    ui->sensorTempLineEdit->clear();

    if (stickRunner_) {
        ui->refreshSensorsPushButton->setVisible(false);
        ui->screenshotButton->setVisible(false);
        ui->sensorTempLabel->setVisible(false);
        ui->sensorTempLineEdit->setVisible(false);

        ui->halVersionLabel->setText(tr("Chip Version:"));
        ui->mcuSysClockLabel->setText(tr("System Clock:"));
        ui->uniqueIdLabel->setText(tr("Serial Number:"));

        ui->versionLabel->setVisible(false);
        ui->versionLineEdit->setVisible(false);
        ui->buildDateLabel->setVisible(false);
        ui->buildDateLineEdit->setVisible(false);
        ui->buildCommitLabel->setVisible(false);
        ui->buildCommitLineEdit->setVisible(false);
        ui->buildChecksumLabel->setVisible(false);
        ui->buildChecksumLineEdit->setVisible(false);

        ui->mcuDevIdLabel->setVisible(false);
        ui->mcuDevIdLineEdit->setVisible(false);
        ui->mcuRevIdLabel->setVisible(false);
        ui->mcuRevIdLineEdit->setVisible(false);
        ui->mcuVddaLabel->setVisible(false);
        ui->mcuVddaLineEdit->setVisible(false);
        ui->mcuTempLabel->setVisible(false);
        ui->mcuTempLineEdit->setVisible(false);

        onSystemVersionResponse();
    } else {
        ui->refreshSensorsPushButton->setVisible(true);
        ui->screenshotButton->setVisible(true);

        DensInterface::DeviceType deviceType;
        if (densInterface_->connected()) {
            deviceType = densInterface_->deviceType();
            lastDeviceType_ = deviceType;
        } else {
            deviceType = lastDeviceType_;
        }

        if (deviceType == DensInterface::DeviceUvVis) {
            ui->sensorTempLabel->setVisible(true);
            ui->sensorTempLineEdit->setVisible(true);
        } else {
            ui->sensorTempLabel->setVisible(false);
            ui->sensorTempLineEdit->setVisible(false);
        }

        ui->halVersionLabel->setText(tr("HAL Version:"));
        ui->mcuSysClockLabel->setText(tr("MCU SysClock:"));
        ui->uniqueIdLabel->setText(tr("Unique ID:"));

        ui->versionLabel->setVisible(true);
        ui->versionLineEdit->setVisible(true);
        ui->buildDateLabel->setVisible(true);
        ui->buildDateLineEdit->setVisible(true);
        ui->buildCommitLabel->setVisible(true);
        ui->buildCommitLineEdit->setVisible(true);
        ui->buildChecksumLabel->setVisible(true);
        ui->buildChecksumLineEdit->setVisible(true);

        ui->mcuDevIdLabel->setVisible(true);
        ui->mcuDevIdLineEdit->setVisible(true);
        ui->mcuRevIdLabel->setVisible(true);
        ui->mcuRevIdLineEdit->setVisible(true);
        ui->mcuVddaLabel->setVisible(true);
        ui->mcuVddaLineEdit->setVisible(true);
        ui->mcuTempLabel->setVisible(true);
        ui->mcuTempLineEdit->setVisible(true);
    }
}

void DiagnosticsTab::refreshButtonState()
{
    bool connected;
    if (stickRunner_) {
        connected = stickRunner_->stickInterface()->connected();
    } else {
        connected = densInterface_->connected();
    }

    if (connected) {
        ui->refreshSensorsPushButton->setEnabled(true);
        ui->screenshotButton->setEnabled(true);
        ui->remotePushButton->setEnabled(true);

    } else {
        ui->refreshSensorsPushButton->setEnabled(false);
        ui->screenshotButton->setEnabled(false);
        ui->remotePushButton->setEnabled(false);
    }
}

QString DiagnosticsTab::formatDisplayTemp(const QString &tempStr)
{
    bool ok;
    QString text = tempStr;
    if (text.isEmpty()) {
        return text;
    }
    if (text.endsWith(QChar('C'))) {
        text.chop(1);
    }
    float temp = text.toFloat(&ok);
    if (ok) {
        text = QString::number(temp, 'f', 1) + "\u00B0C";
    } else {
        text.clear();
    }
    return text;
}
