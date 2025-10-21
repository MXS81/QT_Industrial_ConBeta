// OverviewPage.h
#ifndef OVERVIEWPAGE_H
#define OVERVIEWPAGE_H

#include <QWidget>
#include <QList>
#include "MoverData.h"
#include "LogWidget.h"
#include "MainWindow.h"

class TrackWidget;
class MoverWidget;
class QTableWidget;
class QTextEdit;
class QLabel;
class QPushButton;
class QGridLayout;
class QScrollArea;

class OverviewPage : public QWidget
{
    Q_OBJECT

public:
    explicit OverviewPage(QList<MoverData>* movers, const QString& currentUser, QWidget *parent = nullptr);
    ~OverviewPage();

public slots:
    void updateMovers(const QList<MoverData> &movers);
    void onUserChanged(const QString& username);
    void addLogEntry(const QString &message, const QString &type = "info");

private slots:
    void onMoverSelected(int id);

private:
    void setupUI();
    void createMoverWidgets();
    void initializeTableWithData();       // 初始化表格数据
    void updateTableData(const QList<MoverData> &movers);  // 更新表格数据
    MainWindow *m_mainWindow;

    // UI组件
    TrackWidget *m_trackWidget;
    QTableWidget *m_statusTable;
    LogWidget *m_logWidget;
    QList<MoverWidget*> m_moverWidgets;
    QGridLayout *m_moverGridLayout;
    QLabel *m_userInfoLabel;
    QScrollArea *m_trackScrollArea;
    QLabel *m_recipeInfoLabel;

    // 数据
    QList<MoverData>* m_movers;
    QString m_currentUser;
    int m_selectedMover;
};
#endif // OVERVIEWPAGE_H
