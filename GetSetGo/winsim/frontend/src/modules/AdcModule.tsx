import React, { useState, useEffect } from 'react';
import { AdcState, AdcChannel, BasePacket } from '../types/modules';
import { packetConnection } from '../services/packetConnection';
import { Gauge, Sliders, Play, Waves, SlidersHorizontal, ArrowUpRight } from 'lucide-react';

export const AdcModule: React.FC = () => {
  const [state, setState] = useState<AdcState>(() => {
    const channels: AdcChannel[] = [];
    for (let i = 0; i < 8; i++) {
      const v = Number((Math.sin(i * 0.8) * 1.2 + 1.65).toFixed(2));
      const raw = Math.round((v / 3.3) * 4095);
      channels.push({
        channel: i,
        raw,
        voltage: v,
        name: i === 0 ? 'ADC_VBAT' : i === 1 ? 'ADC_TEMP' : i === 2 ? 'ADC_POT' : `CH_${i}`,
        mode: i === 2 ? 'user' : 'mcu',
        history: [v, v, v, v, v, v, v, v, v, v],
      });
    }
    return {
      vref: 3.3,
      resolution: 12,
      channels,
    };
  });

  const [activeSignalGen, setActiveSignalGen] = useState<boolean>(false);

  // Subscribe to incoming ADC packets from MCU
  useEffect(() => {
    const unsubscribe = packetConnection.subscribe('adc', (packet: BasePacket) => {
      if (packet.channel !== undefined) {
        const chNum = Number(packet.channel);
        const maxRaw = (1 << state.resolution) - 1;

        let raw = 0;
        let voltage = 0;

        if (packet.raw !== undefined) {
          raw = Number(packet.raw);
          voltage = Number(((raw / maxRaw) * state.vref).toFixed(2));
        } else if (packet.voltage !== undefined) {
          voltage = Number(packet.voltage);
          raw = Math.round((voltage / state.vref) * maxRaw);
        }

        setState((prev) => {
          const nextChannels = prev.channels.map((ch) => {
            if (ch.channel === chNum) {
              const nextHistory = [...ch.history.slice(-19), voltage];
              return {
                ...ch,
                raw,
                voltage,
                history: nextHistory,
              };
            }
            return ch;
          });
          return { ...prev, channels: nextChannels };
        });
      }
    });

    return () => unsubscribe();
  }, [state.resolution, state.vref]);

  // User moves potentiometer slider (sends analog input to MCU FreeRTOS driver)
  const handleSliderChange = (channelId: number, newVoltage: number) => {
    const maxRaw = (1 << state.resolution) - 1;
    const raw = Math.round((newVoltage / state.vref) * maxRaw);

    // Send packet to MCU
    packetConnection.sendPacket({
      module: 'adc',
      action: 'input_change',
      channel: channelId,
      raw,
      voltage: newVoltage,
    });

    setState((prev) => {
      const nextChannels = prev.channels.map((ch) => {
        if (ch.channel === channelId) {
          return {
            ...ch,
            voltage: newVoltage,
            raw,
            history: [...ch.history.slice(-19), newVoltage],
          };
        }
        return ch;
      });
      return { ...prev, channels: nextChannels };
    });
  };

  const toggleChannelMode = (channelId: number) => {
    setState((prev) => ({
      ...prev,
      channels: prev.channels.map((ch) =>
        ch.channel === channelId
          ? { ...ch, mode: ch.mode === 'mcu' ? 'user' : 'mcu' }
          : ch
      ),
    }));
  };

  return (
    <div className="flex flex-col h-full bg-[#0a0e17] text-slate-200 select-none overflow-hidden">
      {/* Top Toolbar */}
      <div className="flex items-center justify-between px-3 py-2 bg-[#121927] border-b border-cyan-900/40 text-xs">
        <div className="flex items-center gap-2">
          <div className="flex items-center gap-1.5 font-semibold text-cyan-400">
            <Gauge className="w-4 h-4 text-cyan-400" />
            <span>ADC SAMPLER / ANALOG INJECTOR</span>
          </div>
          <span className="text-slate-500">|</span>
          <span className="font-mono text-[11px] text-slate-400">
            Vref: <span className="text-cyan-400 font-bold">{state.vref}V</span>
          </span>
          <span className="text-slate-500">|</span>
          <span className="font-mono text-[11px] text-slate-400">
            Res: <span className="text-emerald-400 font-bold">{state.resolution}-bit</span> (0-4095)
          </span>
        </div>

        <div className="flex items-center gap-2">
          <button
            onClick={() => {
              // Sweep sine wave across user channels
              let step = 0;
              const interval = setInterval(() => {
                if (step > 40) {
                  clearInterval(interval);
                  return;
                }
                const v = Number((1.65 + 1.5 * Math.sin(step * 0.3)).toFixed(2));
                handleSliderChange(2, v);
                step++;
              }, 100);
            }}
            className="px-2 py-1 rounded bg-cyan-950/70 hover:bg-cyan-900 border border-cyan-800 text-[11px] text-cyan-300 font-medium flex items-center gap-1"
          >
            <Waves className="w-3.5 h-3.5" />
            <span>Sweep Signal</span>
          </button>
        </div>
      </div>

      {/* Main Channels Grid */}
      <div className="flex-1 p-3 overflow-y-auto">
        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-3">
          {state.channels.map((ch) => {
            const pct = Math.min(100, Math.max(0, (ch.voltage / state.vref) * 100));
            const isUserControlled = ch.mode === 'user';

            return (
              <div
                key={ch.channel}
                className={`p-3 rounded-lg border flex flex-col justify-between transition-all ${
                  isUserControlled
                    ? 'border-cyan-500/40 bg-gradient-to-b from-[#132233] to-[#0d1624] shadow-sm'
                    : 'border-slate-800 bg-[#0e1626]'
                }`}
              >
                {/* Header */}
                <div className="flex items-center justify-between mb-2">
                  <div className="flex items-center gap-1.5 font-mono">
                    <span className="font-bold text-xs text-slate-100">ADC_{ch.channel}</span>
                    <span className="text-[10px] text-slate-500">({ch.name})</span>
                  </div>
                  <button
                    onClick={() => toggleChannelMode(ch.channel)}
                    title="Click to toggle between MCU Sampling readout and User Potentiometer Injection"
                    className={`text-[9px] font-mono px-1.5 py-0.5 rounded font-bold uppercase ${
                      isUserControlled
                        ? 'bg-cyan-500/20 text-cyan-300 border border-cyan-500/40'
                        : 'bg-slate-800 text-slate-400 border border-slate-700'
                    }`}
                  >
                    {isUserControlled ? 'POT INJECT' : 'MCU READ'}
                  </button>
                </div>

                {/* Big Voltage Readout */}
                <div className="my-1 flex items-baseline justify-between font-mono">
                  <div className="flex items-baseline gap-1">
                    <span className="text-2xl font-bold text-cyan-400 tracking-tight">
                      {ch.voltage.toFixed(2)}
                    </span>
                    <span className="text-xs text-slate-400">V</span>
                  </div>
                  <div className="text-right">
                    <div className="text-[11px] text-emerald-400 font-semibold">
                      Raw: {ch.raw}
                    </div>
                    <div className="text-[9px] text-slate-500">
                      {pct.toFixed(0)}% FSR
                    </div>
                  </div>
                </div>

                {/* Progress Bar / Level Meter */}
                <div className="w-full h-3 bg-slate-900 rounded-full overflow-hidden p-0.5 border border-slate-700/60 my-2">
                  <div
                    className="h-full rounded-full transition-all duration-100 bg-gradient-to-r from-emerald-500 via-cyan-400 to-amber-400"
                    style={{ width: `${pct}%` }}
                  />
                </div>

                {/* Sparkline mini-history */}
                <div className="h-8 flex items-end gap-1 bg-[#090e18] p-1 rounded border border-slate-800/80 mb-2">
                  {ch.history.map((val, idx) => {
                    const hPct = Math.min(100, Math.max(10, (val / state.vref) * 100));
                    return (
                      <div
                        key={idx}
                        className="flex-1 bg-cyan-600/70 rounded-xs hover:bg-cyan-400 transition-all"
                        style={{ height: `${hPct}%` }}
                        title={`${val.toFixed(2)}V`}
                      />
                    );
                  })}
                </div>

                {/* Interactive Slider if User Controlled */}
                {isUserControlled ? (
                  <div className="mt-1 flex flex-col gap-1">
                    <div className="flex items-center justify-between text-[10px] font-mono text-cyan-300">
                      <span>Dial Voltage:</span>
                      <span>{ch.voltage.toFixed(2)} V</span>
                    </div>
                    <input
                      type="range"
                      min={0}
                      max={state.vref}
                      step={0.05}
                      value={ch.voltage}
                      onChange={(e) => handleSliderChange(ch.channel, parseFloat(e.target.value))}
                      className="w-full accent-cyan-400 cursor-pointer"
                    />
                  </div>
                ) : (
                  <div className="text-[10px] font-mono text-slate-500 italic text-center py-1">
                    Live reading from FreeRTOS ADC driver
                  </div>
                )}
              </div>
            );
          })}
        </div>
      </div>
    </div>
  );
};
