import type { WifiConfig, ApiResponse } from '../types';

export function useConfigService() {
  const fetchConfig = async (): Promise<WifiConfig> => {
    const response = await fetch('/get_config');
    if (!response.ok) {
      throw new Error('Failed to fetch config');
    }
    return await response.json() as WifiConfig;
  };

  const setWifiConfig = async (config: WifiConfig): Promise<ApiResponse> => {
    const response = await fetch('/set_wifi_ssid_and_passwd', {
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
    const response = await fetch('/fetch_nearby_wifi_ssids');
    if (!response.ok) {
      throw new Error('Failed to fetch WiFi networks');
    }
    return await response.json() as string[];
  };

  const setRelaySwitch = async (state: boolean): Promise<ApiResponse> => {
    const response = await fetch('/set_relay_switch', {
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
