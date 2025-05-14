export class DeviceConfig {
  wifi_sta_ssid: string = '';
  wifi_sta_password: string = '';
  relay_schedule_on: number = 0;
  relay_schedule_off: number = 0;
  lbm_smart_enabled: boolean = false;
  lbm_smart_upper_freq: number = 0;
}

enum wl_status_t {
  WL_NO_SHIELD = 255, // 没有 WiFi shield
  WL_IDLE_STATUS = 0, // 临时状态，等待扫描和连接
  WL_NO_SSID_AVAIL = 1, // 没有可用的 SSID
  WL_SCAN_COMPLETED = 2, // 扫描网络完成
  WL_CONNECTED = 3, // 成功连接
  WL_CONNECT_FAILED = 4, // 连接失败
  WL_CONNECTION_LOST = 5, // 连接丢失
  WL_DISCONNECTED = 6, // 已断开连接
  WL_CONNECTING = 7, // 正在连接
}

enum lbm_state_info {
  DISABLED,
  WAITING_RISING,
  WAITING_DROPPING,
  PREPARING_FOR_CHECKING_FREQ,
  CHECKING_FREQ_IN_DROPPING,
}

export class DeviceStatus {
  millis: number = 0; // 设备运行时间，单位 毫秒
  sta_conn_status: wl_status_t = wl_status_t.WL_IDLE_STATUS;
  ip?: string = undefined;
  frequency?: number = undefined;
  btn_pressed: boolean = false;
  relay_state: boolean = true;
  lbm_smart_info: lbm_state_info = lbm_state_info.WAITING_RISING;
}

export interface ApiResponse {
  message: string;
  success?: boolean;
}
