#pragma once
#define STORED_PATH "/lbm_model_data.bin"

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
    void start();
    float finish();
};

extern LBMModelData lbm_model_data;