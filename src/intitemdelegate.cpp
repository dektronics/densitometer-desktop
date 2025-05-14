#include "intitemdelegate.h"

#include <QLineEdit>
#include <QDebug>

IntItemDelegate::IntItemDelegate(QObject *parent)
    : IntItemDelegate(std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), parent)
{
}

IntItemDelegate::IntItemDelegate(int min, int max, QObject *parent) :
    QStyledItemDelegate(parent),
    min_(min), max_(max)
{
}

int IntItemDelegate::min() const { return min_; }
int IntItemDelegate::max() const { return max_; }

QWidget *IntItemDelegate::createEditor(QWidget *parent,
                                         const QStyleOptionViewItem &option,
                                         const QModelIndex &index) const
{
    QWidget *widget = QStyledItemDelegate::createEditor(parent, option, index);
    QLineEdit *lineEdit = dynamic_cast<QLineEdit *>(widget);
    if (lineEdit) {
        QIntValidator *validator = new QIntValidator(min_, max_);
        lineEdit->setValidator(validator);
    }
    return widget;
}
