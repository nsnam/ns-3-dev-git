// Copyright (c) 2026 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
//
// SPDX-License-Identifier: GPL-2.0-only
//
// Author: Gabriel Ferreira <gabrielcarvfer@gmail.com>

// Presentation only: everything shown here comes from the Python side, and
// every user action is posted back to it.
const $ = (selector) => document.querySelector(selector);
const scene = $('#scene'), viewport = $('#viewport'), clock = $('#clock');
// The server hands out its token only in the fragment of the URL it opens,
// which is kept for this tab (reloads) and removed from the address bar.
const token = (() => {
  const fromUrl = new URLSearchParams(location.hash.slice(1)).get('token');
  if (fromUrl) {
    try { sessionStorage.setItem('pyviz-token', fromUrl); } catch (_) {}
    history.replaceState(null, '', location.pathname + location.search);
    return fromUrl;
  }
  try { return sessionStorage.getItem('pyviz-token') || ''; } catch (_) { return ''; }
})();
const tokenHeader = {'X-PyViz-Token': token};
const svgNs = 'http://www.w3.org/2000/svg';
const view = {scale: 1, ox: 0, oy: 0, fitted: false, follow: null, nodeSize: 1};
const activeFlows = [];
const openPanels = new Map();
let state = null, catalogue = [], drag = null, lastPositions = new Map(), speedSynced = false;
// Packet hops are far too short to see at real speed (a few ms), so each one
// is shown for at least MIN_HOP_MS, starting at its real offset within the
// step, with a trail from the sender so its direction is clear.
const MIN_HOP_MS = 700, MAX_PACKETS = 80;
// Python paces the simulation by wall-clock time, so polling faster only
// makes motion smoother; it does not change the simulation speed.
const POLL_GAP_MS = 30;
let lastPollAt = null;
// Set once Exit returned control to the scenario; the server is gone then.
let exited = false;

function svg(name, attrs, text) {
  const e = document.createElementNS(svgNs, name);
  for (const [k, v] of Object.entries(attrs)) e.setAttribute(k, v);
  if (text !== undefined) e.textContent = text;
  return e;
}
function html(name, props = {}, children = []) {
  const e = Object.assign(document.createElement(name), props);
  e.append(...children);
  return e;
}
async function post(url, body) {
  const response = await fetch(url, {method: 'POST', headers: {'Content-Type': 'application/json', ...tokenHeader}, body: JSON.stringify(body)});
  return response.json();
}
const getJson = async (url) => (await fetch(url, {cache: 'no-store', headers: tokenHeader})).json();
// Icons need the token too, which an <image href> cannot send, so each one
// is fetched once and shown from a blob URL.
const iconUrls = new Map();
function iconUrl(node) {
  if (!iconUrls.has(node)) {
    iconUrls.set(node, null);
    fetch(`/api/icon?node=${node}`, {headers: tokenHeader}).then(r => r.ok ? r.blob() : null).then(blob => {
      if (blob) { iconUrls.set(node, URL.createObjectURL(blob)); render(); }
    }).catch(() => {});
  }
  return iconUrls.get(node);
}
const control = (body) => post('/api/control', body).then(r => { if (!r.ok) clock.textContent = r.error || 'Action failed'; return r; });

// View transform

const toScreen = (x, y) => [x * view.scale + view.ox, y * view.scale + view.oy];
const toWorld = (sx, sy) => [(sx - view.ox) / view.scale, (sy - view.oy) / view.scale];
function fitView(points) {
  if (!points.length) return;
  const w = scene.clientWidth, h = scene.clientHeight, pad = 60;
  let [x1, y1, x2, y2] = state.bounds || [Math.min(...points.map(p => p.x)), Math.min(...points.map(p => p.y)), Math.max(...points.map(p => p.x)), Math.max(...points.map(p => p.y))];
  const dx = Math.max(x2 - x1, 1), dy = Math.max(y2 - y1, 1);
  view.scale = Math.min((w - 2 * pad) / dx, (h - 2 * pad) / dy);
  view.ox = (w - dx * view.scale) / 2 - x1 * view.scale;
  view.oy = (h - dy * view.scale) / 2 - y1 * view.scale;
  view.fitted = true;
}

// Rendering

function render() {
  if (!state) return;
  const w = scene.clientWidth, h = scene.clientHeight;
  scene.setAttribute('viewBox', `0 0 ${w} ${h}`);
  const points = [...state.nodes, ...state.hubs];
  if (!view.fitted) fitView(points);
  if (view.follow !== null) {
    const node = state.nodes.find(n => n.id === view.follow);
    if (node) { view.ox = w / 2 - node.x * view.scale; view.oy = h / 2 - node.y * view.scale; }
  }
  const pos = new Map(points.map(p => [p.id, toScreen(p.x, p.y)]));
  const items = [];
  for (const link of state.links) {
    const a = pos.get(link.source), b = pos.get(link.target);
    if (a && b) items.push(svg('line', {x1: a[0], y1: a[1], x2: b[0], y2: b[1], class: 'link'}));
  }
  for (const overlay of state.overlays) {
    const a = pos.get(overlay.source), b = pos.get(overlay.target);
    if (!a || !b) continue;
    const line = svg('line', {x1: a[0], y1: a[1], x2: b[0], y2: b[1], class: `overlay ${overlay.class || ''}`});
    if (overlay.title) line.append(svg('title', {}, overlay.title));
    items.push(line);
  }
  for (const tx of state.transmissions) {
    const a = pos.get(tx.source), b = pos.get(tx.target);
    if (!a || !b) continue;
    const dx = b[0] - a[0], dy = b[1] - a[1], length = Math.hypot(dx, dy);
    if (length < 1) continue;
    // Each direction is drawn on its own side of the link (to the right of
    // travel), so A->B and B->A arrows and labels do not overlap.
    const ux = dx / length, uy = dy / length, nx = -uy, ny = ux;
    const width = tx.width * view.nodeSize, side = width / 2 + 3, inset = 14 * view.nodeSize;
    const x1 = a[0] + ux * inset + nx * side, y1 = a[1] + uy * inset + ny * side;
    const x2 = b[0] - ux * inset + nx * side, y2 = b[1] - uy * inset + ny * side;
    const arrow = svg('line', {x1, y1, x2, y2, class: 'transmission', 'stroke-width': width, 'marker-end': 'url(#arrow-tx)'});
    arrow.append(svg('title', {}, `Node ${tx.source} -> Node ${tx.target}: ${formatRate(tx.kbps * 1e3)}`));
    items.push(arrow);
    // Skip the label when the arrow is too short to hold it; the title
    // still shows it.
    if (length - 2 * inset < 110) continue;
    let angle = Math.atan2(dy, dx) * 180 / Math.PI;
    const flipped = angle > 90 || angle < -90;
    if (flipped) angle += 180;
    const labelOffset = side + width / 2 + 8;
    const mx = (a[0] + b[0]) / 2 + nx * labelOffset, my = (a[1] + b[1]) / 2 + ny * labelOffset;
    const text = flipped ? `\u2190 ${formatRate(tx.kbps * 1e3)}` : `${formatRate(tx.kbps * 1e3)} \u2192`;
    items.push(svg('text', {x: mx, y: my, class: 'rate', 'dominant-baseline': 'middle', transform: `rotate(${angle} ${mx} ${my})`}, text));
  }
  // Nothing received these transmissions: a stub towards the peer ends in a cross.
  for (const tx of state.unreceived || []) {
    const stub = unreceivedStub(tx.node, pos);
    if (!stub) continue;
    const [a, b] = stub, [ex, ey] = [5, 5];
    const group = svg('g', {class: 'unreceived'});
    group.append(svg('line', {x1: a[0], y1: a[1], x2: b[0], y2: b[1]}),
                 svg('path', {d: `M${b[0] - ex},${b[1] - ey} L${b[0] + ex},${b[1] + ey} M${b[0] - ex},${b[1] + ey} L${b[0] + ex},${b[1] - ey}`, class: 'unreceived-cross'}),
                 svg('text', {x: b[0], y: b[1] - 10, class: 'rate unreceived-rate'}, `not received ${formatRate(tx.kbps * 1e3)}`),
                 svg('title', {}, `Node ${tx.node} transmits ${formatRate(tx.kbps * 1e3)}, but no device received it`));
    items.push(group);
  }
  // A node dropping too much to read each packet's size shows the smoothed rate.
  const dropLabels = new Map();
  for (const drop of state.drops) {
    const a = pos.get(drop.node);
    if (!a || !dropsAreBusy(drop.node)) continue;
    const row = dropLabels.get(drop.node) || 0;
    dropLabels.set(drop.node, row + 1);
    items.push(svg('text', {x: a[0] + 16, y: a[1] - 46 - row * 14, class: 'rate drop-rate'}, `${DROP_LABELS[drop.kind] || DROP_LABELS.other} ${formatRate(drop.kbps * 1e3)}`));
  }
  // Trails of moving nodes (recorded by Python while enabled), oldest end faded.
  for (const [id, points] of Object.entries(state.trails || {})) {
    const screen = points.map(([x, y]) => toScreen(x, y));
    const head = pos.get(Number(id));
    if (head) screen.push(head);
    items.push(svg('polyline', {points: screen.map(p => p.join(',')).join(' '), class: 'trail', 'data-node': id}));
    const recent = screen.slice(-Math.max(2, Math.floor(screen.length / 3)));
    items.push(svg('polyline', {points: recent.map(p => p.join(',')).join(' '), class: 'trail recent'}));
  }
  // Mobility area of each selected node whose model has bounds.
  for (const [id, [x1, y1, x2, y2]] of Object.entries(state.selectedBounds || {})) {
    const [ax, ay] = toScreen(x1, y1), [bx, by] = toScreen(x2, y2);
    const area = svg('rect', {x: Math.min(ax, bx), y: Math.min(ay, by), width: Math.abs(bx - ax), height: Math.abs(by - ay), class: 'mobility-bounds', 'data-node': id});
    area.append(svg('title', {}, `Mobility bounds of node ${id}: x ${x1} to ${x2}, y ${y1} to ${y2}`));
    items.push(area);
  }
  for (const hub of state.hubs) {
    const [x, y] = pos.get(hub.id);
    items.push(svg('circle', {cx: x, cy: y, r: 7 * view.nodeSize, class: 'hub'}));
  }
  for (const node of state.nodes) {
    const [x, y] = pos.get(node.id);
    const r = 12 * node.size * view.nodeSize;
    const group = svg('g', {class: 'node-group', 'data-node': node.id});
    const icon = node.icon ? iconUrl(node.id) : null;
    if (icon) group.append(svg('image', {href: icon, x: x - r, y: y - r, width: 2 * r, height: 2 * r}));
    else {
      const circle = svg('circle', {cx: x, cy: y, r, class: `node${node.mobility ? ' mobile' : ''}${state.selectedNodes.includes(node.id) ? ' selected' : ''}${node.id === state.selected ? ' primary' : ''}`});
      if (node.color) circle.style.fill = node.color;
      group.append(circle);
    }
    group.append(svg('text', {x, y: y + r + 14, class: 'label'}, node.label || (node.name ? `${node.name} (${node.id})` : `Node ${node.id}`)));
    if (node.battery) group.append(batteryIcon(node.battery, x + r + 4, y - r));
    items.push(group);
  }
  viewport.replaceChildren(...items);
  lastPositions = pos;
  const playing = state.running && !state.finished;
  $('#play-pause').textContent = playing ? 'Pause' : 'Play';
  $('#play-pause').classList.toggle('selected', !playing);
  const status = state.finished ? 'Finished' : state.stopped ? 'Stopped' : state.held ? 'Held' : state.pauseMessages.length ? `Paused: ${state.pauseMessages.join('; ')}` : state.running ? 'Running' : 'Paused';
  // Python shortens steps when the simulation cannot keep up with the
  // requested speed; say so instead of silently running slower.
  const lagging = state.running && !state.finished && state.effectiveSpeed !== null && state.effectiveSpeed < 0.8 * state.speed;
  // Separate parts so narrow toolbars can drop the least useful ones (CSS);
  // the tooltip always has the whole line.
  const parts = [[status, 'clock-status'], [`${state.time.toFixed(3)} s`, 'clock-time'], [`${state.nodes.length} nodes`, 'clock-nodes']];
  if (lagging) parts.push([`running at ${state.effectiveSpeed.toFixed(2)}x`, 'clock-lag']);
  clock.replaceChildren(...parts.map(([text, cls], index) => html('span', {className: cls, textContent: (index ? ' \u00b7 ' : '') + text})));
  if (!speedSynced) { speedSynced = true; $('#speed').value = Math.log10(state.speed); $('#speed-value').textContent = `${state.speed.toFixed(2)}x`; }
  clock.title = clock.textContent;
}
const packetLayer = svg('g', {id: 'packets'});
scene.append(packetLayer);
// Where a transmission nobody received was heading: the node's wired peer, or
// else its nearest neighbour. Screen positions in, [from, to] out.
const UNRECEIVED_STUB = 80;
function unreceivedStub(node, pos) {
  const a = pos.get(node);
  if (!a) return null;
  const link = state.links.find(l => l.source === node || l.target === node);
  let peer = link ? pos.get(link.source === node ? link.target : link.source) : null;
  if (!peer) {
    let best = Infinity;
    for (const [id, p] of pos) {
      const distance = Math.hypot(p[0] - a[0], p[1] - a[1]);
      if (id !== node && distance < best) { best = distance; peer = p; }
    }
  }
  const dx = peer ? peer[0] - a[0] : 1, dy = peer ? peer[1] - a[1] : 0, length = Math.hypot(dx, dy) || 1;
  const reach = peer ? Math.min(UNRECEIVED_STUB, length * 0.6) : UNRECEIVED_STUB;
  return [a, [a[0] + dx / length * reach, a[1] + dy / length * reach]];
}
function unreceivedItems(now) {
  const progress = (now % 1000) / 1000;
  return (state ? state.unreceived || [] : []).flatMap(tx => {
    const stub = unreceivedStub(tx.node, lastPositions);
    if (!stub) return [];
    const [a, b] = stub, angle = Math.atan2(b[1] - a[1], b[0] - a[0]) * 180 / Math.PI;
    const x = a[0] + (b[0] - a[0]) * progress, y = a[1] + (b[1] - a[1]) * progress;
    return [svg('path', {d: 'M7,0 L-5,-5 L-2,0 L-5,5 Z', class: 'packet unreceived-packet', opacity: (1 - progress * progress).toFixed(2), transform: `translate(${x} ${y}) rotate(${angle})`})];
  });
}
// Dropped packets fly off their node in an arc, one side then the other.
const DROP_LABELS = {queue: 'queue full', channel: 'channel loss', ttl: 'TTL expired', other: 'discarded'};
const DROP_FLIGHT_MS = 1400, MAX_DROPS_PER_POLL = 2, BUSY_DROPS = 3, MAX_DROPS_PER_NODE = 5;
const activeDrops = [];
let dropSide = 1;
const dropsAreBusy = (node) => activeDrops.filter(d => d.node === node).length >= BUSY_DROPS;
function launchDrops(events, now) {
  for (const event of events) {
    const size = event.bytes / event.packets;
    const room = MAX_DROPS_PER_NODE - activeDrops.filter(d => d.node === event.node).length;
    for (let i = 0; i < Math.min(event.packets, MAX_DROPS_PER_POLL, room); ++i) {
      dropSide = -dropSide;
      activeDrops.push({node: event.node, kind: event.kind, size, side: dropSide, start: now + i * 120});
    }
  }
}
function dropItems(now) {
  const items = [];
  for (let i = activeDrops.length - 1; i >= 0; --i) {
    const drop = activeDrops[i], progress = (now - drop.start) / DROP_FLIGHT_MS;
    if (progress >= 1) { activeDrops.splice(i, 1); continue; }
    const a = lastPositions.get(drop.node);
    if (progress < 0 || !a) continue;
    // Starts atop the node that discarded the packet.
    const x = a[0] + drop.side * 70 * progress, y = a[1] - 16 - 90 * progress + 130 * progress * progress;
    const group = svg('g', {class: `drop-packet drop-${drop.kind}`, opacity: (1 - progress * progress).toFixed(2), transform: `translate(${x} ${y})`});
    group.dataset.node = drop.node;
    const envelope = svg('g', {transform: `rotate(${drop.side * 200 * progress})`});
    envelope.append(svg('rect', {x: -8, y: -5.5, width: 16, height: 11, rx: 1.5}), svg('path', {d: 'M-8,-5 L0,1 L8,-5'}));
    group.append(envelope);
    if (!dropsAreBusy(drop.node)) group.append(svg('text', {y: -10, class: 'rate drop-rate'}, formatBytes(drop.size)));
    items.push(group);
  }
  return items;
}
function formatBytes(bytes) { return bytes < 1000 ? `${Math.round(bytes)} B` : `${(bytes / 1000).toFixed(1)} kB`; }
function animatePackets() {
  const now = performance.now(), items = [];
  for (let i = activeFlows.length - 1; i >= 0; --i) {
    const flight = activeFlows[i], progress = (now - flight.start) / flight.duration;
    if (progress >= 1) { activeFlows.splice(i, 1); continue; }
    if (progress < 0) continue;
    const a = lastPositions.get(flight.flow.source), b = lastPositions.get(flight.flow.target);
    if (!a || !b) continue;
    // Travel on the right-hand side of the link (as the rate arrows do), so
    // the two directions of a link stay apart, with a short tail behind.
    const dx = b[0] - a[0], dy = b[1] - a[1], length = Math.hypot(dx, dy) || 1;
    const ox = -dy / length * 5, oy = dx / length * 5, tail = Math.max(0, progress - 24 / length);
    const x = a[0] + dx * progress + ox, y = a[1] + dy * progress + oy;
    const angle = Math.atan2(dy, dx) * 180 / Math.PI;
    const group = svg('g', {class: 'packet-hop', opacity: (1 - progress * 0.5).toFixed(2)});
    group.dataset.packet = flight.flow.packet;
    group.dataset.hop = `${flight.flow.source}->${flight.flow.target}`;
    group.append(svg('line', {x1: a[0] + dx * tail + ox, y1: a[1] + dy * tail + oy, x2: x, y2: y, class: 'packet-trail'}),
                 svg('path', {d: 'M7,0 L-5,-5 L-2,0 L-5,5 Z', class: 'packet', transform: `translate(${x} ${y}) rotate(${angle})`}));
    items.push(group);
  }
  packetLayer.replaceChildren(...items, ...unreceivedItems(now), ...dropItems(now));
  requestAnimationFrame(animatePackets);
}
requestAnimationFrame(animatePackets);
scene.prepend(svg('defs', {}));
scene.firstChild.innerHTML = '<marker id="arrow-tx" viewBox="0 0 10 10" refX="10" refY="5" markerWidth="12" markerHeight="12" markerUnits="userSpaceOnUse" orient="auto"><path d="M0,0L10,5L0,10z" class="arrow-tx"/></marker>';

let refreshing = false;
// Battery next to a node: outline, fill proportional to the charge (green,
// yellow below 50%, red below 20%), and the percentage. The tooltip has mAh.
function batteryIcon(battery, x, y) {
  const level = battery.fraction, width = 18, height = 9;
  const group = svg('g', {class: 'battery', 'data-fraction': level.toFixed(3)});
  const tone = level < 0.2 ? 'low' : level < 0.5 ? 'mid' : 'high';
  group.append(svg('rect', {x, y, width, height, rx: 2, class: 'battery-body'}),
               svg('rect', {x: x + width, y: y + 2.5, width: 2, height: 4, class: 'battery-tip'}),
               svg('rect', {x: x + 1.5, y: y + 1.5, width: Math.max(0, (width - 3) * level), height: height - 3, class: `battery-level ${tone}`}),
               svg('text', {x: x + width + 5, y: y + height - 1, class: 'battery-text'}, `${Math.round(level * 100)}%`),
               svg('title', {}, `Battery ${(level * 100).toFixed(1)}%: ${battery.remainingmAh.toFixed(2)} of ${battery.capacitymAh.toFixed(2)} mAh (${battery.remainingJ.toFixed(2)} J)`));
  return group;
}

async function refresh() {
  if (refreshing || exited) return;
  refreshing = true;
  try {
    const previous = state;
    state = await getJson('/api/state');
    if (drag && drag.node !== null && drag.moved && previous) {
      const held = previous.nodes.find(n => n.id === drag.node), node = state.nodes.find(n => n.id === drag.node);
      if (held && node) { node.x = held.x; node.y = held.y; }
    }
    const receivedAt = performance.now(), pollGap = lastPollAt === null ? 0 : receivedAt - lastPollAt;
    lastPollAt = receivedAt;
    const stepStart = previous ? previous.time : state.time, stepLength = Math.max(state.time - stepStart, 1e-9);
    for (const flow of state.flows) {
      const offset = Math.min(Math.max((flow.startTx - stepStart) / stepLength, 0), 1) * pollGap;
      // When the screen is full, skip new packets rather than removing ones
      // already in flight, so every arrow shown reaches its destination.
      if (activeFlows.length >= MAX_PACKETS) break;
      activeFlows.push({flow, start: receivedAt + offset, duration: Math.max(MIN_HOP_MS, flow.delay * 1000 / Math.max(state.speed, 0.01))});
    }
    launchDrops(state.dropEvents || [], receivedAt);
    $('#transmissions').value = state.showTransmissions;
    $('#toggle-trails').setAttribute('aria-pressed', String(state.showTrails));
    $('#toggle-trails').classList.toggle('selected', state.showTrails);
    $('#toggle-trails').disabled = !state.canMove;
    $('#toggle-trails').title = state.canMove ? 'Draw the recent path of moving nodes' : 'No node in this scenario has a mobility model that moves it';
    render();
  } catch (_) { if (!exited) clock.textContent = 'Waiting for simulation state'; }
  finally { refreshing = false; }
}

// Interaction on the topology

function nodeAt(target) { const g = target.closest && target.closest('.node-group'); return g ? Number(g.dataset.node) : null; }
scene.addEventListener('wheel', (event) => {
  event.preventDefault();
  const factor = Math.exp(-event.deltaY * 0.001), rect = scene.getBoundingClientRect();
  const sx = event.clientX - rect.left, sy = event.clientY - rect.top;
  view.ox = sx - (sx - view.ox) * factor; view.oy = sy - (sy - view.oy) * factor; view.scale *= factor;
  render();
}, {passive: false});
scene.addEventListener('pointerdown', (event) => {
  if (event.button !== 0) return;
  const node = nodeAt(event.target), rect = scene.getBoundingClientRect();
  // Remember whether the press started on a battery: with pointer capture,
  // the release event's target is the scene itself.
  const battery = !!(event.target.closest && event.target.closest('.battery'));
  drag = {node, battery, x: event.clientX, y: event.clientY, ox: view.ox, oy: view.oy, rect, moved: false};
  scene.setPointerCapture(event.pointerId);
});
scene.addEventListener('pointermove', (event) => {
  if (!drag) { showTooltip(event); return; }
  const dx = event.clientX - drag.x, dy = event.clientY - drag.y;
  if (Math.abs(dx) + Math.abs(dy) > 3) drag.moved = true;
  if (!drag.moved) return;
  if (drag.node === null) { view.ox = drag.ox + dx; view.oy = drag.oy + dy; render(); }
  else {
    const [x, y] = toWorld(event.clientX - drag.rect.left, event.clientY - drag.rect.top);
    const node = state.nodes.find(n => n.id === drag.node); if (node) { node.x = x; node.y = y; render(); }
    // Cartesian position under the pointer while dragging; z is kept.
    if (node) {
      const label = $('#drag-coords'), area = scene.parentElement.getBoundingClientRect();
      label.textContent = `(${x.toFixed(2)}, ${y.toFixed(2)}, ${(node.z || 0).toFixed(2)}) m`;
      label.style.left = `${event.clientX - area.left + 14}px`; label.style.top = `${event.clientY - area.top - 28}px`;
      label.hidden = false;
      $('#tooltip').hidden = true;
    }
  }
});
scene.addEventListener('pointerup', async (event) => {
  const current = drag; drag = null;
  $('#drag-coords').hidden = true;
  if (!current) return;
  if (current.node !== null && current.moved) {
    const [x, y] = toWorld(event.clientX - current.rect.left, event.clientY - current.rect.top);
    await control({action: 'move', node: current.node, x, y});
  } else if (!current.moved) {
    // Clicking a node's battery opens its charge history.
    if (current.node !== null && current.battery) openPanel(current.node, 'battery');
    selectNode(current.node, event.shiftKey || event.ctrlKey || event.metaKey);
  }
});
scene.addEventListener('contextmenu', async (event) => {
  event.preventDefault();
  const node = nodeAt(event.target);
  const menu = $('#node-menu');
  if (node === null) { menu.hidden = true; return; }
  const data = await getJson(`/api/panels?node=${node}`);
  const entries = data.panels.map(p => html('button', {textContent: `Show ${p.title}`, onclick: () => { menu.hidden = true; openPanel(node, p.id); }}));
  entries.push(html('button', {textContent: view.follow === node ? 'Stop following' : 'Follow node', onclick: () => { menu.hidden = true; view.follow = view.follow === node ? null : node; render(); }}));
  menu.replaceChildren(...entries);
  const rect = scene.parentElement.getBoundingClientRect();
  menu.hidden = false;
  placeInTopology(menu, {x: event.clientX - rect.left, y: event.clientY - rect.top});
});
scene.addEventListener('pointerleave', () => { $('#tooltip').hidden = true; tooltipNode = null; clearTimeout(tooltipTimer); });
document.addEventListener('click', (event) => { if (!event.target.closest('#node-menu')) $('#node-menu').hidden = true; });

let tooltipTimer = null, tooltipNode = null, lastTipPoint = {x: 0, y: 0};
// Keep a floating element inside the topology, so it neither slides under the
// sidebar nor gets squeezed by the little width left next to it.
function placeInTopology(element, point) {
  const area = scene.parentElement;
  element.style.left = `${Math.max(0, Math.min(point.x, area.clientWidth - element.offsetWidth))}px`;
  element.style.top = `${Math.max(0, Math.min(point.y, area.clientHeight - element.offsetHeight))}px`;
}
function showTooltip(event) {
  const node = nodeAt(event.target), tip = $('#tooltip');
  if (node === null) { tip.hidden = true; tooltipNode = null; return; }
  const rect = scene.parentElement.getBoundingClientRect();
  lastTipPoint = {x: event.clientX - rect.left + 14, y: event.clientY - rect.top + 14};
  placeInTopology(tip, lastTipPoint);
  if (node === tooltipNode) return;
  tooltipNode = node; clearTimeout(tooltipTimer);
  tooltipTimer = setTimeout(async () => {
    const panel = await getJson(`/api/panel?node=${node}&id=node-info`);
    if (tooltipNode !== node) return;
    tip.replaceChildren(...panel.sections.map(renderSection)); tip.hidden = false;
    placeInTopology(tip, lastTipPoint);
  }, 400);
}

// Node selection, object tree and plugin panels

async function selectNode(node, add = false) {
  await control({action: 'select', node, add});
  await refresh();
  if (node === null || add) return;
  showTab('node');
  const objects = $('#objects');
  objects.replaceChildren(html('h3', {textContent: `Node ${node} object tree`}));
  const data = await getJson(`/api/objects?node=${node}`);
  objects.append(data.error ? html('p', {textContent: data.error}) : renderObject(data, `Node ${node}`, true));
  const panels = await getJson(`/api/panels?node=${node}`);
  $('#panels').replaceChildren(html('div', {className: 'panel-buttons'}, panels.panels.map(p => html('button', {textContent: p.title, onclick: () => openPanel(node, p.id)}))));
}
function renderSection(section) {
  const box = html('section', {className: 'panel-section'});
  const chart = section.plot || section.histogram;
  const heading = html('h4', {textContent: chart ? `${section.title} (${chart.unit})` : section.title});
  if (section.title) box.append(heading);
  if (section.note) box.append(html('small', {textContent: section.note}));
  if (section.items) box.append(html('dl', {}, section.items.flatMap(([k, v]) => [html('dt', {textContent: k}), html('dd', {textContent: v})])));
  if (section.plot) box.append(renderPlot(section.plot, section.title, heading));
  if (section.histogram) box.append(renderHistogram(section.histogram, section.title));
  if (section.columns) {
    const head = html('tr', {}, section.columns.map(c => html('th', {textContent: c})));
    const rows = section.rows.map(r => html('tr', {}, r.map(c => html('td', {textContent: c}))));
    box.append(html('div', {className: 'table-wrap'}, [html('table', {}, [html('thead', {}, [head]), html('tbody', {}, rows)])]));
  }
  return box;
}
function renderControls(node, id, controls) {
  const form = html('form', {className: 'panel-controls'});
  for (const c of controls) {
    const field = html('fieldset', {}, [html('legend', {textContent: c.label})]);
    if (c.kind === 'checklist') {
      field.append(html('button', {type: 'button', textContent: 'Sel. all', onclick: () => { field.querySelectorAll('input').forEach(i => i.checked = true); form.requestSubmit(); }}),
                   html('button', {type: 'button', textContent: 'Sel. none', onclick: () => { field.querySelectorAll('input').forEach(i => i.checked = false); form.requestSubmit(); }}));
      const list = html('div', {className: 'checklist'});
      for (const option of c.options) list.append(html('label', {}, [html('input', {type: 'checkbox', name: c.name, value: option, checked: c.value.includes(option), onchange: () => form.requestSubmit()}), option]));
      field.append(list);
    } else if (c.kind === 'select') {
      const select = html('select', {name: c.name}, [html('option', {value: '', textContent: '-'}), ...c.options.map(o => html('option', {value: o, textContent: o, selected: o === c.value}))]);
      field.append(select, html('button', {type: 'button', textContent: c.submit || 'Apply', onclick: () => { if (select.value) post('/api/panel-action', {node, id, values: {[c.name]: select.value}}); }}));
    } else if (c.kind === 'button') {
      field.replaceChildren(html('button', {type: 'button', textContent: c.label, onclick: () => post('/api/panel-action', {node, id, values: {[c.name]: true}})}));
    } else if (c.kind === 'radio') {
      for (const option of c.options) field.append(html('label', {}, [html('input', {type: 'radio', name: c.name, value: option, checked: c.value === option, onchange: () => form.requestSubmit()}), option]));
    }
    form.append(field);
  }
  form.onsubmit = async (event) => {
    event.preventDefault();
    const values = {};
    for (const c of controls) {
      const inputs = [...form.querySelectorAll(`input[name="${c.name}"]`)].filter(i => i.checked).map(i => i.value);
      values[c.name] = c.kind === 'checklist' ? inputs : inputs[0];
    }
    await post('/api/panel-action', {node, id, values});
  };
  return form;
}
// Panel windows behave like desktop windows inside the topology area: drag
// them by the title bar, click to bring to front, minimize to the taskbar at
// the bottom, restore from there, or close.
let topZ = 10;
function raise(win) { win.style.zIndex = String(++topZ); }
function makeDraggable(win, handle) {
  handle.addEventListener('pointerdown', (event) => {
    if (event.target.closest('button')) return;
    const area = win.parentElement.getBoundingClientRect(), rect = win.getBoundingClientRect();
    const dx = event.clientX - rect.left, dy = event.clientY - rect.top;
    raise(win);
    handle.setPointerCapture(event.pointerId);
    const move = (e) => {
      // Keep the title bar inside the topology area so it can always be grabbed.
      const x = Math.min(Math.max(e.clientX - area.left - dx, 0), area.width - 80);
      const y = Math.min(Math.max(e.clientY - area.top - dy, 0), area.height - 32);
      win.style.left = `${x}px`; win.style.top = `${y}px`; win.style.right = 'auto';
    };
    const up = () => { handle.removeEventListener('pointermove', move); handle.removeEventListener('pointerup', up); };
    handle.addEventListener('pointermove', move);
    handle.addEventListener('pointerup', up);
  });
}
function openPanel(node, id) {
  const key = `${node}:${id}`;
  if (openPanels.has(key)) { const existing = openPanels.get(key); existing.restore(); return; }
  const win = html('div', {className: 'panel-window'});
  win.dataset.key = key;
  const title = html('strong');
  const close = () => { win.remove(); panel.taskButton.remove(); openPanels.delete(key); };
  const header = html('header', {title: 'Drag to move'}, [title, html('div', {className: 'window-buttons'}, [
    html('button', {textContent: 'Minimize', className: 'minimize', onclick: () => panel.minimize()}),
    html('button', {textContent: 'Close', className: 'close', onclick: close}),
  ])]);
  const body = html('div', {className: 'panel-body'}), controlsBox = html('div');
  win.append(header, controlsBox, body);
  const offset = 28 * (openPanels.size % 8);
  win.style.top = `${12 + offset}px`; win.style.right = `${12 + offset}px`;
  win.addEventListener('pointerdown', () => raise(win));
  makeDraggable(win, header);
  scene.parentElement.append(win);
  raise(win);
  const query = node === null ? `id=${id}` : `node=${node}&id=${id}`;
  const panel = {win, controls: null, minimized: false,
    taskButton: html('button', {className: 'task', title: 'Restore', onclick: () => panel.restore()}),
    // Only minimized windows are listed in the taskbar.
    minimize() { panel.minimized = true; win.hidden = true; panel.taskButton.classList.add('minimized'); $('#taskbar').append(panel.taskButton); },
    restore() { panel.minimized = false; win.hidden = false; panel.taskButton.remove(); raise(win); panel.update(); },
    update: async () => {
      if (panel.minimized) return;
      const data = await getJson(`/api/panel?${query}`);
      title.textContent = data.title || id;
      panel.taskButton.textContent = data.title || id;
      if (data.error) { body.textContent = data.error; return; }
      // Controls keep their input state across refreshes; rebuild them only
      // when Python changes which controls exist.
      const signature = JSON.stringify(data.controls.map(c => [c.name, c.kind, c.label, c.options]));
      if (signature !== panel.controls) { controlsBox.replaceChildren(...(data.controls.length ? [renderControls(node, id, data.controls)] : [])); panel.controls = signature; }
      body.replaceChildren(...data.sections.map(renderSection));
    }};
  panel.taskButton.textContent = id;
  openPanels.set(key, panel);
  panel.update();
}
setInterval(() => { if (!plotDragging) for (const panel of openPanels.values()) panel.update(); }, 1000);

// Generic time-series chart: one y axis, 2px lines, crosshair tooltip.
// Series colors come from a fixed slot order chosen in Python, so a flow keeps
// its color while the filter changes.
const SERIES_COLORS = ['#3987e5', '#d95926', '#199e70', '#c98500', '#d55181', '#008300', '#9085e9', '#e66767'];
const plotHover = new Map();
// Zoomed time range per chart title; kept across the 1 s panel refreshes.
const plotZoom = new Map();
let plotDragging = false;  // pauses panel refresh so a zoom drag is not cut off
// SI prefix of a bit rate in bit/s: [divisor, unit], from bit/s to Pbit/s.
function rateUnit(bps) {
  const prefixes = ['', 'k', 'M', 'G', 'T', 'P'];
  const i = Math.min(Math.max(Math.floor(Math.log10(Math.abs(bps) || 1) / 3), 0), prefixes.length - 1);
  return [10 ** (3 * i), `${prefixes[i]}bit/s`];
}
function formatRate(bps) {
  const [divisor, unit] = rateUnit(bps);
  return `${(bps / divisor).toFixed(2)} ${unit}`;
}
function formatValue(v, unit) {
  if (v === null || v === undefined) return '-';
  const abs = Math.abs(v);
  const text = v === 0 ? '0' : abs >= 1e9 ? `${(v / 1e9).toFixed(2)}G` : abs >= 1e6 ? `${(v / 1e6).toFixed(2)}M` : abs >= 1e3 ? `${(v / 1e3).toFixed(1)}k` : abs >= 10 ? v.toFixed(1) : v.toPrecision(3);
  return unit ? `${text} ${unit}` : text;
}
// Bit rates (unit 'bit/s') are drawn in the SI unit that fits the largest
// value shown, which is also written in the heading.
function renderPlot(plot, title, heading) {
  const W = 600, H = 190, L = 56, R = 12, T = 10, B = 26;
  const zoomKey = title || plot.unit, zoom = plotZoom.get(zoomKey);
  // When zoomed, keep the points in range plus one on each side so lines
  // still reach the chart edges.
  const clip = (points) => {
    if (!zoom) return points;
    const first = Math.max(0, points.findIndex(p => p[0] >= zoom[0]) - 1);
    let last = points.length - 1;
    while (last > 0 && points[last - 1][0] > zoom[1]) --last;
    return points.slice(first, last + 1);
  };
  const series = plot.series.map(s => ({...s, points: clip(s.points)})).filter(s => s.points.length);
  const wrap = html('div', {className: 'plot'});
  // plot.empty / histogram.empty: why there is nothing to draw, shown instead of bare axes.
  if (plot.empty || !series.length) { wrap.append(html('small', {textContent: plot.empty || 'No samples yet.'})); return wrap; }
  const all = series.flatMap(s => s.points);
  const inRange = zoom ? all.filter(p => p[0] >= zoom[0] && p[0] <= zoom[1]) : all;
  const values = (inRange.length ? inRange : all).map(p => p[1]).filter(v => v !== null);
  // plot.range, when given, fixes the unzoomed time span (e.g. 0 s to now).
  const t0 = zoom ? zoom[0] : plot.range ? plot.range[0] : Math.min(...all.map(p => p[0]));
  const t1 = zoom ? zoom[1] : Math.max(plot.range ? plot.range[1] : -Infinity, ...all.map(p => p[0]), t0 + 1e-9);
  const vMax = Math.max(...values, 0) * 1.08 || 1, vMin = Math.min(0, ...values);
  const [divisor, axisUnit] = plot.unit === 'bit/s' ? rateUnit(Math.max(Math.abs(vMax), Math.abs(vMin))) : [1, plot.unit];
  if (heading && title) heading.textContent = `${title} (${axisUnit})`;
  const sx = (t) => L + (t - t0) / (t1 - t0) * (W - L - R), sy = (v) => H - B - (v - vMin) / (vMax - vMin) * (H - T - B);
  const chart = svg('svg', {viewBox: `0 0 ${W} ${H}`, class: 'plot-svg', role: 'img', 'aria-label': `Line chart in ${plot.unit}`});
  for (let i = 0; i <= 4; ++i) {
    const v = vMin + (vMax - vMin) * i / 4, y = sy(v);
    chart.append(svg('line', {x1: L, x2: W - R, y1: y, y2: y, class: 'grid'}), svg('text', {x: L - 6, y: y + 4, class: 'axis-label', 'text-anchor': 'end'}, formatValue(v / divisor, '')));
  }
  for (let i = 0; i <= 4; ++i) {
    const t = t0 + (t1 - t0) * i / 4;
    chart.append(svg('text', {x: sx(t), y: H - 8, class: 'axis-label', 'text-anchor': 'middle'}, `${t.toFixed(1)} s`));
  }
  for (const s of series) {
    // Missing samples (no packets received) break the line instead of dropping to zero.
    let d = '', pen = false;
    for (const [t, v] of s.points) {
      if (v === null) { pen = false; continue; }
      d += `${pen ? 'L' : 'M'}${sx(t).toFixed(1)},${sy(v).toFixed(1)}`; pen = true;
    }
    chart.append(svg('path', {d, class: 'series', stroke: SERIES_COLORS[s.slot % SERIES_COLORS.length]}));
  }
  // Lines outside the zoomed range are clipped to the plot area.
  const clipId = `clip-${Math.random().toString(36).slice(2)}`;
  chart.prepend(svg('clipPath', {id: clipId}));
  chart.firstChild.append(svg('rect', {x: L, y: T, width: W - L - R, height: H - T - B}));
  for (const path of chart.querySelectorAll('path.series')) path.setAttribute('clip-path', `url(#${clipId})`);
  const cross = svg('line', {y1: T, y2: H - B, class: 'crosshair', visibility: 'hidden'});
  const band = svg('rect', {y: T, height: H - T - B, class: 'zoom-band', visibility: 'hidden'});
  const hit = svg('rect', {x: L, y: T, width: W - L - R, height: H - T - B, fill: 'transparent', class: 'plot-hit'});
  chart.append(cross, band, hit);
  // Drag across the chart to zoom into that time range; double-click resets.
  const timeAt = (e) => {
    const rect = chart.getBoundingClientRect(), x = (e.clientX - rect.left) / rect.width * W;
    return t0 + (Math.min(Math.max(x, L), W - R) - L) / (W - L - R) * (t1 - t0);
  };
  let dragStart = null;
  hit.addEventListener('pointerdown', (e) => { dragStart = timeAt(e); plotDragging = true; hit.setPointerCapture(e.pointerId); });
  hit.addEventListener('pointerup', (e) => {
    if (dragStart === null) return;
    const a = Math.min(dragStart, timeAt(e)), b = Math.max(dragStart, timeAt(e));
    dragStart = null; plotDragging = false; band.setAttribute('visibility', 'hidden');
    if (sx(b) - sx(a) > 6) { plotZoom.set(zoomKey, [a, b]); wrap.replaceWith(renderPlot(plot, title, heading)); }
  });
  hit.addEventListener('dblclick', () => { plotZoom.delete(zoomKey); wrap.replaceWith(renderPlot(plot, title, heading)); });
  const tip = html('div', {className: 'plot-tip', hidden: true});
  const key = `${plot.unit}:${series.map(s => s.name).join('|')}`;
  const show = (t) => {
    if (t === null) { cross.setAttribute('visibility', 'hidden'); tip.hidden = true; return; }
    cross.setAttribute('x1', sx(t)); cross.setAttribute('x2', sx(t)); cross.setAttribute('visibility', 'visible');
    const rows = series.map(s => {
      let best = null;
      for (const p of s.points) if (p[0] <= t) best = p;
      return html('div', {}, [html('span', {className: 'swatch', style: `background:${SERIES_COLORS[s.slot % SERIES_COLORS.length]}`}), `${s.name}: ${plot.unit === 'bit/s' && best && best[1] !== null ? formatRate(best[1]) : formatValue(best ? best[1] : null, plot.unit)}`]);
    });
    tip.replaceChildren(html('strong', {textContent: `${t.toFixed(3)} s`}), ...rows);
    tip.style.left = `${Math.min(sx(t) / W * 100, 60)}%`; tip.hidden = false;
  };
  hit.addEventListener('pointermove', (e) => {
    const t = timeAt(e);
    if (dragStart !== null) {
      const a = Math.min(dragStart, t), b = Math.max(dragStart, t);
      band.setAttribute('x', sx(a)); band.setAttribute('width', Math.max(sx(b) - sx(a), 0)); band.setAttribute('visibility', 'visible');
    }
    plotHover.set(key, t); show(t);
  });
  hit.addEventListener('pointerleave', () => { plotHover.delete(key); show(null); });
  wrap.append(chart, tip);
  if (zoom) wrap.append(html('button', {type: 'button', className: 'zoom-reset', title: 'Or double-click the chart', textContent: 'Reset zoom', onclick: () => { plotZoom.delete(zoomKey); wrap.replaceWith(renderPlot(plot, title, heading)); }}));
  if (plotHover.has(key)) show(plotHover.get(key));
  if (series.length >= 2) wrap.append(html('div', {className: 'legend'}, series.map(s => html('span', {}, [html('span', {className: 'swatch', style: `background:${SERIES_COLORS[s.slot % SERIES_COLORS.length]}`}), s.name]))));
  return wrap;
}

// Histogram: bins of each series side by side, colored like the time series.
// Bins are [index, count]; a bin covers [index, index + 1) * binWidth. Wide
// ranges are shown with merged bins so bars stay readable; zooming in (drag
// across the chart, like the time series) shows the finer bins again.
const MAX_BARS = 100;
function renderHistogram(histogram, title) {
  const W = 600, H = 190, L = 56, R = 12, T = 10, B = 26;
  const zoomKey = `histogram:${title || histogram.unit}`, zoom = plotZoom.get(zoomKey);
  const series = histogram.series.filter(s => s.bins.length);
  const wrap = html('div', {className: 'plot'});
  if (histogram.empty || !series.length) { wrap.append(html('small', {textContent: histogram.empty || 'No samples yet.'})); return wrap; }
  const indices = series.flatMap(s => s.bins.map(b => b[0]));
  // Bin index range shown: the zoomed value range, or all bins with samples.
  const first = zoom ? Math.floor(zoom[0] / histogram.binWidth) : Math.min(...indices);
  const lastIndex = zoom ? Math.max(Math.ceil(zoom[1] / histogram.binWidth) - 1, first) : Math.max(...indices);
  const merge = Math.ceil((lastIndex - first + 1) / MAX_BARS);
  const bars = Math.floor((lastIndex - first) / merge) + 1, width = histogram.binWidth * merge;
  const counts = series.map(s => {
    const c = new Array(bars).fill(0);
    for (const [index, count] of s.bins) if (index >= first && index <= lastIndex) c[Math.floor((index - first) / merge)] += count;
    return c;
  });
  const cMax = Math.max(...counts.flat()) * 1.08 || 1;
  const x0 = first * histogram.binWidth;
  const slotW = (W - L - R) / bars, barW = Math.max(slotW / series.length - 1, 1);
  const sy = (v) => H - B - v / cMax * (H - T - B);
  const chart = svg('svg', {viewBox: `0 0 ${W} ${H}`, class: 'plot-svg', role: 'img', 'aria-label': `Histogram in ${histogram.unit}`});
  for (let i = 0; i <= 4; ++i) {
    const v = cMax * i / 4, y = sy(v);
    chart.append(svg('line', {x1: L, x2: W - R, y1: y, y2: y, class: 'grid'}), svg('text', {x: L - 6, y: y + 4, class: 'axis-label', 'text-anchor': 'end'}, formatValue(v, '')));
  }
  for (let i = 0; i <= 4; ++i) {
    const v = x0 + width * bars * i / 4;
    chart.append(svg('text', {x: L + (W - L - R) * i / 4, y: H - 8, class: 'axis-label', 'text-anchor': 'middle'}, formatValue(v, histogram.unit)));
  }
  counts.forEach((c, k) => c.forEach((count, bar) => {
    if (count) chart.append(svg('rect', {x: L + bar * slotW + k * barW, y: sy(count), width: barW, height: H - B - sy(count), fill: SERIES_COLORS[series[k].slot % SERIES_COLORS.length]}));
  }));
  const band = svg('rect', {y: T, height: H - T - B, class: 'zoom-band', visibility: 'hidden'});
  const hit = svg('rect', {x: L, y: T, width: W - L - R, height: H - T - B, fill: 'transparent', class: 'plot-hit'});
  chart.append(band, hit);
  const tip = html('div', {className: 'plot-tip', hidden: true});
  // Drag across the chart to zoom into that value range; double-click resets.
  const xAt = (e) => { const rect = chart.getBoundingClientRect(); return Math.min(Math.max((e.clientX - rect.left) / rect.width * W, L), W - R); };
  const valueAt = (x) => x0 + (x - L) / (W - L - R) * width * bars;
  const rerender = () => wrap.replaceWith(renderHistogram(histogram, title));
  let dragStart = null;
  hit.addEventListener('pointerdown', (e) => { dragStart = xAt(e); plotDragging = true; hit.setPointerCapture(e.pointerId); });
  hit.addEventListener('pointerup', (e) => {
    if (dragStart === null) return;
    const a = Math.min(dragStart, xAt(e)), b = Math.max(dragStart, xAt(e));
    dragStart = null; plotDragging = false; band.setAttribute('visibility', 'hidden');
    if (b - a > 6) { plotZoom.set(zoomKey, [valueAt(a), valueAt(b)]); rerender(); }
  });
  hit.addEventListener('dblclick', () => { plotZoom.delete(zoomKey); rerender(); });
  hit.addEventListener('pointermove', (e) => {
    const x = xAt(e);
    if (dragStart !== null) {
      band.setAttribute('x', Math.min(dragStart, x)); band.setAttribute('width', Math.abs(x - dragStart)); band.setAttribute('visibility', 'visible');
    }
    const bar = Math.min(Math.max(Math.floor((x - L) / slotW), 0), bars - 1), start = x0 + bar * width;
    tip.replaceChildren(html('strong', {textContent: `${formatValue(start, '')} - ${formatValue(start + width, histogram.unit)}`}),
      ...series.map((s, k) => html('div', {}, [html('span', {className: 'swatch', style: `background:${SERIES_COLORS[s.slot % SERIES_COLORS.length]}`}), `${s.name}: ${counts[k][bar]}`])));
    tip.style.left = `${Math.min(x / W * 100, 60)}%`; tip.hidden = false;
  });
  hit.addEventListener('pointerleave', () => { tip.hidden = true; });
  wrap.append(chart, tip);
  if (zoom) wrap.append(html('button', {type: 'button', className: 'zoom-reset', title: 'Or double-click the chart', textContent: 'Reset zoom', onclick: () => { plotZoom.delete(zoomKey); rerender(); }}));
  if (series.length >= 2) wrap.append(html('div', {className: 'legend'}, series.map(s => html('span', {}, [html('span', {className: 'swatch', style: `background:${SERIES_COLORS[s.slot % SERIES_COLORS.length]}`}), s.name]))));
  return wrap;
}

function renderObject(object, label, open) {
  const box = html('details', {className: 'object', open}, [html('summary', {textContent: `${label}: ${object.type}`})]);
  box.fill = (data) => {
    if (data.error) { box.append(html('small', {textContent: data.error})); return; }
    for (const item of data.details) box.append(html('small', {textContent: `${item.name}: ${item.value}`}));
    for (const attr of data.attributes) box.append(attributeRow(data.id, attr));
    for (const child of data.children) {
      const node = renderObject(child, child.label, false);
      node.addEventListener('toggle', async () => {
        if (!node.open || node.dataset.loaded) return;
        node.dataset.loaded = '1';
        node.fill(await getJson(`/api/objects?id=${child.id}`));
      });
      box.append(node);
    }
  };
  if (object.attributes) box.fill(object);
  return box;
}
function attributeRow(objectId, attr) {
  const input = html('input', {value: attr.value, disabled: !attr.writable});
  const row = html('div', {className: 'attribute'}, [html('small', {textContent: `${attr.name} (${attr.type})`, title: attr.help || ''}), input]);
  if (attr.writable) {
    const status = html('small');
    row.append(html('button', {textContent: 'Apply', onclick: async () => {
      const result = await post('/api/object-attribute', {object: objectId, attribute: attr.name, value: input.value});
      status.textContent = result.ok ? 'Updated' : (result.error || 'Value rejected');
    }}), status);
  }
  return row;
}

// Type catalogue

function showType(type) {
  const details = $('#details');
  details.replaceChildren(html('h2', {textContent: type.name}), html('small', {textContent: type.group || 'Ungrouped'}));
  for (const [heading, key, label] of [['Attributes', 'attributes', 'type'], ['Trace sources', 'traces', 'callback'], ['Cppyy methods', 'methods', null]]) {
    const list = html('ul');
    for (const item of type[key]) {
      const li = html('li', {textContent: typeof item === 'string' ? item : item.name});
      if (label && (item[label] || item.help)) li.append(html('small', {textContent: item[label] || item.help}));
      if (key === 'attributes' && (item.flags & 4)) {
        const input = html('input', {value: item.initial}), status = html('small');
        input.setAttribute('aria-label', `Default value for ${item.name}`);
        li.append(html('div', {}, [input, html('button', {textContent: 'Set default for new objects', onclick: async () => {
          const result = await post('/api/default-attribute', {type: type.name, attribute: item.name, value: input.value});
          status.textContent = result.ok ? 'Default updated' : (result.error || 'Value was rejected');
        }}), status]));
      }
      list.append(li);
    }
    details.append(html('section', {}, [html('h3', {textContent: `${heading} (${type[key].length})`}), list]));
  }
  for (const button of $('#types').children) button.classList.toggle('selected', button.dataset.name === type.name);
}
let typesInUse = new Set(), catalogueLoad = null;
// Reading the whole TypeId registry is slow and blocks the simulation, so it
// waits until the tab is opened; what is in use changes with each restart.
async function loadTypes() {
  catalogueLoad = catalogueLoad || getJson('/api/types').then(data => { catalogue = data.types; });
  const [inUse] = await Promise.all([getJson('/api/types-in-use'), catalogueLoad]);
  typesInUse = new Set(inUse.types);
  renderTypes();
}
function renderTypes() {
  const query = $('#type-filter').value.toLowerCase();
  const onlyInUse = $('#types-in-use').checked;
  $('#types').replaceChildren(...catalogue.filter(t => `${t.name} ${t.group}`.toLowerCase().includes(query) && (!onlyInUse || typesInUse.has(t.name)))
    .map(type => { const b = html('button', {className: 'type', textContent: type.name, onclick: () => showType(type)}); b.dataset.name = type.name; return b; }));
}

// Console

async function runConsole() {
  const input = $('#console-input'), output = $('#console-output');
  const source = input.value;
  if (!source.trim()) return;
  output.textContent += `>>> ${source}\n`;
  const result = await post('/api/console', {code: source});
  output.textContent += result.ok ? result.output : `${result.error}\n`;
  output.scrollTop = output.scrollHeight;
  input.value = '';
}

// Toolbar and tabs

function showTab(name) {
  for (const b of document.querySelectorAll('[data-tab]')) b.classList.toggle('selected', b.dataset.tab === name);
  for (const t of ['node', 'types', 'console']) $(`#tab-${t}`).hidden = t !== name;
  if (name === 'types') loadTypes();
}
for (const b of document.querySelectorAll('[data-tab]')) b.onclick = () => showTab(b.dataset.tab);
for (const button of document.querySelectorAll('[data-action]')) {
  button.onclick = async () => {
    button.disabled = true;
    try {
      const result = await control({action: button.dataset.action});
      if (result.ok && button.dataset.action === 'restart') { view.fitted = false; iconUrls.clear(); openPanels.forEach(p => { p.win.remove(); p.taskButton.remove(); }); openPanels.clear(); }
      if (result.ok && button.dataset.action === 'exit') {
        exited = true;
        for (const b of document.querySelectorAll('header button:not(#theme-toggle), header input, header select')) b.disabled = true;
        clock.textContent = 'Exited: the scenario continues after Simulator::Run()';
        return;
      }
      await refresh();
    } finally { if (!exited) button.disabled = false; }
  };
}
$('#speed').oninput = (e) => { const s = 10 ** Number(e.target.value); $('#speed-value').textContent = `${s.toFixed(2)}x`; control({action: 'speed', value: s}); };
$('#smoothing').onchange = (e) => control({action: 'smoothing', value: Number(e.target.value)});
$('#transmissions').onchange = (e) => control({action: 'transmissions', value: e.target.value});
$('#node-size').oninput = (e) => { view.nodeSize = Number(e.target.value); render(); };
// The sidebar minimizes to the taskbar like the panel windows.
const sidebarTask = html('button', {className: 'task minimized', textContent: 'Sidebar', title: 'Restore the sidebar'});
function setSidebarMinimized(minimized) {
  document.body.classList.toggle('sidebar-collapsed', minimized);
  if (minimized) $('#taskbar').prepend(sidebarTask); else sidebarTask.remove();
  try { localStorage.setItem('pyviz-sidebar-collapsed', minimized ? '1' : '0'); } catch (_) {}
  render();
}
$('#minimize-sidebar').onclick = () => setSidebarMinimized(true);
sidebarTask.onclick = () => setSidebarMinimized(false);
try { if (localStorage.getItem('pyviz-sidebar-collapsed') === '1') setSidebarMinimized(true); } catch (_) {}
$('#play-pause').onclick = () => control({action: state && state.running && !state.finished ? 'pause' : 'play'}).then(refresh);
$('#toggle-trails').onclick = async () => {
  const enable = !(state && state.showTrails);
  await control({action: 'trails', value: enable});
};
// The system theme applies until the button picks one, which is then remembered.
const systemTheme = matchMedia('(prefers-color-scheme: light)');
const currentTheme = () => document.documentElement.dataset.theme || (systemTheme.matches ? 'light' : 'dark');
function showThemeButton() {
  const other = currentTheme() === 'light' ? 'dark' : 'light';
  $('#theme-toggle').textContent = other === 'light' ? 'Light' : 'Dark';
  $('#theme-toggle').title = `Switch to the ${other} theme`;
}
try { const saved = localStorage.getItem('pyviz-theme'); if (saved) document.documentElement.dataset.theme = saved; } catch (_) {}
showThemeButton();
systemTheme.addEventListener('change', showThemeButton);
$('#theme-toggle').onclick = () => {
  const theme = currentTheme() === 'light' ? 'dark' : 'light';
  document.documentElement.dataset.theme = theme;
  try { localStorage.setItem('pyviz-theme', theme); } catch (_) {}
  showThemeButton();
};
$('#zoom-fit').onclick = () => { view.fitted = false; view.follow = null; render(); };
$('#screenshot').onclick = () => {
  const clone = scene.cloneNode(true);
  clone.setAttribute('xmlns', svgNs);
  clone.dataset.theme = currentTheme();
  const style = svg('style', {}, [...document.styleSheets].flatMap(s => { try { return [...s.cssRules].map(r => r.cssText); } catch (_) { return []; } }).join('\n'));
  clone.prepend(style);
  const blob = new Blob([new XMLSerializer().serializeToString(clone)], {type: 'image/svg+xml'});
  const link = html('a', {href: URL.createObjectURL(blob), download: `pyviz-${state ? state.time.toFixed(3) : 0}s.svg`});
  link.click(); URL.revokeObjectURL(link.href);
};
$('#console-run').onclick = runConsole;
$('#console-input').addEventListener('keydown', (e) => { if (e.key === 'Enter' && e.shiftKey) { e.preventDefault(); runConsole(); } });
$('#type-filter').addEventListener('input', renderTypes);
$('#types-in-use').addEventListener('change', renderTypes);
for (const example of document.querySelectorAll('#console-examples code')) example.onclick = () => { $('#console-input').value = example.textContent; $('#console-input').focus(); };
getJson('/api/panels').then(data => {
  const select = $('#global-panels');
  for (const p of data.panels) {
    select.append(html('option', {value: p.id, textContent: p.title, disabled: !!p.unavailable, title: p.unavailable || ''}));
  }
  select.hidden = !data.panels.length;
  select.onchange = () => { if (select.value) openPanel(null, select.value); select.value = ''; };
});
window.addEventListener('resize', render);
// Poll again as soon as the previous poll is done (plus a small gap), so a
// slow request or render never piles up requests.
async function pollLoop() {
  if (exited) return;
  await refresh();
  if (!exited) setTimeout(pollLoop, POLL_GAP_MS);
}
pollLoop();
