#ifndef RECIPEMANAGER_H
#define RECIPEMANAGER_H

#include <QObject>
#include <QVector>
#include <QDateTime>
#include "basecontroller.h"

// 摆渡位置枚举（与下位机统一）
enum class FerryPosition : quint16 {
    None = 0,
    Pos1 = 1,
    Pos2 = 2  // 扩展用
};

// 工艺类型枚举
enum class ProcessType : quint16 {
    Winding = 1,    // 绕线
    Gluing = 2,     // 点胶
    Welding = 3,    // 焊接
    Molding = 4,    // 注塑
    Unloading = 5   // 下料
};

// 工位配方数据结构（对应文档中的ModbusStationData）
struct StationRecipe {
    // 基本参数（按文档定义，每个工位占8个寄存器）
    quint8 stationNo;           // 工位NO（bit0，索引0）
    quint8 taskId;              // 任务ID（bit1，索引0）
    quint8 segmentNo;           // 所属路段号（bit0，索引1）
    quint8 nextStationId;       // 目标工位号（bit1，索引1）
    
    // 16位无符号整数参数（每个占1个寄存器）
    quint16 segmentPosition;    // 路段位置（索引2）
    quint16 segmentSpeed;       // 路段速度（索引3）
    quint16 startPosition;      // 起始位置（索引4）
    quint16 endPosition;        // 终止位置（索引5）
    quint16 arrivalDelay;       // 到位延时（索引6）
    
    // 枚举与布尔（16位寄存器）
    FerryPosition ferryPos;     // 摆渡位置（bit0，索引7）
    bool stationMask;           // 工位屏蔽（bit1，索引7）
    
    // 配方管理字段（上位机保存用）
    quint16 recipeId;           // 配方ID
    
    // 辅助字段
    ProcessType processType() const {
        return static_cast<ProcessType>(taskId);
    }
    
    QString processDescription() const {
        switch (processType()) {
            case ProcessType::Winding: return "耳机线圈绕制";
            case ProcessType::Gluing: return "线圈与骨架点胶固定";
            case ProcessType::Welding: return "线圈引脚与PCB板焊接";
            case ProcessType::Molding: return "耳机线圈模块注塑封装";
            case ProcessType::Unloading: return stationNo == 9 ? "工件分拣（合格件）" : "工件分拣（不合格件）";
            default: return "未知工艺";
        }
    }
};

/**
 * @brief 完整的配方数据结构
 * 包含所有工位的完整参数，作为一个整体进行保存和加载
 */
struct CompleteRecipe {
    QString recipeName;                    // 配方名称（如"配方1"、"配方2"）
    QString description;                   // 配方描述
    QDateTime createTime;                  // 创建时间
    QDateTime modifyTime;                  // 修改时间
    QVector<StationRecipe> stationRecipes; // 所有工位的配方数据
    
    // 辅助方法
    bool isValid() const {
        return !recipeName.isEmpty() && !stationRecipes.isEmpty();
    }
    
    int stationCount() const {
        return stationRecipes.size();
    }
};

class RecipeManager : public BaseController
{
    Q_OBJECT

public:
    explicit RecipeManager(ModbusManager* modbusManager, QObject *parent = nullptr);
    
    // 工艺类型相关常量
    static const QStringList ProcessTypeNames;
    static const QMap<ProcessType, QString> ProcessDescriptions;
    static const QMap<ProcessType, QPair<int, int>> ProcessStationRanges; // 每种工艺对应的工位范围
    
    // 静态辅助方法
    static QString processTypeToString(ProcessType type);
    static ProcessType stringToProcessType(const QString& str);
    static QVector<int> getStationsByProcessType(ProcessType type);
    
    // 单工位配方操作
    bool saveRecipe(int stationIndex, const StationRecipe& recipe);
    bool loadRecipe(int stationIndex, StationRecipe& recipe);
    bool saveAllRecipes();
    bool loadAllRecipes();
    
    // 配方管理
    bool saveCompleteRecipe(const QString& recipeName, const QString& description = "");
    bool loadCompleteRecipe(const QString& recipeName);
    bool deleteCompleteRecipe(const QString& recipeName);
    QStringList getCompleteRecipeNames() const;
    CompleteRecipe getCompleteRecipe(const QString& recipeName) const;
    bool renameCompleteRecipe(const QString& oldName, const QString& newName);
    
    // 配方应用到设备
    bool applyCompleteRecipeToDevice(const QString& recipeName);
    
    // 工位管理
    void addStation(const StationRecipe& recipe);
    void removeStation(int stationIndex);
    void updateStation(int stationIndex, const StationRecipe& recipe);
    StationRecipe getStation(int stationIndex) const;
    
    // 获取当前工位数据
    QVector<StationRecipe> getAllStations() const { return m_stations; }
    int getStationCount() const { return m_stations.size(); }
    
    // 数据文件管理
    void saveDataToFile(const QString& filename);
    void loadDataFromFile(const QString& filename);

signals:
    void stationAdded(int index);
    void stationUpdated(int index);
    void stationRemoved(int index);
    void dataChanged();
    void errorOccurred(const QString& error);
    void statusChanged(const QString& message);
    
    // 完整配方相关信号
    void completeRecipeSaved(const QString& recipeName);
    void completeRecipeLoaded(const QString& recipeName);
    void completeRecipeDeleted(const QString& recipeName);
    void completeRecipeApplied(const QString& recipeName);

  private:
      QVector<StationRecipe> m_stations;                    // 当前工位数据
    QMap<QString, CompleteRecipe> m_completeRecipes;      // 完整配方存储
    QString m_currentCompleteRecipeName;                  // 当前加载的完整配方名称
    
    // Modbus寄存器地址计算（按文档要求从寄存器36开始）
    static const int STATION_COUNT_ADDR = 0x0023;  // 寄存器[35]：工位总数
    static const int BASE_ADDR = 0x0024;           // 寄存器[36]：配方数据基址
    static const int STATION_SIZE = 8;             // 每个工位占用8个寄存器
    
    // 寄存器偏移定义（按文档索引对应，每工位8个寄存器）
    enum RegisterOffset {
        OFFSET_STATION_TASK = 0,    // 工位NO(bit0) + 任务ID(bit1)，索引0
        OFFSET_SEGMENT_NEXT = 1,    // 路段号(bit0) + 目标工位号(bit1)，索引1
        OFFSET_SEG_POSITION = 2,    // 路段位置，索引2
        OFFSET_SEG_SPEED = 3,       // 路段速度，索引3
        OFFSET_START_POSITION = 4,  // 起始位置，索引4
        OFFSET_END_POSITION = 5,    // 终止位置，索引5
        OFFSET_ARRIVAL_DELAY = 6,   // 到位延时，索引6
        OFFSET_FERRY_MASK = 7       // 摆渡位置(bit0) + 工位屏蔽(bit1)，索引7
    };
    
    // 数据转换辅助函数（按文档格式：两个8位值合并为一个16位寄存器）
    quint16 packTwoBytes(quint8 lowByte, quint8 highByte) const;
    void unpackTwoBytes(quint16 value, quint8& lowByte, quint8& highByte) const;
    
    // Modbus地址计算
    int getStationBaseAddr(int stationIndex) const;
    
    // Modbus写入方法
    bool writeStationToModbus(int stationIndex, const StationRecipe& recipe);
    bool writeStationCountToModbus(int stationCount);
};

#endif // RECIPEMANAGER_H
