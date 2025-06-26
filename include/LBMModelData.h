#pragma once
#include <LittleFS.h>
#define MODEL_DATA_PATH "/lbm_model_data.bin"

class LBMModelData
{
private:
    enum lbmModelDataState
    {
        DISABLED,
        RECORDING
    } lbmModelDataState = DISABLED;
    unsigned long last_record_time = 0;
    void recordOnce(float data);

public:
    void begin();
    void update();
    void reset() {
        lbmModelDataState = DISABLED;
        last_record_time = 0;
        LittleFS.remove(MODEL_DATA_PATH);
    }
    void start();
    float finish();
};

extern LBMModelData lbm_model_data;