
#include "recipemanager.h"
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QStandardPaths>
#include <QDir>
#include <cstring>

//

// 静态常量定义
const QStringList RecipeManager::ProcessTypeNames = {
    "绕线", "点胶", "焊接", "注塑", "下料"
};

const QMap<ProcessType, QString> RecipeManager::ProcessDescriptions = {
    {ProcessType::Winding, "耳机线圈绕制"},
    {ProcessType::Gluing, "线圈与骨架点胶固定"},
    {ProcessType::Welding, "线圈引脚与PCB板焊接"},
    {ProcessType::Molding, "耳机线圈模块注塑封装"},
    {ProcessType::Unloading, "工件分拣"}
};

const QMap<ProcessType, QPair<int, int>> RecipeManager::ProcessStationRanges = {
    {ProcessType::Winding, {1, 2}},    // S1-S2
    {ProcessType::Gluing, {3, 4}},     // S3-S4
    {ProcessType::Welding, {5, 6}},    // S5-S6
    {ProcessType::Molding, {7, 8}},    // S7-S8
    {ProcessType::Unloading, {9, 10}}  // S9-S10
};

RecipeManager::RecipeManager(ModbusManager* modbusManager, QObject *parent)
    : BaseController(modbusManager, parent)
{
    // 初始化默认配方数据（10个工位，按工艺分工，按新数据结构）
    for (int i = 0; i < 10; ++i) {
        StationRecipe recipe;
        recipe.stationNo = i + 1;
        recipe.segmentNo = i + 1;
        recipe.nextStationId = (i < 9) ? i + 2 : 1;  // 下一工位，最后一个回到第一个
        recipe.segmentPosition = 188 + i * 120;      // 使用整数
        recipe.segmentSpeed = 1000;                  // 默认速度
        recipe.startPosition = 0;
        recipe.endPosition = 100;
        recipe.arrivalDelay = 500;                   // 默认延时500ms
        recipe.ferryPos = (i == 0) ? FerryPosition::Pos1 : FerryPosition::None;
        recipe.stationMask = false;
        recipe.recipeId = 1;
        
        // 默认任务ID：01（Winding）
        recipe.taskId = static_cast<quint8>(ProcessType::Winding);
        
        m_stations.append(recipe);
    }
}

bool RecipeManager::saveRecipe(int stationIndex, const StationRecipe& recipe)
{
    if (stationIndex < 0 || stationIndex >= m_stations.size()) {
        emitError(ErrorMessages::INVALID_STATION_INDEX, stationIndex);
        return false;
    }
    
    // 更新内存中的数据
    m_stations[stationIndex] = recipe;
    
    // 写入到Modbus设备
    return writeStationToModbus(stationIndex, recipe);
}

bool RecipeManager::loadRecipe(int stationIndex, StationRecipe& recipe)
{
    if (stationIndex < 0 || stationIndex >= m_stations.size()) {
        emitError(ErrorMessages::INVALID_STATION_INDEX, stationIndex);
        return false;
    }
    
    // 从本地缓存获取
    recipe = m_stations[stationIndex];
    return true;
}

bool RecipeManager::saveAllRecipes()
{
    qDebug() << "--- Starting saveAllRecipes ---";
    if (!checkConnection()) {
        qDebug() << "--- Aborting saveAllRecipes: No connection ---";
        return false;
    }
    
    // 检查工位数据有效性
    if (m_stations.isEmpty()) {
        emit errorOccurred("没有工位数据可保存");
        qDebug() << "--- Aborting saveAllRecipes: No station data ---";
        return false;
    }
    
    qDebug() << "Writing station count:" << m_stations.size();
    // 首先写入工位总数到寄存器[35]
    if (!writeStationCountToModbus(m_stations.size())) {
        emit errorOccurred("写入工位总数失败");
        qDebug() << "--- Aborting saveAllRecipes: Failed to write station count ---";
        return false;
    }
    
    // 然后从寄存器[36]开始写入所有工位配方数据
    for (int i = 0; i < m_stations.size(); ++i) {
        qDebug() << "Saving recipe for station index:" << i;
        if (!saveRecipe(i, m_stations[i])) {
            emit errorOccurred(QString("保存工位 %1 失败").arg(i + 1));
            qDebug() << "--- Aborting saveAllRecipes: Failed to save station" << i;
            return false;
        }
    }
    
    emit statusChanged(QString("所有配方已保存到设备 (工位总数: %1)").arg(m_stations.size()));
    qDebug() << "--- Finished saveAllRecipes successfully ---";
    return true;
}

bool RecipeManager::loadAllRecipes()
{
    // 这里可以从设备读取所有配方数据
    // 当前版本使用本地数据
    emit statusChanged("所有配方已加载");
    return true;
}

void RecipeManager::addStation(const StationRecipe& recipe)
{
    m_stations.append(recipe);
    emit stationAdded(m_stations.size() - 1);
    emit dataChanged();
}

void RecipeManager::removeStation(int stationIndex)
{
    if (stationIndex >= 0 && stationIndex < m_stations.size()) {
        m_stations.removeAt(stationIndex);
        emit stationRemoved(stationIndex);
        emit dataChanged();
    }
}

void RecipeManager::updateStation(int stationIndex, const StationRecipe& recipe)
{
    if (stationIndex >= 0 && stationIndex < m_stations.size()) {
        m_stations[stationIndex] = recipe;
        emit stationUpdated(stationIndex);
        emit dataChanged();
    }
}

StationRecipe RecipeManager::getStation(int stationIndex) const
{
    if (stationIndex >= 0 && stationIndex < m_stations.size()) {
        return m_stations[stationIndex];
    }
    return StationRecipe();
}

void RecipeManager::saveDataToFile(const QString& filename)
{
    QJsonObject rootObj;
    
    // 保存当前工位数据
    QJsonArray stationsArray;
    for (const StationRecipe& station : m_stations) {
        QJsonObject stationObj;
        stationObj["stationNo"] = static_cast<int>(station.stationNo);
        stationObj["segmentNo"] = static_cast<int>(station.segmentNo);
        stationObj["segmentPosition"] = static_cast<double>(station.segmentPosition);
        stationObj["segmentSpeed"] = static_cast<double>(station.segmentSpeed);
        stationObj["startPosition"] = static_cast<double>(station.startPosition);
        stationObj["endPosition"] = static_cast<double>(station.endPosition);
        stationObj["arrivalDelay"] = static_cast<double>(station.arrivalDelay);
        stationObj["ferryPos"] = static_cast<int>(station.ferryPos);
        stationObj["stationMask"] = station.stationMask;
        stationObj["recipeId"] = static_cast<int>(station.recipeId);
        stationObj["taskId"] = static_cast<int>(station.taskId);
        
        stationsArray.append(stationObj);
    }
    rootObj["stations"] = stationsArray;
    
    // 保存完整配方数据
    QJsonObject completeRecipesObj;
    for (auto it = m_completeRecipes.constBegin(); it != m_completeRecipes.constEnd(); ++it) {
        const CompleteRecipe& recipe = it.value();
        
        QJsonObject recipeObj;
        recipeObj["recipeName"] = recipe.recipeName;
        recipeObj["description"] = recipe.description;
        recipeObj["createTime"] = recipe.createTime.toString(Qt::ISODate);
        recipeObj["modifyTime"] = recipe.modifyTime.toString(Qt::ISODate);
        
        QJsonArray recipeStationsArray;
        for (const StationRecipe& station : recipe.stationRecipes) {
            QJsonObject stationObj;
            stationObj["stationNo"] = static_cast<int>(station.stationNo);
            stationObj["segmentNo"] = static_cast<int>(station.segmentNo);
            stationObj["segmentPosition"] = static_cast<double>(station.segmentPosition);
            stationObj["segmentSpeed"] = static_cast<double>(station.segmentSpeed);
            stationObj["startPosition"] = static_cast<double>(station.startPosition);
            stationObj["endPosition"] = static_cast<double>(station.endPosition);
            stationObj["arrivalDelay"] = static_cast<double>(station.arrivalDelay);
            stationObj["ferryPos"] = static_cast<int>(station.ferryPos);
            stationObj["stationMask"] = station.stationMask;
            stationObj["recipeId"] = static_cast<int>(station.recipeId);
            stationObj["taskId"] = static_cast<int>(station.taskId);
            
            recipeStationsArray.append(stationObj);
        }
        recipeObj["stationRecipes"] = recipeStationsArray;
        
        completeRecipesObj[recipe.recipeName] = recipeObj;
    }
    rootObj["completeRecipes"] = completeRecipesObj;
    rootObj["currentCompleteRecipeName"] = m_currentCompleteRecipeName;
    
    QJsonDocument doc(rootObj);
    
    // 确保目录存在
    QFileInfo fileInfo(filename);
    QDir().mkpath(fileInfo.absolutePath());
    
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
        emit statusChanged("配方数据已保存到文件: " + filename);
    } else {
        emitError(ErrorMessages::FILE_SAVE_ERROR, filename);
    }
}

void RecipeManager::loadDataFromFile(const QString& filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        emitError(ErrorMessages::FILE_OPEN_ERROR, filename);
        return;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        emit errorOccurred("JSON解析错误: " + error.errorString());
        return;
    }
    
    QJsonObject rootObj = doc.object();
    
    // 加载当前工位数据
    if (rootObj.contains("stations")) {
        QJsonArray stationsArray = rootObj["stations"].toArray();
        m_stations.clear();
        
        for (const QJsonValue& value : stationsArray) {
            QJsonObject stationObj = value.toObject();
            
            StationRecipe station;
            station.stationNo = static_cast<quint8>(stationObj["stationNo"].toInt());
            station.segmentNo = static_cast<quint8>(stationObj["segmentNo"].toInt());
            station.segmentPosition = static_cast<quint16>(stationObj["segmentPosition"].toInt());
            station.segmentSpeed = static_cast<quint16>(stationObj["segmentSpeed"].toInt());
            station.startPosition = static_cast<quint16>(stationObj["startPosition"].toInt());
            station.endPosition = static_cast<quint16>(stationObj["endPosition"].toInt());
            station.arrivalDelay = static_cast<quint16>(stationObj["arrivalDelay"].toInt());
            station.ferryPos = static_cast<FerryPosition>(stationObj["ferryPos"].toInt());
            station.stationMask = stationObj["stationMask"].toBool();
            station.recipeId = static_cast<quint16>(stationObj["recipeId"].toInt());
            station.taskId = static_cast<quint8>(stationObj["taskId"].toInt());
            
            m_stations.append(station);
        }
    }
    
    // 加载完整配方数据
    if (rootObj.contains("completeRecipes")) {
        m_completeRecipes.clear();
        QJsonObject completeRecipesObj = rootObj["completeRecipes"].toObject();
        
        for (auto it = completeRecipesObj.constBegin(); it != completeRecipesObj.constEnd(); ++it) {
            QJsonObject recipeObj = it.value().toObject();
            
            CompleteRecipe recipe;
            recipe.recipeName = recipeObj["recipeName"].toString();
            recipe.description = recipeObj["description"].toString();
            recipe.createTime = QDateTime::fromString(recipeObj["createTime"].toString(), Qt::ISODate);
            recipe.modifyTime = QDateTime::fromString(recipeObj["modifyTime"].toString(), Qt::ISODate);
            
            // 加载配方中的工位数据
            QJsonArray recipeStationsArray = recipeObj["stationRecipes"].toArray();
            for (const QJsonValue& value : recipeStationsArray) {
                QJsonObject stationObj = value.toObject();
                
                StationRecipe station;
                station.stationNo = static_cast<quint8>(stationObj["stationNo"].toInt());
                station.segmentNo = static_cast<quint8>(stationObj["segmentNo"].toInt());
                station.segmentPosition = static_cast<quint16>(stationObj["segmentPosition"].toInt());
                station.segmentSpeed = static_cast<quint16>(stationObj["segmentSpeed"].toInt());
                station.startPosition = static_cast<quint16>(stationObj["startPosition"].toInt());
                station.endPosition = static_cast<quint16>(stationObj["endPosition"].toInt());
                station.arrivalDelay = static_cast<quint16>(stationObj["arrivalDelay"].toInt());
                station.ferryPos = static_cast<FerryPosition>(stationObj["ferryPos"].toInt());
                station.stationMask = stationObj["stationMask"].toBool();
                station.recipeId = static_cast<quint16>(stationObj["recipeId"].toInt());
                station.taskId = static_cast<quint8>(stationObj["taskId"].toInt());
                
                recipe.stationRecipes.append(station);
            }
            
            m_completeRecipes[recipe.recipeName] = recipe;
        }
    }
    
    // 加载当前配方名称
    if (rootObj.contains("currentCompleteRecipeName")) {
        m_currentCompleteRecipeName = rootObj["currentCompleteRecipeName"].toString();
    }
    
    emit dataChanged();
    emit statusChanged("配方数据已从文件加载: " + filename);
}


// 静态辅助方法实现
QString RecipeManager::processTypeToString(ProcessType type)
{
    switch (type) {
        case ProcessType::Winding: return "绕线";
        case ProcessType::Gluing: return "点胶";
        case ProcessType::Welding: return "焊接";
        case ProcessType::Molding: return "注塑";
        case ProcessType::Unloading: return "下料";
        default: return "未知";
    }
}

ProcessType RecipeManager::stringToProcessType(const QString& str)
{
    if (str == "绕线") return ProcessType::Winding;
    if (str == "点胶") return ProcessType::Gluing;
    if (str == "焊接") return ProcessType::Welding;
    if (str == "注塑") return ProcessType::Molding;
    if (str == "下料") return ProcessType::Unloading;
    return ProcessType::Winding; // 默认值
}

QVector<int> RecipeManager::getStationsByProcessType(ProcessType type)
{
    QVector<int> stations;
    if (ProcessStationRanges.contains(type)) {
        QPair<int, int> range = ProcessStationRanges[type];
        for (int i = range.first; i <= range.second; ++i) {
            stations.append(i);
        }
    }
    return stations;
}

// 配方管理实现
bool RecipeManager::saveCompleteRecipe(const QString& recipeName, const QString& description)
{
    if (recipeName.isEmpty()) {
        emitError(ErrorMessages::RECIPE_NAME_EMPTY);
        return false;
    }
    
    CompleteRecipe recipe;
    recipe.recipeName = recipeName;
    recipe.description = description;
    recipe.createTime = QDateTime::currentDateTime();
    recipe.modifyTime = recipe.createTime;
    recipe.stationRecipes = m_stations;
    
    // 如果配方已存在，更新修改时间
    if (m_completeRecipes.contains(recipeName)) {
        recipe.createTime = m_completeRecipes[recipeName].createTime;
        recipe.modifyTime = QDateTime::currentDateTime();
    }
    
    m_completeRecipes[recipeName] = recipe;
    m_currentCompleteRecipeName = recipeName;
    
    emit completeRecipeSaved(recipeName);
    emit statusChanged(QString("配方 '%1' 已保存").arg(recipeName));
    
    return true;
}

bool RecipeManager::loadCompleteRecipe(const QString& recipeName)
{
    if (!m_completeRecipes.contains(recipeName)) {
        emit errorOccurred(QString("配方 '%1' 不存在").arg(recipeName));
        return false;
    }
    
    const CompleteRecipe& recipe = m_completeRecipes[recipeName];
    m_stations = recipe.stationRecipes;
    m_currentCompleteRecipeName = recipeName;
    
    emit completeRecipeLoaded(recipeName);
    emit dataChanged();
    emit statusChanged(QString("配方 '%1' 已加载").arg(recipeName));
    
    return true;
}

bool RecipeManager::deleteCompleteRecipe(const QString& recipeName)
{
    if (!m_completeRecipes.contains(recipeName)) {
        emit errorOccurred(QString("配方 '%1' 不存在").arg(recipeName));
        return false;
    }
    
    m_completeRecipes.remove(recipeName);
    
    if (m_currentCompleteRecipeName == recipeName) {
        m_currentCompleteRecipeName.clear();
    }
    
    emit completeRecipeDeleted(recipeName);
    emit statusChanged(QString("配方 '%1' 已删除").arg(recipeName));
    
    return true;
}

QStringList RecipeManager::getCompleteRecipeNames() const
{
    return m_completeRecipes.keys();
}

CompleteRecipe RecipeManager::getCompleteRecipe(const QString& recipeName) const
{
    if (m_completeRecipes.contains(recipeName)) {
        return m_completeRecipes[recipeName];
    }
    return CompleteRecipe();
}

bool RecipeManager::renameCompleteRecipe(const QString& oldName, const QString& newName)
{
    if (!m_completeRecipes.contains(oldName)) {
        emit errorOccurred(QString("配方 '%1' 不存在").arg(oldName));
        return false;
    }
    
    if (m_completeRecipes.contains(newName)) {
        emit errorOccurred(QString("配方名称 '%1' 已存在").arg(newName));
        return false;
    }
    
    CompleteRecipe recipe = m_completeRecipes[oldName];
    recipe.recipeName = newName;
    recipe.modifyTime = QDateTime::currentDateTime();
    
    m_completeRecipes.remove(oldName);
    m_completeRecipes[newName] = recipe;
    
    if (m_currentCompleteRecipeName == oldName) {
        m_currentCompleteRecipeName = newName;
    }
    
    emit statusChanged(QString("配方已重命名：'%1' → '%2'").arg(oldName).arg(newName));
    
    return true;
}

bool RecipeManager::applyCompleteRecipeToDevice(const QString& recipeName)
{
    if (!m_completeRecipes.contains(recipeName)) {
        emit errorOccurred(QString("配方 '%1' 不存在").arg(recipeName));
        return false;
    }
    
    // 首先加载配方到当前数据
    if (!loadCompleteRecipe(recipeName)) {
        return false;
    }
    
    // 然后将所有工位数据写入设备
    if (!saveAllRecipes()) {
        emit errorOccurred(QString("配方 '%1' 应用到设备失败").arg(recipeName));
        return false;
    }
    
    emit completeRecipeApplied(recipeName);
    emit statusChanged(QString("配方 '%1' 已应用到设备").arg(recipeName));
    
    return true;
}

int RecipeManager::getStationBaseAddr(int stationIndex) const
{
    return BASE_ADDR + stationIndex * STATION_SIZE;
}

quint16 RecipeManager::packTwoBytes(quint8 lowByte, quint8 highByte) const
{
    return (static_cast<quint16>(highByte) << 8) | static_cast<quint16>(lowByte);
}

void RecipeManager::unpackTwoBytes(quint16 value, quint8& lowByte, quint8& highByte) const
{
    lowByte = static_cast<quint8>(value & 0xFF);
    highByte = static_cast<quint8>((value >> 8) & 0xFF);
}

bool RecipeManager::writeStationToModbus(int stationIndex, const StationRecipe& recipe)
{
    qDebug() << "  -> Entering writeStationToModbus for station index:" << stationIndex;
    if (!checkConnection()) {
        qDebug() << "  -> Aborting writeStationToModbus: No connection";
        return false;
    }
    
    // 边界检查
    if (stationIndex < 0 || stationIndex >= m_stations.size()) {
        emit errorOccurred(QString("工位索引 %1 超出范围").arg(stationIndex));
        qDebug() << "  -> Aborting writeStationToModbus: Index out of range";
        return false;
    }

    int baseAddr = getStationBaseAddr(stationIndex);
    qDebug() << "  -> Base address for station" << stationIndex << "is" << QString("0x%1").arg(baseAddr, 4, 16, QChar('0'));
    
    // 写入数据（每个工位8个寄存器）
    
    // 寄存器0: 工位NO(bit0) + 任务ID(bit1)
    quint16 reg0 = packTwoBytes(recipe.stationNo, recipe.taskId);
    if (!m_modbusManager->writeRegisterSync(baseAddr + OFFSET_STATION_TASK, reg0)) {
        emit errorOccurred("写入工位号和任务ID失败");
        return false;
    }
    
    // 寄存器1: 所属路段号(bit0) + 目标工位号(bit1)
    quint16 reg1 = packTwoBytes(recipe.segmentNo, recipe.nextStationId);
    if (!m_modbusManager->writeRegisterSync(baseAddr + OFFSET_SEGMENT_NEXT, reg1)) {
        emit errorOccurred("写入路段号和目标工位号失败");
        return false;
    }
    
    // 寄存器2: 路段位置
    if (!m_modbusManager->writeRegisterSync(baseAddr + OFFSET_SEG_POSITION, recipe.segmentPosition)) {
        emit errorOccurred("写入路段位置失败");
        return false;
    }
    
    // 寄存器3: 路段速度
    if (!m_modbusManager->writeRegisterSync(baseAddr + OFFSET_SEG_SPEED, recipe.segmentSpeed)) {
        emit errorOccurred("写入路段速度失败");
        return false;
    }
    
    // 寄存器4: 起始位置
    if (!m_modbusManager->writeRegisterSync(baseAddr + OFFSET_START_POSITION, recipe.startPosition)) {
        emit errorOccurred("写入起始位置失败");
        return false;
    }
    
    // 寄存器5: 终止位置
    if (!m_modbusManager->writeRegisterSync(baseAddr + OFFSET_END_POSITION, recipe.endPosition)) {
        emit errorOccurred("写入终止位置失败");
        return false;
    }
    
    // 寄存器6: 到位延时
    if (!m_modbusManager->writeRegisterSync(baseAddr + OFFSET_ARRIVAL_DELAY, recipe.arrivalDelay)) {
        emit errorOccurred("写入到位延时失败");
        return false;
    }
    
    // 寄存器7: 摆渡位置(bit0) + 工位屏蔽(bit1)
    quint8 ferryPos = static_cast<quint8>(recipe.ferryPos);
    quint8 stationMask = recipe.stationMask ? 1 : 0;
    quint16 reg7 = packTwoBytes(ferryPos, stationMask);
    if (!m_modbusManager->writeRegisterSync(baseAddr + OFFSET_FERRY_MASK, reg7)) {
        emit errorOccurred("写入摆渡位置和工位屏蔽失败");
        return false;
    }
    
    emit statusChanged(QString("工位 %1 配方已写入设备").arg(stationIndex + 1));
    emit stationUpdated(stationIndex);
    
    qDebug() << "  -> Finished writeStationToModbus for station index:" << stationIndex;
    return true;
}

bool RecipeManager::writeStationCountToModbus(int stationCount)
{
    qDebug() << "  -> Writing station count to Modbus:" << stationCount;
    if (!checkConnection()) {
        qDebug() << "  -> Aborting writeStationCountToModbus: No connection";
        return false;
    }

    // 设备上限保护
    
    // 写入工位总数到寄存器[35]
    if (!m_modbusManager->writeRegisterSync(STATION_COUNT_ADDR, static_cast<quint16>(stationCount))) {
        emit errorOccurred("写入工位总数失败");
        qDebug() << "  -> Failed to write station count to Modbus";
        return false;
    }
    
    emit statusChanged(QString("工位总数 %1 已写入设备").arg(stationCount));
    qDebug() << "  -> Successfully wrote station count to Modbus";
    return true;
}



