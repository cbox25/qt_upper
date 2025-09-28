#ifndef TOOLBAR_CTRL_H
#define TOOLBAR_CTRL_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QMouseEvent>

class DraggableDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DraggableDialog(QWidget* parent = nullptr);

protected:

    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    bool m_bDrag = false;
    QPoint m_DragPosition;
};

class AboutDialog : public DraggableDialog
{
    Q_OBJECT

public:
    explicit AboutDialog(QWidget* parent = nullptr,
                         const QString& fpgaVersion = "",
                         const QString& softKernelVersion = "");

private slots:
    void OnCopyButtonClicked();
    void OnCloseButtonClicked();

private:
    void InitializeUI();
    void InitializeStyles();
    QWidget* CreateTitleBar();
    QWidget* CreateContentArea();
    QHBoxLayout* CreateHeaderLayout();
    QVBoxLayout* CreateInfoLayout();
    QHBoxLayout* CreateButtonLayout();
    QFrame* CreateSeparator();

    void SetupConnections();

private:
    QLabel* m_appTitleLabel;
    QLabel* m_versionLabel;
    QLabel* m_fpgaLabel;
    QLabel* m_kernelLabel;

    QString m_fpgaVersion;
    QString m_softKernelVersion;
};

#endif
