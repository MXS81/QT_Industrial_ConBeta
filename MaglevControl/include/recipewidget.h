#ifndef RECIPEWIDGET_H
#define RECIPEWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QHeaderView>
#include <QLineEdit>
#include <QInputDialog>
#include <QSplitter>
#include <QScrollArea>
#include "recipemanager.h"

class RecipeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RecipeWidget(RecipeManager* recipeManager, QWidget *parent = nullptr);

signals:
    // 发送日志信息到主窗口
    void logMessage(const QString& message);

private slots:
    void onStationSelectionChanged();
    void onStationTotalChanged();
    void onResetCurrentStation();
    void onTableCellClicked(int row, int column);
    
    // 简化的配方管理槽函数
    void onSaveRecipeFile();
    void onLoadRecipeFile();
    void onApplyToDevice();
    
    // 数据变更槽函数
    void onRecipeDataChanged();
    void onStationUpdated(int index);
    void onErrorOccurred(const QString& error);
    void onStatusChanged(const QString& message);
    
    // 完整配方相关槽函数
    void onSaveCurrentStation();
    void onSaveAllStations();
    void onLoadAllStations();
    void onDeleteCompleteRecipe();
    void onApplyCompleteRecipe();
    void onRenameCompleteRecipe();
    void onCompleteRecipeSelectionChanged();
    
    // RecipeManager信号响应
    void onCompleteRecipeSaved(const QString& recipeName);
    void onCompleteRecipeLoaded(const QString& recipeName);
    void onCompleteRecipeDeleted(const QString& recipeName);
    void onCompleteRecipeApplied(const QString& recipeName);

private:
    RecipeManager* m_recipeManager;
    
    // 主界面布局
    QVBoxLayout* m_mainLayout;
    QSplitter* m_mainSplitter;
    QWidget* m_editAndTableWidget;
    QVBoxLayout* m_editAndTableLayout;
    
    // 顶部控制区域
    QWidget* m_controlWidget;
    QHBoxLayout* m_controlLayout;
    QLabel* m_stationCountLabel;
    QLabel* m_stationTotalLabel;
    QSpinBox* m_stationTotalSpin;
    QSpinBox* m_currentStationSpin;
    QPushButton* m_resetBtn;

    // 简化的配方管理区域
    QGroupBox* m_recipeManagementGroup;
    QHBoxLayout* m_recipeManagementLayout;
    QPushButton* m_saveRecipeBtn;
    QPushButton* m_loadRecipeBtn;
    QPushButton* m_applyRecipeBtn;
    
    // 当前工位编辑区域
    QGroupBox* m_editGroup;
    QGridLayout* m_editLayout;
    
    // 编辑控件
    QLineEdit* m_stationNoEdit;
    QSpinBox* m_taskIdSpin;
    QLineEdit* m_segmentNoEdit;
    QLineEdit* m_nextStationEdit;        // 新增：目标工位号
    QLineEdit* m_segmentPosEdit;         // 改为直接输入
    QLineEdit* m_segmentSpeedEdit;       // 改为直接输入
    QLineEdit* m_startPosEdit;           // 改为直接输入
    QLineEdit* m_endPosEdit;             // 改为直接输入
    QLineEdit* m_arrivalDelayEdit;       // 改为直接输入
    QComboBox* m_ferryPosCombo;          // 摆渡位置保持原有形式
    QCheckBox* m_stationMaskCheck;
    
    // 表格显示区域
    QTableWidget* m_stationTable;
    
    // 完整配方管理相关
    QComboBox* m_recipeCombo;
    QPushButton* m_deleteRecipeBtn;
    QPushButton* m_renameRecipeBtn;
    QLabel* m_currentRecipeLabel;
    
    // 初始化方法
    void initUI();
    void initControlArea();
    void initSimpleRecipeArea();
    void initEditArea();
    void initTableArea();
    void setupConnections();
    void applyStyles();
    
    // 数据同步方法
    void updateEditControls();
    void updateTableData();
    void updateCurrentStationFromControls();
    void saveCurrentStationToMemory(); // 将当前界面修改保存到内存数据
    StationRecipe getRecipeFromControls();
    void setControlsFromRecipe(const StationRecipe& recipe);

    // 文件操作方法
    QString getRecipeFilePath(bool save = true);
    void showSaveSuccess(const QString& filePath);
    void showLoadSuccess(const QString& filePath);
    
    // 表格相关
    void setupTable();
    void populateTable();
    QString ferryPositionToString(FerryPosition pos);
    FerryPosition stringToFerryPosition(const QString& str);
    
    // 完整配方相关辅助方法
    void updateRecipeCombo();
    void updateCurrentRecipeLabel();
    QString getNewRecipeName();
};

#endif // RECIPEWIDGET_H
