import React from 'react';
import { ModuleId } from '../../types/modules';
import { GpioModule } from '../../modules/GpioModule';
import { ConsoleModule } from '../../modules/ConsoleModule';
import { EepromModule } from '../../modules/EepromModule';
import { FlashModule } from '../../modules/FlashModule';
import { AdcModule } from '../../modules/AdcModule';
import { Display16x2Module } from '../../modules/Display16x2Module';
import { Cpu, Terminal, HardDrive, Layers, Gauge, Monitor, X, Plus } from 'lucide-react';

interface TabLayoutProps {
  openModules: ModuleId[];
  activeTab: ModuleId;
  onSelectTab: (id: ModuleId) => void;
  onCloseTab: (id: ModuleId) => void;
  onOpenViewMenu: () => void;
}

export const TabLayout: React.FC<TabLayoutProps> = ({
  openModules,
  activeTab,
  onSelectTab,
  onCloseTab,
  onOpenViewMenu,
}) => {
  const getTabMeta = (id: ModuleId) => {
    switch (id) {
      case 'gpio':
        return { title: 'GPIO Controller', icon: <Cpu className="w-3.5 h-3.5 text-cyan-400" /> };
      case 'console':
        return { title: 'UART Terminal', icon: <Terminal className="w-3.5 h-3.5 text-emerald-400" /> };
      case 'eeprom':
        return { title: 'EEPROM Memory', icon: <HardDrive className="w-3.5 h-3.5 text-purple-400" /> };
      case 'flash':
        return { title: 'NOR Flash', icon: <Layers className="w-3.5 h-3.5 text-amber-400" /> };
      case 'adc':
        return { title: 'ADC Channels', icon: <Gauge className="w-3.5 h-3.5 text-sky-400" /> };
      case 'display':
        return { title: '16x2 LCD', icon: <Monitor className="w-3.5 h-3.5 text-blue-400" /> };
    }
  };

  const renderActiveModule = () => {
    switch (activeTab) {
      case 'gpio':
        return <GpioModule />;
      case 'console':
        return <ConsoleModule />;
      case 'eeprom':
        return <EepromModule />;
      case 'flash':
        return <FlashModule />;
      case 'adc':
        return <AdcModule />;
      case 'display':
        return <Display16x2Module />;
      default:
        return null;
    }
  };

  if (openModules.length === 0) {
    return (
      <div className="flex-1 flex flex-col items-center justify-center p-8 text-center bg-[#070b13]">
        <div className="p-4 rounded-full bg-slate-900 border border-slate-800 mb-4">
          <Cpu className="w-10 h-10 text-cyan-500/60" />
        </div>
        <h3 className="text-lg font-bold text-slate-200 mb-2">No Open Document Tabs</h3>
        <p className="text-xs text-slate-400 max-w-sm mb-4">
          Open a module from the <strong className="text-cyan-400">View</strong> menu to work with it in Tabbed mode.
        </p>
        <button
          onClick={onOpenViewMenu}
          className="px-4 py-2 rounded bg-cyan-600 hover:bg-cyan-500 text-black font-semibold text-xs transition-colors"
        >
          Select Module to Open
        </button>
      </div>
    );
  }

  return (
    <div className="flex-1 flex flex-col h-full overflow-hidden bg-[#070b13]">
      {/* Tab Navigation Strip */}
      <div className="flex items-center px-2 bg-[#0c121e] border-b border-slate-800 overflow-x-auto select-none gap-1">
        {openModules.map((id) => {
          const meta = getTabMeta(id);
          const isActive = activeTab === id;

          return (
            <div
              key={id}
              onClick={() => onSelectTab(id)}
              className={`group flex items-center gap-2 px-3 py-2 border-b-2 text-xs font-mono font-medium cursor-pointer transition-all ${
                isActive
                  ? 'bg-[#121c2d] border-cyan-400 text-slate-100 shadow-inner'
                  : 'bg-transparent border-transparent text-slate-400 hover:text-slate-200 hover:bg-slate-900/50'
              }`}
            >
              {meta.icon}
              <span>{meta.title}</span>
              <button
                onClick={(e) => {
                  e.stopPropagation();
                  onCloseTab(id);
                }}
                className="opacity-0 group-hover:opacity-100 p-0.5 rounded hover:bg-slate-700 text-slate-400 hover:text-white transition-opacity"
                title="Close Tab"
              >
                <X className="w-3 h-3" />
              </button>
            </div>
          );
        })}

        <button
          onClick={onOpenViewMenu}
          className="p-1.5 ml-1 rounded text-slate-500 hover:text-cyan-400 hover:bg-slate-800 transition-colors"
          title="Open more modules..."
        >
          <Plus className="w-3.5 h-3.5" />
        </button>
      </div>

      {/* Full-Window Active Module Workspace */}
      <div className="flex-1 overflow-hidden relative">
        {renderActiveModule()}
      </div>
    </div>
  );
};
