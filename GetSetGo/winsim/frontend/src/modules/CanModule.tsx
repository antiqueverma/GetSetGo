import React, { useState, useEffect, useRef } from 'react';
import { CanMessage, CanTxConfig, BasePacket } from '../types/modules';
import { packetConnection } from '../services/packetConnection';
import { Radio, Send, Play, Square, Trash2, Download, Filter, Repeat, Zap, ShieldCheck } from 'lucide-react';

export const CanModule: React.FC = () => {
  const [messages, setMessages] = useState<CanMessage[]>([]);
  const [baudRate, setBaudRate] = useState<string>('500 kbps');
  const [autoScroll, setAutoScroll] = useState<boolean>(true);
  const [filterId, setFilterId] = useState<string>('');

  // Transmitter state
  const [txId, setTxId] = useState<string>('0x123');
  const [txExtended, setTxExtended] = useState<boolean>(false);
  const [txDlc, setTxDlc] = useState<number>(8);
  const [txDataStr, setTxDataStr] = useState<string>('01 02 03 04 AA BB CC DD');
  const [txTrigger, setTxTrigger] = useState<'manual' | 'periodic' | 'rx_trigger'>('manual');
  const [txIntervalMs, setTxIntervalMs] = useState<number>(500);
  const [txRxTriggerId, setTxRxTriggerId] = useState<string>('0x100');
  const [isPeriodicRunning, setIsPeriodicRunning] = useState<boolean>(false);

  const periodicTimerRef = useRef<any>(null);
  const tableEndRef = useRef<HTMLDivElement>(null);

  // Auto-scroll
  useEffect(() => {
    if (autoScroll) {
      tableEndRef.current?.scrollIntoView({ behavior: 'smooth' });
    }
  }, [messages, autoScroll]);

  // Subscribe to incoming CAN packets
  useEffect(() => {
    const unsub = packetConnection.subscribe('can', (packet: BasePacket) => {
      const canId = Number(packet.canId || 0);
      const isExt = Boolean(packet.isExtended || canId > 0x7FF);
      const dlc = Number(packet.dlc || (Array.isArray(packet.data) ? packet.data.length : 8));
      const rawData = Array.isArray(packet.data) ? packet.data : [0, 0, 0, 0, 0, 0, 0, 0];

      const dataHex = rawData.map((b: number) => b.toString(16).padStart(2, '0').toUpperCase()).join(' ');
      const dataAscii = rawData.map((b: number) => (b >= 32 && b <= 126 ? String.fromCharCode(b) : '.')).join('');

      // Simulated 15-bit CRC calculation
      const crcVal = ((canId * 17 + rawData.reduce((a: number, b: number) => a + b, 0) * 31) & 0x7FFF)
        .toString(16)
        .padStart(4, '0')
        .toUpperCase();

      const newMsg: CanMessage = {
        id: Math.random().toString(36).substring(2, 9),
        timestamp: new Date().toLocaleTimeString() + '.' + String(new Date().getMilliseconds()).padStart(3, '0'),
        sof: '0',
        canId,
        canIdHex: '0x' + canId.toString(16).toUpperCase(),
        isExtended: isExt,
        isRtr: Boolean(packet.isRtr),
        dlc,
        data: rawData,
        dataHex,
        dataAscii,
        crc: '0x' + crcVal,
        ack: true,
        eof: '1111111',
        direction: (packet.direction as any) || 'rx',
      };

      setMessages((prev) => [...prev.slice(-300), newMsg]);

      // Check Rx Trigger sequence
      if (txTrigger === 'rx_trigger') {
        const trigIdNum = parseInt(txRxTriggerId, txRxTriggerId.startsWith('0x') ? 16 : 10);
        if (!isNaN(trigIdNum) && canId === trigIdNum) {
          // Trigger automatic reply!
          setTimeout(() => {
            handleSend(true);
          }, 10);
        }
      }
    });

    return () => unsub();
  }, [txTrigger, txRxTriggerId]);

  // Periodic sender handling
  useEffect(() => {
    if (isPeriodicRunning && txTrigger === 'periodic') {
      periodicTimerRef.current = setInterval(() => {
        handleSend(false);
      }, Math.max(20, txIntervalMs));
    } else {
      if (periodicTimerRef.current) {
        clearInterval(periodicTimerRef.current);
        periodicTimerRef.current = null;
      }
    }
    return () => {
      if (periodicTimerRef.current) clearInterval(periodicTimerRef.current);
    };
  }, [isPeriodicRunning, txTrigger, txIntervalMs, txId, txDataStr, txDlc]);

  const handleSend = (fromRxTrigger: boolean = false) => {
    const idVal = parseInt(txId, txId.startsWith('0x') ? 16 : 10) || 0x100;
    const cleanBytes = txDataStr
      .trim()
      .split(/\s+/)
      .map((s) => parseInt(s, 16) || 0)
      .slice(0, txDlc);

    // Send packet to FreeRTOS over TCP
    packetConnection.sendPacket({
      module: 'can',
      action: 'tx_frame',
      canId: idVal,
      isExtended: txExtended,
      dlc: txDlc,
      data: cleanBytes,
      direction: 'tx',
    });

    packetConnection.addHardwareEvent(
      'can',
      'info',
      `CAN Frame Sent: ID 0x${idVal.toString(16).toUpperCase()} [DLC: ${txDlc}]`
    );

    // Record locally in message list
    const dataHex = cleanBytes.map((b) => b.toString(16).padStart(2, '0').toUpperCase()).join(' ');
    const dataAscii = cleanBytes.map((b) => (b >= 32 && b <= 126 ? String.fromCharCode(b) : '.')).join('');

    const newMsg: CanMessage = {
      id: Math.random().toString(36).substring(2, 9),
      timestamp: new Date().toLocaleTimeString() + '.' + String(new Date().getMilliseconds()).padStart(3, '0'),
      sof: '0',
      canId: idVal,
      canIdHex: '0x' + idVal.toString(16).toUpperCase(),
      isExtended: txExtended,
      isRtr: false,
      dlc: txDlc,
      data: cleanBytes,
      dataHex,
      dataAscii,
      crc: '0x' + Math.floor(Math.random() * 0x7fff).toString(16).toUpperCase(),
      ack: true,
      eof: '1111111',
      direction: 'tx',
    };

    setMessages((prev) => [...prev.slice(-300), newMsg]);
  };

  const exportCanLog = () => {
    const header = 'TIMESTAMP,DIR,SOF,ID,EXT,DLC,DATA_HEX,DATA_ASCII,CRC,ACK\n';
    const lines = messages.map(
      (m) => `${m.timestamp},${m.direction},${m.sof},${m.canIdHex},${m.isExtended},${m.dlc},"${m.dataHex}","${m.dataAscii}",${m.crc},${m.ack}`
    );
    const blob = new Blob([header + lines.join('\n')], { type: 'text/csv' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `can-trace-${Date.now()}.csv`;
    a.click();
    URL.revokeObjectURL(url);
  };

  const filteredMessages = messages.filter((m) => {
    if (!filterId.trim()) return true;
    const q = filterId.trim().toLowerCase();
    return m.canIdHex.toLowerCase().includes(q) || m.canId.toString().includes(q);
  });

  return (
    <div className="flex flex-col h-full bg-[#0a0f18] text-slate-200 select-none overflow-hidden text-sm">
      {/* Top Toolbar */}
      <div className="flex flex-wrap items-center justify-between px-3 py-2 bg-[#121927] border-b border-cyan-900/40 text-xs gap-2">
        <div className="flex items-center gap-2">
          <Radio className="w-4 h-4 text-amber-400" />
          <span className="font-bold text-amber-300">CAN 2.0A/B BUS MONITOR &amp; TRANSMITTER</span>
          <span className="text-slate-500">|</span>
          <span className="text-slate-400 font-mono text-[11px]">
            Baud: <strong className="text-cyan-400">{baudRate}</strong>
          </span>
          <span className="text-slate-500">|</span>
          <span className="text-slate-400 font-mono text-[11px]">
            Frames: <strong className="text-emerald-400">{messages.length}</strong>
          </span>
        </div>

        <div className="flex items-center gap-2">
          <div className="flex items-center gap-1 font-mono text-xs">
            <span className="text-slate-400">Filter ID:</span>
            <input
              type="text"
              placeholder="0x..."
              value={filterId}
              onChange={(e) => setFilterId(e.target.value)}
              className="w-20 bg-[#080d16] border border-slate-700 rounded px-1.5 py-0.5 text-xs text-cyan-300 focus:outline-none"
            />
          </div>

          <button
            onClick={() => setAutoScroll(!autoScroll)}
            className={`px-2 py-0.5 rounded border text-xs ${
              autoScroll ? 'bg-cyan-950 border-cyan-700 text-cyan-300' : 'bg-slate-800 border-slate-700 text-slate-400'
            }`}
          >
            Scroll
          </button>
          <button
            onClick={exportCanLog}
            className="p-1 rounded bg-slate-800 hover:bg-slate-700 border border-slate-700 text-slate-300"
            title="Export CSV Trace"
          >
            <Download className="w-3.5 h-3.5" />
          </button>
          <button
            onClick={() => setMessages([])}
            className="p-1 rounded bg-slate-800 hover:bg-rose-950/60 hover:text-rose-400 border border-slate-700 text-slate-300"
            title="Clear Trace Buffer"
          >
            <Trash2 className="w-3.5 h-3.5" />
          </button>
        </div>
      </div>

      {/* Main Container: Top Table, Bottom Transmitter Panel */}
      <div className="flex-1 flex flex-col overflow-hidden">
        {/* Parsed CAN Messages Table */}
        <div className="flex-1 overflow-y-auto font-mono text-xs bg-[#070b13]">
          <table className="w-full text-left border-collapse">
            <thead className="sticky top-0 bg-[#0f1728] border-b border-slate-800 text-[11px] text-slate-400 z-10">
              <tr>
                <th className="py-1.5 px-2">Time</th>
                <th className="py-1.5 px-1.5 text-center">Dir</th>
                <th className="py-1.5 px-1.5 text-center">SOF</th>
                <th className="py-1.5 px-2">ID</th>
                <th className="py-1.5 px-1.5 text-center">Type</th>
                <th className="py-1.5 px-1.5 text-center">DLC</th>
                <th className="py-1.5 px-2">DATA (HEX)</th>
                <th className="py-1.5 px-2">ASCII</th>
                <th className="py-1.5 px-2">CRC</th>
                <th className="py-1.5 px-1.5 text-center">ACK</th>
              </tr>
            </thead>
            <tbody>
              {filteredMessages.length === 0 ? (
                <tr>
                  <td colSpan={10} className="text-center py-16 text-slate-500 italic">
                    Waiting for CAN bus frames...
                  </td>
                </tr>
              ) : (
                filteredMessages.map((msg) => {
                  const isRx = msg.direction === 'rx';

                  return (
                    <tr
                      key={msg.id}
                      className="border-b border-slate-900/80 hover:bg-slate-900/50 transition-colors"
                    >
                      <td className="py-1 px-2 text-slate-500 text-[11px] whitespace-nowrap">{msg.timestamp}</td>
                      <td className="py-1 px-1.5 text-center">
                        <span className={`px-1 py-0.2 rounded text-[10px] font-bold ${
                          isRx ? 'bg-cyan-950 text-cyan-300' : 'bg-amber-950 text-amber-300'
                        }`}>
                          {msg.direction.toUpperCase()}
                        </span>
                      </td>
                      <td className="py-1 px-1.5 text-center text-slate-600 font-bold">{msg.sof}</td>
                      <td className="py-1 px-2 font-bold text-amber-300">
                        {msg.canIdHex}
                        <span className="text-[10px] text-slate-500 ml-1">({msg.canId})</span>
                      </td>
                      <td className="py-1 px-1.5 text-center">
                        <span className={`text-[10px] px-1 rounded ${
                          msg.isExtended ? 'bg-purple-950 text-purple-300' : 'bg-slate-800 text-slate-400'
                        }`}>
                          {msg.isExtended ? 'EXT' : 'STD'}
                        </span>
                      </td>
                      <td className="py-1 px-1.5 text-center font-bold text-cyan-400">{msg.dlc}</td>
                      <td className="py-1 px-2 text-emerald-400 font-semibold tracking-wider whitespace-nowrap">
                        {msg.dataHex}
                      </td>
                      <td className="py-1 px-2 text-slate-400 tracking-wider whitespace-nowrap">
                        {msg.dataAscii}
                      </td>
                      <td className="py-1 px-2 text-slate-500 text-[10px]">{msg.crc}</td>
                      <td className="py-1 px-1.5 text-center text-emerald-400 font-bold">ACK</td>
                    </tr>
                  );
                })
              )}
            </tbody>
          </table>
          <div ref={tableEndRef} />
        </div>

        {/* Bottom Configurable Transmitter Panel */}
        <div className="p-3 bg-[#0e1625] border-t border-cyan-900/40 flex flex-col gap-2.5 font-mono text-xs">
          <div className="flex items-center justify-between text-[11px] font-bold text-slate-300">
            <span className="flex items-center gap-1.5">
              <Send className="w-3.5 h-3.5 text-amber-400" />
              <span>CONFIGURABLE CAN TRANSMITTER &amp; TRIGGER GENERATOR</span>
            </span>
            <div className="flex items-center gap-2">
              <span className="text-slate-400">Trigger Mode:</span>
              <div className="flex bg-[#080d16] p-0.5 rounded border border-slate-700">
                {(['manual', 'periodic', 'rx_trigger'] as const).map((trig) => (
                  <button
                    key={trig}
                    type="button"
                    onClick={() => {
                      setTxTrigger(trig);
                      setIsPeriodicRunning(false);
                    }}
                    className={`px-2 py-0.5 rounded text-[10px] font-bold uppercase transition-all ${
                      txTrigger === trig
                        ? 'bg-amber-500 text-black shadow-sm'
                        : 'text-slate-400 hover:text-slate-200'
                    }`}
                  >
                    {trig === 'rx_trigger' ? 'Rx Trigger' : trig}
                  </button>
                ))}
              </div>
            </div>
          </div>

          {/* Form Controls */}
          <div className="grid grid-cols-1 md:grid-cols-5 gap-2 items-center">
            {/* CAN ID */}
            <div>
              <label className="block text-[10px] text-slate-400 mb-0.5">CAN ID (Hex)</label>
              <input
                type="text"
                value={txId}
                onChange={(e) => setTxId(e.target.value)}
                placeholder="0x123"
                className="w-full bg-[#080d16] border border-slate-700 rounded px-2 py-1 text-xs text-amber-300 font-bold focus:outline-none focus:border-amber-500"
              />
            </div>

            {/* DLC */}
            <div>
              <label className="block text-[10px] text-slate-400 mb-0.5">DLC (0—8)</label>
              <select
                value={txDlc}
                onChange={(e) => setTxDlc(parseInt(e.target.value))}
                className="w-full bg-[#080d16] border border-slate-700 rounded px-2 py-1 text-xs text-cyan-300 focus:outline-none"
              >
                {[0, 1, 2, 3, 4, 5, 6, 7, 8].map((n) => (
                  <option key={n} value={n}>
                    {n} Bytes
                  </option>
                ))}
              </select>
            </div>

            {/* Data Payload Hex */}
            <div className="md:col-span-2">
              <label className="block text-[10px] text-slate-400 mb-0.5">Payload Data (Hex bytes)</label>
              <input
                type="text"
                value={txDataStr}
                onChange={(e) => setTxDataStr(e.target.value)}
                placeholder="01 02 03 04 05 06 07 08"
                className="w-full bg-[#080d16] border border-slate-700 rounded px-2 py-1 text-xs text-emerald-300 font-bold focus:outline-none focus:border-cyan-500"
              />
            </div>

            {/* Trigger Actions */}
            <div className="flex items-end gap-1.5">
              {txTrigger === 'manual' && (
                <button
                  onClick={() => handleSend(false)}
                  className="w-full py-1.5 px-3 rounded bg-amber-500 hover:bg-amber-400 text-black font-bold text-xs flex items-center justify-center gap-1.5 transition-colors shadow-md"
                >
                  <Send className="w-3.5 h-3.5" />
                  <span>Send Frame</span>
                </button>
              )}

              {txTrigger === 'periodic' && (
                <div className="flex items-center gap-1.5 w-full">
                  <input
                    type="number"
                    min={20}
                    step={50}
                    value={txIntervalMs}
                    onChange={(e) => setTxIntervalMs(parseInt(e.target.value) || 100)}
                    className="w-16 bg-[#080d16] border border-slate-700 rounded px-1 py-1 text-xs text-slate-100"
                    placeholder="ms"
                  />
                  <button
                    onClick={() => setIsPeriodicRunning(!isPeriodicRunning)}
                    className={`flex-1 py-1.5 px-2 rounded font-bold text-xs flex items-center justify-center gap-1 transition-colors ${
                      isPeriodicRunning
                        ? 'bg-rose-600 hover:bg-rose-500 text-white'
                        : 'bg-emerald-600 hover:bg-emerald-500 text-white'
                    }`}
                  >
                    {isPeriodicRunning ? <Square className="w-3 h-3" /> : <Play className="w-3 h-3" />}
                    <span>{isPeriodicRunning ? 'Stop' : 'Start'}</span>
                  </button>
                </div>
              )}

              {txTrigger === 'rx_trigger' && (
                <div className="flex items-center gap-1.5 w-full">
                  <input
                    type="text"
                    value={txRxTriggerId}
                    onChange={(e) => setTxRxTriggerId(e.target.value)}
                    placeholder="On Rx ID"
                    className="w-20 bg-[#080d16] border border-slate-700 rounded px-1 py-1 text-xs text-cyan-300 font-bold"
                  />
                  <div className="flex-1 text-[10px] text-cyan-400 font-bold bg-cyan-950/70 px-2 py-1 rounded border border-cyan-800 text-center">
                    Trigger Armed
                  </div>
                </div>
              )}
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};
