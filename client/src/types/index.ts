export interface WifiConfig {
  wifi_sta_ssid: string;
  wifi_sta_password: string;
}

export interface DeviceStatus {
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
