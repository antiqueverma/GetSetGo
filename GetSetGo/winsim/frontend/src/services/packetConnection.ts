import { BasePacket, ConnectionConfig, ConnectionStats, ModuleId, PacketLogEntry } from '../types/modules';

export type PacketCallback = (packet: BasePacket) => void;
export type StatsCallback = (stats: ConnectionStats) => void;

class PacketConnectionService {
  private ws: WebSocket | null = null;
  private wsUrl: string = 'ws://localhost:8000/ws';
  private subscribers: Map<string, Set<PacketCallback>> = new Map();
  private statsSubscribers: Set<StatsCallback> = new Set();
  private packetLog: PacketLogEntry[] = [];
  private maxLogEntries: number = 200;
  private reconnectTimer: any = null;
  private isAutoReconnectEnabled: boolean = true;
  private isSimulating: boolean = false;
  private simInterval: any = null;

  public config: ConnectionConfig = {
    ip: '127.0.0.1',
    port: 9000,
    mode: 'server',
    autoReconnect: true,
  };

  public stats: ConnectionStats = {
    backendConnected: false,
    mcuConnected: false,
    bytesReceived: 0,
    bytesSent: 0,
    packetsReceived: 0,
    packetsSent: 0,
  };

  constructor() {
    this.initWebSocket();
  }

  public setConfig(newConfig: Partial<ConnectionConfig>) {
    this.config = { ...this.config, ...newConfig };
    this.isAutoReconnectEnabled = this.config.autoReconnect;
    // Notify backend about new config
    this.sendControlMessage('update_config', this.config);
  }

  public initWebSocket(url?: string) {
    if (url) this.wsUrl = url;
    if (this.ws) {
      try {
        this.ws.close();
      } catch (e) {
        // ignore
      }
    }

    try {
      this.ws = new WebSocket(this.wsUrl);

      this.ws.onopen = () => {
        this.stats.backendConnected = true;
        this.notifyStats();
        // Request current config and status from backend
        this.sendControlMessage('get_status', {});
      };

      this.ws.onclose = () => {
        this.stats.backendConnected = false;
        this.stats.mcuConnected = false;
        this.notifyStats();
        if (this.isAutoReconnectEnabled) {
          clearTimeout(this.reconnectTimer);
          this.reconnectTimer = setTimeout(() => this.initWebSocket(), 3000);
        }
      };

      this.ws.onerror = () => {
        this.stats.backendConnected = false;
        this.notifyStats();
      };

      this.ws.onmessage = (event) => {
        try {
          const raw = event.data;
          this.stats.bytesReceived += (typeof raw === 'string' ? raw.length : 0);
          const data = JSON.parse(raw);

          // Check if this is an internal server message
          if (data._type === 'server_status') {
            this.stats.mcuConnected = data.mcuConnected;
            this.stats.clientAddress = data.clientAddress;
            if (data.config) {
              this.config = { ...this.config, ...data.config };
            }
            this.notifyStats();
            return;
          }

          // Module packet
          if (data.module) {
            this.stats.packetsReceived++;
            this.stats.lastPacketTimestamp = new Date().toLocaleTimeString();
            this.addLogEntry('in', data.module, data.action || 'update', raw);
            this.dispatchPacket(data);
            this.notifyStats();
          }
        } catch (e) {
          console.error('[PacketConnection] Failed to parse message:', e);
        }
      };
    } catch (e) {
      console.warn('[PacketConnection] WebSocket creation error:', e);
    }
  }

  public subscribe(moduleId: ModuleId | 'all', callback: PacketCallback) {
    if (!this.subscribers.has(moduleId)) {
      this.subscribers.set(moduleId, new Set());
    }
    this.subscribers.get(moduleId)!.add(callback);

    return () => {
      this.subscribers.get(moduleId)?.delete(callback);
    };
  }

  public subscribeStats(callback: StatsCallback) {
    this.statsSubscribers.add(callback);
    callback(this.stats);
    return () => {
      this.statsSubscribers.delete(callback);
    };
  }

  private dispatchPacket(packet: BasePacket) {
    // Notify module-specific listeners
    const specific = this.subscribers.get(packet.module);
    if (specific) {
      specific.forEach((cb) => {
        try {
          cb(packet);
        } catch (err) {
          console.error(`Error in subscriber for ${packet.module}:`, err);
        }
      });
    }

    // Notify universal listeners
    const all = this.subscribers.get('all');
    if (all) {
      all.forEach((cb) => {
        try {
          cb(packet);
        } catch (err) {
          console.error('Error in universal subscriber:', err);
        }
      });
    }
  }

  public sendPacket(packet: BasePacket): boolean {
    const raw = JSON.stringify(packet);
    this.stats.bytesSent += raw.length;
    this.stats.packetsSent++;
    this.addLogEntry('out', packet.module, packet.action || 'send', raw);
    this.notifyStats();

    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      this.ws.send(raw);
      return true;
    } else {
      console.warn('[PacketConnection] WebSocket not open. Packet queued or simulated:', packet);
      return false;
    }
  }

  public sendControlMessage(action: string, payload: any) {
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      this.ws.send(JSON.stringify({ _type: 'control', action, ...payload }));
    }
  }

  public connectTcp(config?: Partial<ConnectionConfig>) {
    if (config) this.setConfig(config);
    this.sendControlMessage('start_tcp', this.config);
  }

  public disconnectTcp() {
    this.sendControlMessage('stop_tcp', {});
  }

  private notifyStats() {
    this.statsSubscribers.forEach((cb) => cb({ ...this.stats }));
  }

  private addLogEntry(direction: 'in' | 'out', module: ModuleId, action: string, rawJson: string) {
    const entry: PacketLogEntry = {
      id: Math.random().toString(36).substring(2, 9),
      timestamp: new Date().toLocaleTimeString() + '.' + String(new Date().getMilliseconds()).padStart(3, '0'),
      direction,
      module,
      action,
      rawJson,
    };
    this.packetLog.unshift(entry);
    if (this.packetLog.length > this.maxLogEntries) {
      this.packetLog.pop();
    }
  }

  public getPacketLogs(): PacketLogEntry[] {
    return [...this.packetLog];
  }

  public clearLogs() {
    this.packetLog = [];
  }

  /**
   * Built-in Client Simulation:
   * Generates realistic MCU driver packets for testing the UI immediately!
   */
  public toggleSimulation(enable?: boolean): boolean {
    const target = enable !== undefined ? enable : !this.isSimulating;
    this.isSimulating = target;

    if (this.isSimulating) {
      let tick = 0;
      this.stats.mcuConnected = true;
      this.stats.clientAddress = '127.0.0.1:54321 [SIM]';
      this.notifyStats();

      // Initial packets
      this.dispatchPacket({
        module: 'console',
        action: 'tx',
        text: '\r\n[FreeRTOS-WinSim] Bootloader v2.4 initialized\r\n[FreeRTOS-WinSim] FreeRTOS Kernel v10.4.3 started\r\n[FreeRTOS-WinSim] All peripheral drivers registered successfully.\r\n'
      });

      this.dispatchPacket({
        module: 'display',
        action: 'write',
        line: 0,
        text: 'FreeRTOS WinSim',
        backlight: true
      });
      this.dispatchPacket({
        module: 'display',
        action: 'write',
        line: 1,
        text: 'Status: ONLINE'
      });

      this.simInterval = setInterval(() => {
        tick++;

        // GPIO toggle
        if (tick % 2 === 0) {
          const pin = tick % 8;
          const state = (tick % 4 === 0) ? 1 : 0;
          this.dispatchPacket({
            module: 'gpio',
            action: 'write',
            port: 'A',
            pin: pin,
            state: state,
            direction: 'out'
          });
        }

        // ADC update
        if (tick % 3 === 0) {
          const ch = (tick % 4);
          const raw = Math.floor(2048 + 1500 * Math.sin(tick * 0.4));
          const voltage = Number(((raw / 4095) * 3.3).toFixed(2));
          this.dispatchPacket({
            module: 'adc',
            action: 'update',
            channel: ch,
            raw,
            voltage
          });
        }

        // Console message periodically
        if (tick % 7 === 0) {
          this.dispatchPacket({
            module: 'console',
            action: 'tx',
            text: `[Task_Sensor] ADC read channel 0 => ${(Math.random() * 3.3).toFixed(2)}V (tick #${tick})\r\n`
          });
        }

        // EEPROM write periodically
        if (tick % 10 === 0) {
          const addr = (tick * 4) % 256;
          const data = [Math.floor(Math.random() * 256), Math.floor(Math.random() * 256), 0xAA, 0x55];
          this.dispatchPacket({
            module: 'eeprom',
            action: 'write',
            address: addr,
            data
          });
        }

        // Display update
        if (tick % 5 === 0) {
          this.dispatchPacket({
            module: 'display',
            action: 'write',
            line: 1,
            text: `T:${(22.5 + Math.sin(tick * 0.3) * 5).toFixed(1)}C Pk:${this.stats.packetsReceived}`
          });
        }
      }, 700);
    } else {
      if (this.simInterval) {
        clearInterval(this.simInterval);
        this.simInterval = null;
      }
      this.stats.mcuConnected = false;
      this.notifyStats();
    }

    return this.isSimulating;
  }

  public isSimulatorActive(): boolean {
    return this.isSimulating;
  }
}

export const packetConnection = new PacketConnectionService();
