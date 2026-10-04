import React, { useState, useEffect, useRef } from 'react';
import { HardwareEvent, ModuleId } from '../types/modules';
import { packetConnection } from '../services/packetConnection';
import { Activity, Trash2, Download, Filter, Search, ArrowDown, ShieldAlert, CheckCircle2, AlertTriangle, Info } from 'lucide-react';

export const EventsModule: React.FC = () => {
  const [events, setEvents] = useState<HardwareEvent[]>(() => packetConnection.getHardwareEvents());
  const [filterModule, setFilterModule] = useState<string>('ALL');
  const [filterLevel, setFilterLevel] = useState<string>('ALL');
  const [searchQuery, setSearchQuery] = useState<string>('');
  const [autoScroll, setAutoScroll] = useState<boolean>(true);
  const logEndRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    const unsub = packetConnection.subscribeEvents(() => {
      setEvents(packetConnection.getHardwareEvents());
    });
    return () => unsub();
  }, []);

  useEffect(() => {
    if (autoScroll) {
      logEndRef.current?.scrollIntoView({ behavior: 'smooth' });
    }
  }, [events, autoScroll]);

  const filteredEvents = events.filter((e) => {
    if (filterModule !== 'ALL' && e.module.toUpperCase() !== filterModule) return false;
    if (filterLevel !== 'ALL' && e.level.toUpperCase() !== filterLevel) return false;
    if (searchQuery.trim()) {
      const q = searchQuery.toLowerCase();
      return e.message.toLowerCase().includes(q) || (e.details && e.details.toLowerCase().includes(q));
    }
    return true;
  });

  const clearLogs = () => {
    packetConnection.clearHardwareEvents();
    setEvents([]);
  };

  const exportLogs = () => {
    const content = events.map((e) => `[${e.timestamp}] [${e.level.toUpperCase()}] [${e.module.toUpperCase()}] ${e.message} ${e.details ? '- ' + e.details : ''}`).join('\n');
    const blob = new Blob([content], { type: 'text/plain' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `hardware-events-${Date.now()}.txt`;
    a.click();
    URL.revokeObjectURL(url);
  };

  const getLevelBadge = (level: HardwareEvent['level']) => {
    switch (level) {
      case 'error':
        return (
          <span className="flex items-center gap-1 text-rose-400 bg-rose-950/70 px-1.5 py-0.5 rounded font-bold border border-rose-800">
            <ShieldAlert className="w-3 h-3" />
            <span>ERR</span>
          </span>
        );
      case 'warn':
        return (
          <span className="flex items-center gap-1 text-amber-400 bg-amber-950/70 px-1.5 py-0.5 rounded font-bold border border-amber-800">
            <AlertTriangle className="w-3 h-3" />
            <span>WARN</span>
          </span>
        );
      case 'success':
        return (
          <span className="flex items-center gap-1 text-emerald-400 bg-emerald-950/70 px-1.5 py-0.5 rounded font-bold border border-emerald-800">
            <CheckCircle2 className="w-3 h-3" />
            <span>OK</span>
          </span>
        );
      default:
        return (
          <span className="flex items-center gap-1 text-cyan-400 bg-cyan-950/70 px-1.5 py-0.5 rounded font-bold border border-cyan-800">
            <Info className="w-3 h-3" />
            <span>INFO</span>
          </span>
        );
    }
  };

  return (
    <div className="flex flex-col h-full bg-[#0a0f18] text-slate-200 select-none overflow-hidden text-sm">
      {/* Top Filter & Toolbar */}
      <div className="flex flex-wrap items-center justify-between px-3 py-2 bg-[#121927] border-b border-cyan-900/40 text-xs gap-2">
        <div className="flex items-center gap-2">
          <div className="flex items-center gap-1.5 font-bold text-cyan-400">
            <Activity className="w-4 h-4 text-cyan-400" />
            <span>HARDWARE &amp; SYSTEM EVENT LOG</span>
          </div>
          <span className="text-slate-500">|</span>
          <span className="font-mono text-slate-400 text-xs">
            Events: <strong className="text-cyan-400">{events.length}</strong>
          </span>
        </div>

        {/* Filter controls */}
        <div className="flex items-center gap-2">
          {/* Module Filter */}
          <select
            value={filterModule}
            onChange={(e) => setFilterModule(e.target.value)}
            className="bg-[#090e18] border border-slate-700 rounded px-2 py-1 text-xs text-cyan-300 focus:outline-none"
          >
            <option value="ALL">All Modules</option>
            <option value="SYSTEM">System</option>
            <option value="GPIO">GPIO</option>
            <option value="ADC">ADC</option>
            <option value="TIMERS">Timers</option>
            <option value="CAN">CAN Bus</option>
            <option value="CANOPEN">CANopen</option>
            <option value="CONSOLE">Console</option>
          </select>

          {/* Severity Filter */}
          <select
            value={filterLevel}
            onChange={(e) => setFilterLevel(e.target.value)}
            className="bg-[#090e18] border border-slate-700 rounded px-2 py-1 text-xs text-slate-200 focus:outline-none"
          >
            <option value="ALL">All Levels</option>
            <option value="INFO">Info</option>
            <option value="WARN">Warnings</option>
            <option value="ERROR">Errors</option>
            <option value="SUCCESS">Success</option>
          </select>

          {/* Search box */}
          <div className="relative flex items-center">
            <Search className="w-3.5 h-3.5 absolute left-2 text-slate-500" />
            <input
              type="text"
              placeholder="Search events..."
              value={searchQuery}
              onChange={(e) => setSearchQuery(e.target.value)}
              className="bg-[#090e18] border border-slate-700 rounded pl-7 pr-2 py-1 text-xs text-slate-100 placeholder-slate-500 focus:outline-none focus:border-cyan-500 w-32 md:w-44"
            />
          </div>

          <button
            onClick={() => setAutoScroll(!autoScroll)}
            className={`p-1 rounded border text-xs ${
              autoScroll ? 'bg-cyan-950 border-cyan-700 text-cyan-300' : 'bg-slate-800 border-slate-700 text-slate-400'
            }`}
            title="Auto-scroll"
          >
            <ArrowDown className="w-3.5 h-3.5" />
          </button>
          <button
            onClick={exportLogs}
            className="p-1 rounded bg-slate-800 hover:bg-slate-700 border border-slate-700 text-slate-300"
            title="Export event log"
          >
            <Download className="w-3.5 h-3.5" />
          </button>
          <button
            onClick={clearLogs}
            className="p-1 rounded bg-slate-800 hover:bg-rose-950/60 hover:text-rose-400 border border-slate-700 text-slate-300"
            title="Clear event log"
          >
            <Trash2 className="w-3.5 h-3.5" />
          </button>
        </div>
      </div>

      {/* Main Events Feed */}
      <div className="flex-1 p-2.5 overflow-y-auto font-mono text-xs flex flex-col gap-1.5 bg-[#070b13]">
        {filteredEvents.length === 0 ? (
          <div className="text-center text-slate-500 py-16 italic">
            No events logged matching current filter.
          </div>
        ) : (
          filteredEvents.map((e) => (
            <div
              key={e.id}
              className="flex items-center gap-2.5 px-3 py-1.5 rounded-lg border border-slate-800/80 bg-[#0e1625] hover:border-slate-700 transition-colors"
            >
              <span className="text-[10px] text-slate-500 shrink-0">{e.timestamp}</span>
              {getLevelBadge(e.level)}
              <span className="text-cyan-400 font-bold shrink-0 uppercase text-[11px] bg-[#142033] px-1.5 py-0.5 rounded border border-cyan-900/50">
                {e.module}
              </span>
              <span className="text-slate-200 font-medium flex-1 truncate">{e.message}</span>
              {e.details && (
                <span className="text-[11px] text-slate-400 italic shrink-0 max-w-xs truncate">
                  {e.details}
                </span>
              )}
            </div>
          ))
        )}
        <div ref={logEndRef} />
      </div>
    </div>
  );
};
