#include "RelayControler.h"
#include "CurrentProcessor.h"
#include "InteractiveInterface.h"
#include "StoredConfig.h"
#include "LBMModelData.h"
#include "UDPLogger.h"
#include <Arduino.h>

#define BATTERY_FULL_FREQ_THRESHOLD 2.5f // 认为电池满电的频率阈值，差不多是 2.7w

void RelayControler::begin()
{
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // 继电器默认打开
}

void RelayControler::update()
{
  updateSchedule();
  updateLbmSmart();
  applyHW();
}

void RelayControler::updateSchedule()
{
  if (stored_config.relay_schedule_on == 0 ||
      stored_config.relay_schedule_off == 0)
    return;

  // relay_schedule_on 和 relay_schedule_off 是秒数
  unsigned long cycleDuration =
      stored_config.relay_schedule_on + stored_config.relay_schedule_off;
  unsigned long elapsedSeconds = (millis()) / 1000;
  // millis 溢出后会从 0 重新计数。模运算在数值回绕后依然是有效的
  unsigned long positionInCycle = elapsedSeconds % cycleDuration;
  if (positionInCycle < stored_config.relay_schedule_on)
  {
    relayState = true;
  }
  else
  {
    relayState = false;
  }
}

void RelayControler::updateLbmSmart()
{
  if (!stored_config.lbm_smart_enabled)
  {
    lbmState = DISABLED;
    return;
  }
  else if (lbmState == DISABLED)
  {
    // 之前关着，现在开了。那就应该恢复 LBM 智能控制
    lbmStartTimeOfCheckingFreq = millis();
    relayState = true;
    lbmState = PREPARING_FOR_CHECKING_FREQ;
  }

  if (stored_config.lbm_smart_upper_freq == -1.0f &&
      lbmState != LEARNING_MODELDATA &&
      lbmState != DISABLED)
  {
    // 如果 modelData 没有东西，那就需要学习
    lbmState = TO_LEARN_MODELDATA;
  }

  switch (lbmState)
  {
  case TO_LEARN_MODELDATA:
    if (current_processor.frequency_overall >= 0.4f)
    {
      // 检测到设备开始充电，则开始学习 ModelData
      udpLogger.println("[RelayControler] LBM Model Data learning started.");
      lbm_model_data.start();
      lbmState = LEARNING_MODELDATA;
    }
    // 等待学习期间，stored_config.lbm_smart_upper_freq 一直会保持 -1.0f
    break;
  case LEARNING_MODELDATA:
    // 在学习，则检查是不是需要结束学习（已经完全充满了）
    // 频率持续接近 0（小于 BATTERY_FULL_FREQ_THRESHOLD），才认为设备已充满
    // 如果电流频率超过阈值，重置计时器并继续等待
    if (current_processor.frequency >= BATTERY_FULL_FREQ_THRESHOLD)
    {
      lbmLastFoundUpperFreq = 0;
      break;
    }
    else if (lbmLastFoundUpperFreq == 0)
    {
      // 频率低于阈值，开始计时或检查消抖时间
      lbmLastFoundUpperFreq = millis();
      break;
    }
    // 检查是否满足消抖条件（持续 1 秒）
    if (millis() - lbmLastFoundUpperFreq >= 1000)
    {
      // 频率持续低于阈值，认为设备已充满
      // 完成学习，确定 modelData
      stored_config.lbm_smart_upper_freq = lbm_model_data.finish();
      stored_config.save();
      udpLogger.println("[RelayControler] LBM Model Data learning finished, upper_freq: " +
                        String(stored_config.lbm_smart_upper_freq));
      // 开始放电
      relayState = false;
      lbmLastTurnon = millis();
      lbmState = WAITING_DROPPING;
      lbmLastFoundUpperFreq = 0;
    }
    break;
  case WAITING_RISING:
    // 如果电流频率超过阈值，重置计时器并继续等待
    if (current_processor.frequency >= stored_config.lbm_smart_upper_freq)
    {
      lbmLastFoundUpperFreq = 0;
      break;
    }
    else if (lbmLastFoundUpperFreq == 0)
    {
      // 频率低于阈值，开始计时或检查消抖时间
      lbmLastFoundUpperFreq = millis();
      break;
    }
    // 检查是否满足消抖条件（持续1秒）
    if (millis() - lbmLastFoundUpperFreq >= 1000)
    {
      // 频率持续低于阈值，切换状态
      relayState = false;
      lbmLastTurnon = millis();
      lbmState = WAITING_DROPPING;
      lbmLastFoundUpperFreq = 0;
    }
    break;
  case WAITING_DROPPING:
    if (millis() - lbmLastTurnon > stored_config.lbm_smart_lower_time)
    {
      // 如果时间超过了 lower_time（3 小时），则检查频率
      lbmStartTimeOfCheckingFreq = millis();
      relayState = true;
      lbmState = PREPARING_FOR_CHECKING_FREQ;
    }
    break;
  case PREPARING_FOR_CHECKING_FREQ:
    // 无论如何先充 25s，等待手机熄屏进入全速充电
    if (millis() - lbmStartTimeOfCheckingFreq > 1000 * 25)
    {
      // 15s 后，检查频率
      lbmState = CHECKING_FREQ_IN_DROPPING;
    }
    break;
  case CHECKING_FREQ_IN_DROPPING:
    if (current_processor.frequency >= stored_config.lbm_smart_upper_freq)
    {
      // 如果电流频率超过 upper（电量已低于预期），则开启，并等待电池充电。（考虑 0 值。说明没有用电器）
      relayState = true;
      lbmState = WAITING_RISING;
      break;
    }
    if (millis() - lbmStartTimeOfCheckingFreq > 1000 * 30)
    {
      // 持续检测 30s 后（准备时间 25s，监测
      // 5s），不再检查频率，继续等待电池耗电
      relayState = false;
      lbmLastTurnon = millis();
      lbmState = WAITING_DROPPING;
      break;
    }
  default:
    break;
  }
}

void RelayControler::applyHW()
{
  if (relayState)
  {
    digitalWrite(RELAY_PIN, HIGH);
    interactive_interface.led_state = true;
  }
  else
  {
    digitalWrite(RELAY_PIN, LOW);
    interactive_interface.led_state = false;
  }
}

RelayControler relay_controler;