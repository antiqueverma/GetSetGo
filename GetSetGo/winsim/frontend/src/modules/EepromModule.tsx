import React, { useState, useEffect } from 'react';
import { EepromState, BasePacket } from '../types/modules';
import { packetConnection } from '../services/packetConnection';
import { HardDrive, Edit3, RotateCcw, Download, Search, Check, Save } from 'lucide-react';

export const EepromModule: React.FC = () => {
  const EEPROM_SIZE = 512; // 512 bytes (32 rows of 16 bytes)

  const [state, setState] = useState<EepromState>(() => {
    const data = new Array(EEPROM_SIZE).fill(0xff);
    // Add sample signature in beginning: "GETSETGO_EEPROM"
    const sig = "GETSETGO_EEPROM";
    for (let i = 0; i < sig.length; i++) {
      data[i] = sig.charCodeAt(i);
    }
    return {
      size: EEPROM_SIZE,
      data,
      lastWriteAddress: null,
      lastWriteTime: null,
      writeCount: 0,
    };
  });

  const [highlightedAddress, setHighlightedAddress] = useState<number | null>(null);
  const [editingAddress, setEditingAddress] = useState<number | null>(null);
  const [editValue, setEditValue] = useState<string>('');
  const [jumpAddrInput, setJumpAddrInput] = useState<string>('');

  // Subscribe to EEPROM packets from MCU
  useEffect(() => {
    const unsubscribe = packetConnection.subscribe('eeprom', (packet: BasePacket) => {
      if (packet.address !== undefined) {
        const addr = Number(packet.address);
        let writeData: number[] = [];

        if (Array.isArray(packet.data)) {
          writeData = packet.data.map((x) => Number(x) & 0xff);
        } else if (typeof packet.data === 'number') {
          writeData = [packet.data & 0xff];
        } else if (typeof packet.data === 'string') {
          // Hex string e.g. "A0B1"
          const clean = packet.data.replace(/\s+/g, '');
          for (let i = 0; i < clean.length; i += 2) {
            writeData.push(parseInt(clean.substring(i, i + 2), 16) || 0);
          }
        }

        if (writeData.length > 0) {
          setState((prev) => {
            const nextData = [...prev.data];
            for (let i = 0; i < writeData.length; i++) {
              if (addr + i < nextData.length) {
                nextData[addr + i] = writeData[i];
              }
            }
            return {
              ...prev,
              data: nextData,
              lastWriteAddress: addr,
              lastWriteTime: Date.now(),
              writeCount: prev.writeCount + writeData.length,
            };
          });

          setHighlightedAddress(addr);
          setTimeout(() => setHighlightedAddress(null), 1200);
        }
      }
    });

    return () => unsubscribe();
  }, []);

  const handleCellClick = (addr: number) => {
    setEditingAddress(addr);
    setEditValue(state.data[addr].toString(16).padStart(2, '0').toUpperCase());
  };

  const saveCellEdit = () => {
    if (editingAddress === null) return;
    const val = parseInt(editValue, 16);
    if (!isNaN(val) && val >= 0 && val <= 255) {
      // Send packet to MCU driver
      packetConnection.sendPacket({
        module: 'eeprom',
        action: 'write',
        address: editingAddress,
        data: [val],
      });

      setState((prev) => {
        const next = [...prev.data];
        next[editingAddress] = val;
        return {
          ...prev,
          data: next,
          lastWriteAddress: editingAddress,
          lastWriteTime: Date.now(),
          writeCount: prev.writeCount + 1,
        };
      });
    }
    setEditingAddress(null);
  };

  const fillMemory = (fillByte: number) => {
    packetConnection.sendPacket({
      module: 'eeprom',
      action: 'fill',
      value: fillByte,
    });

    setState((prev) => ({
      ...prev,
      data: new Array(prev.size).fill(fillByte),
      lastWriteAddress: 0,
      lastWriteTime: Date.now(),
      writeCount: prev.writeCount + prev.size,
    }));
  };

  const handleJumpTo = () => {
    const target = parseInt(jumpAddrInput, jumpAddrInput.startsWith('0x') ? 16 : 10);
    if (!isNaN(target) && target >= 0 && target < EEPROM_SIZE) {
      setHighlightedAddress(target);
      const el = document.getElementById(`eeprom-cell-${target}`);
      el?.scrollIntoView({ behavior: 'smooth', block: 'center' });
      setTimeout(() => setHighlightedAddress(null), 1500);
    }
  };

  const exportDump = () => {
    const hexLines: string[] = [];
    for (let r = 0; r < EEPROM_SIZE; r += 16) {
      const slice = state.data.slice(r, r + 16);
      const hex = slice.map((b) => b.toString(16).padStart(2, '0').toUpperCase()).join(' ');
      const ascii = slice.map((b) => (b >= 32 && b <= 126 ? String.fromCharCode(b) : '.')).join('');
      hexLines.push(`0x${r.toString(16).padStart(4, '0').toUpperCase()}: ${hex.padEnd(48, ' ')} | ${ascii}`);
    }
    const blob = new Blob([hexLines.join('\n')], { type: 'text/plain' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `eeprom-dump-${Date.now()}.hex`;
    a.click();
    URL.revokeObjectURL(url);
  };

  const rows = Math.ceil(EEPROM_SIZE / 16);

  return (
    <div className="flex flex-col h-full bg-[#0a0e17] text-slate-200 select-none overflow-hidden">
      {/* Top Toolbar */}
      <div className="flex items-center justify-between px-3 py-2 bg-[#121927] border-b border-cyan-900/40 text-xs">
        <div className="flex items-center gap-2">
          <div className="flex items-center gap-1.5 font-semibold text-purple-400">
            <HardDrive className="w-4 h-4 text-purple-400" />
            <span>EEPROM MEMORY VIEWER (I2C/SPI)</span>
          </div>
          <span className="text-slate-500">|</span>
          <span className="font-mono text-[11px] text-slate-400">
            Cap: <span className="text-cyan-400 font-bold">{EEPROM_SIZE} B</span>
          </span>
          <span className="text-slate-500">|</span>
          <span className="font-mono text-[11px] text-slate-400">
            Writes: <span className="text-emerald-400 font-bold">{state.writeCount}</span>
          </span>
          {state.lastWriteAddress !== null && (
            <span className="font-mono text-[11px] text-amber-400">
              Last: 0x{state.lastWriteAddress.toString(16).padStart(4, '0').toUpperCase()}
            </span>
          )}
        </div>

        <div className="flex items-center gap-2">
          {/* Jump Address input */}
          <div className="flex items-center gap-1">
            <input
              type="text"
              placeholder="0x0000"
              value={jumpAddrInput}
              onChange={(e) => setJumpAddrInput(e.target.value)}
              onKeyDown={(e) => e.key === 'Enter' && handleJumpTo()}
              className="w-16 bg-[#080d18] border border-slate-700 rounded px-1.5 py-0.5 text-xs font-mono text-cyan-300 focus:outline-none focus:border-cyan-500"
            />
            <button
              onClick={handleJumpTo}
              className="p-1 rounded bg-slate-800 hover:bg-slate-700 text-slate-300 border border-slate-700"
              title="Jump to address"
            >
              <Search className="w-3 h-3" />
            </button>
          </div>

          <button
            onClick={() => fillMemory(0xff)}
            className="px-2 py-0.5 rounded bg-slate-800 hover:bg-slate-700 border border-slate-700 text-[11px] text-slate-300"
            title="Erase all to 0xFF"
          >
            Clear (0xFF)
          </button>
          <button
            onClick={() => fillMemory(0x00)}
            className="px-2 py-0.5 rounded bg-slate-800 hover:bg-slate-700 border border-slate-700 text-[11px] text-slate-300"
            title="Zero all to 0x00"
          >
            Zero (0x00)
          </button>
          <button
            onClick={exportDump}
            className="p-1 rounded bg-slate-800 hover:bg-slate-700 border border-slate-700 text-slate-300"
            title="Export Hex Dump"
          >
            <Download className="w-3.5 h-3.5" />
          </button>
        </div>
      </div>

      {/* Hex Dump Table */}
      <div className="flex-1 p-3 overflow-y-auto font-mono text-xs">
        <div className="border border-slate-800/80 rounded bg-[#070b13] overflow-x-auto">
          {/* Header Row */}
          <div className="flex bg-[#0f1728] border-b border-slate-800 text-[11px] text-slate-400 font-bold sticky top-0 z-10 py-1 px-2">
            <div className="w-16 shrink-0 text-slate-500">Offset</div>
            <div className="flex-1 grid grid-cols-16 gap-1 text-center min-w-[340px]">
              {Array.from({ length: 16 }).map((_, i) => (
                <span key={i} className="text-cyan-400/80">
                  {i.toString(16).toUpperCase()}
                </span>
              ))}
            </div>
            <div className="w-36 shrink-0 pl-3 border-l border-slate-800 text-slate-400">
              Decoded ASCII
            </div>
          </div>

          {/* Data Rows */}
          {Array.from({ length: rows }).map((_, rowIndex) => {
            const baseAddr = rowIndex * 16;
            const slice = state.data.slice(baseAddr, baseAddr + 16);

            return (
              <div
                key={rowIndex}
                className="flex items-center hover:bg-slate-900/60 border-b border-slate-900 px-2 py-0.5 transition-colors"
              >
                {/* Offset Label */}
                <div className="w-16 shrink-0 text-slate-500 text-[11px] select-none font-bold">
                  0x{baseAddr.toString(16).padStart(4, '0').toUpperCase()}
                </div>

                {/* 16 Hex Bytes */}
                <div className="flex-1 grid grid-cols-16 gap-1 min-w-[340px]">
                  {slice.map((byte, colIndex) => {
                    const addr = baseAddr + colIndex;
                    const isHighlighted = highlightedAddress === addr;
                    const isEditing = editingAddress === addr;
                    const isZero = byte === 0x00;
                    const isFF = byte === 0xff;

                    if (isEditing) {
                      return (
                        <div key={colIndex} className="relative">
                          <input
                            type="text"
                            autoFocus
                            maxLength={2}
                            value={editValue}
                            onChange={(e) => setEditValue(e.target.value.toUpperCase())}
                            onBlur={saveCellEdit}
                            onKeyDown={(e) => e.key === 'Enter' && saveCellEdit()}
                            className="w-full text-center bg-cyan-900 text-cyan-200 border border-cyan-400 rounded text-xs py-0.5 focus:outline-none font-bold"
                          />
                        </div>
                      );
                    }

                    return (
                      <div
                        id={`eeprom-cell-${addr}`}
                        key={colIndex}
                        onClick={() => handleCellClick(addr)}
                        title={`Address: 0x${addr.toString(16).padStart(4, '0').toUpperCase()} (${addr})\nValue: 0x${byte.toString(16).padStart(2, '0').toUpperCase()} (${byte})\nClick to edit`}
                        className={`text-center py-0.5 rounded cursor-pointer transition-all duration-150 ${
                          isHighlighted
                            ? 'bg-amber-400 text-black font-bold shadow-[0_0_10px_#f59e0b]'
                            : isFF
                            ? 'text-slate-600 hover:text-slate-300 hover:bg-slate-800'
                            : isZero
                            ? 'text-slate-500 hover:text-slate-200 hover:bg-slate-800'
                            : 'text-cyan-300 font-semibold hover:bg-cyan-950/70 hover:text-cyan-100'
                        }`}
                      >
                        {byte.toString(16).padStart(2, '0').toUpperCase()}
                      </div>
                    );
                  })}
                </div>

                {/* Decoded ASCII Representation */}
                <div className="w-36 shrink-0 pl-3 border-l border-slate-800/80 text-[11px] text-emerald-400/90 font-mono tracking-wider truncate">
                  {slice.map((byte) => (byte >= 32 && byte <= 126 ? String.fromCharCode(byte) : '.')).join('')}
                </div>
              </div>
            );
          })}
        </div>
      </div>
    </div>
  );
};
