import React, { useState, useEffect } from 'react';
import { ModuleId, ArrangementMode, AppTheme } from './types/modules';
import { MenuBar } from './components/menu/MenuBar';
import { TileLayout } from './components/layout/TileLayout';
import { TabLayout } from './components/layout/TabLayout';
import { ConnectModal } from './components/modals/ConnectModal';
import { PacketInspectorModal } from './components/modals/PacketInspectorModal';

const DEFAULT_MODULES: ModuleId[] = [
  'gpio',
  'adc',
  'timers',
  'can',
  'canopen',
  'events',
];

export const App: React.FC = () => {
  // Theme state: dark / light
  const [theme, setTheme] = useState<AppTheme>(() => {
    const saved = localStorage.getItem('getsetgo_theme') as AppTheme;
    return saved === 'light' ? 'light' : 'dark';
  });

  // Open modules list
  const [openModules, setOpenModules] = useState<ModuleId[]>(() => {
    try {
      const saved = localStorage.getItem('getsetgo_open_modules');
      if (saved) {
        const parsed = JSON.parse(saved);
        if (Array.isArray(parsed) && parsed.length > 0) return parsed;
      }
    } catch {
      // ignore
    }
    return DEFAULT_MODULES;
  });

  // Arrangement mode: 'tile' | 'tabbed'
  const [arrangementMode, setArrangementMode] = useState<ArrangementMode>(() => {
    return (localStorage.getItem('getsetgo_mode') as ArrangementMode) || 'tile';
  });

  // Active tab in tabbed mode
  const [activeTab, setActiveTab] = useState<ModuleId>(openModules[0] || 'gpio');

  // Modals state
  const [isConnectModalOpen, setIsConnectModalOpen] = useState<boolean>(false);
  const [isPacketInspectorOpen, setIsPacketInspectorOpen] = useState<boolean>(false);

  // Apply theme class to document root
  useEffect(() => {
    document.documentElement.classList.remove('dark', 'light');
    document.documentElement.classList.add(theme);
    localStorage.setItem('getsetgo_theme', theme);
  }, [theme]);

  // Persist modules & arrangement mode
  useEffect(() => {
    localStorage.setItem('getsetgo_open_modules', JSON.stringify(openModules));
    if (!openModules.includes(activeTab) && openModules.length > 0) {
      setActiveTab(openModules[0]);
    }
  }, [openModules, activeTab]);

  useEffect(() => {
    localStorage.setItem('getsetgo_mode', arrangementMode);
  }, [arrangementMode]);

  const toggleTheme = () => {
    setTheme((prev) => (prev === 'dark' ? 'light' : 'dark'));
  };

  const toggleModule = (id: ModuleId) => {
    setOpenModules((prev) => {
      if (prev.includes(id)) {
        return prev.filter((m) => m !== id);
      } else {
        return [...prev, id];
      }
    });
    setActiveTab(id);
  };

  const closeModule = (id: ModuleId) => {
    setOpenModules((prev) => prev.filter((m) => m !== id));
  };

  const setAllModules = (visible: boolean) => {
    if (visible) {
      setOpenModules([
        'gpio',
        'console',
        'eeprom',
        'flash',
        'adc',
        'display',
        'timers',
        'can',
        'canopen',
        'events',
      ]);
    } else {
      setOpenModules([]);
    }
  };

  // Global Keyboard Shortcuts
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.target instanceof HTMLInputElement || e.target instanceof HTMLTextAreaElement) {
        return;
      }

      // Alt+T: Toggle Theme
      if (e.altKey && (e.key === 't' || e.key === 'T')) {
        e.preventDefault();
        toggleTheme();
      }

      // Alt+R: Reset layout
      if (e.altKey && (e.key === 'r' || e.key === 'R')) {
        e.preventDefault();
        setOpenModules([
          'gpio',
          'adc',
          'timers',
          'can',
          'canopen',
          'events',
        ]);
      }

      // Alt+A: Toggle Tile / Tabbed
      if (e.altKey && (e.key === 'a' || e.key === 'A')) {
        e.preventDefault();
        setArrangementMode((prev) => (prev === 'tile' ? 'tabbed' : 'tile'));
      }

      // Ctrl + Number shortcuts for quick module toggling
      if (e.ctrlKey && !e.shiftKey && !e.altKey) {
        const moduleMap: Record<string, ModuleId> = {
          '1': 'gpio',
          '2': 'console',
          '3': 'eeprom',
          '4': 'flash',
          '5': 'adc',
          '6': 'display',
          '7': 'timers',
          '8': 'can',
          '9': 'canopen',
          '0': 'events',
        };
        const target = moduleMap[e.key];
        if (target) {
          e.preventDefault();
          toggleModule(target);
        }
      }
    };

    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, []);

  return (
    <div className={`w-screen h-screen flex flex-col overflow-hidden select-none ${theme}`}>
      {/* Top Application Menu Bar */}
      <MenuBar
        openModules={openModules}
        onToggleModule={toggleModule}
        onSetAllModules={setAllModules}
        arrangementMode={arrangementMode}
        onSetArrangementMode={setArrangementMode}
        theme={theme}
        onToggleTheme={toggleTheme}
        onOpenConnectModal={() => setIsConnectModalOpen(true)}
        onOpenPacketInspector={() => setIsPacketInspectorOpen(true)}
      />

      {/* Main Workspace Layout (Tile Grid or Tabbed Documents) */}
      <div className="flex-1 overflow-hidden relative flex flex-col">
        {arrangementMode === 'tile' ? (
          <TileLayout
            openModules={openModules}
            onCloseModule={closeModule}
            onOpenViewMenu={() => setAllModules(true)}
          />
        ) : (
          <TabLayout
            openModules={openModules}
            activeTab={activeTab}
            onSelectTab={setActiveTab}
            onCloseTab={closeModule}
            onOpenViewMenu={() => setAllModules(true)}
          />
        )}
      </div>

      {/* Popups & Modals */}
      <ConnectModal
        isOpen={isConnectModalOpen}
        onClose={() => setIsConnectModalOpen(false)}
      />

      <PacketInspectorModal
        isOpen={isPacketInspectorOpen}
        onClose={() => setIsPacketInspectorOpen(false)}
      />
    </div>
  );
};

export default App;

