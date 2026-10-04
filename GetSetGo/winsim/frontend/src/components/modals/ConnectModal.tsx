import React, { useState, useEffect } from 'react';
import { ConnectionConfig, ConnectionStats } from '../../types/modules';
import { packetConnection } from '../../services/packetConnection';
import { X, Network, Server, Wifi, WifiOff, CheckCircle2, AlertCircle, Play, Square, Activity } from 'lucide-react';

interface ConnectModalProps {
  isOpen: boolean;
  onClose: () => void;
}

export const ConnectModal: React.FC<ConnectModalProps> = ({ isOpen, onClose }) => {
  const [config, setConfig] = useState<ConnectionConfig>({ ...packetConnection.config });
  const [stats, setStats] = useState<ConnectionStats>({ ...packetConnection.stats });
  const [isSimulating, setIsSimulating] = useState<boolean>(packetConnection.isSimulatorActive());

  useEffect(() => {
    const unsub = packetConnection.subscribeStats((newStats) => {
      setStats(newStats);
      setIsSimulating(packetConnection.isSimulatorActive());
    });
    return () => unsub();
  }, []);

  if (!isOpen) return null;

  const handleSaveAndConnect = () => {
    packetConnection.setConfig(config);
    packetConnection.connectTcp(config);
    onClose();
  };

  const handleDisconnect = () => {
    packetConnection.disconnectTcp();
  };

  const toggleSim = () => {
    const active = packetConnection.toggleSimulation();
    setIsSimulating(active);
  };

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/70 backdrop-blur-xs select-none">
      <div className="relative w-full max-w-lg bg-[#0e1626] border border-cyan-500/30 rounded-xl shadow-2xl overflow-hidden flex flex-col">
        {/* Header */}
        <div className="flex items-center justify-between px-5 py-3.5 bg-[#142033] border-b border-cyan-900/50">
          <div className="flex items-center gap-2 text-cyan-400 font-bold text-sm">
            <Network className="w-5 h-5 text-cyan-400" />
            <span>TCP DRIVER CONNECTION CONFIGURATION</span>
          </div>
          <button
            onClick={onClose}
            className="p-1 rounded-lg text-slate-400 hover:text-white hover:bg-slate-800 transition-colors"
          >
            <X className="w-4 h-4" />
          </button>
        </div>

        {/* Content Body */}
        <div className="p-5 flex flex-col gap-4 text-xs font-mono text-slate-200">
          {/* Connection Status Banner */}
          <div
            className={`p-3 rounded-lg border flex items-center justify-between ${
              stats.mcuConnected
                ? 'bg-emerald-950/40 border-emerald-500/50 text-emerald-300'
                : stats.backendConnected
                ? 'bg-cyan-950/30 border-cyan-500/40 text-cyan-300'
                : 'bg-rose-950/30 border-rose-500/40 text-rose-300'
            }`}
          >
            <div className="flex items-center gap-2">
              {stats.mcuConnected ? (
                <Wifi className="w-5 h-5 text-emerald-400 animate-pulse" />
              ) : (
                <WifiOff className="w-5 h-5 text-slate-400" />
              )}
              <div>
                <div className="font-bold">
                  {stats.mcuConnected
                    ? 'MCU DRIVER CONNECTED'
                    : stats.backendConnected
                    ? 'TCP SOCKET READY / LISTENING'
                    : 'BACKEND DISCONNECTED'}
                </div>
                <div className="text-[10px] text-slate-400 font-normal">
                  {stats.mcuConnected
                    ? `Remote: ${stats.clientAddress || config.ip}`
                    : `Listening on ${config.ip}:${config.port}`}
                </div>
              </div>
            </div>
            <div className="text-right text-[10px]">
              <div>Pkts In: <span className="font-bold">{stats.packetsReceived}</span></div>
              <div>Pkts Out: <span className="font-bold">{stats.packetsSent}</span></div>
            </div>
          </div>

          {/* Form Fields */}
          <div className="grid grid-cols-2 gap-3">
            <div>
              <label className="block text-[11px] text-slate-400 mb-1 font-semibold">
                IP Address / Host
              </label>
              <input
                type="text"
                value={config.ip}
                onChange={(e) => setConfig({ ...config, ip: e.target.value })}
                placeholder="127.0.0.1"
                className="w-full bg-[#080d16] border border-slate-700 focus:border-cyan-500 rounded px-3 py-1.5 text-xs text-slate-100 focus:outline-none"
              />
              <span className="text-[10px] text-slate-500 mt-0.5 block">127.0.0.1 for local FreeRTOS Windows app</span>
            </div>

            <div>
              <label className="block text-[11px] text-slate-400 mb-1 font-semibold">
                TCP Port
              </label>
              <input
                type="number"
                value={config.port}
                onChange={(e) => setConfig({ ...config, port: parseInt(e.target.value) || 9000 })}
                placeholder="9000"
                className="w-full bg-[#080d16] border border-slate-700 focus:border-cyan-500 rounded px-3 py-1.5 text-xs text-slate-100 focus:outline-none"
              />
              <span className="text-[10px] text-slate-500 mt-0.5 block">Default port: 9000</span>
            </div>
          </div>

          {/* Socket Mode */}
          <div>
            <label className="block text-[11px] text-slate-400 mb-1 font-semibold">
              Socket Mode
            </label>
            <div className="grid grid-cols-2 gap-2">
              <button
                type="button"
                onClick={() => setConfig({ ...config, mode: 'server' })}
                className={`py-2 px-3 rounded border text-left flex flex-col gap-0.5 transition-all ${
                  config.mode === 'server'
                    ? 'border-cyan-500 bg-cyan-950/40 text-cyan-300 font-bold'
                    : 'border-slate-800 bg-[#0a101d] text-slate-400 hover:border-slate-700'
                }`}
              >
                <div className="flex items-center gap-1.5">
                  <Server className="w-3.5 h-3.5" />
                  <span>TCP Server (Recommended)</span>
                </div>
                <span className="text-[10px] text-slate-500 font-normal">
                  Simulator listens for FreeRTOS client connection
                </span>
              </button>

              <button
                type="button"
                onClick={() => setConfig({ ...config, mode: 'client' })}
                className={`py-2 px-3 rounded border text-left flex flex-col gap-0.5 transition-all ${
                  config.mode === 'client'
                    ? 'border-cyan-500 bg-cyan-950/40 text-cyan-300 font-bold'
                    : 'border-slate-800 bg-[#0a101d] text-slate-400 hover:border-slate-700'
                }`}
              >
                <div className="flex items-center gap-1.5">
                  <Network className="w-3.5 h-3.5" />
                  <span>TCP Client</span>
                </div>
                <span className="text-[10px] text-slate-500 font-normal">
                  Simulator connects to FreeRTOS server socket
                </span>
              </button>
            </div>
          </div>

          {/* Auto Reconnect */}
          <div className="flex items-center justify-between p-2 rounded bg-[#090e18] border border-slate-800/80">
            <span className="text-[11px] text-slate-300 font-medium">Automatic Reconnection</span>
            <input
              type="checkbox"
              checked={config.autoReconnect}
              onChange={(e) => setConfig({ ...config, autoReconnect: e.target.checked })}
              className="accent-cyan-500 w-4 h-4 cursor-pointer"
            />
          </div>

          {/* Quick Demo Simulator Toggle */}
          <div className="p-3 rounded-lg bg-indigo-950/30 border border-indigo-500/30 flex items-center justify-between">
            <div>
              <div className="font-bold text-indigo-300 flex items-center gap-1.5">
                <Activity className="w-4 h-4 text-indigo-400" />
                <span>Simulate FreeRTOS MCU Traffic</span>
              </div>
              <div className="text-[10px] text-indigo-200/70 font-normal">
                Generate live test packets (LED pulses, ADC waves, LCD text)
              </div>
            </div>
            <button
              onClick={toggleSim}
              className={`px-3 py-1 rounded text-xs font-semibold flex items-center gap-1 transition-all ${
                isSimulating
                  ? 'bg-rose-600 hover:bg-rose-500 text-white'
                  : 'bg-indigo-600 hover:bg-indigo-500 text-white'
              }`}
            >
              {isSimulating ? <Square className="w-3 h-3" /> : <Play className="w-3 h-3" />}
              <span>{isSimulating ? 'Stop Traffic' : 'Start Traffic'}</span>
            </button>
          </div>
        </div>

        {/* Footer */}
        <div className="flex items-center justify-between px-5 py-3 bg-[#0a101c] border-t border-slate-800">
          <button
            onClick={handleDisconnect}
            className="px-3 py-1.5 rounded bg-slate-800 hover:bg-rose-950/60 hover:text-rose-400 border border-slate-700 text-slate-300 text-xs font-medium transition-colors"
          >
            Disconnect Socket
          </button>
          <div className="flex items-center gap-2">
            <button
              onClick={onClose}
              className="px-3 py-1.5 rounded bg-slate-800 hover:bg-slate-700 text-slate-300 text-xs transition-colors"
            >
              Cancel
            </button>
            <button
              onClick={handleSaveAndConnect}
              className="px-4 py-1.5 rounded bg-cyan-500 hover:bg-cyan-400 text-black font-bold text-xs shadow-md transition-colors"
            >
              Apply &amp; Connect
            </button>
          </div>
        </div>
      </div>
    </div>
  );
};
