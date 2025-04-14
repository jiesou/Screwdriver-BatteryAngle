export interface DeviceConfig {
  wifi_sta_ssid: string;
  wifi_sta_password: string;
  relay_schedule_on: number; // 单位秒。ts 中的 number 是 64 位 IEEE 754 双精度浮点数。esp8266 中采用 unsigned long 是 32 位整数
  relay_schedule_off: number;
}

export interface DeviceStatus {
  millis: number; // 设备运行时间，单位 毫秒
  sta_conn_status: string;
  ip?: string;
  frequency?: string;
  btn_pressed: boolean;
  relay_state: boolean;
}

export interface ApiResponse {
  message: string;
  success?: boolean;
}
