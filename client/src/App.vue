<script setup lang="ts">
import { ref, onMounted, computed } from 'vue';
import { useConfigService } from './services/configService';
import { useStatusService } from './services/statusService';
import { DeviceConfig, DeviceStatus } from './types/device';

import { snackbar } from 'mdui/functions/snackbar.js';



// 响应式状态
const deviceConfig = ref(new DeviceConfig());
const deviceStatus = ref<DeviceStatus | null>(null);
const wifiNetworks = ref<string[]>([]);
const isConnecting = ref<{
  config: boolean
  status: boolean
}>({
  config: true,
  status: true
});
const wifiDropdownOpen = ref<boolean>(false);
const relaySwitchState = ref<boolean>(false);

// 服务
const { fetchConfig, setConfig, fetchWifiNetworks, setRelaySwitch } = useConfigService();
const { connectStatusTo, disconnectStatusService } = useStatusService();

// 加载配置
async function loadConfig() {
  try {
    isConnecting.value.config = true;
    const config = await fetchConfig();
    isConnecting.value.config = false;

    deviceConfig.value = { ...deviceConfig.value, ...config };
  } catch (error) {
    snackbar({ message: '加载配置失败，请检查网络连接或设备状态。', closeable: true });
    console.error("Error loading config:", error);
  }
};

// 加载WiFi网络
async function loadWifiNetworks() {
  try {
    wifiNetworks.value = await fetchWifiNetworks();
  } catch (error) {
    snackbar({ message: '加载 WiFi 网络失败，请检查网络连接或设备状态。', closeable: true });
    console.error("Error loading WiFi networks:", error);
    wifiNetworks.value = [];
  }
};


// 节流
let lastCall = 0;
function throttle(func: Function, delay: number) {
  return function (...args: any[]) {
    const now = Date.now();
    if (now - lastCall < delay) return;
    lastCall = now;
    return func(...args);
  };
};

// 500ms内连续调用只会执行一次
const submitConfig = throttle(async () => {
  try {
    const result = await setConfig(deviceConfig.value);
  } catch (error) {
    snackbar({ message: '配置失败，请检查网络连接或设备状态。' });
    console.error("Error:", error);
  }
}, 500);

// 更新继电器开关状态
async function updateRelaySwitch() {
  try {
    const result = await setRelaySwitch(relaySwitchState.value);
  } catch (error) {
    snackbar({ message: '更新继电器状态失败，请检查网络连接或设备状态。' });
    console.error("Error:", error);
  }
};

// 重新连接状态
async function reconnectStatus() {
  isConnecting.value.status = true;
  disconnectStatusService();

  await connectStatusTo((status: DeviceStatus) => {
    isConnecting.value.status = false;
    deviceStatus.value = status;
    relaySwitchState.value = status.relay_state;
  });
};

onMounted(() => {
  reconnectStatus();
  loadConfig();
});

import '@mdui/icons/power.js';
import '@mdui/icons/power-off.js';
import '@mdui/icons/refresh.js';
import '@mdui/icons/wifi.js';
import '@mdui/icons/wifi-off.js';
import '@mdui/icons/wifi-find.js';
import '@mdui/icons/insights.js';

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
import 'mdui/components/tooltip.js';
import zhCN from 'ant-design-vue/es/locale/zh_CN';


const positionInCycle = computed(() => {
  if (!deviceStatus.value || !deviceConfig.value) return 0;
  const cycleDuration = deviceConfig.value.relay_schedule_on + deviceConfig.value.relay_schedule_off;
  const elapsedSeconds = deviceStatus.value.millis / 1000;
  return elapsedSeconds % cycleDuration;
});

const handlePowerSwitchClick = () => {
  if (relayScheduleEnabled.value) {
    snackbar({ message: "周期控制开启时无法切换电源开关", closeable: true });
    return;
  }
  if (lbmSmartEnabled.value) {
    snackbar({ message: "智能控制开启时无法切换电源开关", closeable: true })
    return;
  }
  relaySwitchState.value = !relaySwitchState.value;
  updateRelaySwitch();
  reconnectStatus();
}

const relayScheduleText = computed({
  get: () => {
    return {
      on: new Date(deviceConfig.value.relay_schedule_on * 1000).toISOString().slice(11, 19),
      off: new Date(deviceConfig.value.relay_schedule_off * 1000).toISOString().slice(11, 19),
    };
  },
  set: (val: { on: string; off: string }) => {
    deviceConfig.value.relay_schedule_on = new Date(`1970-01-01T${val.on}Z`).getTime() / 1000;
    deviceConfig.value.relay_schedule_off = new Date(`1970-01-01T${val.off}Z`).getTime() / 1000;
    submitConfig();
  }
});


// 开启关闭周期控制会主动更新 relayScheduleText 周期控制时长
const relayScheduleEnabled = computed({
  get: () => {
    return deviceConfig.value.relay_schedule_on > 0 && deviceConfig.value.relay_schedule_off > 0;
  },
  set: (val: boolean) => {
    if (val) {
      if (lbmSmartEnabled.value) lbmSmartEnabled.value = false;
      relayScheduleText.value = {
        on: "00:01:00",
        off: "00:01:00",
      };
    } else {
      relayScheduleText.value = {
        on: "00:00:00",
        off: "00:00:00",
      };
    }
  }
});
const lbmSmartEnabled = computed({
  get: () => {
    return (deviceConfig.value.lbm_smart_enabled == true);
  },
  set: (val: boolean) => {
    if (relayScheduleEnabled.value) relayScheduleEnabled.value = false;
    deviceConfig.value.lbm_smart_enabled = val;
    // 重复提交被节流，导致 string 的更新不触发到 deviceConfig 里，需要自己更新
    submitConfig();
  }
});
// 电量表征 = 100 / 频率
const deviceConfigUpperBattLevelFp = computed({
  get: () => deviceConfig.value.lbm_smart_upper_freq ? Number((100 / deviceConfig.value.lbm_smart_upper_freq).toFixed(2)) : 0,
  set: (val: string) => {
    const num = Number(val);
    if (num > 0) {
      deviceConfig.value.lbm_smart_upper_freq = Number((100 / num).toFixed(2));
    }
  }
});
</script>

<template>
  <a-config-provider :locale="zhCN">
    <mdui-top-app-bar>
      <mdui-top-app-bar-title>电池天使 配置</mdui-top-app-bar-title>
      <span style="display: flex; align-items: center;">
        重新连接
        <mdui-button-icon @click="reconnectStatus(); loadConfig();"
          :loading="isConnecting.status || isConnecting.config">
          <mdui-icon-refresh></mdui-icon-refresh>
        </mdui-button-icon>
      </span>
    </mdui-top-app-bar>
    <div style="max-width: 1200px; margin: auto; padding: 16px;">
      <div style="display: flex; gap: 16px; flex-wrap: wrap;">
        <!-- 配置卡片 -->
        <!-- <mdui-card class="config-card">
          <mdui-card-content class="card-content">
            <h2>WiFi 配置</h2>
            <span style="display: flex; align-items: center; gap: 8px;">
              <template v-if="!deviceStatus">
                <mdui-icon-wifi-find style="color: gray;"></mdui-icon-wifi-find>
                <span>无状态</span>
              </template>
<template v-else-if="deviceStatus.sta_conn_status === 3">
                <mdui-icon-wifi style="color: green;"></mdui-icon-wifi>
                <span>已连接</span>
              </template>
<template v-else-if="deviceStatus.sta_conn_status === 255">
                <mdui-icon-wifi-off style="color: gray;"></mdui-icon-wifi-off>
                <span>没有 WiFi shield</span>
              </template>
<template v-else-if="deviceStatus.sta_conn_status === 0">
                <mdui-icon-wifi-find style="color: chocolate;"></mdui-icon-wifi-find>
                <span>正在扫描</span>
              </template>
<template v-else-if="deviceStatus.sta_conn_status === 1">
                <mdui-icon-wifi-off style="color: red;"></mdui-icon-wifi-off>
                <span>没有可用 SSID</span>
              </template>
<template v-else-if="deviceStatus.sta_conn_status === 2">
                <mdui-icon-wifi-find style="color: blue;"></mdui-icon-wifi-find>
                <span>扫描完成</span>
              </template>
<template v-else-if="deviceStatus.sta_conn_status === 4">
                <mdui-icon-wifi-off style="color: red;"></mdui-icon-wifi-off>
                <span>连接失败</span>
              </template>
<template v-else-if="deviceStatus.sta_conn_status === 5">
                <mdui-icon-wifi-off style="color: red;"></mdui-icon-wifi-off>
                <span>连接丢失</span>
              </template>
<template v-else-if="deviceStatus.sta_conn_status === 6">
                <mdui-icon-wifi-off style="color: red;"></mdui-icon-wifi-off>
                <span>已断开连接</span>
              </template>
<template v-else-if="deviceStatus.sta_conn_status === 7">
                <mdui-icon-wifi-find style="color: gray;"></mdui-icon-wifi-find>
                <span>正在更新连接</span>
              </template>
<template v-else>
                <mdui-icon-wifi-find style="color: gray;"></mdui-icon-wifi-find>
                <span>处理中</span>
              </template>
<span>{{ deviceStatus?.ip || '--' }}</span>
</span>
<mdui-dropdown trigger="hover" :open="wifiDropdownOpen" @open="loadWifiNetworks">
  <mdui-text-field slot="trigger" label="WiFi 网络名称（SSID）" variant="outlined" :value="deviceConfig.wifi_sta_ssid"
    @input="deviceConfig.wifi_sta_ssid = $event.target.value" @focus="wifiDropdownOpen = true">
  </mdui-text-field>
  <mdui-menu>
    <mdui-menu-item v-if="wifiNetworks.length === 0">
      <mdui-circular-progress class="inline-loader" style="height: 1em;"></mdui-circular-progress>
      WiFi 扫描中……
    </mdui-menu-item>
    <mdui-menu-item v-for="network in wifiNetworks" :key="network" @click="deviceConfig.wifi_sta_ssid = network">
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
</mdui-card> -->

        <!-- 电源控制 -->
        <mdui-card class="power-card" clickable @click="handlePowerSwitchClick"
          :disabled="relayScheduleEnabled || lbmSmartEnabled">
          <mdui-card-content class="card-content">
            <h2>电源控制</h2>
            <div v-if="!relaySwitchState" style="display: flex; flex-direction: column; align-items: center;">
              <mdui-icon-power-off style="height: 100%; width: 100%; max-width: 15rem;"></mdui-icon-power-off>
              <mdui-switch></mdui-switch>
              <div style="font-size: larger;">已关闭</div>
              <div>当前电量表征：--</div>
            </div>
            <div v-else style="display: flex; flex-direction: column; align-items: center;">
              <mdui-icon-power style="height: 100%; width: 100%; max-width: 15rem;"></mdui-icon-power>
              <mdui-switch checked :disabled="relayScheduleEnabled || lbmSmartEnabled"></mdui-switch>
              <div style="font-size: larger;">已开启</div>
              <!-- 电量表征 = 100 / 频率 -->
              <div>当前电量表征：{{ deviceStatus?.frequency ? (100 / Number(deviceStatus.frequency)).toFixed(2) : '--' }}</div>
            </div>
            <mdui-chip v-if="lbmSmartEnabled">智能控制开启—单击设备上的按钮来强制充电</mdui-chip>
          </mdui-card-content>
        </mdui-card>

        <!-- LBM 智能充电控制 -->
        <mdui-card class="control-card">
          <mdui-card-content class="card-content">
            <mdui-tooltip variant="rich">
              <div style="display: flex; justify-content: space-between; align-items: end;">
                <h2>智能充电控制</h2>
                <mdui-switch :checked="lbmSmartEnabled"
                  @change="lbmSmartEnabled = ($event.target as HTMLInputElement).checked"></mdui-switch>
              </div>
              <div slot="headline"><strong>智能充电控制 [ 大电池模型 ]</strong></div>
              <div slot="content">通过动态电流感知，自动识别手机电池特性，并实时调整充放电时间。</div>
            </mdui-tooltip>
            <div style="display: flex; align-items: center; gap: 8px;">
              <mdui-icon-insights style="margin-left: 8px;"></mdui-icon-insights>
              <span>控制状态</span>
              <mdui-chip style="pointer-events: none;">
                {{ ['未开启智能控制', '充电中', '耗电中', '准备分析电池状态', '分析电池状态中', '未知'][(deviceStatus?.lbm_smart_info ?? 5)] }}
              </mdui-chip>
            </div>
            <mdui-tooltip variant="rich">
              <mdui-text-field label="基准电量表征" variant="outlined" :value="deviceConfigUpperBattLevelFp" type="decimal"
                @input="deviceConfigUpperBattLevelFp = $event.target.value" @change="submitConfig">
              </mdui-text-field>
              <div slot="headline"><strong>什么是 “基准电量表征”？</strong></div>
              <div slot="content">一般与所充电的设备电量正相关。智能充电控制 会将设备电量控制在 <strong>基准电量表征</strong> 附近</div>
            </mdui-tooltip>
          </mdui-card-content>
        </mdui-card>

        <!-- 周期控制 -->
        <mdui-card class="control-card">
          <mdui-card-content class="card-content">
            <div style="display: flex; justify-content: space-between; align-items: end;">
              <h2>周期控制</h2>
              <mdui-switch :checked="relayScheduleEnabled"
                @change="relayScheduleEnabled = ($event.target as HTMLInputElement).checked;"></mdui-switch>
            </div>

            <div class="mdui-typo mdui-typo-title">
              开启时长：
              <a-time-picker v-model:value="relayScheduleText.on" format="HH 时 mm 分 ss 秒" value-format="HH:mm:ss"
                :showNow="false" :allowClear="false" :disabled="!relayScheduleEnabled" />
            </div>
            <div class="mdui-typo mdui-typo-title">
              关闭时长：
              <a-time-picker v-model:value="relayScheduleText.off" format="HH 时 mm 分 ss 秒" value-format="HH:mm:ss"
                :showNow="false" :allowClear="false" :disabled="!relayScheduleEnabled" />
            </div>
            <div v-if="relayScheduleEnabled">
              <!-- 开启时的进度条 -->
              <div v-if="positionInCycle < deviceConfig.relay_schedule_on"
                style="display:flex; flex-direction: column; gap: 10px;">
                {{ new Date((deviceConfig.relay_schedule_on - positionInCycle) * 1000).toISOString().substr(11, 8) }}
                后关闭
                <mdui-linear-progress :value="deviceConfig.relay_schedule_on - positionInCycle"
                  :max="deviceConfig.relay_schedule_on">
                </mdui-linear-progress>
              </div>
              <!-- 关闭时的进度条 -->
              <div v-else style="display:flex; flex-direction: column; gap: 10px;">
                {{ new Date((deviceConfig.relay_schedule_off - (positionInCycle - deviceConfig.relay_schedule_on)) *
                  1000).toISOString().substr(11, 8) }}
                后开启
                <mdui-linear-progress :value="positionInCycle - deviceConfig.relay_schedule_on"
                  :max="deviceConfig.relay_schedule_off">
                </mdui-linear-progress>
              </div>
            </div>
          </mdui-card-content>
        </mdui-card>
      </div>
    </div>
  </a-config-provider>
</template>


<style scoped>
* {
  font-family: sans-serif;
  -webkit-tap-highlight-color: transparent;
}

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

/* 基本样式：在宽屏幕(桌面)上保持固定最大宽度 */
.power-card,
.control-card {
  width: 100%;
  min-width: 280px;
  max-width: 400px;
  flex-basis: 300px;
  /* 允许卡片长满空间 */
  flex-grow: 1;
}

/* 手机样式 */
@media (max-width: 900px) {

  .power-card,
  .control-card {
    max-width: none;
  }
}


/* 让 ant-time-picker 贴合 mdui 设计 */
.ant-picker {
  background-color: rgb(var(--mdui-color-surface));
  border-color: rgb(var(--mdui-color-outline));
  color: rgb(var(--mdui-color-on-surface));
  box-sizing: border-box;
}

.ant-picker-focused {
  border-color: rgb(var(--mdui-color-primary));
  box-shadow: 0 0 0 2px rgba(var(--mdui-color-primary-rgb), 0.2);
}
</style>