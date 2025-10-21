// RecipeManagerPage.h 配方管理界面
#ifndef RECIPEMANAGERPAGE_H
#define RECIPEMANAGERPAGE_H

#include <QWidget>
#include <QList>
#include <QJsonObject>
#include "MoverData.h"
#include "LogWidget.h"
#include "MainWindow.h"

class QListWidget;
class QTableWidget;
class QPushButton;
class QLineEdit;
class QTextEdit;
class QLabel;
class ModbusManager;

struct Recipe {
    int id = -1;
    QString name;
    QString description;
    QList<QJsonObject> moverConfigs; // 每个动子的配置
    QString createTime;
    QString modifyTime;
    QString lastModifiedBy; // 记录最后修改配方的用
};

class RecipeManagerPage : public QWidget
{
    Q_OBJECT

public:
    explicit RecipeManagerPage(QList<MoverData> *movers, const QString &currentUser,ModbusManager *modbusManager, QWidget *parent = nullptr);
    ~RecipeManagerPage();

signals:
    void recipeApplied(int id, const QString &name);

private slots:
    void onRecipeSelected();
    void onNewRecipe();
    void onSaveRecipe();
    void onDeleteRecipe();
    void onApplyRecipe();
    void onExportRecipe();
    void onImportRecipe();
    void onRecipeNameChanged();

private:
    void setupUI();
    void loadRecipes();
    void saveRecipes();
    void updateRecipeDetails();
    void createSampleRecipes();
    MainWindow *m_mainWindow;
    void addLogEntry(const QString &message, const QString &type = "info");

    // UI组件
    QListWidget *m_recipeList;
    QLineEdit *m_recipeNameEdit;
    QTextEdit *m_descriptionEdit;
    QTableWidget *m_configTable;
    QPushButton *m_newBtn;
    QPushButton *m_saveBtn;
    QPushButton *m_deleteBtn;
    QPushButton *m_applyBtn;
    QPushButton *m_exportBtn;
    QPushButton *m_importBtn;
    QLabel *m_createTimeLabel;
    QLabel *m_modifyTimeLabel;
    LogWidget *m_logWidget;
    QLabel *m_recipeIdLabel;

    // 数据
    QList<MoverData> *m_movers;
    QList<Recipe> m_recipes;
    QString m_currentUser;  // 添加用户信息
    static int s_nextRecipeId;
    int m_currentRecipeIndex;
    bool m_isModified;
    ModbusManager *m_modbusManager;
};

#endif // RECIPEMANAGERPAGE_H
