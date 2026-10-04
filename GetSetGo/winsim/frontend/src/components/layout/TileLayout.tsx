import React, { useState } from 'react';
import { ModuleId } from '../../types/modules';
import { ModuleWrapper } from './ModuleWrapper';
import { GpioModule } from '../../modules/GpioModule';
import { ConsoleModule } from '../../modules/ConsoleModule';
import { EepromModule } from '../../modules/EepromModule';
import { FlashModule } from '../../modules/FlashModule';
import { AdcModule } from '../../modules/AdcModule';
import { Display16x2Module } from '../../modules/Display16x2Module';
import { Cpu, Terminal, HardDrive, Layers, Gauge, Monitor } from 'lucide-react';

interface TileLayoutProps {
  openModules: ModuleId[];
  onCloseModule: (id: ModuleId) => void;
  onOpenViewMenu: () => void;
}

export const TileLayout: React.FC<TileLayoutProps> = ({
  openModules,
  onCloseModule,
  onOpenViewMenu,
}) => {
  const [maximizedId, setMaximizedId] = useState<ModuleId | null>(null);

  const getModuleInfo = (id: ModuleId) => {
    switch (id) {
      case 'gpio':
        return { title: 'GPIO Controller', icon: <Cpu className="w-4 h-4 text-cyan-400" />, component: <GpioModule /> };
      case 'console':
        return { title: 'UART Serial Terminal', icon: <Terminal className="w-4 h-4 text-emerald-400" />, component: <ConsoleModule /> };
      case 'eeprom':
        return { title: 'EEPROM Memory Hex/ASCII', icon: <HardDrive className="w-4 h-4 text-purple-400" />, component: <EepromModule /> };
      case 'flash':
        return { title: 'NOR/Internal Flash Blocks', icon: <Layers className="w-4 h-4 text-amber-400" />, component: <FlashModule /> };
      case 'adc':
        return { title: 'ADC Sampler & Analog Injector', icon: <Gauge className="w-4 h-4 text-sky-400" />, component: <AdcModule /> };
      case 'display':
        return { title: '16x2 Character LCD', icon: <Monitor className="w-4 h-4 text-blue-400" />, component: <Display16x2Module /> };
    }
  };

  if (openModules.length === 0) {
    return (
      <div className="flex-1 flex flex-col items-center justify-center p-8 text-center bg-[#070b13]">
        <div className="p-4 rounded-full bg-slate-900 border border-slate-800 mb-4">
          <Cpu className="w-10 h-10 text-cyan-500/60" />
        </div>
        <h3 className="text-lg font-bold text-slate-200 mb-2">No Modules Currently Visible</h3>
        <p className="text-xs text-slate-400 max-w-sm mb-4">
          Select modules from the <strong className="text-cyan-400">View</strong> menu on the top menu bar to display them on your workbench.
        </p>
        <button
          onClick={onOpenViewMenu}
          className="px-4 py-2 rounded bg-cyan-600 hover:bg-cyan-500 text-black font-semibold text-xs transition-colors"
        >
          Open View Menu
        </button>
      </div>
    );
  }

  // Dynamic grid classes based on module count
  const getGridClass = () => {
    const count = openModules.length;
    if (count === 1) return 'grid-cols-1 grid-rows-1';
    if (count === 2) return 'grid-cols-1 md:grid-cols-2 grid-rows-1';
    if (count === 3) return 'grid-cols-1 md:grid-cols-3 grid-rows-1';
    if (count === 4) return 'grid-cols-1 md:grid-cols-2 grid-rows-2';
    if (count <= 6) return 'grid-cols-1 md:grid-cols-3 grid-rows-2';
    return 'grid-cols-1 md:grid-cols-3 grid-rows-2';
  };

  return (
    <div className={`flex-1 p-2.5 grid ${getGridClass()} gap-2.5 overflow-hidden bg-[#070b13]`}>
      {openModules.map((id) => {
        const info = getModuleInfo(id);
        const isMax = maximizedId === id;

        return (
          <div key={id} className="min-h-0 min-w-0 h-full w-full">
            <ModuleWrapper
              id={id}
              title={info.title}
              icon={info.icon}
              isMaximized={isMax}
              onToggleMaximize={() => setMaximizedId(isMax ? null : id)}
              onClose={() => {
                if (isMax) setMaximizedId(null);
                onCloseModule(id);
              }}
            >
              {info.component}
            </ModuleWrapper>
          </div>
        );
      })}
    </div>
  );
};
