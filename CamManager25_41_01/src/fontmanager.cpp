#include "fontmanager.h"
#include <QApplication>
#include <QDebug>
#include <QResource>
#include <QFile>

FontManager& FontManager::instance()
{
    static FontManager instance;
    return instance;
}

FontManager::FontManager()
{
}

bool FontManager::loadApplicationFonts()
{
    // Use the system font directly without loading custom font files
    //qDebug() << "Using system fonts directly";
    return true;
}

void FontManager::setApplicationFont()
{
    QFontDatabase fontDb;
    
    QStringList preferredFonts = {
        "DengXian",             // 等线字体
        "NSimSun",              //新宋体
        "YouYuan",              //幼圆
        "SimHei",               // 黑体
        "Microsoft YaHei UI",   // 微软雅黑 UI
        "Microsoft YaHei"       // 微软雅黑
    };
        
    QFont appFont;
    bool fontSet = false;
    
    // 按照优先级顺序选择字体
    for (const QString &fontFamily : preferredFonts) {
        if (fontDb.hasFamily(fontFamily)) {
            appFont.setFamily(fontFamily);
            appFont.setPointSize(10);
            fontSet = true;
            break;
        }
    }
    
    if (!fontSet) {
        appFont = QApplication::font();
        appFont.setPointSize(10);
        qDebug() << "Using system default font:" << appFont.family();
    }
    
    m_currentFont = appFont;
    QApplication::setFont(appFont);
    
    // Verify the final font used
    //qDebug() << "最终使用的字体:" << appFont.family();
    //qDebug() << "字体大小:" << appFont.pointSize();
}

QFont FontManager::getCurrentFont() const
{
    return m_currentFont;
}

QStringList FontManager::getAvailableFonts() const
{
    return m_loadedFonts;
}
