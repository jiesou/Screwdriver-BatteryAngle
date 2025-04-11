<script setup lang="ts">
import { ref, onMounted, watch } from 'vue';
import { useConfigService } from './services/configService';
import { useStatusService } from './services/statusService';
import type { DeviceConfig, DeviceStatus } from './types';

import { snackbar } from 'mdui/functions/snackbar.js';

// 响应式状态
const deviceConfig = ref<DeviceConfig>({
  wifi_sta_ssid: '',
  wifi_sta_password: '',
  relay_schedule_on: 0,
  relay_schedule_off: 0
});
const deviceStatus = ref<DeviceStatus | null>(null);
const wifiNetworks = ref<string[]>([]);
const isConnecting = ref<boolean>(false);
const wifiDropdownOpen = ref<boolean>(false);
const relaySwitchState = ref<boolean>(false);
const relayScheduleText = ref<{
  on: string
  off: string
}>({
  on: '00:00:00',
  off: '00:00:00',
});

// 服务
const { fetchConfig, setConfig, fetchWifiNetworks, setRelaySwitch } = useConfigService();
const { connectStatusTo, disconnectStatusService } = useStatusService();

// 加载配置
const loadConfig = async () => {
  try {
    const config = await fetchConfig();
    deviceConfig.value = { ...deviceConfig.value, ...config };
  } catch (error) {
    snackbar({ message: '加载配置失败，请检查网络连接或设备状态。' });
    console.error("Error loading config:", error);
  }
};

// 加载WiFi网络
const loadWifiNetworks = async () => {
  try {
    wifiNetworks.value = await fetchWifiNetworks();
  } catch (error) {
    snackbar({ message: '加载 WiFi 网络失败，请检查网络连接或设备状态。' });
    console.error("Error loading WiFi networks:", error);
    wifiNetworks.value = [];
  }
};


const submitConfig = async () => {
  try {
    const result = await setConfig(deviceConfig.value);
    snackbar({ message: result.message });
  } catch (error) {
    snackbar({ message: '配置失败，请检查网络连接或设备状态。' });
    console.error("Error:", error);
  }
};

// 更新继电器开关状态
const updateRelaySwitch = async () => {
  try {
    const result = await setRelaySwitch(relaySwitchState.value);
    snackbar({ message: result.message });
  } catch (error) {
    snackbar({ message: '更新继电器状态失败，请检查网络连接或设备状态。' });
    console.error("Error:", error);
  }
};

// 重新连接状态
const reconnectStatus = async () => {
  isConnecting.value = true;
  disconnectStatusService();

  await connectStatusTo((status: DeviceStatus) => {
    isConnecting.value = false;
    deviceStatus.value = status;
    relaySwitchState.value = status.relay_state;
  });
};

// relayScheduleText 数据稳定之后自动提交配置
watch(relayScheduleText, (newValue) => {
  if (newValue.on && newValue.off) {
    deviceConfig.value.relay_schedule_on = new Date(`1970-01-01T${newValue.on}Z`).getTime() / 1000;
    deviceConfig.value.relay_schedule_off = new Date(`1970-01-01T${newValue.off}Z`).getTime() / 1000;
    submitConfig();
  }
}, { deep: true });

// watch(deviceConfig, (newValue) => {
//   relayScheduleText.value.on = new Date(newValue.relay_schedule_on * 1000).toTimeString().slice(0, 8);
//   relayScheduleText.value.off = new Date(newValue.relay_schedule_off * 1000).toTimeString().slice(0, 8);
// }, { deep: true });

onMounted(() => {
  loadConfig();
  reconnectStatus();
});

import '@mdui/icons/power.js';
import '@mdui/icons/power-off.js';
import '@mdui/icons/refresh.js';
import '@mdui/icons/wifi.js';
import '@mdui/icons/wifi-off.js';
import '@mdui/icons/wifi-find.js';

import 'mdui/mdui.css';
import 'mdui/components/button.js';
import 'mdui/components/text-field.js';
import 'mdui/components/switch.js';
import 'mdui/components/card.js';
import 'mdui/components/top-app-bar.js';
import 'mdui/components/top-app-bar-title.js';
import 'mdui/components/dropdown.js';
import 'mdui/components/menu.js';
import 'mdui/components/menu-item.js';
import 'mdui/components/circular-progress.js';
import 'mdui/components/slider.js';
import 'mdui/components/chip.js';
import 'mdui/components/linear-progress.js';

import zhCN from 'ant-design-vue/es/locale/zh_CN';

</script>

<template>
  <a-config-provider :locale="zhCN">
    <mdui-top-app-bar>
      <mdui-top-app-bar-title>BatteryAngle 配置</mdui-top-app-bar-title>
      <span style="display: flex; align-items: center;">
        重新连接
        <mdui-button-icon @click="reconnectStatus" :loading="isConnecting">
          <mdui-icon-refresh></mdui-icon-refresh>
        </mdui-button-icon>
      </span>
    </mdui-top-app-bar>
    <div style="max-width: 1000px; margin: 0 auto; padding: 16px;">
      <div style="display: flex; gap: 16px; flex-wrap: wrap;">
        <!-- 配置卡片 -->
        <mdui-card class="config-card">
          <mdui-card-content class="card-content">
            <h2>WiFi 配置</h2>
            <!-- WiFi 信息 -->
            <span style="display: flex; align-items: center; gap: 8px;">
              <!-- 状态未知组件 -->
              <span v-if="!deviceStatus" class="wifi-status-label">
                <mdui-icon-wifi-find></mdui-icon-wifi-find>
                <span style="color: rgb(var(--mdui-color-warnin)g)">状态未知</span>
              </span>

              <!-- 已连接组件 -->
              <span v-else-if="deviceStatus.sta_conn_status === '已连接'" class="wifi-status-label">
                <mdui-icon-wifi></mdui-icon-wifi>
                <div style="color: rgb(var(--mdui-color-succes)s)">已连接</div>
              </span>

              <!-- 未连接组件 -->
              <span v-else class="wifi-status-label">
                <mdui-icon-wifi-off></mdui-icon-wifi-off>
                <span style="color: rgb(var(--mdui-color-error))">未连接</span>
              </span>

              <span>{{ deviceStatus?.ip || '--' }}</span>
            </span>
            <!-- WiFi 配置 -->
            <mdui-dropdown trigger="hover" :open="wifiDropdownOpen" @open="loadWifiNetworks">
              <mdui-text-field slot="trigger" label="WiFi 网络名称（SSID）" variant="outlined"
                :value="deviceConfig.wifi_sta_ssid" @input="deviceConfig.wifi_sta_ssid = $event.target.value"
                @focus="wifiDropdownOpen = true">
              </mdui-text-field>
              <mdui-menu>
                <mdui-menu-item v-if="wifiNetworks.length === 0">
                  <mdui-circular-progress class="inline-loader" style="height: 1em;"></mdui-circular-progress>
                  WiFi 扫描中……
                </mdui-menu-item>
                <mdui-menu-item v-for="network in wifiNetworks" :key="network"
                  @click="deviceConfig.wifi_sta_ssid = network">
                  {{ network }}
                </mdui-menu-item>
              </mdui-menu>
            </mdui-dropdown>

            <mdui-text-field label="WiFi 网络密码" type="password" variant="outlined" toggle-password
              :value="deviceConfig.wifi_sta_password" @input="deviceConfig.wifi_sta_password = $event.target.value">
            </mdui-text-field>

            <mdui-button color="primary" variant="filled" @click="submitConfig">
              应用
            </mdui-button>
          </mdui-card-content>
        </mdui-card>

        <!-- 电源控制卡片 -->
        <mdui-card class="power-card" clickable
          @click="relaySwitchState = !relaySwitchState; updateRelaySwitch(); reconnectStatus();">
          <mdui-card-content class="card-content">
            <h2>电源控制</h2>
            <div v-if="!relaySwitchState" style="display: flex; flex-direction: column; align-items: center;">
              <mdui-icon-power-off style="height: 100%; width: 100%; max-width: 10rem;"></mdui-icon-power-off>
              <div style="font-size: larger;">已关闭</div>
              <div>频率：{{ deviceStatus?.frequency || '--' }}</div>
            </div>
            <div v-else style="display: flex; flex-direction: column; align-items: center;">
              <mdui-icon-power style="height: 100%; width: 100%; max-width: 10rem;"></mdui-icon-power>
              <div style="font-size: larger;">已开启</div>
              <div>频率：{{ deviceStatus?.frequency || '--' }}</div>
            </div>
          </mdui-card-content>
        </mdui-card>

        <!-- 状态卡片 -->
        <mdui-card class="schedule-card">
          <mdui-card-content class="card-content" style="min-width: 300px;">
            <div style="display: flex; justify-content: space-between; align-items: end;">
              <h2>周期控制</h2>
              <a-tag v-if="deviceConfig.relay_schedule_on && deviceConfig.relay_schedule_off" color="blue">已启用</a-tag>
              <a-tag v-else color="red">未启用</a-tag>
            </div>
            <div class="mdui-typo mdui-typo-title">
              开启时间：
              <a-time-picker v-model:value="relayScheduleText.on" value-format="HH:mm:ss" />
            </div>
            <div class="mdui-typo mdui-typo-title">
              关闭时间：
              <a-time-picker v-model:value="relayScheduleText.off" value-format="HH:mm:ss" />
            </div>
            <mdui-linear-progress v-if="deviceStatus"
              :value="deviceStatus.secs % (deviceConfig.relay_schedule_on + deviceConfig.relay_schedule_off)"
              :max="deviceConfig.relay_schedule_on + deviceConfig.relay_schedule_off"></mdui-linear-progress>
          </mdui-card-content>
        </mdui-card>
      </div>
    </div>
  </a-config-provider>
</template>


<style scoped>
h2 {
  margin-bottom: 0;
}

.card-content {
  padding: 8px;
  gap: 10px;
  display: grid;
}

.wifi-status-label {
  display: flex;
  align-items: center;
  gap: 8px;
}
</style>