#include "connectdialog.h"
#include "ui_connectdialog.h"

#include <QSerialPortInfo>
#include <QPushButton>

#include "densinterface.h"
#include "densistick/ft260.h"

static const char blankString[] = QT_TRANSLATE_NOOP("ConnectDialog", "N/A");

ConnectDialog::ConnectDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConnectDialog)
{
    ui->setupUi(this);

    connect(ui->serialPortInfoListBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ConnectDialog::showPortInfo);

    fillPortsInfo();
}

ConnectDialog::~ConnectDialog()
{
    delete ui;
}

QVariant ConnectDialog::portInfo() const
{
    return portInfo_;
}

void ConnectDialog::showPortInfo(int idx)
{
    if (idx == -1) {
        return;
    }

    const QVariant dataVariant = ui->serialPortInfoListBox->itemData(idx);
    if (dataVariant.canConvert<QSerialPortInfo>()) {
        const QSerialPortInfo info = dataVariant.value<QSerialPortInfo>();
        ui->descriptionLabel->setText(tr("Description: %1").arg(info.description()));
        ui->manufacturerLabel->setText(tr("Manufacturer: %1").arg(info.manufacturer()));
        ui->serialNumberLabel->setText(tr("Serial number: %1").arg(info.serialNumber()));
        ui->locationLabel->setText(tr("Location: %1").arg(info.systemLocation()));
        ui->vidLabel->setText(tr("Vendor Identifier: %1").arg((info.vendorIdentifier() ? QString::number(info.vendorIdentifier(), 16) : blankString)));
        ui->pidLabel->setText(tr("Product Identifier: %1").arg((info.productIdentifier() ? QString::number(info.productIdentifier(), 16) : blankString)));

    } else if (dataVariant.canConvert<Ft260DeviceInfo>()) {
        const Ft260DeviceInfo info = dataVariant.value<Ft260DeviceInfo>();
        ui->descriptionLabel->setText(tr("Description: %1").arg(info.description()));
        ui->descriptionLabel->setText(tr("Description: %1").arg(info.description()));
        ui->manufacturerLabel->setText(tr("Manufacturer: %1").arg(info.manufacturer()));
        ui->serialNumberLabel->setText(tr("Serial number: %1").arg(info.serialNumber()));
        ui->locationLabel->setText(tr("Location: %1").arg(blankString));
        ui->vidLabel->setText(tr("Vendor Identifier: %1").arg((info.vendorId() ? QString::number(info.vendorId(), 16) : blankString)));
        ui->pidLabel->setText(tr("Product Identifier: %1").arg((info.productId() ? QString::number(info.productId(), 16) : blankString)));
    }
}

void ConnectDialog::accept()
{
    portInfo_ = ui->serialPortInfoListBox->currentData();
    QDialog::accept();
}

void ConnectDialog::fillPortsInfo()
{
    ui->serialPortInfoListBox->clear();

    const auto serInfos = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : serInfos) {
        // Filter the list to only contain entries that match the VID/PID
        // values assigned to Printalyzer Densitometer devices
        if (DensInterface::portDeviceType(info) != DensInterface::DeviceUnknown) {
            const QString displayName = info.portName();
            ui->serialPortInfoListBox->addItem(displayName, QVariant::fromValue(info));
        }
    }

    const auto ftInfos = Ft260::listDevices();
    for (const Ft260DeviceInfo &info : ftInfos) {
        const QString displayName = info.deviceDisplayPath();
        ui->serialPortInfoListBox->addItem(displayName, QVariant::fromValue(info));
    }

    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(ui->serialPortInfoListBox->count() > 0);
}
