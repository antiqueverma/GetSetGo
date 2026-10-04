import React, { useState, useEffect } from 'react';
import { FlashState, FlashSector, BasePacket } from '../types/modules';
import { packetConnection } from '../services/packetConnection';
import { Layers, Flame, FileCode, CheckCircle2, AlertTriangle, RefreshCw } from 'lucide-react';

export const FlashModule: React.FC = () => {
  const NUM_SECTORS = 8;
  const SECTOR_SIZE = 4096; // 4KB per sector

  const [state, setState] = useState<FlashState>(() => {
    const sectors: FlashSector[] = [];
    for (let i = 0; i < NUM_SECTORS; i++) {
      // 256 sample bytes per sector for preview
      const data = new Array(256).fill(0xff);
      if (i === 0) {
        // Bootloader signature
        const sig = "GSG_FLASH_V1";
        for (let j = 0; j < sig.length; j++) data[j] = sig.charCodeAt(j);
      }
      sectors.push({
        id: i,
        address: i * SECTOR_SIZE,
        size: SECTOR_SIZE,
        eraseCycles: i === 0 ? 12 : 2,
        data,
      });
    }

    return {
      totalSize: NUM_SECTORS * SECTOR_SIZE,
      sectorSize: SECTOR_SIZE,
      sectors,
      activeSectorId: 0,
      lastOperation: 'IDLE',
    };
  });

  const [erasingSectorId, setErasingSectorId] = useState<number | null>(null);

  // Subscribe to incoming Flash packets from FreeRTOS
  useEffect(() => {
    const unsubscribe = packetConnection.subscribe('flash', (packet: BasePacket) => {
      const action = packet.action || 'write';

      if (action === 'sector_erase' || action === 'erase') {
        const sectorId = packet.sector !== undefined ? Number(packet.sector) : 0;
        setErasingSectorId(sectorId);

        setTimeout(() => {
          setState((prev) => {
            const nextSectors = prev.sectors.map((sec) => {
              if (sec.id === sectorId) {
                return {
                  ...sec,
                  eraseCycles: sec.eraseCycles + 1,
                  data: new Array(256).fill(0xff),
                };
              }
              return sec;
            });
            return {
              ...prev,
              sectors: nextSectors,
              lastOperation: `SECTOR_ERASE: Sector #${sectorId} (0x${(sectorId * SECTOR_SIZE).toString(16).toUpperCase()})`,
            };
          });
          setErasingSectorId(null);
        }, 400);
      } else if (action === 'page_program' || action === 'write') {
        const addr = packet.address !== undefined ? Number(packet.address) : 0;
        const targetSectorId = Math.floor(addr / SECTOR_SIZE);
        const offsetInSector = addr % SECTOR_SIZE;

        const writeBytes: number[] = Array.isArray(packet.data)
          ? packet.data
          : typeof packet.data === 'number'
          ? [packet.data]
          : [0x55, 0xaa];

        setState((prev) => {
          const nextSectors = prev.sectors.map((sec) => {
            if (sec.id === targetSectorId) {
              const nextData = [...sec.data];
              for (let i = 0; i < writeBytes.length; i++) {
                if (offsetInSector + i < nextData.length) {
                  // Flash bitwise AND (can only write 0s unless erased)
                  nextData[offsetInSector + i] = (nextData[offsetInSector + i] & writeBytes[i]) & 0xff;
                }
              }
              return { ...sec, data: nextData };
            }
            return sec;
          });

          return {
            ...prev,
            sectors: nextSectors,
            lastOperation: `PAGE_PROGRAM: Addr 0x${addr.toString(16).toUpperCase()} [${writeBytes.length}B]`,
          };
        });
      }
    });

    return () => unsubscribe();
  }, []);

  const handleEraseSector = (sectorId: number) => {
    // Send packet to MCU driver
    packetConnection.sendPacket({
      module: 'flash',
      action: 'sector_erase',
      sector: sectorId,
      address: sectorId * SECTOR_SIZE,
    });

    setErasingSectorId(sectorId);
    setTimeout(() => {
      setState((prev) => {
        const next = prev.sectors.map((sec) =>
          sec.id === sectorId
            ? { ...sec, eraseCycles: sec.eraseCycles + 1, data: new Array(256).fill(0xff) }
            : sec
        );
        return {
          ...prev,
          sectors: next,
          lastOperation: `USER_ERASE: Sector #${sectorId}`,
        };
      });
      setErasingSectorId(null);
    }, 350);
  };

  const handleProgramTest = (sectorId: number) => {
    const testBytes = [0xde, 0xad, 0xbe, 0xef, 0xca, 0xfe, 0xba, 0xbe, 0x12, 0x34, 0x56, 0x78];
    packetConnection.sendPacket({
      module: 'flash',
      action: 'page_program',
      sector: sectorId,
      address: sectorId * SECTOR_SIZE,
      data: testBytes,
    });
  };

  const activeSector = state.sectors[state.activeSectorId] || state.sectors[0];

  return (
    <div className="flex flex-col h-full bg-[#0a0e17] text-slate-200 select-none overflow-hidden">
      {/* Top Toolbar */}
      <div className="flex items-center justify-between px-3 py-2 bg-[#121927] border-b border-cyan-900/40 text-xs">
        <div className="flex items-center gap-2">
          <div className="flex items-center gap-1.5 font-semibold text-amber-400">
            <Layers className="w-4 h-4 text-amber-400" />
            <span>FLASH MEMORY CONTROLLER (NOR/ON-CHIP)</span>
          </div>
          <span className="text-slate-500">|</span>
          <span className="font-mono text-[11px] text-slate-400">
            Total: <span className="text-cyan-400 font-bold">{state.totalSize / 1024} KB</span>
          </span>
          <span className="text-slate-500">|</span>
          <span className="font-mono text-[11px] text-slate-400">
            Sectors: <span className="text-emerald-400 font-bold">{NUM_SECTORS}</span> (4KB ea)
          </span>
        </div>

        <div className="flex items-center gap-2">
          <span className="font-mono text-[10px] text-cyan-300/80 bg-[#0c1322] px-2 py-0.5 rounded border border-slate-800">
            {state.lastOperation}
          </span>
          <button
            onClick={() => handleEraseSector(state.activeSectorId)}
            className="px-2.5 py-1 rounded bg-rose-950/70 hover:bg-rose-900 border border-rose-800/80 text-[11px] font-semibold text-rose-300 flex items-center gap-1 transition-colors"
          >
            <Flame className="w-3.5 h-3.5 text-rose-400" />
            <span>Erase Sector {state.activeSectorId}</span>
          </button>
        </div>
      </div>

      {/* Main Grid: Left Sector List, Right Sector Hex Inspector */}
      <div className="flex-1 flex flex-col md:flex-row overflow-hidden p-3 gap-3">
        {/* Left: Sector Blocks Map */}
        <div className="w-full md:w-64 flex flex-col gap-2 shrink-0">
          <div className="text-[11px] font-mono text-slate-400 font-semibold flex items-center justify-between">
            <span>SECTOR MAP</span>
            <span className="text-slate-500">Click to inspect</span>
          </div>

          <div className="flex-1 overflow-y-auto pr-1 flex flex-col gap-1.5">
            {state.sectors.map((sec) => {
              const isSelected = sec.id === state.activeSectorId;
              const isErasing = erasingSectorId === sec.id;
              const isErasedClean = sec.data.every((b) => b === 0xff);

              return (
                <div
                  key={sec.id}
                  onClick={() => setState((p) => ({ ...p, activeSectorId: sec.id }))}
                  className={`p-2 rounded border cursor-pointer transition-all ${
                    isErasing
                      ? 'bg-rose-950/80 border-rose-500 shadow-[0_0_12px_#f43f5e]'
                      : isSelected
                      ? 'bg-[#15233a] border-cyan-500/80 shadow-[0_0_8px_rgba(6,182,212,0.25)]'
                      : 'bg-[#0e1524] border-slate-800 hover:border-slate-700'
                  }`}
                >
                  <div className="flex items-center justify-between font-mono text-xs">
                    <span className="font-bold text-slate-200">Sector {sec.id}</span>
                    <span className="text-[10px] text-slate-500">
                      0x{sec.address.toString(16).padStart(6, '0').toUpperCase()}
                    </span>
                  </div>

                  <div className="mt-1 flex items-center justify-between text-[10px] font-mono">
                    <span className={`px-1 py-0.5 rounded ${isErasedClean ? 'text-emerald-400 bg-emerald-950/60' : 'text-amber-300 bg-amber-950/60'}`}>
                      {isErasedClean ? 'CLEAN (0xFF)' : 'PROGRAMMED'}
                    </span>
                    <span className="text-slate-400">Cycles: {sec.eraseCycles}</span>
                  </div>
                </div>
              );
            })}
          </div>
        </div>

        {/* Right: Sector Content Inspector */}
        <div className="flex-1 flex flex-col border border-slate-800 rounded bg-[#080d16] overflow-hidden">
          <div className="flex items-center justify-between px-3 py-1.5 bg-[#0f1728] border-b border-slate-800 font-mono text-xs">
            <div className="flex items-center gap-2">
              <span className="font-bold text-cyan-400">Sector {activeSector.id} Preview (First 256B)</span>
              <span className="text-slate-500 text-[11px]">
                Base: 0x{activeSector.address.toString(16).padStart(6, '0').toUpperCase()}
              </span>
            </div>
            <button
              onClick={() => handleProgramTest(activeSector.id)}
              className="px-2 py-0.5 rounded bg-cyan-950/80 hover:bg-cyan-900 border border-cyan-800 text-[10px] text-cyan-300 font-medium"
            >
              + Program Test Header
            </button>
          </div>

          <div className="flex-1 p-2.5 overflow-y-auto font-mono text-xs">
            <div className="grid grid-cols-16 gap-1 min-w-[420px]">
              {activeSector.data.map((byte, idx) => {
                const isFF = byte === 0xff;
                return (
                  <div
                    key={idx}
                    title={`Offset: +0x${idx.toString(16).toUpperCase()} | Val: 0x${byte.toString(16).toUpperCase()}`}
                    className={`text-center py-0.5 rounded text-[11px] ${
                      isFF
                        ? 'text-slate-600 bg-slate-900/30'
                        : 'text-amber-300 font-bold bg-amber-950/40 border border-amber-900/50'
                    }`}
                  >
                    {byte.toString(16).padStart(2, '0').toUpperCase()}
                  </div>
                );
              })}
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};
