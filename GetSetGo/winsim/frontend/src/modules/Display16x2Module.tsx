import React, { useState, useEffect } from 'react';
import { DisplayState, DisplayTheme, BasePacket } from '../types/modules';
import { packetConnection } from '../services/packetConnection';
import { Monitor, Sun, Moon, Palette, Sliders, Type } from 'lucide-react';

export const Display16x2Module: React.FC = () => {
  const [state, setState] = useState<DisplayState>({
    lines: ['FreeRTOS WinSim ', 'System Ready... '],
    cursor: { row: 0, col: 0, visible: true, blink: true },
    backlight: true,
    theme: 'blue',
    contrast: 90,
  });

  const [inputLine1, setInputLine1] = useState('');
  const [inputLine2, setInputLine2] = useState('');

  // Subscribe to incoming Display packets from FreeRTOS
  useEffect(() => {
    const unsubscribe = packetConnection.subscribe('display', (packet: BasePacket) => {
      const action = packet.action || 'write';

      if (action === 'clear') {
        setState((prev) => ({
          ...prev,
          lines: ['                ', '                '],
          cursor: { ...prev.cursor, row: 0, col: 0 },
        }));
      } else if (action === 'write' || action === 'print') {
        const lineIdx = packet.line !== undefined ? Number(packet.line) : 0;
        const col = packet.col !== undefined ? Number(packet.col) : 0;
        const text = String(packet.text ?? '');

        setState((prev) => {
          const nextLines = [...prev.lines] as [string, string];
          if (lineIdx === 0 || lineIdx === 1) {
            const currentLine = nextLines[lineIdx].padEnd(16, ' ');
            const before = currentLine.substring(0, col);
            const after = currentLine.substring(col + text.length);
            const merged = (before + text + after).substring(0, 16).padEnd(16, ' ');
            nextLines[lineIdx] = merged;
          }
          return {
            ...prev,
            lines: nextLines,
            backlight: packet.backlight !== undefined ? Boolean(packet.backlight) : prev.backlight,
          };
        });
      } else if (action === 'backlight') {
        setState((prev) => ({
          ...prev,
          backlight: Boolean(packet.enabled ?? packet.state ?? true),
        }));
      } else if (action === 'set_cursor') {
        setState((prev) => ({
          ...prev,
          cursor: {
            ...prev.cursor,
            row: packet.line ?? packet.row ?? 0,
            col: packet.col ?? 0,
            visible: packet.visible !== undefined ? Boolean(packet.visible) : prev.cursor.visible,
          },
        }));
      }
    });

    return () => unsubscribe();
  }, []);

  const handleSendManual = () => {
    if (inputLine1) {
      packetConnection.sendPacket({
        module: 'display',
        action: 'write',
        line: 0,
        col: 0,
        text: inputLine1.padEnd(16, ' ').substring(0, 16),
      });
    }
    if (inputLine2) {
      packetConnection.sendPacket({
        module: 'display',
        action: 'write',
        line: 1,
        col: 0,
        text: inputLine2.padEnd(16, ' ').substring(0, 16),
      });
    }
    setInputLine1('');
    setInputLine2('');
  };

  const handleClear = () => {
    packetConnection.sendPacket({
      module: 'display',
      action: 'clear',
    });
    setState((prev) => ({
      ...prev,
      lines: ['                ', '                '],
    }));
  };

  // Theme styling configurations
  const themeStyles: Record<DisplayTheme, { bg: string; text: string; bezel: string; glow: string }> = {
    blue: {
      bg: state.backlight ? 'bg-[#002fbe]' : 'bg-[#000d33]',
      text: state.backlight ? 'text-[#ffffff] drop-shadow-[0_0_8px_rgba(255,255,255,0.85)]' : 'text-[#001c66]',
      bezel: 'bg-[#1b253b] border-[#293b5e]',
      glow: 'shadow-[0_0_30px_rgba(0,47,190,0.5)]',
    },
    green: {
      bg: state.backlight ? 'bg-[#82a400]' : 'bg-[#293500]',
      text: state.backlight ? 'text-[#0b1600]' : 'text-[#182000]',
      bezel: 'bg-[#1e2722] border-[#2c3d33]',
      glow: 'shadow-[0_0_30px_rgba(130,164,0,0.4)]',
    },
    amber: {
      bg: state.backlight ? 'bg-[#c26d00]' : 'bg-[#3b2000]',
      text: state.backlight ? 'text-[#200e00]' : 'text-[#1f1000]',
      bezel: 'bg-[#261f1d] border-[#44332e]',
      glow: 'shadow-[0_0_30px_rgba(194,109,0,0.4)]',
    },
    dark: {
      bg: state.backlight ? 'bg-[#050b14]' : 'bg-[#010408]',
      text: state.backlight ? 'text-[#38bdf8] drop-shadow-[0_0_8px_#38bdf8]' : 'text-[#0e3b52]',
      bezel: 'bg-[#111827] border-[#1e293b]',
      glow: 'shadow-[0_0_30px_rgba(56,189,248,0.25)]',
    },
  };

  const currentTheme = themeStyles[state.theme];

  return (
    <div className="flex flex-col h-full bg-[#0a0e17] text-slate-200 select-none overflow-hidden">
      {/* Top Toolbar */}
      <div className="flex items-center justify-between px-3 py-2 bg-[#121927] border-b border-cyan-900/40 text-xs">
        <div className="flex items-center gap-2">
          <div className="flex items-center gap-1.5 font-semibold text-sky-400">
            <Monitor className="w-4 h-4 text-sky-400" />
            <span>HD44780 16x2 LCD CONTROLLER</span>
          </div>
          <span className="text-slate-500">|</span>
          <div className="flex items-center gap-1 text-[11px] font-mono text-slate-300">
            <span>Color:</span>
            <select
              value={state.theme}
              onChange={(e) => setState((p) => ({ ...p, theme: e.target.value as DisplayTheme }))}
              className="bg-[#0b101b] border border-slate-700 rounded px-1.5 py-0.5 text-cyan-400 focus:outline-none"
            >
              <option value="blue">Blue Backlight</option>
              <option value="green">Yellow-Green (Classic)</option>
              <option value="amber">Amber</option>
              <option value="dark">Dark OLED</option>
            </select>
          </div>
        </div>

        <div className="flex items-center gap-2">
          <button
            onClick={() => setState((p) => ({ ...p, backlight: !p.backlight }))}
            className={`px-2 py-0.5 rounded text-[11px] font-mono flex items-center gap-1 border transition-all ${
              state.backlight
                ? 'bg-amber-950/70 border-amber-600/70 text-amber-300'
                : 'bg-slate-800 border-slate-700 text-slate-400'
            }`}
          >
            {state.backlight ? <Sun className="w-3.5 h-3.5" /> : <Moon className="w-3.5 h-3.5" />}
            <span>BL: {state.backlight ? 'ON' : 'OFF'}</span>
          </button>
          <button
            onClick={handleClear}
            className="px-2 py-0.5 rounded bg-slate-800 hover:bg-slate-700 border border-slate-700 text-[11px] text-slate-300"
          >
            Clear Screen
          </button>
        </div>
      </div>

      {/* Main LCD Realistic Bezel Container */}
      <div className="flex-1 flex flex-col items-center justify-center p-6 bg-[#080c14] overflow-y-auto">
        {/* PCB Module Frame */}
        <div className={`p-6 rounded-2xl border-4 ${currentTheme.bezel} shadow-2xl relative flex flex-col items-center max-w-2xl w-full`}>
          {/* Header PCB mounting screws and pinout label */}
          <div className="w-full flex items-center justify-between text-[10px] font-mono text-slate-400 mb-3 px-2">
            <div className="flex items-center gap-1">
              <div className="w-2.5 h-2.5 rounded-full bg-slate-700 border border-slate-500 shadow-inner" />
              <span>VSS VDD V0 RS RW E D0..D7 A K</span>
            </div>
            <div className="flex items-center gap-1">
              <span className="text-cyan-400 font-bold">1602A LCD MODULE</span>
              <div className="w-2.5 h-2.5 rounded-full bg-slate-700 border border-slate-500 shadow-inner" />
            </div>
          </div>

          {/* LCD Bezel / Glass Display */}
          <div
            className={`w-full p-6 rounded-lg transition-all duration-300 border-4 border-black/80 ${currentTheme.bg} ${currentTheme.glow} relative flex flex-col justify-center`}
            style={{ opacity: state.contrast / 100 }}
          >
            {/* Dot Matrix Screen Grid */}
            <div className="flex flex-col gap-3 font-mono font-bold tracking-[0.25em] text-lg sm:text-2xl md:text-3xl select-all">
              {/* Row 0 */}
              <div className={`flex justify-between items-center ${currentTheme.text} whitespace-pre`}>
                {state.lines[0].padEnd(16, ' ').split('').map((char, idx) => (
                  <span
                    key={idx}
                    className="relative w-[1ch] text-center inline-block"
                  >
                    {char === ' ' ? '\u00A0' : char}
                    {state.cursor.visible && state.cursor.row === 0 && state.cursor.col === idx && (
                      <span className="absolute bottom-0 left-0 w-full h-[3px] bg-white animate-pulse" />
                    )}
                  </span>
                ))}
              </div>

              {/* Row 1 */}
              <div className={`flex justify-between items-center ${currentTheme.text} whitespace-pre`}>
                {state.lines[1].padEnd(16, ' ').split('').map((char, idx) => (
                  <span
                    key={idx}
                    className="relative w-[1ch] text-center inline-block"
                  >
                    {char === ' ' ? '\u00A0' : char}
                    {state.cursor.visible && state.cursor.row === 1 && state.cursor.col === idx && (
                      <span className="absolute bottom-0 left-0 w-full h-[3px] bg-white animate-pulse" />
                    )}
                  </span>
                ))}
              </div>
            </div>
          </div>

          {/* Bottom PCB labels */}
          <div className="w-full flex items-center justify-between text-[10px] font-mono text-slate-500 mt-3 px-2">
            <span>DDRAM: Line1 0x00-0x0F | Line2 0x40-0x4F</span>
            <span>HD44780 COMPLIANT</span>
          </div>
        </div>

        {/* Quick Test Input Controls */}
        <div className="mt-6 w-full max-w-xl p-3 bg-[#101726] border border-slate-800 rounded-lg flex flex-col gap-2">
          <div className="text-xs font-mono font-semibold text-slate-300 flex items-center gap-1.5">
            <Type className="w-3.5 h-3.5 text-cyan-400" />
            <span>Interactive LCD Injector</span>
          </div>
          <div className="grid grid-cols-1 sm:grid-cols-2 gap-2">
            <input
              type="text"
              maxLength={16}
              placeholder="Row 1 text (max 16)..."
              value={inputLine1}
              onChange={(e) => setInputLine1(e.target.value)}
              className="bg-[#090e18] border border-slate-700 rounded px-2.5 py-1 text-xs font-mono text-slate-100 focus:outline-none focus:border-cyan-500"
            />
            <input
              type="text"
              maxLength={16}
              placeholder="Row 2 text (max 16)..."
              value={inputLine2}
              onChange={(e) => setInputLine2(e.target.value)}
              className="bg-[#090e18] border border-slate-700 rounded px-2.5 py-1 text-xs font-mono text-slate-100 focus:outline-none focus:border-cyan-500"
            />
          </div>
          <div className="flex items-center justify-between mt-1">
            <div className="flex items-center gap-2">
              <span className="text-[10px] text-slate-400 font-mono">Contrast:</span>
              <input
                type="range"
                min={30}
                max={100}
                value={state.contrast}
                onChange={(e) => setState((p) => ({ ...p, contrast: Number(e.target.value) }))}
                className="w-24 accent-sky-400"
              />
            </div>
            <button
              onClick={handleSendManual}
              className="px-3 py-1 rounded bg-sky-600 hover:bg-sky-500 text-black font-semibold text-xs transition-colors"
            >
              Write to LCD
            </button>
          </div>
        </div>
      </div>
    </div>
  );
};
