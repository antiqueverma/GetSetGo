import React, { useState, useEffect } from 'react';
import { TimerUnit, TimerChannel, BasePacket } from '../types/modules';
import { packetConnection } from '../services/packetConnection';
import { Clock, Play, Square, RotateCcw, Cpu, Sliders, Activity, Zap } from 'lucide-react';

export const TimersModule: React.FC = () => {
  const [timers, setTimers] = useState<TimerUnit[]>([
    {
      id: 'TIM1',
      name: 'Advanced Control Timer (16-bit)',
      bitWidth: 16,
      cnt: 250,
      arr: 1000,
      psc: 83,
      clockMhz: 84,
      running: true,
      direction: 'up',
      channels: [
        { id: 1, mode: 'pwm1', ccr: 250, outputState: 1, dutyCycle: 25, enabled: true },
        { id: 2, mode: 'pwm1', ccr: 500, outputState: 1, dutyCycle: 50, enabled: true },
        { id: 3, mode: 'pwm1', ccr: 750, outputState: 0, dutyCycle: 75, enabled: true },
        { id: 4, mode: 'output_compare', ccr: 900, outputState: 0, dutyCycle: 90, enabled: false },
      ],
    },
    {
      id: 'TIM2',
      name: 'General Purpose Timer (32-bit)',
      bitWidth: 32,
      cnt: 15420,
      arr: 65535,
      psc: 0,
      clockMhz: 84,
      running: true,
      direction: 'up',
      channels: [
        { id: 1, mode: 'pwm1', ccr: 32768, outputState: 1, dutyCycle: 50, enabled: true },
        { id: 2, mode: 'input_capture', ccr: 12000, outputState: 0, dutyCycle: 18, enabled: true },
      ],
    },
    {
      id: 'TIM3',
      name: 'General Purpose Timer (16-bit)',
      bitWidth: 16,
      cnt: 80,
      arr: 200,
      psc: 839,
      clockMhz: 84,
      running: true,
      direction: 'up',
      channels: [
        { id: 1, mode: 'pwm1', ccr: 100, outputState: 1, dutyCycle: 50, enabled: true },
      ],
    },
  ]);

  const [selectedTimerId, setSelectedTimerId] = useState<string>('TIM1');

  // Subscribe to timer packets from FreeRTOS drivers
  useEffect(() => {
    const unsub = packetConnection.subscribe('timers', (packet: BasePacket) => {
      const timerId = packet.timerId || 'TIM1';
      setTimers((prev) =>
        prev.map((t) => {
          if (t.id === timerId) {
            const nextArr = packet.arr !== undefined ? Number(packet.arr) : t.arr;
            const nextCnt = packet.cnt !== undefined ? Number(packet.cnt) : t.cnt;
            const nextPsc = packet.psc !== undefined ? Number(packet.psc) : t.psc;

            let updatedChannels = t.channels;
            if (Array.isArray(packet.channels)) {
              updatedChannels = packet.channels.map((ch: any) => {
                const ccr = ch.ccr !== undefined ? Number(ch.ccr) : 0;
                const duty = nextArr > 0 ? Number(((ccr / nextArr) * 100).toFixed(1)) : 0;
                return {
                  id: ch.id || 1,
                  mode: ch.mode || 'pwm1',
                  ccr,
                  outputState: (ch.outputState ?? (nextCnt <= ccr ? 1 : 0)) as 0 | 1,
                  dutyCycle: duty,
                  enabled: ch.enabled !== false,
                };
              });
            }

            return {
              ...t,
              cnt: nextCnt,
              arr: nextArr,
              psc: nextPsc,
              running: packet.running !== undefined ? Boolean(packet.running) : t.running,
              channels: updatedChannels,
            };
          }
          return t;
        })
      );
    });

    return () => unsub();
  }, []);

  const activeTimer = timers.find((t) => t.id === selectedTimerId) || timers[0];

  const toggleRun = (tId: string) => {
    setTimers((prev) =>
      prev.map((t) => {
        if (t.id === tId) {
          const nextRun = !t.running;
          packetConnection.sendPacket({
            module: 'timers',
            action: nextRun ? 'start' : 'stop',
            timerId: tId,
          });
          return { ...t, running: nextRun };
        }
        return t;
      })
    );
  };

  const resetCounter = (tId: string) => {
    packetConnection.sendPacket({
      module: 'timers',
      action: 'reset',
      timerId: tId,
    });
    setTimers((prev) =>
      prev.map((t) => (t.id === tId ? { ...t, cnt: 0 } : t))
    );
  };

  const handleCcrChange = (chId: number, newCcr: number) => {
    packetConnection.sendPacket({
      module: 'timers',
      action: 'set_ccr',
      timerId: activeTimer.id,
      channel: chId,
      ccr: newCcr,
    });

    setTimers((prev) =>
      prev.map((t) => {
        if (t.id === activeTimer.id) {
          return {
            ...t,
            channels: t.channels.map((c) =>
              c.id === chId
                ? { ...c, ccr: newCcr, dutyCycle: Number(((newCcr / t.arr) * 100).toFixed(1)) }
                : c
            ),
          };
        }
        return t;
      })
    );
  };

  // Calculate Timer Output Frequency
  const timerFreqHz =
    (activeTimer.clockMhz * 1000000) / ((activeTimer.psc + 1) * (activeTimer.arr + 1));
  const cntPct = Math.min(100, Math.max(0, (activeTimer.cnt / activeTimer.arr) * 100));

  return (
    <div className="flex flex-col h-full bg-[#0a0f18] text-slate-200 select-none overflow-hidden text-sm">
      {/* Top Toolbar */}
      <div className="flex items-center justify-between px-3 py-2 bg-[#121927] border-b border-cyan-900/40 text-xs">
        <div className="flex items-center gap-2">
          <Clock className="w-4 h-4 text-emerald-400" />
          <span className="font-bold text-emerald-300">HARDWARE TIMERS &amp; PWM GENERATION</span>
          <span className="text-slate-500">|</span>
          <div className="flex bg-[#090e18] p-0.5 rounded border border-slate-800">
            {timers.map((t) => (
              <button
                key={t.id}
                onClick={() => setSelectedTimerId(t.id)}
                className={`px-2.5 py-0.5 rounded text-xs font-mono font-bold transition-all ${
                  selectedTimerId === t.id
                    ? 'bg-emerald-500 text-black shadow-sm'
                    : 'text-slate-400 hover:text-slate-200'
                }`}
              >
                {t.id} ({t.bitWidth}b)
              </button>
            ))}
          </div>
        </div>

        <div className="flex items-center gap-2">
          <button
            onClick={() => toggleRun(activeTimer.id)}
            className={`px-2.5 py-1 rounded text-xs font-bold flex items-center gap-1 border transition-colors ${
              activeTimer.running
                ? 'bg-rose-950/70 border-rose-800 text-rose-300 hover:bg-rose-900'
                : 'bg-emerald-950/70 border-emerald-800 text-emerald-300 hover:bg-emerald-900'
            }`}
          >
            {activeTimer.running ? <Square className="w-3.5 h-3.5" /> : <Play className="w-3.5 h-3.5" />}
            <span>{activeTimer.running ? 'Halt Timer' : 'Run Timer'}</span>
          </button>
          <button
            onClick={() => resetCounter(activeTimer.id)}
            className="p-1 rounded bg-slate-800 hover:bg-slate-700 border border-slate-700 text-slate-300"
            title="Reset Counter"
          >
            <RotateCcw className="w-3.5 h-3.5" />
          </button>
        </div>
      </div>

      {/* Main Content: Top Register Panel, Bottom Channels Matrix */}
      <div className="flex-1 p-3 overflow-y-auto flex flex-col gap-3">
        {/* Timer Registers Overview Card */}
        <div className="p-3.5 rounded-xl border border-slate-800 bg-[#0e1625] flex flex-col gap-3">
          <div className="flex items-center justify-between">
            <div>
              <span className="font-bold text-sm text-slate-100 font-mono">{activeTimer.id} — {activeTimer.name}</span>
              <div className="text-xs text-slate-400 font-mono mt-0.5">
                Core Clock: <strong>{activeTimer.clockMhz} MHz</strong> | Frequency: <strong className="text-emerald-400">{timerFreqHz.toFixed(1)} Hz</strong> ({((1 / timerFreqHz) * 1000).toFixed(2)} ms)
              </div>
            </div>
            <div className="flex items-center gap-2 font-mono text-xs">
              <span className={`px-2 py-0.5 rounded font-bold ${activeTimer.running ? 'bg-emerald-950 text-emerald-300 border border-emerald-800' : 'bg-slate-800 text-slate-400'}`}>
                {activeTimer.running ? 'RUNNING' : 'STOPPED'}
              </span>
            </div>
          </div>

          {/* Progress Bar of Counter CNT vs ARR */}
          <div className="flex flex-col gap-1">
            <div className="flex items-center justify-between text-xs font-mono">
              <span className="text-cyan-400 font-bold">
                CNT: {activeTimer.cnt} (0x{activeTimer.cnt.toString(16).toUpperCase()})
              </span>
              <span className="text-slate-400">
                ARR: {activeTimer.arr} (0x{activeTimer.arr.toString(16).toUpperCase()})
              </span>
            </div>
            <div className="w-full h-3 bg-slate-900 rounded-full overflow-hidden p-0.5 border border-slate-700">
              <div
                className="h-full bg-gradient-to-r from-cyan-500 to-emerald-400 rounded-full transition-all duration-75"
                style={{ width: `${cntPct}%` }}
              />
            </div>
          </div>

          {/* Register Parameter Badges */}
          <div className="grid grid-cols-2 md:grid-cols-4 gap-2 text-xs font-mono">
            <div className="p-2 rounded bg-[#080d16] border border-slate-800">
              <span className="text-slate-500 block text-[10px]">PRESCALER (PSC)</span>
              <span className="text-amber-400 font-bold text-sm">{activeTimer.psc}</span>
              <span className="text-slate-500 text-[10px]"> (div by {activeTimer.psc + 1})</span>
            </div>
            <div className="p-2 rounded bg-[#080d16] border border-slate-800">
              <span className="text-slate-500 block text-[10px]">AUTO-RELOAD (ARR)</span>
              <span className="text-cyan-400 font-bold text-sm">{activeTimer.arr}</span>
              <span className="text-slate-500 text-[10px]"> (period ticks)</span>
            </div>
            <div className="p-2 rounded bg-[#080d16] border border-slate-800">
              <span className="text-slate-500 block text-[10px]">TIMER WIDTH</span>
              <span className="text-purple-400 font-bold text-sm">{activeTimer.bitWidth}-Bit</span>
              <span className="text-slate-500 text-[10px]"> (max {Math.pow(2, activeTimer.bitWidth) - 1})</span>
            </div>
            <div className="p-2 rounded bg-[#080d16] border border-slate-800">
              <span className="text-slate-500 block text-[10px]">DIRECTION</span>
              <span className="text-emerald-400 font-bold text-sm uppercase">{activeTimer.direction}</span>
              <span className="text-slate-500 text-[10px]"> (Edge-aligned)</span>
            </div>
          </div>
        </div>

        {/* Capture / Compare Channels (PWM) */}
        <div className="flex flex-col gap-2">
          <div className="flex items-center justify-between text-xs font-mono font-bold text-slate-300">
            <span>CAPTURE / COMPARE CHANNELS (CH1 — CH4)</span>
            <span className="text-slate-500">{activeTimer.channels.length} Channels Active</span>
          </div>

          <div className="grid grid-cols-1 md:grid-cols-2 gap-3">
            {activeTimer.channels.map((ch) => {
              const isHigh = ch.outputState === 1;

              return (
                <div
                  key={ch.id}
                  className="p-3 rounded-lg border border-slate-800 bg-[#0e1625] flex flex-col justify-between gap-2.5"
                >
                  <div className="flex items-center justify-between">
                    <div className="flex items-center gap-1.5 font-mono">
                      <span className="font-bold text-xs text-cyan-400">CH{ch.id}</span>
                      <span className="text-[10px] text-slate-400 uppercase bg-[#080d16] px-1.5 py-0.5 rounded border border-slate-800">
                        {ch.mode.replace('_', ' ')}
                      </span>
                    </div>

                    <div className="flex items-center gap-2">
                      {/* Live Output Logic Pin LED */}
                      <div className="flex items-center gap-1 font-mono text-xs">
                        <span className="text-slate-400 text-[10px]">OUT:</span>
                        <div
                          className={`w-3.5 h-3.5 rounded-full border ${
                            isHigh
                              ? 'bg-emerald-400 border-emerald-300 shadow-[0_0_8px_#10b981]'
                              : 'bg-slate-900 border-slate-700'
                          }`}
                        />
                        <span className={isHigh ? 'text-emerald-400 font-bold' : 'text-slate-500'}>
                          {isHigh ? '1' : '0'}
                        </span>
                      </div>
                    </div>
                  </div>

                  {/* CCR Slider and Duty Cycle */}
                  <div className="flex flex-col gap-1">
                    <div className="flex items-center justify-between text-xs font-mono">
                      <span>CCR{ch.id}: <strong className="text-cyan-300">{ch.ccr}</strong></span>
                      <span>Duty: <strong className="text-emerald-400">{ch.dutyCycle}%</strong></span>
                    </div>
                    <input
                      type="range"
                      min={0}
                      max={activeTimer.arr}
                      value={ch.ccr}
                      onChange={(e) => handleCcrChange(ch.id, parseInt(e.target.value) || 0)}
                      className="w-full accent-cyan-400 cursor-pointer"
                    />
                  </div>

                  {/* Synthetic PWM Waveform Graphic */}
                  <div className="h-8 w-full bg-[#080d16] rounded border border-slate-800 flex items-center px-1 overflow-hidden">
                    <svg className="w-full h-6" preserveAspectRatio="none" viewBox="0 0 100 24">
                      {/* Draw 2 periods of square wave corresponding to duty cycle */}
                      <path
                        d={`M 0,20 L 0,4 L ${ch.dutyCycle / 2},4 L ${ch.dutyCycle / 2},20 L 50,20 L 50,4 L ${50 + ch.dutyCycle / 2},4 L ${50 + ch.dutyCycle / 2},20 L 100,20`}
                        fill="none"
                        stroke="#10b981"
                        strokeWidth="2"
                      />
                    </svg>
                  </div>
                </div>
              );
            })}
          </div>
        </div>
      </div>
    </div>
  );
};
