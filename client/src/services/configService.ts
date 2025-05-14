import type { DeviceConfig, ApiResponse } from '../types/device';

export function useConfigService() {
  const fetchConfig = async (): Promise<DeviceConfig> => {
    const response = await fetch('/api/get_config');
    if (!response.ok) {
      throw new Error('Failed to fetch config');
    }

    const text = await response.text();
    if (text === 'null') {
      return {} as DeviceConfig;
    }
    return JSON.parse(text) as DeviceConfig;
  };

  const setConfig = async (config: DeviceConfig): Promise<ApiResponse> => {
    const response = await fetch('/api/set_config', {
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
    setConfig,
    fetchWifiNetworks,
    setRelaySwitch
  };
}
