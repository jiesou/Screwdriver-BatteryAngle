import type { DeviceStatus } from '../types';

let statusController: AbortController | null = null;

export function useStatusService() {
  const connectStatusTo = async (
    callback: (status: DeviceStatus) => void
  ): Promise<void> => {
    // 断开旧连接
    if (statusController) {
      statusController.abort();
    }
    
    // 创建新连接
    statusController = new AbortController();
    
    try {
      const response = await fetch('/status', { 
        signal: statusController.signal 
      });
      
      const reader = response.body!.getReader();
      const decoder = new TextDecoder();
      let buffer = '';

      while (true) {
        const { done, value } = await reader.read();
        if (done) break;
        
        buffer += decoder.decode(value, { stream: true });
        const lines = buffer.split('\n');
        buffer = lines.pop() || '';

        for (const line of lines) {
          try {
            if (line === "\r") continue;
            const data = JSON.parse(line.replace(/^data: /, ''));
            callback(data);
          } catch (error) {
            console.error("Error parsing status:", error);
          }
        }
      }
    } catch (error: unknown) {
      if (error instanceof Error && error.name !== 'AbortError') {
        console.error("Status connection error:", error);
      }
    }
  };

  const disconnectStatusService = (): void => {
    if (statusController) {
      statusController.abort();
      statusController = null;
    }
  };

  return {
    connectStatusTo,
    disconnectStatusService
  };
}
