#include "ft260deviceinfo.h"
#include "ft260deviceinfo_p.h"

Ft260DeviceInfo::Ft260DeviceInfo()
{
}

Ft260DeviceInfo::Ft260DeviceInfo(const Ft260DeviceInfo &other)
    : d_ptr(other.d_ptr ? new Ft260DeviceInfoPrivate(*other.d_ptr) : nullptr)
{
}

Ft260DeviceInfo::Ft260DeviceInfo(const Ft260DeviceInfoPrivate &dd)
    : d_ptr(new Ft260DeviceInfoPrivate(dd))
{
}

Ft260DeviceInfo::~Ft260DeviceInfo()
{
}

void Ft260DeviceInfo::swap(Ft260DeviceInfo &other)
{
    d_ptr.swap(other.d_ptr);
}

Ft260DeviceInfo& Ft260DeviceInfo::operator=(const Ft260DeviceInfo &other)
{
    Ft260DeviceInfo(other).swap(*this);
    return *this;
}

QString Ft260DeviceInfo::devicePath() const
{
    Q_D(const Ft260DeviceInfo);
    return !d ? QString() : d->devicePath;
}

QString Ft260DeviceInfo::deviceDisplayPath() const
{
    Q_D(const Ft260DeviceInfo);
    if (!d) { return QString(); }
    if (!d->deviceDisplayPath.isEmpty()) {
        return d->deviceDisplayPath;
    } else {
        return d->devicePath;
    }
}

Ft260DeviceInfo::Driver Ft260DeviceInfo::deviceDriver() const
{
    Q_D(const Ft260DeviceInfo);
    return !d ? Ft260DeviceInfo::DriverUnknown : d->deviceDriver;
}

QString Ft260DeviceInfo::description() const
{
    Q_D(const Ft260DeviceInfo);
    return !d ? QString() : d->description;
}

QString Ft260DeviceInfo::manufacturer() const
{
    Q_D(const Ft260DeviceInfo);
    return !d ? QString() : d->manufacturer;
}

QString Ft260DeviceInfo::product() const
{
    Q_D(const Ft260DeviceInfo);
    return !d ? QString() : d->product;
}

QString Ft260DeviceInfo::serialNumber() const
{
    Q_D(const Ft260DeviceInfo);
    return !d ? QString() : d->serialNumber;
}

quint16 Ft260DeviceInfo::vendorId() const
{
    Q_D(const Ft260DeviceInfo);
    return !d ? 0 : d->vendorId;
}

quint16 Ft260DeviceInfo::productId() const
{
    Q_D(const Ft260DeviceInfo);
    return !d ? 0 : d->productId;
}

quint16 Ft260DeviceInfo::interfaceNumber() const
{
    Q_D(const Ft260DeviceInfo);
    return !d ? 0 : d->interfaceNumber;
}
