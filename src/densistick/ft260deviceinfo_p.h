#ifndef FT260DEVICEINFO_P_H
#define FT260DEVICEINFO_P_H

#include <QtTypes>
#include <QString>

#include "ft260deviceinfo.h"

class Ft260DeviceInfoPrivate
{
public:
    QString devicePath;
    QString deviceDisplayPath;
    Ft260DeviceInfo::Driver deviceDriver = Ft260DeviceInfo::DriverUnknown;

    QString description;
    QString manufacturer;
    QString product;
    QString serialNumber;

    quint16 vendorId = 0;
    quint16 productId = 0;
    quint16 interfaceNumber = 0;
};

#endif // FT260DEVICEINFO_P_H
