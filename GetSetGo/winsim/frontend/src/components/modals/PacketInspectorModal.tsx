import React, { useState, useEffect } from 'react';
import { PacketLogEntry, ModuleId } from '../../types/modules';
import { packetConnection } from '../../services/packetConnection';
import { X, Bug, ArrowDownLeft, ArrowUpRight, Trash2, Filter } from 'lucide-react';

interface PacketInspectorModalProps {
  isOpen: boolean;
  onClose: () => void;
}

export const PacketInspectorModal: React.FC<PacketInspectorModalProps> = ({ isOpen, onClose }) => {
  const [logs, setLogs] = useState<PacketLogEntry[]>([]);
  const [filterModule, setFilterModule] = useState<string>('all');
  const [autoRefresh, setAutoRefresh] = useState<boolean>(true);

  useEffect(() => {
    if (!isOpen) return;

    const interval = setInterval(() => {
      if (autoRefresh) {
        setLogs(packetConnection.getPacketLogs());
      }
    }, 200);

    return () => clearInterval(interval);
  }, [isOpen, autoRefresh]);

  if (!isOpen) return null;

  const filteredLogs = logs.filter((log) => {
    if (filterModule === 'all') return true;
    return log.module === filterModule;
  });

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/75 backdrop-blur-xs select-none">
      <div className="relative w-full max-w-3xl h-[80vh] bg-[#0c1322] border border-cyan-500/30 rounded-xl shadow-2xl overflow-hidden flex flex-col">
        {/* Header */}
        <div className="flex items-center justify-between px-4 py-3 bg-[#131d30] border-b border-cyan-900/50">
          <div className="flex items-center gap-2 text-cyan-400 font-bold text-sm">
            <Bug className="w-5 h-5 text-cyan-400" />
            <span>RAW TCP JSON PACKET INSPECTOR</span>
          </div>
          <div className="flex items-center gap-2">
            <div className="flex items-center gap-1 text-xs font-mono">
              <Filter className="w-3.5 h-3.5 text-slate-400" />
              <select
                value={filterModule}
                onChange={(e) => setFilterModule(e.target.value)}
                className="bg-[#090e18] border border-slate-700 rounded px-2 py-0.5 text-cyan-300 text-xs focus:outline-none"
              >
                <option value="all">All Modules</option>
                <option value="gpio">GPIO</option>
                <option value="console">Console</option>
                <option value="eeprom">EEPROM</option>
                <option value="flash">Flash</option>
                <option value="adc">ADC</option>
                <option value="display">Display</option>
              </select>
            </div>
            <button
              onClick={() => packetConnection.clearLogs()}
              className="p-1 rounded bg-slate-800 hover:bg-slate-700 text-slate-300 border border-slate-700"
              title="Clear Log"
            >
              <Trash2 className="w-3.5 h-3.5" />
            </button>
            <button
              onClick={onClose}
              className="p-1 rounded-lg text-slate-400 hover:text-white hover:bg-slate-800 transition-colors"
            >
              <X className="w-4 h-4" />
            </button>
          </div>
        </div>

        {/* Log Stream */}
        <div className="flex-1 p-3 overflow-y-auto font-mono text-xs flex flex-col gap-1.5 bg-[#080c14]">
          {filteredLogs.length === 0 ? (
            <div className="text-center text-slate-500 py-12 italic">
              No packets captured yet. Start FreeRTOS client or toggle simulation!
            </div>
          ) : (
            filteredLogs.map((log) => {
              const isIn = log.direction === 'in';
              return (
                <div
                  key={log.id}
                  className={`p-2 rounded border transition-colors ${
                    isIn
                      ? 'bg-[#0f1b29] border-cyan-900/50 hover:border-cyan-700'
                      : 'bg-[#1a1727] border-purple-900/50 hover:border-purple-700'
                  }`}
                >
                  <div className="flex items-center justify-between text-[10px] text-slate-400 mb-1">
                    <div className="flex items-center gap-1.5 font-bold">
                      {isIn ? (
                        <span className="flex items-center gap-0.5 text-cyan-400">
                          <ArrowDownLeft className="w-3 h-3" />
                          IN (MCU-&gt;SIM)
                        </span>
                      ) : (
                        <span className="flex items-center gap-0.5 text-purple-400">
                          <ArrowUpRight className="w-3 h-3" />
                          OUT (SIM-&gt;MCU)
                        </span>
                      )}
                      <span className="text-slate-500">•</span>
                      <span className="uppercase text-slate-300">[{log.module}]</span>
                      <span className="text-slate-400">{log.action}</span>
                    </div>
                    <span className="text-slate-500">{log.timestamp}</span>
                  </div>
                  <pre className="text-[11px] text-slate-200 whitespace-pre-wrap break-all overflow-x-auto bg-black/40 p-1.5 rounded">
                    {log.rawJson}
                  </pre>
                </div>
              );
            })
          )}
        </div>
      </div>
    </div>
  );
};
