#include "LBMModelData.h"
#include "CurrentProcessor.h"
#include "StoredConfig.h"
#include "UDPLogger.h"
#define MINUTE_MILL 1*1000

void LBMModelData::recordOnce(float data) {
    File file = LittleFS.open(MODEL_DATA_PATH, "a"); // 追加模式
    // 二进制形式写入
    if (file.write((const uint8_t *)&data, sizeof(data)) != sizeof(data)) {
        udpLogger.println("[LBMModelData] Failed to write data to file");
        file.close();
        stored_config.clear(); // 写入失败就重置
        ESP.restart();
        return;
    }
    file.close();
}

void LBMModelData::begin() {
    if (!LittleFS.exists(MODEL_DATA_PATH))
    {
        udpLogger.println(
            "[LBMModelData] No Config file found, record once");
        recordOnce(0.0f);
        return;
    }
}

void LBMModelData::update() {
    if (lbmModelDataState == DISABLED)
    return;
    
    unsigned long currentMillis = millis();
    // 每分钟记录一次数据
    if (currentMillis - last_record_time >= MINUTE_MILL)
    {
        udpLogger.printf("[LBMModelData] Recording data: %f Hz", current_processor.frequency_overall);
        float data = current_processor.frequency_overall; // 获取当前（平滑后的）频率
        recordOnce(data);
        last_record_time = currentMillis;
    }
}

void LBMModelData::start() {
    lbmModelDataState = RECORDING;
    last_record_time = millis();
}

float LBMModelData::finish() {
    File file = LittleFS.open(MODEL_DATA_PATH, "r");
    if (!file) {
        udpLogger.println("[LBMModelData] Failed to open file for reading");
        file.close();
        return 0;
    }
    
    size_t fileSize = file.size();
    u_int32_t recordCount = fileSize / sizeof(float);
    if (recordCount == 0)
    {
        udpLogger.println("[LBMModelData] 文件中没有数据。");
        file.close();
        return 0;
    }
    udpLogger.printf("[LBMModelData] 文件中共有 %d 条 float 数据 (总大小：%lu 字节)。\n", recordCount, (unsigned long)fileSize);

    // recordCount / 2 会自动进行整数除法，得到中间或偏前的索引
    int midPointIndex = recordCount / 2;
    
    file.seek(midPointIndex * sizeof(float), SeekSet);

    float midPointValue;
    if (file.readBytes((char *)&midPointValue, sizeof(float)) != sizeof(float))
    {
        udpLogger.println("[LBMModelData] 读取中点数据失败！");
        file.close();
        stored_config.clear();
        ESP.restart();
        return 0;
    }
    udpLogger.printf("[LBMModelData] 总记录时长中点的值为：%f (对应索引：%d)\n", midPointValue, midPointIndex);
    file.close();
    LittleFS.remove(MODEL_DATA_PATH);
    lbmModelDataState = DISABLED; // 结束记录

    return midPointValue;
}

LBMModelData lbm_model_data;