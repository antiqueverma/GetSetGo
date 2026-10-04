import React, { useState, useEffect, useRef } from 'react';
import { ConsoleMessage, BasePacket } from '../types/modules';
import { packetConnection } from '../services/packetConnection';
import { Terminal, Send, Trash2, Download, CornerDownLeft, ArrowDown, Settings2 } from 'lucide-react';

export const ConsoleModule: React.FC = () => {
  const [messages, setMessages] = useState<ConsoleMessage[]>([
    {
      id: 'init-1',
      timestamp: new Date().toLocaleTimeString(),
      direction: 'sys',
      text: '--- Virtual UART Port [COM-VIRT-0] Ready @ 115200 8N1 ---',
    },
  ]);
  const [inputValue, setInputValue] = useState('');
  const [baudRate, setBaudRate] = useState<number>(115200);
  const [lineEnding, setLineEnding] = useState<string>('\\r\\n');
  const [autoScroll, setAutoScroll] = useState<boolean>(true);
  const [historyIndex, setHistoryIndex] = useState<number>(-1);
  const [commandHistory, setCommandHistory] = useState<string[]>([]);

  const terminalEndRef = useRef<HTMLDivElement>(null);
  const inputRef = useRef<HTMLInputElement>(null);

  // Auto scroll to bottom
  useEffect(() => {
    if (autoScroll) {
      terminalEndRef.current?.scrollIntoView({ behavior: 'smooth' });
    }
  }, [messages, autoScroll]);

  // Subscribe to console packets from MCU
  useEffect(() => {
    const unsubscribe = packetConnection.subscribe('console', (packet: BasePacket) => {
      if (packet.text !== undefined) {
        const newMsg: ConsoleMessage = {
          id: Math.random().toString(36).substring(2, 9),
          timestamp: new Date().toLocaleTimeString(),
          direction: (packet.direction as any) || 'tx',
          text: String(packet.text),
        };

        setMessages((prev) => [...prev.slice(-499), newMsg]);
      }
    });

    return () => unsubscribe();
  }, []);

  const handleSendMessage = (textToSend?: string) => {
    const text = textToSend !== undefined ? textToSend : inputValue;
    if (!text.trim() && textToSend === undefined) return;

    // Format line ending
    let payloadText = text;
    if (lineEnding === '\\r\\n') payloadText += '\r\n';
    else if (lineEnding === '\\n') payloadText += '\n';
    else if (lineEnding === '\\r') payloadText += '\r';

    // Send packet to FreeRTOS over TCP
    packetConnection.sendPacket({
      module: 'console',
      action: 'rx',
      text: payloadText,
      baudRate,
    });

    // Record locally in message list
    const userMsg: ConsoleMessage = {
      id: Math.random().toString(36).substring(2, 9),
      timestamp: new Date().toLocaleTimeString(),
      direction: 'rx',
      text,
    };

    setMessages((prev) => [...prev.slice(-499), userMsg]);
    setCommandHistory((prev) => [text, ...prev.filter((h) => h !== text)].slice(0, 30));
    setHistoryIndex(-1);
    setInputValue('');
  };

  const handleKeyDown = (e: React.KeyboardEvent<HTMLInputElement>) => {
    if (e.key === 'Enter') {
      handleSendMessage();
    } else if (e.key === 'ArrowUp') {
      e.preventDefault();
      if (commandHistory.length > 0) {
        const nextIdx = Math.min(historyIndex + 1, commandHistory.length - 1);
        setHistoryIndex(nextIdx);
        setInputValue(commandHistory[nextIdx]);
      }
    } else if (e.key === 'ArrowDown') {
      e.preventDefault();
      if (historyIndex > 0) {
        const nextIdx = historyIndex - 1;
        setHistoryIndex(nextIdx);
        setInputValue(commandHistory[nextIdx]);
      } else if (historyIndex === 0) {
        setHistoryIndex(-1);
        setInputValue('');
      }
    }
  };

  const clearConsole = () => {
    setMessages([
      {
        id: 'cleared',
        timestamp: new Date().toLocaleTimeString(),
        direction: 'sys',
        text: '--- Console buffer cleared ---',
      },
    ]);
  };

  const exportLog = () => {
    const content = messages.map((m) => `[${m.timestamp}] [${m.direction.toUpperCase()}] ${m.text}`).join('\n');
    const blob = new Blob([content], { type: 'text/plain' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `uart-log-${Date.now()}.txt`;
    a.click();
    URL.revokeObjectURL(url);
  };

  return (
    <div className="flex flex-col h-full bg-[#0a0e17] text-slate-200 select-none overflow-hidden">
      {/* Top Toolbar */}
      <div className="flex items-center justify-between px-3 py-2 bg-[#121927] border-b border-cyan-900/40 text-xs">
        <div className="flex items-center gap-2">
          <div className="flex items-center gap-1.5 font-semibold text-emerald-400">
            <Terminal className="w-4 h-4 text-emerald-400" />
            <span>UART / SERIAL CONSOLE</span>
          </div>
          <span className="text-slate-500">|</span>
          <div className="flex items-center gap-1 text-[11px] font-mono text-slate-300">
            <span>Baud:</span>
            <select
              value={baudRate}
              onChange={(e) => setBaudRate(Number(e.target.value))}
              className="bg-[#0b101b] border border-slate-700 rounded px-1.5 py-0.5 text-cyan-400 focus:outline-none focus:border-cyan-500"
            >
              {[9600, 19200, 38400, 57600, 115200, 921600].map((b) => (
                <option key={b} value={b}>
                  {b}
                </option>
              ))}
            </select>
          </div>
          <div className="flex items-center gap-1 text-[11px] font-mono text-slate-300">
            <span>EOL:</span>
            <select
              value={lineEnding}
              onChange={(e) => setLineEnding(e.target.value)}
              className="bg-[#0b101b] border border-slate-700 rounded px-1.5 py-0.5 text-cyan-400 focus:outline-none focus:border-cyan-500"
            >
              <option value="\r\n">\r\n (CRLF)</option>
              <option value="\n">\n (LF)</option>
              <option value="\r">\r (CR)</option>
              <option value="none">None</option>
            </select>
          </div>
        </div>

        <div className="flex items-center gap-1.5">
          <button
            onClick={() => setAutoScroll(!autoScroll)}
            className={`px-2 py-1 rounded text-[11px] font-mono flex items-center gap-1 border transition-all ${
              autoScroll
                ? 'bg-cyan-950/60 border-cyan-700/60 text-cyan-300'
                : 'bg-slate-800 border-slate-700 text-slate-400'
            }`}
            title="Toggle Auto-Scroll"
          >
            <ArrowDown className="w-3 h-3" />
            <span>Scroll</span>
          </button>
          <button
            onClick={exportLog}
            className="p-1 rounded bg-slate-800 hover:bg-slate-700 border border-slate-700 text-slate-300 transition-colors"
            title="Download Logs"
          >
            <Download className="w-3.5 h-3.5" />
          </button>
          <button
            onClick={clearConsole}
            className="p-1 rounded bg-slate-800 hover:bg-rose-950/60 hover:text-rose-400 border border-slate-700 text-slate-300 transition-colors"
            title="Clear Console"
          >
            <Trash2 className="w-3.5 h-3.5" />
          </button>
        </div>
      </div>

      {/* Terminal Output Area */}
      <div className="flex-1 p-3 overflow-y-auto font-mono text-xs leading-relaxed bg-[#070b12] text-slate-300">
        {messages.map((msg) => {
          if (msg.direction === 'sys') {
            return (
              <div key={msg.id} className="py-0.5 text-slate-500 italic flex items-start gap-2">
                <span className="text-[10px] text-slate-600 select-none">[{msg.timestamp}]</span>
                <span>{msg.text}</span>
              </div>
            );
          }

          if (msg.direction === 'rx') {
            return (
              <div key={msg.id} className="py-0.5 text-amber-300 flex items-start gap-2">
                <span className="text-[10px] text-slate-600 select-none">[{msg.timestamp}]</span>
                <span className="text-amber-500 select-none font-bold">&gt;&gt;</span>
                <span className="font-semibold whitespace-pre-wrap">{msg.text}</span>
              </div>
            );
          }

          // MCU Tx
          return (
            <div key={msg.id} className="py-0.5 text-emerald-400 flex items-start gap-2">
              <span className="text-[10px] text-slate-600 select-none">[{msg.timestamp}]</span>
              <span className="text-cyan-500 select-none font-bold">&lt;&lt;</span>
              <span className="whitespace-pre-wrap">{msg.text}</span>
            </div>
          );
        })}
        <div ref={terminalEndRef} />
      </div>

      {/* Quick Macro Suggestions */}
      <div className="px-3 py-1 bg-[#0b101b] border-t border-slate-800/80 flex items-center gap-1.5 overflow-x-auto text-[10px] font-mono">
        <span className="text-slate-500 select-none">Quick:</span>
        {['help', 'status', 'version', 'reset', 'ping', 'led_test'].map((cmd) => (
          <button
            key={cmd}
            onClick={() => handleSendMessage(cmd)}
            className="px-2 py-0.5 rounded bg-slate-800/70 hover:bg-cyan-950 hover:text-cyan-300 border border-slate-700/60 text-slate-300 transition-colors"
          >
            {cmd}
          </button>
        ))}
      </div>

      {/* Bottom Command Input Bar */}
      <div className="p-2.5 bg-[#0f172a] border-t border-cyan-900/40 flex items-center gap-2">
        <div className="relative flex-1 flex items-center">
          <span className="absolute left-2.5 text-emerald-400 font-mono font-bold select-none">&gt;</span>
          <input
            ref={inputRef}
            type="text"
            value={inputValue}
            onChange={(e) => setInputValue(e.target.value)}
            onKeyDown={handleKeyDown}
            placeholder="Type command or string to send to MCU (press Enter)..."
            className="w-full bg-[#080d18] border border-slate-700/80 focus:border-cyan-500 rounded py-1.5 pl-7 pr-3 text-xs font-mono text-slate-100 placeholder-slate-600 focus:outline-none transition-colors"
          />
        </div>
        <button
          onClick={() => handleSendMessage()}
          className="px-3 py-1.5 rounded bg-cyan-600 hover:bg-cyan-500 text-black font-semibold text-xs flex items-center gap-1.5 shadow-sm transition-all"
        >
          <span>Send</span>
          <CornerDownLeft className="w-3.5 h-3.5" />
        </button>
      </div>
    </div>
  );
};
