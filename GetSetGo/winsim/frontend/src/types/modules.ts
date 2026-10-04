export type ModuleId =
  | 'gpio'
  | 'console'
  | 'eeprom'
  | 'flash'
  | 'adc'
  | 'display'
  | 'timers'
  | 'can'
  | 'canopen'
  | 'events';

export type ArrangementMode = 'tile' | 'tabbed';
export type AppTheme = 'dark' | 'light';

export interface ModuleMetadata {
  id: ModuleId;
  name: string;
  shortDescription: string;
  icon: string;
  defaultVisible: boolean;
}

// ---------------- GPIO ----------------
export interface GpioPin {
  port: string;
  pin: number;
  state: 0 | 1;
  direction: 'in' | 'out';
  pull: 'none' | 'up' | 'down';
  label?: string;
  lastUpdated?: number;
}

export interface GpioState {
  pins: Record<string, GpioPin>;
  history: Array<{ timestamp: string; port: string; pin: number; state: 0 | 1; dir: string }>;
}

// ---------------- Console / UART ----------------
export interface ConsoleMessage {
  id: string;
  timestamp: string;
  direction: 'rx' | 'tx' | 'sys';
  text: string;
}

export interface ConsoleState {
  baudRate: number;
  messages: ConsoleMessage[];
  autoScroll: boolean;
}

// ---------------- EEPROM ----------------
export interface EepromState {
  size: number;
  data: number[];
  lastWriteAddress: number | null;
  lastWriteTime: number | null;
  writeCount: number;
}

// ---------------- Flash ----------------
export interface FlashSector {
  id: number;
  address: number;
  size: number;
  eraseCycles: number;
  data: number[];
}

export interface FlashState {
  totalSize: number;
  sectorSize: number;
  sectors: FlashSector[];
  activeSectorId: number;
  lastOperation: string;
}

// ---------------- ADC & Wave Generator ----------------
export type WaveType = 'square' | 'ramp' | 'sine' | 'cosine' | 'noise';

export interface WaveGenerator {
  id: string;
  name: string;
  enabled: boolean;
  targetChannel: number;
  type: WaveType;
  frequencyHz: number;
  minVoltage: number;
  maxVoltage: number;
  dutyCyclePct: number;
  symmetryPct: number;
  edgeTimeMs: number;
  phaseDeg: number;
}

export interface AdcChannel {
  channel: number;
  raw: number;
  voltage: number;
  name: string;
  mode: 'mcu' | 'user' | 'wave';
  history: number[];
  connectedWaveId?: string;
}

export interface AdcConfig {
  channelCount: number;
  vref: number;
  vmin: number;
  resolution: 10 | 12 | 16;
}

export interface AdcState {
  vref: number;
  vmin: number;
  resolution: 10 | 12 | 16;
  channels: AdcChannel[];
  waveGenerators: WaveGenerator[];
}

// ---------------- 16x2 Display ----------------
export type DisplayTheme = 'blue' | 'green' | 'amber' | 'dark';

export interface DisplayState {
  lines: [string, string];
  cursor: { row: number; col: number; visible: boolean; blink: boolean };
  backlight: boolean;
  theme: DisplayTheme;
  contrast: number;
}

// ---------------- Timers ----------------
export interface TimerChannel {
  id: number;
  mode: 'pwm1' | 'pwm2' | 'output_compare' | 'input_capture';
  ccr: number;
  outputState: 0 | 1;
  dutyCycle: number;
  enabled?: boolean;
}

export interface TimerUnit {
  id: string;
  name: string;
  bitWidth: 16 | 32;
  cnt: number;
  arr: number;
  psc: number;
  clockMhz: number;
  running: boolean;
  direction: 'up' | 'down';
  channels: TimerChannel[];
}

export interface TimersState {
  timers: TimerUnit[];
}

// ---------------- CAN Bus ----------------
export interface CanMessage {
  id: string;
  timestamp: string;
  sof: string;
  canId: number;
  canIdHex: string;
  isExtended: boolean;
  isRtr: boolean;
  dlc: number;
  data: number[];
  dataHex: string;
  dataAscii: string;
  crc: string;
  ack: boolean;
  eof: string;
  direction: 'rx' | 'tx';
}

export interface CanTxConfig {
  canId: string;
  isExtended: boolean;
  dlc: number;
  dataStr: string;
  trigger: 'manual' | 'periodic' | 'rx_trigger';
  intervalMs: number;
  rxTriggerId: string;
}

// ---------------- CANopen ----------------
export type CanopenCobType = 'NMT' | 'SYNC' | 'EMCY' | 'PDO' | 'SDO' | 'HBT' | 'UNKNOWN';

export interface CanopenNode {
  nodeId: number;
  name: string;
  state: 'BOOTUP' | 'STOPPED' | 'OPERATIONAL' | 'PRE_OPERATIONAL' | 'UNKNOWN';
  lastSeenMs: number;
  isAlive: boolean;
}

export interface CanopenMessage {
  id: string;
  timestamp: string;
  cobType: CanopenCobType;
  cobId: number;
  cobIdHex: string;
  nodeId: number;
  summary: string;
  details?: string;
  rawCan: CanMessage;
}

// ---------------- Hardware / System Events ----------------
export interface HardwareEvent {
  id: string;
  timestamp: string;
  module: 'system' | 'gpio' | 'adc' | 'timers' | 'can' | 'canopen' | 'console' | 'eeprom' | 'flash' | 'display';
  level: 'info' | 'warn' | 'error' | 'success';
  message: string;
  details?: string;
}

// ---------------- Connection & Stats ----------------
export interface ConnectionConfig {
  ip: string;
  port: number;
  mode: 'server' | 'client';
  autoReconnect: boolean;
}

export interface ConnectionStats {
  backendConnected: boolean;
  mcuConnected: boolean;
  clientAddress?: string;
  bytesReceived: number;
  bytesSent: number;
  packetsReceived: number;
  packetsSent: number;
  connectedSince?: string;
  lastPacketTimestamp?: string;
}

export interface PacketLogEntry {
  id: string;
  timestamp: string;
  direction: 'in' | 'out';
  module: ModuleId;
  action: string;
  rawJson: string;
}

export interface BasePacket {
  module: ModuleId;
  action: string;
  [key: string]: any;
}

