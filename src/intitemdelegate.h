#ifndef INTITEMDELEGATE_H
#define INTITEMDELEGATE_H

#include <QStyledItemDelegate>

class IntItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit IntItemDelegate(QObject *parent = nullptr);
    explicit IntItemDelegate(int min, int max, QObject *parent = nullptr);

    int min() const;
    int max() const;

    QWidget *createEditor(QWidget *parent,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const override;
private:
    int min_;
    int max_;
};

#endif // INTITEMDELEGATE_H
