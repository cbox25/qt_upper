#ifndef FONTMANAGER_H
#define FONTMANAGER_H

#include <QObject>
#include <QFont>
#include <QFontDatabase>
#include <QStringList>

class FontManager : public QObject
{
    Q_OBJECT

public:
    static FontManager& instance();
    
    // Load the application font
    bool loadApplicationFonts();
    
    // Set the font of the application
    void setApplicationFont();
    
    // Get the font currently in use
    QFont getCurrentFont() const;
    
    // Get the list of available fonts
    QStringList getAvailableFonts() const;
    QStringList getAvailableChineseFonts() const;  // New addition: Detect available Chinese fonts

private:
    FontManager();
    ~FontManager() = default;
    FontManager(const FontManager&) = delete;
    FontManager& operator=(const FontManager&) = delete;
    
    QStringList m_loadedFonts;
    QFont m_currentFont;
};

#endif // FONTMANAGER_H 
