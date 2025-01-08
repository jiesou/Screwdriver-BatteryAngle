#pragma once
#include <ArduinoOTA.h>

class OTAHandler {
public:
    OTAHandler();
    void begin();
    void update();
};
