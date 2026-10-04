import React, { useState, useRef, useEffect } from 'react';
import { ModuleId, ArrangementMode, ConnectionStats } from '../../types/modules';
import { packetConnection } from '../../services/packetConnection';
import {
  Cpu,
  Layers,
  LayoutGrid,
  FileText,
  Sliders,
  HardDrive,
  Terminal,
  Monitor,
  Gauge,
  Wifi,
  WifiOff,
  Check,
  ChevronDown,
  Bug,
  Play,
  Square,
  RefreshCw,
  FolderOpen,
  Info,
  Maximize,
  HelpCircle,
} from 'lucide-react';

interface MenuBarProps {
  openModules: ModuleId[];
  onToggleModule: (id: ModuleId) => void;
  onSetAllModules: (visible: boolean) => void;
  arrangementMode: ArrangementMode;
  onSetArrangementMode: (mode: ArrangementMode) => void;
  onOpenConnectModal: () => void;
  onOpenPacketInspector: () => void;
  activeViewDropdown?: boolean;
}

export const MenuBar: React.FC<MenuBarProps> = ({
  openModules,
  onToggleModule,
  onSetAllModules,
  arrangementMode,
  onSetArrangementMode,
  onOpenConnectModal,
  onOpenPacketInspector,
}) => {
  const [activeMenu, setActiveMenu] = useState<string | null>(null);
  const [stats, setStats] = useState<ConnectionStats>({ ...packetConnection.stats });
  const [isSimulating, setIsSimulating] = useState<boolean>(packetConnection.isSimulatorActive());
  const menuBarRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    const unsub = packetConnection.subscribeStats((newStats) => {
      setStats(newStats);
      setIsSimulating(packetConnection.isSimulatorActive());
    });
    return () => unsub();
  }, []);

  // Close menus when clicking outside
  useEffect(() => {
    const handleClickOutside = (event: MouseEvent) => {
      if (menuBarRef.current && !menuBarRef.current.contains(event.target as Node)) {
        setActiveMenu(null);
      }
    };
    document.addEventListener('mousedown', handleClickOutside);
    return () => document.removeEventListener('mousedown', handleClickOutside);
  }, []);

  const toggleMenu = (menuName: string) => {
    setActiveMenu(activeMenu === menuName ? null : menuName);
  };

  const handleMenuHover = (menuName: string) => {
    if (activeMenu !== null) {
      setActiveMenu(menuName);
    }
  };

  const isModuleOpen = (id: ModuleId) => openModules.includes(id);

  const availableModules: Array<{ id: ModuleId; name: string; icon: React.ReactNode; shortcut: string }> = [
    { id: 'gpio', name: 'GPIO', icon: <Cpu className="w-3.5 h-3.5 text-cyan-400" />, shortcut: 'Ctrl+1' },
    { id: 'console', name: 'Console', icon: <Terminal className="w-3.5 h-3.5 text-emerald-400" />, shortcut: 'Ctrl+2' },
    { id: 'eeprom', name: 'EEPROM', icon: <HardDrive className="w-3.5 h-3.5 text-purple-400" />, shortcut: 'Ctrl+3' },
    { id: 'flash', name: 'Flash', icon: <Layers className="w-3.5 h-3.5 text-amber-400" />, shortcut: 'Ctrl+4' },
    { id: 'adc', name: 'ADC', icon: <Gauge className="w-3.5 h-3.5 text-sky-400" />, shortcut: 'Ctrl+5' },
    { id: 'display', name: '16x2 Display', icon: <Monitor className="w-3.5 h-3.5 text-blue-400" />, shortcut: 'Ctrl+6' },
  ];

  return (
    <div
      ref={menuBarRef}
      className="flex items-center justify-between px-3 py-1 bg-[#0a0f1b] border-b border-cyan-950/60 select-none text-xs text-slate-200 z-30"
    >
      {/* Left: Brand + File Menu Bar */}
      <div className="flex items-center gap-1">
        {/* Application Brand Logo */}
        <div className="flex items-center gap-2 mr-3 px-1 py-0.5">
          <div className="w-6 h-6 rounded bg-gradient-to-br from-cyan-500 to-blue-600 flex items-center justify-center shadow-[0_0_10px_rgba(6,182,212,0.4)]">
            <Cpu className="w-4 h-4 text-black font-bold" />
          </div>
          <div className="flex items-baseline gap-1">
            <span className="font-extrabold tracking-wider text-sm bg-gradient-to-r from-cyan-400 via-sky-300 to-indigo-300 bg-clip-text text-transparent">
              GET SET GO
            </span>
            <span className="text-[9px] font-mono px-1 py-0.2 rounded bg-cyan-950/90 text-cyan-400 border border-cyan-800/80 font-bold">
              MCU SIM
            </span>
          </div>
        </div>

        {/* --- File Menu --- */}
        <div className="relative">
          <button
            onClick={() => toggleMenu('file')}
            onMouseEnter={() => handleMenuHover('file')}
            className={`px-2.5 py-1 rounded text-xs transition-colors ${
              activeMenu === 'file' ? 'bg-[#1b263b] text-cyan-300' : 'hover:bg-slate-800/70 text-slate-300'
            }`}
          >
            File
          </button>

          {activeMenu === 'file' && (
            <div className="absolute left-0 mt-1 w-52 bg-[#0e1627] border border-cyan-900/60 rounded-md shadow-2xl py-1 z-50 text-xs">
              <button
                onClick={() => {
                  onSetAllModules(true);
                  setActiveMenu(null);
                }}
                className="w-full text-left px-3 py-1.5 hover:bg-cyan-950 hover:text-cyan-300 flex items-center justify-between"
              >
                <span>Reset to Default Layout</span>
                <span className="text-[10px] text-slate-500">Alt+R</span>
              </button>
              <button
                onClick={() => {
                  onSetAllModules(false);
                  setActiveMenu(null);
                }}
                className="w-full text-left px-3 py-1.5 hover:bg-cyan-950 hover:text-cyan-300 flex items-center justify-between"
              >
                <span>Close All Modules</span>
              </button>
              <div className="my-1 border-t border-slate-800" />
              <button
                onClick={() => {
                  packetConnection.clearLogs();
                  setActiveMenu(null);
                }}
                className="w-full text-left px-3 py-1.5 hover:bg-cyan-950 hover:text-cyan-300 flex items-center justify-between"
              >
                <span>Clear Packet Logs</span>
              </button>
              <div className="my-1 border-t border-slate-800" />
              <div className="px-3 py-1 text-[10px] text-slate-500 font-mono">
                Get Set Go v1.0.0 (FreeRTOS Simulator)
              </div>
            </div>
          )}
        </div>

        {/* --- View Menu (Requirement 3: List all available module types) --- */}
        <div className="relative">
          <button
            onClick={() => toggleMenu('view')}
            onMouseEnter={() => handleMenuHover('view')}
            className={`px-2.5 py-1 rounded text-xs flex items-center gap-1 transition-colors ${
              activeMenu === 'view' ? 'bg-[#1b263b] text-cyan-300' : 'hover:bg-slate-800/70 text-slate-300'
            }`}
          >
            <span>View</span>
            <ChevronDown className="w-3 h-3 opacity-60" />
          </button>

          {activeMenu === 'view' && (
            <div className="absolute left-0 mt-1 w-56 bg-[#0e1627] border border-cyan-900/60 rounded-md shadow-2xl py-1 z-50 text-xs">
              <div className="px-3 py-1 text-[10px] text-cyan-400 font-mono font-bold tracking-wider uppercase">
                Hardware Modules
              </div>

              {availableModules.map((m) => {
                const isOpen = isModuleOpen(m.id);
                return (
                  <button
                    key={m.id}
                    onClick={() => {
                      onToggleModule(m.id);
                    }}
                    className="w-full text-left px-3 py-1.5 hover:bg-cyan-950 hover:text-cyan-300 flex items-center justify-between transition-colors"
                  >
                    <div className="flex items-center gap-2">
                      <div className="w-4 h-4 flex items-center justify-center text-cyan-400">
                        {isOpen ? <Check className="w-3.5 h-3.5 text-cyan-400 stroke-[3]" /> : null}
                      </div>
                      <div className="flex items-center gap-1.5">
                        {m.icon}
                        <span className={isOpen ? 'text-white font-semibold' : 'text-slate-400'}>
                          {m.name}
                        </span>
                      </div>
                    </div>
                    <span className="text-[10px] font-mono text-slate-500">{m.shortcut}</span>
                  </button>
                );
              })}

              <div className="my-1 border-t border-slate-800" />
              <button
                onClick={() => {
                  onSetAllModules(true);
                  setActiveMenu(null);
                }}
                className="w-full text-left px-3 py-1 hover:bg-slate-800 text-slate-400 hover:text-white text-[11px]"
              >
                Select All Modules
              </button>
              <button
                onClick={() => {
                  onSetAllModules(false);
                  setActiveMenu(null);
                }}
                className="w-full text-left px-3 py-1 hover:bg-slate-800 text-slate-400 hover:text-white text-[11px]"
              >
                Deselect All
              </button>
            </div>
          )}
        </div>

        {/* --- Arrange Menu (Requirement 4: Tile & Tabbed modes) --- */}
        <div className="relative">
          <button
            onClick={() => toggleMenu('arrange')}
            onMouseEnter={() => handleMenuHover('arrange')}
            className={`px-2.5 py-1 rounded text-xs flex items-center gap-1 transition-colors ${
              activeMenu === 'arrange' ? 'bg-[#1b263b] text-cyan-300' : 'hover:bg-slate-800/70 text-slate-300'
            }`}
          >
            <span>Arrange</span>
            <ChevronDown className="w-3 h-3 opacity-60" />
          </button>

          {activeMenu === 'arrange' && (
            <div className="absolute left-0 mt-1 w-64 bg-[#0e1627] border border-cyan-900/60 rounded-md shadow-2xl py-1 z-50 text-xs">
              <div className="px-3 py-1 text-[10px] text-cyan-400 font-mono font-bold tracking-wider uppercase">
                Layout Modes
              </div>

              {/* Tile Mode */}
              <button
                onClick={() => {
                  onSetArrangementMode('tile');
                  setActiveMenu(null);
                }}
                className="w-full text-left px-3 py-2 hover:bg-cyan-950 hover:text-cyan-300 flex items-start gap-2.5 transition-colors"
              >
                <div className="w-4 h-4 mt-0.5 flex items-center justify-center text-cyan-400">
                  {arrangementMode === 'tile' && <Check className="w-4 h-4 text-cyan-400 stroke-[3]" />}
                </div>
                <div className="flex-1">
                  <div className="flex items-center gap-1.5 font-semibold text-slate-200">
                    <LayoutGrid className="w-3.5 h-3.5 text-cyan-400" />
                    <span>Tile</span>
                  </div>
                  <p className="text-[10px] text-slate-400 mt-0.5">
                    Grid of available open modules on the whole screen
                  </p>
                </div>
              </button>

              {/* Tabbed Mode */}
              <button
                onClick={() => {
                  onSetArrangementMode('tabbed');
                  setActiveMenu(null);
                }}
                className="w-full text-left px-3 py-2 hover:bg-cyan-950 hover:text-cyan-300 flex items-start gap-2.5 transition-colors"
              >
                <div className="w-4 h-4 mt-0.5 flex items-center justify-center text-cyan-400">
                  {arrangementMode === 'tabbed' && <Check className="w-4 h-4 text-cyan-400 stroke-[3]" />}
                </div>
                <div className="flex-1">
                  <div className="flex items-center gap-1.5 font-semibold text-slate-200">
                    <FileText className="w-3.5 h-3.5 text-cyan-400" />
                    <span>Tabbed</span>
                  </div>
                  <p className="text-[10px] text-slate-400 mt-0.5">
                    All open modules appear as tabbed documents; selecting a tab opens on whole window
                  </p>
                </div>
              </button>
            </div>
          )}
        </div>

        {/* --- Connect Menu (Requirement 5: Open popup to configure IP & port) --- */}
        <div className="relative">
          <button
            onClick={() => {
              onOpenConnectModal();
              setActiveMenu(null);
            }}
            className="px-2.5 py-1 rounded text-xs flex items-center gap-1.5 transition-colors hover:bg-cyan-950 hover:text-cyan-300 text-cyan-400 font-semibold"
          >
            <Wifi className="w-3.5 h-3.5" />
            <span>Connect...</span>
          </button>
        </div>

        {/* --- Tools Menu --- */}
        <div className="relative">
          <button
            onClick={() => toggleMenu('tools')}
            onMouseEnter={() => handleMenuHover('tools')}
            className={`px-2.5 py-1 rounded text-xs transition-colors ${
              activeMenu === 'tools' ? 'bg-[#1b263b] text-cyan-300' : 'hover:bg-slate-800/70 text-slate-300'
            }`}
          >
            Tools
          </button>

          {activeMenu === 'tools' && (
            <div className="absolute left-0 mt-1 w-56 bg-[#0e1627] border border-cyan-900/60 rounded-md shadow-2xl py-1 z-50 text-xs">
              <button
                onClick={() => {
                  onOpenPacketInspector();
                  setActiveMenu(null);
                }}
                className="w-full text-left px-3 py-1.5 hover:bg-cyan-950 hover:text-cyan-300 flex items-center gap-2"
              >
                <Bug className="w-3.5 h-3.5 text-cyan-400" />
                <span>Raw JSON Packet Inspector</span>
              </button>

              <button
                onClick={() => {
                  const running = packetConnection.toggleSimulation();
                  setIsSimulating(running);
                  setActiveMenu(null);
                }}
                className="w-full text-left px-3 py-1.5 hover:bg-cyan-950 hover:text-cyan-300 flex items-center gap-2"
              >
                {isSimulating ? <Square className="w-3.5 h-3.5 text-rose-400" /> : <Play className="w-3.5 h-3.5 text-emerald-400" />}
                <span>{isSimulating ? 'Stop FreeRTOS Demo Traffic' : 'Simulate FreeRTOS Driver Packets'}</span>
              </button>
            </div>
          )}
        </div>
      </div>

      {/* Right: Live Connection Status Pill & Quick Action Buttons */}
      <div className="flex items-center gap-2">
        {/* Packet Activity Counter */}
        <button
          onClick={onOpenPacketInspector}
          className="flex items-center gap-1.5 px-2 py-0.5 rounded bg-[#0e1726] border border-slate-800 hover:border-cyan-700 text-slate-300 font-mono text-[11px] transition-colors"
          title="Click to view live packet inspector"
        >
          <span className="w-2 h-2 rounded-full bg-cyan-400 animate-ping inline-block" />
          <span>Pkts: {stats.packetsReceived + stats.packetsSent}</span>
        </button>

        {/* Connection status pill */}
        <button
          onClick={onOpenConnectModal}
          className={`flex items-center gap-2 px-2.5 py-1 rounded-full border text-[11px] font-mono font-medium transition-all ${
            stats.mcuConnected
              ? 'bg-emerald-950/60 border-emerald-500/60 text-emerald-300 hover:bg-emerald-900/60'
              : stats.backendConnected
              ? 'bg-cyan-950/60 border-cyan-500/60 text-cyan-300 hover:bg-cyan-900/60'
              : 'bg-rose-950/50 border-rose-600/50 text-rose-300 hover:bg-rose-900/50'
          }`}
          title="Click to configure IP and Port"
        >
          <div
            className={`w-2 h-2 rounded-full ${
              stats.mcuConnected
                ? 'bg-emerald-400 shadow-[0_0_8px_#10b981]'
                : stats.backendConnected
                ? 'bg-cyan-400'
                : 'bg-rose-500'
            }`}
          />
          <span>
            {stats.mcuConnected
              ? `MCU Connected (${packetConnection.config.ip}:${packetConnection.config.port})`
              : stats.backendConnected
              ? `Listening :${packetConnection.config.port}`
              : 'Disconnected'}
          </span>
        </button>

        {/* Mode Quick Switch */}
        <div className="flex items-center bg-[#0e1726] p-0.5 rounded border border-slate-800">
          <button
            onClick={() => onSetArrangementMode('tile')}
            className={`p-1 rounded transition-colors ${
              arrangementMode === 'tile' ? 'bg-cyan-500 text-black font-bold' : 'text-slate-400 hover:text-white'
            }`}
            title="Tile arrangement mode"
          >
            <LayoutGrid className="w-3.5 h-3.5" />
          </button>
          <button
            onClick={() => onSetArrangementMode('tabbed')}
            className={`p-1 rounded transition-colors ${
              arrangementMode === 'tabbed' ? 'bg-cyan-500 text-black font-bold' : 'text-slate-400 hover:text-white'
            }`}
            title="Tabbed arrangement mode"
          >
            <FileText className="w-3.5 h-3.5" />
          </button>
        </div>
      </div>
    </div>
  );
};
