import React from 'react';
import { ModuleId } from '../../types/modules';
import { Maximize2, Minimize2, X, Minus } from 'lucide-react';

interface ModuleWrapperProps {
  id: ModuleId;
  title: string;
  icon: React.ReactNode;
  children: React.ReactNode;
  isMaximized?: boolean;
  onToggleMaximize?: () => void;
  onClose?: () => void;
  badge?: string;
}

export const ModuleWrapper: React.FC<ModuleWrapperProps> = ({
  id,
  title,
  icon,
  children,
  isMaximized = false,
  onToggleMaximize,
  onClose,
  badge,
}) => {
  return (
    <div
      className={`flex flex-col rounded-lg border border-slate-800 bg-[#0c121e] overflow-hidden shadow-lg transition-all duration-150 ${
        isMaximized ? 'fixed inset-4 z-40' : 'h-full w-full'
      }`}
    >
      {/* Window Title Bar */}
      <div className="flex items-center justify-between px-3 py-1.5 bg-[#111928] border-b border-slate-800 select-none">
        <div className="flex items-center gap-2">
          <div className="text-cyan-400">{icon}</div>
          <span className="font-mono font-bold text-xs text-slate-200 tracking-wide">
            {title}
          </span>
          {badge && (
            <span className="text-[10px] font-mono px-1.5 py-0.2 rounded bg-cyan-950 text-cyan-300 border border-cyan-800">
              {badge}
            </span>
          )}
        </div>

        <div className="flex items-center gap-1">
          {onToggleMaximize && (
            <button
              onClick={onToggleMaximize}
              className="p-1 rounded text-slate-400 hover:text-white hover:bg-slate-800 transition-colors"
              title={isMaximized ? 'Restore Window' : 'Maximize Window'}
            >
              {isMaximized ? <Minimize2 className="w-3.5 h-3.5" /> : <Maximize2 className="w-3.5 h-3.5" />}
            </button>
          )}
          {onClose && (
            <button
              onClick={onClose}
              className="p-1 rounded text-slate-400 hover:text-rose-400 hover:bg-rose-950/40 transition-colors"
              title="Close Module Window"
            >
              <X className="w-3.5 h-3.5" />
            </button>
          )}
        </div>
      </div>

      {/* Module Content */}
      <div className="flex-1 overflow-hidden relative">
        {children}
      </div>
    </div>
  );
};
