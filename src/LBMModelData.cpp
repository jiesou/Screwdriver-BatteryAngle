#include "LBMModelData.h"
#include "LittleFS.h"
#include "CurrentProcessor.h"
#include "StoredConfig.h"
#define MINUTE_MILL 60*1000

void LBMModelData::recordOnce(float data) {
    File file = LittleFS.open(STORED_PATH, "a"); // 追加模式
    // 二进制形式写入
    if (file.write((const uint8_t *)&data, sizeof(data)) != sizeof(data)) {
        Serial.println("[LBMModelData] Failed to write data to file");
        file.close();
        stored_config.clear(); // 重置
        ESP.restart();
        return;
    }
    file.close();
}

void LBMModelData::begin() {
    if (!LittleFS.exists(STORED_PATH))
    {
        Serial.println(
            "[LBMModelData] No Config file found, record once");
        recordOnce(0.0f);
        return;
    }
}

void LBMModelData::update() {
    unsigned long currentMillis = millis();
    if (lbmModelDataState == DISABLED)
        return;

    // 每分钟记录一次数据
    if (currentMillis - last_record_time >= MINUTE_MILL)
    {
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
    lbmModelDataState = DISABLED;
    File file = LittleFS.open(STORED_PATH, "r");
    if (!file) {
        Serial.println("[LBMModelData] Failed to open file for reading");
        file.close();
        return 0.0f;
    }
    
    size_t fileSize = file.size();
    u_int32_t recordCount = fileSize / sizeof(float);
    if (recordCount == 0)
    {
        Serial.println("[LBMModelData] 文件中没有数据。");
        file.close();
        return 0.0f;
    }
    Serial.printf("[LBMModelData] 文件中共有 %d 条 float 数据 (总大小：%lu 字节)。\n", recordCount, fileSize);

    // recordCount / 2 会自动进行整数除法，得到中间或偏前的索引
    int midPointIndex = recordCount / 2;
    
    file.seek(midPointIndex * sizeof(float), SeekSet);

    float midPointValue;
    if (file.readBytes((char *)&midPointValue, sizeof(float)) == sizeof(float))
    {
        Serial.printf("[LBMModelData] 总记录时长中点的值为：%f (对应索引：%d)\n", midPointValue, midPointIndex);
    }
    else
    {
        Serial.println("[LBMModelData] 读取中点数据失败！");
        file.close();
        stored_config.clear(); // 重置
        ESP.restart();
        return 0.0f;
    }
    file.close();
    
    return midPointValue;
}

LBMModelData lbm_model_data;