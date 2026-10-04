import React, { useState, useEffect, useRef } from 'react';
import { CanopenMessage, CanopenCobType, CanopenNode, BasePacket } from '../types/modules';
import { packetConnection } from '../services/packetConnection';
import { Network, Activity, Heart, ArrowDownRight, ArrowUpRight, Search, Trash2, Download, Send } from 'lucide-react';

export const CanopenModule: React.FC = () => {
  const [messages, setMessages] = useState<CanopenMessage[]>([]);
  const [filterType, setFilterType] = useState<string>('ALL');
  const [filterNode, setFilterNode] = useState<string>('ALL');

  // Monitored Nodes state (Nodes 1 to 8)
  const [nodes, setNodes] = useState<Record<number, CanopenNode>>(() => {
    const init: Record<number, CanopenNode> = {};
    for (let i = 1; i <= 8; i++) {
      init[i] = {
        nodeId: i,
        name: `Node ${i}`,
        state: i === 1 ? 'OPERATIONAL' : 'UNKNOWN',
        lastSeenMs: Date.now(),
        isAlive: i === 1,
      };
    }
    return init;
  });

  // SDO Query Builder State
  const [sdoNodeId, setSdoNodeId] = useState<number>(1);
  const [sdoIndex, setSdoIndex] = useState<string>('0x1000');
  const [sdoSubIndex, setSdoSubIndex] = useState<string>('0x00');
  const [sdoCommand, setSdoCommand] = useState<'read' | 'write'>('read');
  const [sdoData, setSdoData] = useState<string>('00 00 00 00');

  const streamEndRef = useRef<HTMLDivElement>(null);

  // Parse incoming CAN / CANopen packets
  useEffect(() => {
    const unsub = packetConnection.subscribe('all', (packet: BasePacket) => {
      // Process packets originating from 'can' or 'canopen'
      if (packet.module === 'can' || packet.module === 'canopen') {
        const canId = Number(packet.canId || 0);
        const data: number[] = Array.isArray(packet.data) ? packet.data : [];

        // Dissect CANopen COB-ID
        let cobType: CanopenCobType = 'UNKNOWN';
        let nodeId = 0;
        let summary = '';
        let details = '';

        if (canId === 0x000) {
          cobType = 'NMT';
          const cs = data[0] || 0;
          nodeId = data[1] || 0;
          const csMap: Record<number, string> = {
            1: 'Start Remote Node (Operational)',
            2: 'Stop Remote Node',
            128: 'Enter Pre-Operational',
            129: 'Reset Node',
            130: 'Reset Communication',
          };
          summary = `NMT Command: ${csMap[cs] || '0x' + cs.toString(16)} (Target Node: ${nodeId === 0 ? 'ALL' : nodeId})`;
        } else if (canId === 0x080) {
          cobType = 'SYNC';
          summary = 'SYNC Object Broadcast';
        } else if (canId >= 0x081 && canId <= 0x0ff) {
          cobType = 'EMCY';
          nodeId = canId - 0x080;
          const errCode = (data[1] << 8) | data[0];
          const errReg = data[2] || 0;
          summary = `EMCY from Node ${nodeId}: Error Code 0x${errCode.toString(16).padStart(4, '0').toUpperCase()} (Reg 0x${errReg.toString(16)})`;
        } else if (canId >= 0x181 && canId <= 0x1ff) {
          cobType = 'PDO';
          nodeId = canId - 0x180;
          summary = `TPDO1 from Node ${nodeId}`;
          details = data.map((b) => b.toString(16).padStart(2, '0').toUpperCase()).join(' ');
        } else if (canId >= 0x201 && canId <= 0x27f) {
          cobType = 'PDO';
          nodeId = canId - 0x200;
          summary = `RPDO1 to Node ${nodeId}`;
          details = data.map((b) => b.toString(16).padStart(2, '0').toUpperCase()).join(' ');
        } else if (canId >= 0x581 && canId <= 0x5ff) {
          cobType = 'SDO';
          nodeId = canId - 0x580;
          const cs = data[0] || 0;
          const idx = (data[2] << 8) | data[1];
          const subIdx = data[3] || 0;
          const valHex = data.slice(4, 8).map((b) => b.toString(16).padStart(2, '0').toUpperCase()).join(' ');
          summary = `SDO Tx (Response) from Node ${nodeId}: Index 0x${idx.toString(16).padStart(4, '0').toUpperCase()}:${subIdx.toString(16).padStart(2, '0').toUpperCase()}`;
          details = `Data: [${valHex}] CS: 0x${cs.toString(16).toUpperCase()}`;
        } else if (canId >= 0x601 && canId <= 0x67f) {
          cobType = 'SDO';
          nodeId = canId - 0x600;
          const idx = (data[2] << 8) | data[1];
          const subIdx = data[3] || 0;
          summary = `SDO Rx (Request) to Node ${nodeId}: Index 0x${idx.toString(16).padStart(4, '0').toUpperCase()}:${subIdx.toString(16).padStart(2, '0').toUpperCase()}`;
        } else if (canId >= 0x701 && canId <= 0x77f) {
          cobType = 'HBT';
          nodeId = canId - 0x700;
          const st = data[0] || 0;
          const stMap: Record<number, CanopenNode['state']> = {
            0: 'BOOTUP',
            4: 'STOPPED',
            5: 'OPERATIONAL',
            127: 'PRE_OPERATIONAL',
          };
          const resolvedState = stMap[st] || 'UNKNOWN';
          summary = `Heartbeat from Node ${nodeId}: State = ${resolvedState} (0x${st.toString(16).padStart(2, '0').toUpperCase()})`;

          // Update Monitored Node state
          setNodes((prev) => ({
            ...prev,
            [nodeId]: {
              nodeId,
              name: `Node ${nodeId}`,
              state: resolvedState,
              lastSeenMs: Date.now(),
              isAlive: true,
            },
          }));
        } else {
          return; // Ignore non-CANopen standard CAN messages
        }

        const msg: CanopenMessage = {
          id: Math.random().toString(36).substring(2, 9),
          timestamp: new Date().toLocaleTimeString() + '.' + String(new Date().getMilliseconds()).padStart(3, '0'),
          cobType,
          cobId: canId,
          cobIdHex: '0x' + canId.toString(16).toUpperCase(),
          nodeId,
          summary,
          details,
          rawCan: {
            id: Math.random().toString(36).substring(2, 9),
            timestamp: new Date().toLocaleTimeString(),
            sof: '0',
            canId,
            canIdHex: '0x' + canId.toString(16).toUpperCase(),
            isExtended: false,
            isRtr: false,
            dlc: data.length,
            data,
            dataHex: data.map((b) => b.toString(16).padStart(2, '0').toUpperCase()).join(' '),
            dataAscii: '',
            crc: '0x0000',
            ack: true,
            eof: '1111111',
            direction: 'rx',
          },
        };

        setMessages((prev) => [...prev.slice(-300), msg]);
      }
    });

    return () => unsub();
  }, []);

  const handleSendSdo = () => {
    // SDO Client Upload (Read): CS = 0x40
    // SDO Client Download (Write): CS = 0x23 (4 bytes)
    const idxNum = parseInt(sdoIndex, sdoIndex.startsWith('0x') ? 16 : 10) || 0x1000;
    const subIdxNum = parseInt(sdoSubIndex, sdoSubIndex.startsWith('0x') ? 16 : 10) || 0x00;
    const cobId = 0x600 + sdoNodeId;

    const payload = [
      sdoCommand === 'read' ? 0x40 : 0x23,
      idxNum & 0xff,
      (idxNum >> 8) & 0xff,
      subIdxNum & 0xff,
      0, 0, 0, 0,
    ];

    // Transmit over underlying CAN layer
    packetConnection.sendPacket({
      module: 'can',
      action: 'tx_frame',
      canId: cobId,
      isExtended: false,
      dlc: 8,
      data: payload,
    });

    packetConnection.addHardwareEvent(
      'canopen',
      'info',
      `Sent SDO ${sdoCommand.toUpperCase()} to Node ${sdoNodeId} (Idx: 0x${idxNum.toString(16).toUpperCase()}:${subIdxNum})`
    );
  };

  const filteredMessages = messages.filter((m) => {
    if (filterType !== 'ALL' && m.cobType !== filterType) return false;
    if (filterNode !== 'ALL' && m.nodeId.toString() !== filterNode) return false;
    return true;
  });

  const getCobBadgeClass = (type: CanopenCobType) => {
    switch (type) {
      case 'HBT':
        return 'bg-emerald-950 text-emerald-300 border-emerald-800';
      case 'SDO':
        return 'bg-purple-950 text-purple-300 border-purple-800';
      case 'PDO':
        return 'bg-cyan-950 text-cyan-300 border-cyan-800';
      case 'NMT':
        return 'bg-amber-950 text-amber-300 border-amber-800';
      case 'EMCY':
        return 'bg-rose-950 text-rose-300 border-rose-800';
      default:
        return 'bg-slate-800 text-slate-300 border-slate-700';
    }
  };

  return (
    <div className="flex flex-col h-full bg-[#0a0f18] text-slate-200 select-none overflow-hidden text-sm">
      {/* Top Toolbar */}
      <div className="flex flex-wrap items-center justify-between px-3 py-2 bg-[#121927] border-b border-cyan-900/40 text-xs gap-2">
        <div className="flex items-center gap-2">
          <Network className="w-4 h-4 text-purple-400" />
          <span className="font-bold text-purple-300">CANOPEN PROTOCOL DISSECTOR (CiA 301)</span>
          <span className="text-slate-500">|</span>
          <span className="text-slate-400 font-mono text-[11px]">
            Packets: <strong className="text-cyan-400">{messages.length}</strong>
          </span>
        </div>

        {/* Filter controls */}
        <div className="flex items-center gap-2">
          <div className="flex items-center gap-1 font-mono text-xs">
            <span className="text-slate-400">Type:</span>
            <select
              value={filterType}
              onChange={(e) => setFilterType(e.target.value)}
              className="bg-[#090e18] border border-slate-700 rounded px-1.5 py-0.5 text-xs text-purple-300 focus:outline-none"
            >
              <option value="ALL">All Types</option>
              <option value="HBT">Heartbeat (HBT)</option>
              <option value="SDO">SDO Service</option>
              <option value="PDO">PDO Process</option>
              <option value="NMT">NMT Management</option>
              <option value="EMCY">Emergency (EMCY)</option>
              <option value="SYNC">SYNC</option>
            </select>
          </div>

          <div className="flex items-center gap-1 font-mono text-xs">
            <span className="text-slate-400">Node:</span>
            <select
              value={filterNode}
              onChange={(e) => setFilterNode(e.target.value)}
              className="bg-[#090e18] border border-slate-700 rounded px-1.5 py-0.5 text-xs text-cyan-300 focus:outline-none"
            >
              <option value="ALL">All Nodes</option>
              {[1, 2, 3, 4, 5, 6, 7, 8].map((n) => (
                <option key={n} value={n.toString()}>
                  Node {n}
                </option>
              ))}
            </select>
          </div>

          <button
            onClick={() => setMessages([])}
            className="p-1 rounded bg-slate-800 hover:bg-rose-950/60 hover:text-rose-400 border border-slate-700 text-slate-300"
            title="Clear Feed"
          >
            <Trash2 className="w-3.5 h-3.5" />
          </button>
        </div>
      </div>

      {/* Node Health Grid Matrix (Nodes 1 to 8) */}
      <div className="px-3 py-2 bg-[#0c1322] border-b border-slate-800 overflow-x-auto flex items-center gap-2">
        <span className="text-[11px] font-mono text-slate-400 font-bold uppercase shrink-0 flex items-center gap-1">
          <Heart className="w-3.5 h-3.5 text-rose-400" />
          <span>Nodes:</span>
        </span>
        <div className="flex items-center gap-2">
          {Object.values(nodes).map((n) => {
            const isOp = n.state === 'OPERATIONAL';
            const isPreOp = n.state === 'PRE_OPERATIONAL';
            const isStopped = n.state === 'STOPPED';

            return (
              <div
                key={n.nodeId}
                className="px-2 py-1 rounded border border-slate-800 bg-[#070c16] flex items-center gap-1.5 font-mono text-xs shrink-0"
              >
                <div
                  className={`w-2 h-2 rounded-full ${
                    isOp
                      ? 'bg-emerald-400 shadow-[0_0_6px_#10b981]'
                      : isPreOp
                      ? 'bg-amber-400'
                      : isStopped
                      ? 'bg-rose-400'
                      : 'bg-slate-600'
                  }`}
                />
                <span className="font-bold text-slate-200">N{n.nodeId}</span>
                <span className={`text-[10px] font-semibold ${
                  isOp ? 'text-emerald-400' : isPreOp ? 'text-amber-400' : 'text-slate-500'
                }`}>
                  {n.state}
                </span>
              </div>
            );
          })}
        </div>
      </div>

      {/* Main Container: Top Dissected Message Feed, Bottom SDO Query Box */}
      <div className="flex-1 flex flex-col overflow-hidden">
        {/* Messages Feed */}
        <div className="flex-1 p-2.5 overflow-y-auto font-mono text-xs flex flex-col gap-1.5 bg-[#070b13]">
          {filteredMessages.length === 0 ? (
            <div className="text-center text-slate-500 py-16 italic">
              No CANopen traffic detected on underlying CAN channel.
            </div>
          ) : (
            filteredMessages.map((msg) => (
              <div
                key={msg.id}
                className="flex items-center gap-2.5 px-3 py-1.5 rounded-lg border border-slate-800/80 bg-[#0e1625] hover:border-slate-700 transition-colors"
              >
                <span className="text-[10px] text-slate-500 shrink-0">{msg.timestamp}</span>
                <span className={`px-1.5 py-0.5 rounded text-[10px] font-bold border ${getCobBadgeClass(msg.cobType)}`}>
                  {msg.cobType}
                </span>
                <span className="text-amber-400 font-bold shrink-0">{msg.cobIdHex}</span>
                {msg.nodeId > 0 && (
                  <span className="text-[10px] text-cyan-400 font-bold bg-cyan-950/70 px-1.5 py-0.5 rounded border border-cyan-800">
                    Node {msg.nodeId}
                  </span>
                )}
                <span className="text-slate-200 font-medium flex-1 truncate">{msg.summary}</span>
                {msg.details && (
                  <span className="text-[11px] text-emerald-400 bg-black/40 px-2 py-0.5 rounded border border-slate-800">
                    {msg.details}
                  </span>
                )}
              </div>
            ))
          )}
          <div ref={streamEndRef} />
        </div>

        {/* Bottom SDO Query Dispatcher */}
        <div className="p-3 bg-[#0e1625] border-t border-purple-900/40 flex flex-wrap items-center justify-between gap-2 font-mono text-xs">
          <div className="flex items-center gap-2">
            <span className="font-bold text-purple-300 flex items-center gap-1">
              <Send className="w-3.5 h-3.5" />
              <span>SDO QUERY:</span>
            </span>

            <div className="flex items-center gap-1">
              <span className="text-slate-400">Node:</span>
              <select
                value={sdoNodeId}
                onChange={(e) => setSdoNodeId(parseInt(e.target.value) || 1)}
                className="bg-[#080d16] border border-slate-700 rounded px-1.5 py-1 text-cyan-300 text-xs focus:outline-none"
              >
                {[1, 2, 3, 4, 5, 6, 7, 8].map((n) => (
                  <option key={n} value={n}>
                    Node {n}
                  </option>
                ))}
              </select>
            </div>

            <div className="flex items-center gap-1">
              <span className="text-slate-400">Index:</span>
              <input
                type="text"
                value={sdoIndex}
                onChange={(e) => setSdoIndex(e.target.value)}
                placeholder="0x1000"
                className="w-20 bg-[#080d16] border border-slate-700 rounded px-1.5 py-1 text-amber-300 text-xs font-bold focus:outline-none"
              />
            </div>

            <div className="flex items-center gap-1">
              <span className="text-slate-400">Sub:</span>
              <input
                type="text"
                value={sdoSubIndex}
                onChange={(e) => setSdoSubIndex(e.target.value)}
                placeholder="0x00"
                className="w-14 bg-[#080d16] border border-slate-700 rounded px-1.5 py-1 text-slate-200 text-xs focus:outline-none"
              />
            </div>

            <div className="flex items-center gap-1">
              <button
                type="button"
                onClick={() => setSdoCommand(sdoCommand === 'read' ? 'write' : 'read')}
                className="px-2 py-1 rounded bg-[#080d16] border border-slate-700 font-bold uppercase text-[10px] text-cyan-400"
              >
                {sdoCommand.toUpperCase()}
              </button>
            </div>
          </div>

          <button
            onClick={handleSendSdo}
            className="px-4 py-1.5 rounded bg-purple-600 hover:bg-purple-500 text-white font-bold text-xs flex items-center gap-1.5 shadow-md transition-colors"
          >
            <Send className="w-3.5 h-3.5" />
            <span>Send SDO Request</span>
          </button>
        </div>
      </div>
    </div>
  );
};
