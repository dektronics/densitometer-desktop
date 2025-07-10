#ifndef M24C08_H
#define M24C08_H

#include <QtTypes>
#include <QByteArray>

#define M24C08_PAGE_SIZE  0x10UL
#define M24C08_MEM_SIZE   0x400UL

class Ft260;

class M24C08
{
public:
    explicit M24C08(Ft260 *ft260, quint8 deviceAddress);

    bool valid() const;
    size_t size() const;

    QByteArray readBuffer(uint16_t address, size_t len);
    bool writeBuffer(uint16_t address, const QByteArray &data);

private:
    Ft260 *ft260_ = nullptr;
    quint8 deviceAddress_ = 0;
    bool valid_ = false;
};

#endif // M24C08_H
