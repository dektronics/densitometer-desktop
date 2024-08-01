#ifndef STICKINTERFACE_H
#define STICKINTERFACE_H

#include <QObject>
#include "ft260.h"

class StickInterface : public QObject
{
    Q_OBJECT
public:
    explicit StickInterface(Ft260 *ft260, QObject *parent = nullptr);
    ~StickInterface();

    bool open();
    void close();

private:
    bool valid_ = false;
    Ft260 *ft260_ = nullptr;
};

#endif // STICKINTERFACE_H
