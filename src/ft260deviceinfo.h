#ifndef FT260DEVICEINFO_H
#define FT260DEVICEINFO_H

#include <QtGlobal>
#include <QScopedPointer>
#include <QList>
#include <memory>

class Ft260DeviceInfoPrivate;

class Ft260DeviceInfo
{
    Q_DECLARE_PRIVATE(Ft260DeviceInfo)
public:
    enum Driver {
        DriverUnknown = 0,
        DriverLibUsb,
    };

    Ft260DeviceInfo();
    Ft260DeviceInfo(const Ft260DeviceInfo &other);
    ~Ft260DeviceInfo();

    Ft260DeviceInfo& operator=(const Ft260DeviceInfo &other);
    void swap(Ft260DeviceInfo &other);

    QString devicePath() const;
    QString deviceDisplayPath() const;
    Driver deviceDriver() const;

    QString description() const;
    QString manufacturer() const;
    QString product() const;
    QString serialNumber() const;

    quint16 vendorId() const;
    quint16 productId() const;
    quint16 interfaceNumber() const;

    bool isNull() const;

private:
    Ft260DeviceInfo(const Ft260DeviceInfoPrivate &dd);
    friend QList<Ft260DeviceInfo> listDevicesByLibUsb();
    std::unique_ptr<Ft260DeviceInfoPrivate> d_ptr;
};

inline bool Ft260DeviceInfo::isNull() const
{
    return !d_ptr;
}


#endif // FT260DEVICEINFO_H
