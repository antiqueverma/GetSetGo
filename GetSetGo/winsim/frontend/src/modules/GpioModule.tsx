import React, { useState, useEffect } from 'react';
import { GpioPin, GpioState, BasePacket } from '../types/modules';
import { packetConnection } from '../services/packetConnection';
import { Cpu, Zap, Activity, ToggleLeft, ToggleRight, ArrowUpRight, ArrowDownRight, RefreshCw } from 'lucide-react';

export const GpioModule: React.FC = () => {
  // Initialize 16 pins across Port A and Port B
  const [gpioState, setGpioState] = useState<GpioState>(() => {
    const pins: Record<string, GpioPin> = {};
    ['A', 'B'].forEach((port) => {
      for (let i = 0; i < 8; i++) {
        const id = `${port}${i}`;
        pins[id] = {
          port,
          pin: i,
          state: 0,
          direction: i < 4 ? 'out' : 'in',
          pull: i >= 4 ? 'up' : 'none',
          label: port === 'A' && i === 5 ? 'LED_HEARTBEAT' : port === 'A' && i === 0 ? 'BTN_USER' : undefined,
          lastUpdated: Date.now(),
        };
      }
    });
    return { pins, history: [] };
  });

  const [activePort, setActivePort] = useState<'ALL' | 'A' | 'B'>('ALL');
  const [pulsePin, setPulsePin] = useState<string | null>(null);

  // Subscribe to incoming GPIO packets from MCU
  useEffect(() => {
    const unsubscribe = packetConnection.subscribe('gpio', (packet: BasePacket) => {
      if (packet.port !== undefined && packet.pin !== undefined) {
        const port = String(packet.port).toUpperCase();
        const pinNum = Number(packet.pin);
        const pinId = `${port}${pinNum}`;

        setGpioState((prev) => {
          const existing = prev.pins[pinId] || {
            port,
            pin: pinNum,
            state: 0,
            direction: 'out',
            pull: 'none',
          };

          const newState: 0 | 1 = packet.state !== undefined ? (packet.state ? 1 : 0) : existing.state;
          const newDir: 'in' | 'out' = packet.direction ? packet.direction : existing.direction;
          const newPull = packet.pull ? packet.pull : existing.pull;

          const updatedPins = {
            ...prev.pins,
            [pinId]: {
              ...existing,
              state: newState,
              direction: newDir,
              pull: newPull,
              label: packet.label !== undefined ? packet.label : existing.label,
              lastUpdated: Date.now(),
            },
          };

          const newHistory = [
            {
              timestamp: new Date().toLocaleTimeString() + '.' + String(new Date().getMilliseconds()).padStart(3, '0'),
              port,
              pin: pinNum,
              state: newState,
              dir: newDir,
            },
            ...prev.history.slice(0, 19),
          ];

          return { pins: updatedPins, history: newHistory };
        });

        // Trigger brief visual pulse
        setPulsePin(pinId);
        setTimeout(() => setPulsePin(null), 300);
      }
    });

    return () => unsubscribe();
  }, []);

  // User interacts with pin (send packet back to MCU)
  const handlePinToggle = (pinId: string) => {
    const pin = gpioState.pins[pinId];
    if (!pin) return;

    const nextState: 0 | 1 = pin.state === 1 ? 0 : 1;

    // Send packet to MCU driver over TCP
    packetConnection.sendPacket({
      module: 'gpio',
      action: pin.direction === 'in' ? 'input_change' : 'set_state',
      port: pin.port,
      pin: pin.pin,
      state: nextState,
    });

    // Optimistically update local UI state
    setGpioState((prev) => ({
      ...prev,
      pins: {
        ...prev.pins,
        [pinId]: {
          ...pin,
          state: nextState,
          lastUpdated: Date.now(),
        },
      },
    }));
  };

  const handlePulse = (pinId: string) => {
    const pin = gpioState.pins[pinId];
    if (!pin) return;

    // Simulate a momentary press: High -> Low
    packetConnection.sendPacket({
      module: 'gpio',
      action: 'input_change',
      port: pin.port,
      pin: pin.pin,
      state: 1,
    });

    setTimeout(() => {
      packetConnection.sendPacket({
        module: 'gpio',
        action: 'input_change',
        port: pin.port,
        pin: pin.pin,
        state: 0,
      });
    }, 150);
  };

  const handleToggleDirection = (pinId: string) => {
    const pin = gpioState.pins[pinId];
    if (!pin) return;
    const nextDir = pin.direction === 'in' ? 'out' : 'in';

    packetConnection.sendPacket({
      module: 'gpio',
      action: 'config_direction',
      port: pin.port,
      pin: pin.pin,
      direction: nextDir,
    });

    setGpioState((prev) => ({
      ...prev,
      pins: {
        ...prev.pins,
        [pinId]: { ...pin, direction: nextDir },
      },
    }));
  };

  const setAllState = (port: string, state: 0 | 1) => {
    for (let i = 0; i < 8; i++) {
      packetConnection.sendPacket({
        module: 'gpio',
        action: 'set_state',
        port,
        pin: i,
        state,
      });
    }
  };

  const filteredPinKeys = Object.keys(gpioState.pins).filter((key) => {
    if (activePort === 'ALL') return true;
    return key.startsWith(activePort);
  });

  return (
    <div className="flex flex-col h-full bg-[#0d131f] text-slate-200 select-none overflow-hidden">
      {/* Top Toolbar */}
      <div className="flex items-center justify-between px-3 py-2 bg-[#131c2e] border-b border-cyan-900/40 text-xs">
        <div className="flex items-center gap-2">
          <div className="flex items-center gap-1.5 font-semibold text-cyan-400">
            <Cpu className="w-4 h-4 text-cyan-400" />
            <span>GPIO CONTROLLER</span>
          </div>
          <span className="text-slate-500">|</span>
          <div className="flex bg-[#0b101b] rounded p-0.5 border border-slate-700/50">
            {(['ALL', 'A', 'B'] as const).map((p) => (
              <button
                key={p}
                onClick={() => setActivePort(p)}
                className={`px-2 py-0.5 rounded text-[11px] font-medium transition-all ${
                  activePort === p
                    ? 'bg-cyan-500 text-black shadow-sm font-semibold'
                    : 'text-slate-400 hover:text-slate-200'
                }`}
              >
                Port {p}
              </button>
            ))}
          </div>
        </div>

        {/* Batch actions */}
        <div className="flex items-center gap-1.5">
          <button
            onClick={() => setAllState('A', 0)}
            className="px-2 py-1 rounded bg-slate-800/80 hover:bg-slate-700 border border-slate-700 text-[11px] text-slate-300 flex items-center gap-1"
          >
            <span>Reset Low</span>
          </button>
          <button
            onClick={() => {
              // Rapid sequential LED chaser
              let i = 0;
              const chaser = setInterval(() => {
                if (i >= 8) {
                  clearInterval(chaser);
                  return;
                }
                packetConnection.sendPacket({ module: 'gpio', action: 'write', port: 'A', pin: i, state: 1 });
                setTimeout(() => {
                  packetConnection.sendPacket({ module: 'gpio', action: 'write', port: 'A', pin: i, state: 0 });
                }, 200);
                i++;
              }, 120);
            }}
            className="px-2 py-1 rounded bg-cyan-950/60 hover:bg-cyan-900/80 border border-cyan-700/50 text-[11px] text-cyan-300 flex items-center gap-1"
          >
            <RefreshCw className="w-3 h-3 text-cyan-400" />
            <span>LED Test</span>
          </button>
        </div>
      </div>

      {/* Main Grid View */}
      <div className="flex-1 p-3 overflow-y-auto">
        <div className="grid grid-cols-2 sm:grid-cols-4 md:grid-cols-4 lg:grid-cols-8 gap-2.5">
          {filteredPinKeys.map((pinKey) => {
            const pin = gpioState.pins[pinKey];
            const isHigh = pin.state === 1;
            const isOut = pin.direction === 'out';
            const isPulsing = pulsePin === pinKey;

            return (
              <div
                key={pinKey}
                className={`relative flex flex-col p-2.5 rounded-lg border transition-all duration-150 ${
                  isPulsing
                    ? 'border-cyan-400 bg-cyan-950/40 shadow-[0_0_12px_rgba(6,182,212,0.4)]'
                    : isHigh
                    ? 'border-emerald-500/40 bg-gradient-to-b from-[#142328] to-[#0f172a]'
                    : 'border-slate-800 bg-[#101726] hover:border-slate-700'
                }`}
              >
                {/* Pin Header */}
                <div className="flex items-center justify-between mb-1.5">
                  <span className="font-mono font-bold text-xs tracking-wider text-slate-100 flex items-center gap-1">
                    P{pinKey}
                  </span>
                  <button
                    onClick={() => handleToggleDirection(pinKey)}
                    title={`Click to switch to ${isOut ? 'INPUT' : 'OUTPUT'}`}
                    className={`text-[9px] font-mono px-1.5 py-0.5 rounded font-semibold uppercase transition-colors ${
                      isOut
                        ? 'bg-amber-500/20 text-amber-400 border border-amber-500/30'
                        : 'bg-indigo-500/20 text-indigo-300 border border-indigo-500/30'
                    }`}
                  >
                    {pin.direction}
                  </button>
                </div>

                {/* Pin Label if exists */}
                {pin.label && (
                  <div className="text-[10px] text-cyan-400/90 font-mono truncate mb-1" title={pin.label}>
                    {pin.label}
                  </div>
                )}

                {/* State Indicator / LED */}
                <div className="my-2 flex flex-col items-center justify-center">
                  <div
                    onClick={() => handlePinToggle(pinKey)}
                    className={`cursor-pointer w-10 h-10 rounded-full flex items-center justify-center transition-all duration-200 border-2 ${
                      isHigh
                        ? 'bg-emerald-500/20 border-emerald-400 shadow-[0_0_15px_#10b981]'
                        : 'bg-slate-900 border-slate-700/80 hover:border-slate-500'
                    }`}
                  >
                    <div
                      className={`w-5 h-5 rounded-full transition-all duration-150 ${
                        isHigh
                          ? 'bg-emerald-400 shadow-[0_0_8px_#34d399]'
                          : 'bg-slate-800'
                      }`}
                    />
                  </div>
                  <div className="mt-1.5 flex items-center gap-1 text-[11px] font-mono">
                    <span className={isHigh ? 'text-emerald-400 font-bold' : 'text-slate-500'}>
                      {isHigh ? 'HIGH (1)' : 'LOW (0)'}
                    </span>
                    <span className="text-[10px] text-slate-500">
                      ({isHigh ? '3.3V' : '0.0V'})
                    </span>
                  </div>
                </div>

                {/* Controls for user stimulation */}
                <div className="mt-auto pt-2 border-t border-slate-800/80 flex items-center justify-between gap-1 text-[11px]">
                  {pin.direction === 'in' ? (
                    <>
                      <button
                        onClick={() => handlePinToggle(pinKey)}
                        className={`flex-1 py-1 px-1 rounded text-center font-medium transition-all ${
                          isHigh
                            ? 'bg-emerald-600 text-white hover:bg-emerald-500'
                            : 'bg-slate-800 text-slate-300 hover:bg-slate-700'
                        }`}
                        title="Toggle Input State (Simulates continuous voltage to MCU)"
                      >
                        {isHigh ? 'Press (1)' : 'Release (0)'}
                      </button>
                      <button
                        onClick={() => handlePulse(pinKey)}
                        className="py-1 px-1.5 rounded bg-cyan-950/80 text-cyan-300 hover:bg-cyan-900 border border-cyan-800/60"
                        title="Simulate edge pulse (trigger interrupt)"
                      >
                        <Zap className="w-3 h-3 text-cyan-400" />
                      </button>
                    </>
                  ) : (
                    <button
                      onClick={() => handlePinToggle(pinKey)}
                      className="w-full py-1 rounded bg-slate-800/80 hover:bg-slate-700 text-slate-300 font-mono text-[10px] transition-colors"
                      title="Force output state override"
                    >
                      Override State
                    </button>
                  )}
                </div>
              </div>
            );
          })}
        </div>
      </div>

      {/* Bottom Live History Stream */}
      <div className="h-20 bg-[#0a0e17] border-t border-slate-800/90 px-3 py-1.5 flex flex-col font-mono text-[11px]">
        <div className="flex items-center justify-between text-[10px] text-slate-400 mb-1">
          <span className="flex items-center gap-1 font-semibold text-slate-300">
            <Activity className="w-3 h-3 text-cyan-400" />
            GPIO ACTIVITY LOG
          </span>
          <span>{gpioState.history.length} events</span>
        </div>
        <div className="flex-1 overflow-x-auto overflow-y-hidden flex items-center gap-2">
          {gpioState.history.length === 0 ? (
            <span className="text-slate-600 italic">Awaiting pin transitions from FreeRTOS drivers...</span>
          ) : (
            gpioState.history.map((h, idx) => (
              <div
                key={idx}
                className="shrink-0 px-2 py-0.5 rounded bg-[#111927] border border-slate-800 flex items-center gap-1.5"
              >
                <span className="text-slate-500 text-[9px]">{h.timestamp}</span>
                <span className="text-cyan-400 font-bold">P{h.port}{h.pin}</span>
                <span className={`px-1 rounded text-[9px] font-bold ${h.state ? 'bg-emerald-950 text-emerald-300' : 'bg-rose-950 text-rose-300'}`}>
                  {h.state ? 'HIGH' : 'LOW'}
                </span>
                <span className="text-slate-500 text-[9px]">({h.dir})</span>
              </div>
            ))
          )}
        </div>
      </div>
    </div>
  );
};
