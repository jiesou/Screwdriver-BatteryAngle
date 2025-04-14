export interface DeviceConfig {
  wifi_sta_ssid: string;
  wifi_sta_password: string;
  relay_schedule_on: number; // 单位秒。ts 中的 number 是 64 位 IEEE 754 双精度浮点数。esp8266 中采用 unsigned long 是 32 位整数
  relay_schedule_off: number;
}

enum wl_status_t {
    WL_NO_SHIELD        = 255,   // 没有 WiFi shield
    WL_IDLE_STATUS      = 0,     // 临时状态，等待扫描和连接
    WL_NO_SSID_AVAIL    = 1,     // 没有可用的 SSID
    WL_SCAN_COMPLETED   = 2,     // 扫描网络完成
    WL_CONNECTED        = 3,     // 成功连接
    WL_CONNECT_FAILED   = 4,     // 连接失败
    WL_CONNECTION_LOST  = 5,     // 连接丢失
    WL_DISCONNECTED     = 6,      // 已断开连接
    WL_CONNECTING       = 7,     // 正在连接
}

export interface DeviceStatus {
  millis: number; // 设备运行时间，单位 毫秒
  sta_conn_status: wl_status_t;
  ip?: string;
  frequency?: string;
  btn_pressed: boolean;
  relay_state: boolean;
}

export interface ApiResponse {
  message: string;
  success?: boolean;
}
