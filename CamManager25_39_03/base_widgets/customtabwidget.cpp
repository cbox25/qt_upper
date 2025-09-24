#include "customtabwidget.h"
 
CustomTabWidget::CustomTabWidget(QWidget *parent)
    : QTabWidget(parent)
{
    setTabBar(new CustomTabBar(this));
} 