#include "ft260.h"

#include <QtGlobal>

#include "ft260deviceinfo.h"

#ifdef HAS_LIBUSB
#include "ft260libusb.h"
#endif
#include "ft260hidapi.h"

Ft260::Ft260(Ft260DeviceInfo deviceInfo, QObject *parent) : QObject(parent), deviceInfo_(deviceInfo)
{
}

Ft260::~Ft260()
{
}

Ft260 *Ft260::createDriver(const Ft260DeviceInfo &device)
{
    Ft260 *driver = nullptr;
    if (device.deviceDriver() == Ft260DeviceInfo::DriverHidApi) {
        driver = new Ft260HidApi(device);
    }
#ifdef HAS_LIBUSB
    else if (device.deviceDriver() == Ft260DeviceInfo::DriverLibUsb) {
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
    const QList<Ft260DeviceInfo> listHidApi = Ft260HidApi::listDevices();
    list.append(listHidApi);
    return list;
}

bool Ft260::isMatchingDevice(quint16 vid, quint16 pid)
{
#if 0
    // Default FT260 device ID
    if (vid == 0x403 && pid == 0x6030) {
        return true;
    }
#endif
    // Allocated DensiStick device ID
    if (vid == 0x16D0 && pid == 0x1382) {
        return true;
    }

    return false;
}

Ft260DeviceInfo Ft260::deviceInfo() const
{
    return deviceInfo_;
}
