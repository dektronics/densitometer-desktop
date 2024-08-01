#include "stickinterface.h"

#include <QDebug>
#include "ft260.h"

StickInterface::StickInterface(Ft260 *ft260, QObject *parent)
    : QObject(parent), ft260_(ft260)
{
    if (ft260_ && !ft260_->parent()) {
        ft260_->setParent(this);
    }
}

StickInterface::~StickInterface()
{
    close();
}

bool StickInterface::open()
{
    if (!ft260_) { return false; }
    if (valid_) {
        qWarning() << "Device already open";
        return false;
    }

    if (!ft260_->open()) {
        qWarning() << "Unable to open device";
        close();
        return false;
    }

    //TODO

    valid_ = true;

    return true;
}

void StickInterface::close()
{
    if (ft260_) {
        ft260_->close();
    }

    valid_ = false;
}
