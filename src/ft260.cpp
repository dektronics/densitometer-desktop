#include "ft260.h"

#include <QtGlobal>

#include "ft260deviceinfo.h"

#ifdef HAS_LIBUSB
#include "ft260libusb.h"
#endif

Ft260::Ft260(QObject *parent) : QObject(parent)
{
}

Ft260::~Ft260()
{
}

Ft260 *Ft260::createDriver(const Ft260DeviceInfo &device)
{
    Ft260 *driver = nullptr;
#ifdef HAS_LIBUSB
    if (device.deviceDriver() == Ft260DeviceInfo::DriverLibUsb) {
        driver = new Ft260LibUsb(device);
    }
#endif
    return driver;
}

QList<Ft260DeviceInfo> Ft260::listDevices()
{
    QList<Ft260DeviceInfo> list;
#ifdef HAS_LIBUSB
    const QList<Ft260DeviceInfo> listLibUsb = Ft260LibUsb::listDevices();
    list.append(listLibUsb);
#endif
    return list;
}

bool Ft260::isMatchingDevice(quint16 vid, quint16 pid)
{
    // Default FT260 device ID
    if (vid == 0x403 && pid == 0x6030) {
        return true;
    }
    // Allocated Meter Probe device ID
    else if (vid == 0x16D0 && pid == 0x132C) {
        return true;
    }

    return false;
}
