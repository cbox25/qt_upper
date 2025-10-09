#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QScrollArea>
#include <QTabWidget>
#include <QStackedWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void resizeStackedWidgetAndScrollArea(QStackedWidget *stackedWidget, QScrollArea *scrollArea);
    void UpdateUi(void);

protected:
    void resizeEvent(QResizeEvent *event);

public:
    bool testsRunning = false; // 添加 testsRunning 成员
protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::MainWindow *ui;

signals:
    void resized(); // Custom signal to indicate the window has been resized
    void updateUi(); // signal to indicate the GUI should be updated
};

extern MainWindow *g_window;

void EmitUpdateUiSignal(void);
void UpdateUiTask(void);

#endif // MAINWINDOW_H
