#include "customtabbar.h"
#include <QStylePainter>
#include <QStyleOptionTab>

CustomTabBar::CustomTabBar(QWidget *parent)
    : QTabBar(parent)
{
}

void CustomTabBar::paintEvent(QPaintEvent *event)
{
    QStylePainter painter(this);
    for (int i = 0; i < count(); ++i) {
        QStyleOptionTab opt;
        initStyleOption(&opt, i);
        // 强制文字水平
        opt.shape = QTabBar::RoundedNorth;
        painter.drawControl(QStyle::CE_TabBarTab, opt);
    }
} 