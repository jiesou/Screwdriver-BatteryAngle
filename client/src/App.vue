<script setup lang="ts">
import { ref, onMounted } from 'vue';
import { useConfigService } from './services/configService';
import { useStatusService } from './services/statusService';
import type { WifiConfig, DeviceStatus } from './types';

import 'mdui/mdui.css';
import 'mdui';

// 响应式状态
const wifiConfig = ref<WifiConfig>({
  wifi_sta_ssid: '',
  wifi_sta_password: ''
});
const deviceStatus = ref<DeviceStatus | null>(null);
const wifiNetworks = ref<string[]>([]);
const isLoading = ref(false);
const menuOpen = ref(false);
const relaySwitchState = ref(false);

// 服务
const { fetchConfig, setWifiConfig, fetchWifiNetworks, setRelaySwitch } = useConfigService();
const { connectStatus, disconnectStatus } = useStatusService();

// 加载配置
const loadConfig = async () => {
  try {
    const config = await fetchConfig();
    wifiConfig.value = config;
  } catch (error) {
    console.error("Error loading config:", error);
  }
};

// 加载WiFi网络
const loadWifiNetworks = async () => {
  try {
    wifiNetworks.value = await fetchWifiNetworks();
  } catch (error) {
    console.error("Error loading WiFi networks:", error);
    wifiNetworks.value = [];
  }
};

// 提交表单
const submitForm = async () => {
  try {
    const result = await setWifiConfig(wifiConfig.value);
    mdui.snackbar({ message: result.message });
  } catch (error) {
    console.error("Error:", error);
  }
};

// 更新继电器开关状态
const updateRelaySwitch = async () => {
  try {
    const result = await setRelaySwitch(relaySwitchState.value);
    mdui.snackbar({ message: result.message });
  } catch (error) {
    console.error("Error:", error);
  }
};

// 重新连接状态
const reconnectStatus = async () => {
  isLoading.value = true;
  disconnectStatus();
  
  const updateStatusCallback = (status: DeviceStatus) => {
    deviceStatus.value = status;
    relaySwitchState.value = status.relay_state;
  };
  
  await connectStatus(updateStatusCallback);
  isLoading.value = false;
};

// 选择WiFi网络
const selectWifiNetwork = (ssid: string) => {
  wifiConfig.value.wifi_sta_ssid = ssid;
  menuOpen.value = false;
};

onMounted(() => {
  loadConfig();
  reconnectStatus();
});
</script>

<template>
  <mdui-top-app-bar>
    <mdui-top-app-bar-title>BatteryAngle 配置</mdui-top-app-bar-title>
  </mdui-top-app-bar>

  <div style="max-width: 800px; margin: 0 auto; padding: 16px;">
    <div style="display: flex; gap: 16px; flex-wrap: wrap;">
      <!-- 配置卡片 -->
      <mdui-card style="margin: 16px 0; flex: 1 1 300px; min-width: 280px;">
        <mdui-card-content style="padding: 8px; gap: 10px; display: grid;">
          <mdui-dropdown trigger="hover" :open="menuOpen" @open="loadWifiNetworks">
            <mdui-text-field 
              slot="trigger" 
              label="WiFi 网络名称（SSID）"
              variant="outlined"
              v-model="wifiConfig.wifi_sta_ssid"
            ></mdui-text-field>
            <mdui-menu>
              <mdui-menu-item v-if="wifiNetworks.length === 0">
                WiFi 扫描中……
              </mdui-menu-item>
              <mdui-menu-item 
                v-for="network in wifiNetworks" 
                :key="network"
                @click="selectWifiNetwork(network)"
              >
                {{ network }}
              </mdui-menu-item>
            </mdui-menu>
          </mdui-dropdown>
          
          <mdui-text-field 
            label="WiFi 网络密码" 
            type="password" 
            variant="outlined"
            toggle-password
            v-model="wifiConfig.wifi_sta_password"
          ></mdui-text-field>
          
          <mdui-button @click="submitForm">提交</mdui-button>
          
          <div style="margin-top: 10px; display: flex; align-items: center;"> 
            电源开关 
            <mdui-switch 
              style="margin-left: 10px;" 
              v-model="relaySwitchState"
              @change="updateRelaySwitch"
            />
          </div>
        </mdui-card-content>
      </mdui-card>

      <!-- 状态卡片 -->
      <mdui-card style="flex: 1 1 300px; min-width: 280px; margin: 16px 0;">
        <mdui-card-content style="padding-left: 10px; display: block;">
          <div style="display: flex; align-items: center;">
            <h3>状态</h3>
            <mdui-button-icon @click="reconnectStatus" :loading="isLoading">
              <svg xmlns="http://www.w3.org/2000/svg" width="24" height="24" fill="currentColor" viewBox="0 0 16 16">
                <path fill-rule="evenodd" d="M8 3a5 5 0 1 0 4.546 2.914.5.5 0 0 1 .908-.417A6 6 0 1 1 8 2z" />
                <path d="M8 4.466V.534a.25.25 0 0 1 .41-.192l2.36 1.966c.12.1.12.284 0 .384L8.41 4.658A.25.25 0 0 1 8 4.466" />
              </svg>
            </mdui-button-icon>
          </div>
          <div style="display: flex; align-items: center; gap: 8px;">
            <div v-if="deviceStatus" style="flex: 1; margin-top: 0; padding: 8px;">
              <span :style="{ color: getStatusColor() }">
                <template v-if="deviceStatus.sta_conn_status === '已连接'">
                  公用 WiFi 已连接！IP: {{ deviceStatus.ip }}
                </template>
                <template v-else-if="deviceStatus.sta_conn_status">
                  公用 WiFi {{ deviceStatus.sta_conn_status }}
                </template>
                <template v-else>
                  公用 WiFi 状态待更新
                </template>
                <br>频率: {{ deviceStatus.frequency || '--' }}
                <br>{{ deviceStatus.btn_pressed ? "按钮按下" : "按钮未按下" }}
                <br>{{ deviceStatus.relay_state ? "电源开关已打开" : "电源开关已关闭" }}
              </span>
            </div>
            <div v-else style="flex: 1; margin-top: 0; padding: 8px;">
              公用 WiFi 状态待更新
            </div>
          </div>
        </mdui-card-content>
      </mdui-card>
    </div>
  </div>
</template>

<script lang="ts">
export default {
  methods: {
    getStatusColor() {
      if (!this.deviceStatus) return "var(--mdui-color-warning)";
      
      if (this.deviceStatus.sta_conn_status === "已连接") {
        return "var(--mdui-color-success)";
      } else if (this.deviceStatus.sta_conn_status) {
        return "var(--mdui-color-error)";
      }
      
      return "var(--mdui-color-warning)";
    }
  }
}
</script>

<style scoped>
/* 保留原有样式 */
</style>
