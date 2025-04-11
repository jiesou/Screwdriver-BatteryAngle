import type { DeviceConfig, ApiResponse } from '../types';

export function useConfigService() {
  const fetchConfig = async (): Promise<DeviceConfig> => {
    const response = await fetch('/api/get_config');
    if (!response.ok) {
      throw new Error('Failed to fetch config');
    }

    if (await response.text() === 'null') {
      return {} as DeviceConfig;
    }
    return await response.json() as DeviceConfig;
  };

  const setWifiConfig = async (config: DeviceConfig): Promise<ApiResponse> => {
    const response = await fetch('/api/set_wifi_ssid_and_passwd', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(config)
    });
    
    if (!response.ok) {
      throw new Error('Failed to set WiFi config');
    }
    
    return await response.json() as ApiResponse;
  };

  const fetchWifiNetworks = async (): Promise<string[]> => {
    const response = await fetch('/api/fetch_nearby_wifi_ssids');
    if (!response.ok) {
      throw new Error('Failed to fetch WiFi networks');
    }
    return await response.json() as string[];
  };

  const setRelaySwitch = async (state: boolean): Promise<ApiResponse> => {
    const response = await fetch('/api/set_relay_switch', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ relay_switch: state })
    });
    
    if (!response.ok) {
      throw new Error('Failed to set relay switch');
    }
    
    return await response.json() as ApiResponse;
  };

  return {
    fetchConfig,
    setWifiConfig,
    fetchWifiNetworks,
    setRelaySwitch
  };
}
