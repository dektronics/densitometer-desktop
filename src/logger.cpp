#include "logger.h"

#include <QScrollBar>
#include <QFont>

Logger::Logger(QWidget *parent)
    : QPlainTextEdit(parent)
    , autoScroll_(true)
{
    document()->setMaximumBlockCount(5000);

    const QFont fixedFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    setFont(fixedFont);

    QPalette p = palette();
    p.setColor(QPalette::Base, Qt::black);
    p.setColor(QPalette::Text, Qt::green);
    setPalette(p);
    setReadOnly(true);
}

void Logger::putData(const QByteArray &data)
{
    insertPlainText(data);

    if (autoScroll_) {
        QScrollBar *bar = verticalScrollBar();
        bar->setValue(bar->maximum());
    }
}

void Logger::setAutoScroll(bool enabled)
{
    if (enabled && !autoScroll_) {
        QScrollBar *bar = verticalScrollBar();
        bar->setValue(bar->maximum());
    }
    autoScroll_ = enabled;
}
