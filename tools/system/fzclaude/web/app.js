/* fzclaude-web front end. Every request carries the per-start token. */
'use strict';

const TOKEN = document.querySelector('meta[name=fzc-token]').content;
const ID_RE = /\d{14}\.\d+/;
const $ = (sel, el = document) => el.querySelector(sel);

function esc(s) {
  return String(s ?? '').replace(/[&<>"']/g, c => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c]));
}

async function api(name, { method = 'GET', query = null, body = null } = {}) {
  const url = '/api/' + name + (query ? '?' + new URLSearchParams(query) : '');
  const opts = { method, headers: { 'X-FZC-Token': TOKEN } };
  if (body !== null) {
    opts.headers['Content-Type'] = 'application/json';
    opts.body = JSON.stringify(body);
  }
  const r = await fetch(url, opts);
  const data = await r.json().catch(() => ({ error: `HTTP ${r.status}` }));
  if (!r.ok) throw new Error(data.error || `HTTP ${r.status}`);
  return data;
}

function linkify(text) {
  return esc(text).replace(/\b\d{14}\.\d+\b/g, id => `<a class="nid" data-id="${id}" title="Show this Node">${id}</a>`);
}

document.addEventListener('click', e => {
  const a = e.target.closest('a.nid');
  if (!a) return;
  e.preventDefault();
  showView('use');
  selectCommand('node', { id: a.dataset.id }, true);
});

/* ───────────────────────────── views and theme ───────────────────────────── */

function showView(name) {
  if (!document.getElementById('view-' + name)) name = 'use';
  history.replaceState(null, '', '#' + name);
  document.querySelectorAll('.tab').forEach(t => t.classList.toggle('is-active', t.dataset.view === name));
  document.querySelectorAll('.view').forEach(v => v.classList.toggle('is-active', v.id === 'view-' + name));
  if (name === 'rules') loadRules();
  if (name === 'proposals') loadProposals();
  if (name === 'activity') loadActivity();
}
document.querySelectorAll('.tab').forEach(t => t.addEventListener('click', () => showView(t.dataset.view)));

function applyTheme(t) {
  if (t) document.documentElement.dataset.theme = t;
  else delete document.documentElement.dataset.theme;
}
try { applyTheme(localStorage.getItem('fzc-theme')); } catch (e) { /* storage unavailable */ }
$('#theme').addEventListener('click', () => {
  const dark = document.documentElement.dataset.theme
    ? document.documentElement.dataset.theme === 'dark'
    : matchMedia('(prefers-color-scheme: dark)').matches;
  const next = dark ? 'light' : 'dark';
  applyTheme(next);
  try { localStorage.setItem('fzc-theme', next); } catch (e) { /* storage unavailable */ }
});

$('#stop-server').addEventListener('click', async () => {
  if (!confirm('Stop the fzclaude-web server? The dashboard button starts it again.')) return;
  await api('stop', { method: 'POST', body: {} }).catch(() => {});
  document.body.innerHTML = '<main class="main narrow"><div class="card"><h2>fzclaude-web stopped.</h2>' +
    '<p class="muted">Start it again from the Formalizer dashboard.</p></div></main>';
});

/* ───────────────────────────── Use ───────────────────────────── */

const F = {
  int: (k, label, def) => ({ k, type: 'int', label, def }),
  text: (k, label, required = false, placeholder = '') => ({ k, type: 'text', label, required, placeholder }),
  id: (k, label, required = false) => ({ k, type: 'id', label, required }),
  flag: (k, label) => ({ k, type: 'flag', label }),
  stamp: (k, label) => ({ k, type: 'stamp', label }),
};

const COMMANDS = [
  { cmd: 'today', label: 'Today', desc: 'Schedule entries in scope (the same list the Schedule shows).',
    fields: [F.int('days', 'Days', 1), F.int('max', 'Max', '')] },
  { cmd: 'map', label: 'Map', desc: 'Outline of everything in scope, with the number of Nodes below each one. Active Nodes only, unless "Include inactive" is ticked.',
    fields: [F.int('depth', 'Depth', 2), F.int('width', 'Children per Node', 25),
      F.flag('milestones', 'Milestones only'), F.flag('all', 'Include inactive')] },
  { cmd: 'node', label: 'Node', desc: 'One Node: details, visible superiors and active dependencies, and its description.',
    fields: [F.id('id', 'Node ID', true), F.flag('full', 'Full description'), F.flag('all', 'Include inactive dependencies')] },
  { cmd: 'context', label: 'Context', desc: 'What Claude reads to start working on a Node: the Node, its path up to the nearest Milestone, active dependencies, links found in its text and Log, and recent Log entries.',
    fields: [F.id('id', 'Node ID', true), F.int('log', 'Log entries', 10), F.int('links', 'Links', 20), F.flag('full', 'Full description')] },
  { cmd: 'subtree', label: 'Subtree', desc: 'Everything active below a Node, as an indented tree.',
    fields: [F.id('id', 'Node ID', true), F.int('depth', 'Depth', 3), F.flag('milestones', 'Milestones only'),
      F.flag('all', 'Include inactive'), F.int('max', 'Max', '')] },
  { cmd: 'search', label: 'Search', desc: 'Search the descriptions of active Nodes in scope. Only matches are returned.',
    fields: [F.text('text', 'Text', true), F.flag('regex', 'Regular expression'), F.flag('milestones', 'Milestones only'),
      F.flag('all', 'Include inactive')] },
  { cmd: 'log', label: 'Log', desc: 'Log entries in scope. Without dates, the default number of recent days.',
    fields: [F.int('days', 'Days', ''), F.stamp('since', 'Since (YYYYmmddHHMM)'), F.stamp('until', 'Until'),
      F.id('node', 'Node ID'), F.text('search', 'Search'), F.flag('count', 'Count only'), F.flag('full', 'Full entries')] },
  { cmd: 'changes', label: 'Changes', desc: 'What Claude\'s next "changes" call would return. Previewing here does not move Claude\'s baseline.',
    fields: [] },
  { cmd: 'topics', label: 'Topics', desc: 'Topics on Nodes in scope, with counts.', fields: [] },
  { cmd: 'nnls', label: 'Named Node Lists', desc: 'NNLs that contain Nodes in scope.', fields: [] },
  { cmd: 'nnl', label: 'One NNL', desc: 'The active Nodes in scope in one Named Node List.',
    fields: [F.text('name', 'List name', true), F.flag('milestones', 'Milestones only'), F.flag('all', 'Include inactive')] },
  { cmd: 'scope', label: 'Scope', desc: 'Counts of visible and hidden Nodes, and the include roots.',
    fields: [{ k: 'action', type: 'select', label: 'Action', options: ['status', 'refresh'] }] },
];

let current = null;

function renderCommandList() {
  $('#cmd-list').innerHTML = COMMANDS.map(c =>
    `<button class="cmd" type="button" data-cmd="${c.cmd}"><b>${esc(c.label)}</b><span>${esc(c.desc.split('. ')[0])}</span></button>`).join('');
  document.querySelectorAll('.cmd[data-cmd]').forEach(b => b.addEventListener('click', () => selectCommand(b.dataset.cmd)));
}

function selectCommand(cmd, values = {}, run = false) {
  current = COMMANDS.find(c => c.cmd === cmd);
  document.querySelectorAll('.cmd[data-cmd]').forEach(b => b.classList.toggle('is-active', b.dataset.cmd === cmd));
  const form = $('#cmd-form');
  const fields = current.fields.map(f => {
    const v = values[f.k] ?? (f.def ?? '');
    if (f.type === 'flag') {
      return `<label class="field check"><input type="checkbox" name="${f.k}" ${values[f.k] ? 'checked' : ''}>${esc(f.label)}</label>`;
    }
    if (f.type === 'select') {
      return `<label class="field">${esc(f.label)}<select name="${f.k}">${f.options.map(o =>
        `<option ${o === v ? 'selected' : ''}>${o}</option>`).join('')}</select></label>`;
    }
    const type = f.type === 'int' ? 'number' : 'text';
    const ph = f.type === 'id' ? 'YYYYmmddHHMMSS.n' : (f.placeholder || '');
    return `<label class="field">${esc(f.label)}${f.required ? ' *' : ''}<input type="${type}" name="${f.k}" value="${esc(v)}" placeholder="${ph}" ${type === 'number' ? 'min="0"' : ''}></label>`;
  }).join('');
  form.innerHTML = `<p class="desc">${esc(current.desc)}</p>${fields}<button class="btn btn-primary" type="submit">Run</button>`;
  if (run) runCurrent();
}

$('#cmd-form').addEventListener('submit', e => { e.preventDefault(); runCurrent(); });

function formValues() {
  const opts = {};
  for (const f of current.fields) {
    const el = $(`[name="${f.k}"]`, $('#cmd-form'));
    if (f.type === 'flag') { if (el.checked) opts[f.k] = true; continue; }
    let v = el.value.trim();
    if (f.type === 'id' && v) {
      const m = v.match(ID_RE);
      v = m ? m[0] : v;
    }
    if (v !== '') opts[f.k] = v;
  }
  return opts;
}

async function runCurrent(extra = {}) {
  const opts = Object.assign(formValues(), extra);
  const missing = current.fields.filter(f => f.required && !opts[f.k]).map(f => f.label);
  if (missing.length) { setOutput(`Please fill in: ${missing.join(', ')}`); return; }
  setOutput('Running…');
  $('#out-argv').textContent = '';
  $('#out-meta').innerHTML = '';
  $('#out-actions').innerHTML = '';
  try {
    const r = await api('run', { method: 'POST', body: { cmd: current.cmd, opts } });
    $('#out-argv').textContent = r.argv.join(' ');
    const out = r.output || '';
    const nodes = new Set((out.match(/\b\d{14}\.\d+\b/g) || []));
    $('#out-meta').innerHTML =
      `<span class="pill">~${r.est_tokens} tokens</span><span class="pill">${out.length} characters</span>` +
      `<span class="pill">${nodes.size} Node IDs</span>` + (r.rc ? `<span class="pill neg">exit ${r.rc}</span>` : '');
    $('#out').innerHTML = linkify(out || '(no output)') + (r.stderr ? `\n<span class="muted">${esc(r.stderr)}</span>` : '');
    const actions = [];
    const more = out.match(/use --offset (\d+)\)/);
    if (more) actions.push(`<button class="btn" type="button" data-offset="${more[1]}">Next page</button>`);
    if (current.cmd === 'node' && opts.id) {
      actions.push(`<button class="btn" type="button" data-sub="${opts.id}">Subtree</button>`);
      actions.push(`<button class="btn" type="button" data-log="${opts.id}">Log for this Node</button>`);
      actions.push(`<a class="btn btn-ghost" href="http://localhost/cgi-bin/fzlink.py?id=${opts.id}" target="_blank" rel="noopener">Open in Formalizer ↗</a>`);
    }
    $('#out-actions').innerHTML = actions.join('');
  } catch (err) {
    setOutput('Error: ' + err.message);
  }
}

$('#out-actions').addEventListener('click', e => {
  const b = e.target.closest('button');
  if (!b) return;
  if (b.dataset.offset) runCurrent({ offset: b.dataset.offset });
  if (b.dataset.sub) selectCommand('subtree', { id: b.dataset.sub }, true);
  if (b.dataset.log) selectCommand('log', { node: b.dataset.log, days: 90 }, true);
});

function setOutput(text) { $('#out').textContent = text; }

/* ───────────────────────────── Scope rules ───────────────────────────── */

let rules = null;     // payload from /api/config
let draft = null;     // configuration being edited
let topics = [];
const lookups = {};   // Node ID → lookup result

async function loadRules() {
  try {
    rules = await api('config');
    draft = JSON.parse(JSON.stringify(rules.config));
    if (!topics.length) api('topics').then(r => { topics = r.topics; renderTopicList(); }).catch(() => {});
    renderRules();
    loadScopeStatus();
  } catch (err) {
    $('#rules-banner').innerHTML = `<div class="banner neg">Could not load the configuration: ${esc(err.message)}</div>`;
  }
}

async function loadScopeStatus() {
  $('#scope-status').textContent = rules.exists ? 'Loading…' : 'No configuration yet.';
  if (!rules.exists) return;
  try {
    const r = await api('run', { method: 'POST', body: { cmd: 'scope', opts: {} } });
    $('#scope-status').innerHTML = linkify(r.output || r.stderr);
  } catch (err) { $('#scope-status').textContent = 'Error: ' + err.message; }
}

$('#scope-refresh').addEventListener('click', async () => {
  $('#scope-status').textContent = 'Rebuilding…';
  try {
    const r = await api('run', { method: 'POST', body: { cmd: 'scope', opts: { action: 'refresh' } } });
    $('#scope-status').innerHTML = linkify(r.output || r.stderr);
  } catch (err) { $('#scope-status').textContent = 'Error: ' + err.message; }
});

function renderTopicList() {
  let dl = $('#topic-options');
  if (!dl) { dl = document.createElement('datalist'); dl.id = 'topic-options'; document.body.appendChild(dl); }
  dl.innerHTML = topics.map(t => `<option value="${esc(t.tag)}">${esc(t.title)}</option>`).join('');
}

function renderRules() {
  const banner = [];
  if (!rules.exists) {
    banner.push('<div class="banner warn"><b>No configuration yet, so nothing is visible to Claude.</b> ' +
      'Add at least one Include rule and save.</div>');
  }
  if (rules.errors.length) {
    banner.push(`<div class="banner neg"><b>The saved configuration has problems, and fzclaude refuses to run until they are fixed:</b><ul>${
      rules.errors.map(e => `<li>${esc(e)}</li>`).join('')}</ul></div>`);
  }
  $('#rules-banner').innerHTML = banner.join('');
  $('#history').innerHTML = '<option value="">-</option>' + rules.history.map(h =>
    `<option value="${esc(h)}">${esc(h.replace(/config-(\d{4})(\d\d)(\d\d)(\d\d)(\d\d)(\d\d)\.json/, '$1-$2-$3 $4:$5:$6'))}</option>`).join('');

  const groups = [];
  for (const s of rules.schema) {
    let g = groups.find(x => x.name === s.group);
    if (!g) { g = { name: s.group, items: [] }; groups.push(g); }
    g.items.push(s);
  }
  $('#rules-form').innerHTML = groups.map(g => `
    <div class="card group">
      <h3>${esc(g.name)}</h3>
      ${g.items.map(renderRule).join('')}
    </div>`).join('');
  for (const s of rules.schema) if (s.type === 'ids') draft[s.key].forEach(fetchLookup);
  markDirty();
}

function renderRule(s) {
  const help = s.help ? `<p class="rule-help">${esc(s.help)}</p>` : '';
  const head = `<div class="rule-head"><h4>${esc(s.label)}</h4><span class="rule-key">${esc(s.key)}</span></div>`;
  const def = rules.defaults[s.key];
  if (s.type === 'ids') {
    return `<div class="rule" data-key="${s.key}">${head}${help}
      <div class="chips">${draft[s.key].map(id => idChip(s.key, id)).join('') || '<span class="muted">None.</span>'}</div>
      <div class="adder"><input type="text" placeholder="Paste a Node ID or a Formalizer link to a Node" data-add="${s.key}">
      <button class="btn" type="button" data-addbtn="${s.key}">Add</button></div></div>`;
  }
  if (s.type === 'tags') {
    return `<div class="rule" data-key="${s.key}">${head}${help}
      <div class="chips inline">${draft[s.key].map(t => tagChip(s.key, t)).join('') || '<span class="muted">None.</span>'}</div>
      <div class="adder"><input type="text" list="topic-options" placeholder="Topic tag" data-add="${s.key}">
      <button class="btn" type="button" data-addbtn="${s.key}">Add</button></div></div>`;
  }
  if (s.type === 'bool') {
    return `<div class="rule">${head}${help}<label class="toggle"><input type="checkbox" data-bool="${s.key}" ${draft[s.key] ? 'checked' : ''}>
      On <span class="default-note">(default: ${def ? 'on' : 'off'})</span></label></div>`;
  }
  return `<div class="rule">${head}${help}<label class="toggle"><input type="number" data-num="${s.key}" value="${esc(draft[s.key])}"
    ${s.type === 'float' ? 'step="0.1"' : 'step="1"'} min="0"> <span class="default-note">default ${esc(def)}</span></label></div>`;
}

function chipText(id) {
  const l = lookups[id];
  if (!l) return '<span class="muted">looking up…</span>';
  if (!l.found) return '<span class="pill neg">no such Node</span>';
  return `${esc(l.excerpt)} <span class="muted">· ${l.deps} dependencies${l.topics.length ? ' · ' + esc(l.topics.join(', ')) : ''}</span>`;
}

function idChip(key, id) {
  return `<div class="chip"><code>${id}</code><span class="chip-text" data-lookup="${id}">${chipText(id)}</span>
    <button class="x" type="button" title="Remove" data-remove="${key}" data-value="${id}">×</button></div>`;
}

function tagChip(key, tag) {
  const t = topics.find(x => x.tag === tag);
  const known = !topics.length || t;
  return `<span class="chip" title="${esc(t ? t.title : (known ? '' : 'not a known topic'))}"><code>${esc(tag)}</code>
    ${known ? '' : '<span class="pill warn">unknown</span>'}
    <button class="x" type="button" title="Remove" data-remove="${key}" data-value="${esc(tag)}">×</button></span>`;
}

async function fetchLookup(id) {
  if (lookups[id]) return;
  try { lookups[id] = await api('lookup', { query: { id } }); }
  catch (err) { lookups[id] = { found: false }; }
  document.querySelectorAll(`[data-lookup="${id}"]`).forEach(el => { el.innerHTML = chipText(id); });
}

function rerenderRule(key) {
  const s = rules.schema.find(x => x.key === key);
  const old = $(`.rule[data-key="${key}"]`);
  const tmp = document.createElement('div');
  tmp.innerHTML = renderRule(s);
  old.replaceWith(tmp.firstElementChild);
  markDirty();
}

function addValue(key) {
  const s = rules.schema.find(x => x.key === key);
  const input = $(`[data-add="${key}"]`);
  let v = input.value.trim();
  if (!v) return;
  if (s.type === 'ids') {
    const m = v.match(ID_RE);
    if (!m) { alert('That does not contain a Node ID (YYYYmmddHHMMSS.n).'); return; }
    v = m[0];
  } else if (/[\s,()]/.test(v)) {
    alert('A topic tag has no spaces, commas or parentheses.'); return;
  }
  if (!draft[key].includes(v)) draft[key].push(v);
  rerenderRule(key);
  if (s.type === 'ids') fetchLookup(v);
  const again = $(`[data-add="${key}"]`);
  if (again) again.focus();
}

$('#rules-form').addEventListener('click', e => {
  const add = e.target.closest('[data-addbtn]');
  if (add) { addValue(add.dataset.addbtn); return; }
  const rm = e.target.closest('[data-remove]');
  if (rm) {
    draft[rm.dataset.remove] = draft[rm.dataset.remove].filter(x => x !== rm.dataset.value);
    rerenderRule(rm.dataset.remove);
  }
});
$('#rules-form').addEventListener('keydown', e => {
  if (e.key === 'Enter' && e.target.dataset.add) { e.preventDefault(); addValue(e.target.dataset.add); }
});
$('#rules-form').addEventListener('change', e => {
  const t = e.target;
  if (t.dataset.bool) draft[t.dataset.bool] = t.checked;
  if (t.dataset.num) {
    const s = rules.schema.find(x => x.key === t.dataset.num);
    draft[t.dataset.num] = s.type === 'float' ? parseFloat(t.value) : parseInt(t.value, 10);
  }
  markDirty();
});

function markDirty() {
  const dirty = !rules.exists || JSON.stringify(draft) !== JSON.stringify(rules.config);
  $('#dirty').hidden = !dirty;
  $('#dirty').textContent = rules.exists ? 'Unsaved changes' : 'Not saved yet';
}

$('#revert').addEventListener('click', () => {
  draft = JSON.parse(JSON.stringify(rules.config));
  $('#history').value = '';
  renderRules();
});

$('#history').addEventListener('change', async e => {
  if (!e.target.value) return;
  try {
    const old = await api('history', { query: { name: e.target.value } });
    draft = Object.assign(JSON.parse(JSON.stringify(rules.defaults)), old);
    const keep = e.target.value;
    renderRules();
    $('#history').value = keep;
    $('#save-result').innerHTML = '<div class="banner warn">An earlier version is loaded into the editor. Save to make it current.</div>';
  } catch (err) { alert(err.message); }
});

$('#save').addEventListener('click', async () => {
  $('#save').disabled = true;
  $('#save-result').innerHTML = '<div class="banner">Saving and rebuilding the scope…</div>';
  try {
    const r = await api('config', { method: 'POST', body: { config: draft } });
    if (!r.ok) {
      $('#save-result').innerHTML = `<div class="banner neg"><b>Not saved:</b><ul>${r.errors.map(x => `<li>${esc(x)}</li>`).join('')}</ul></div>`;
    } else {
      const scope = r.scope.output || r.scope.stderr;
      $('#save-result').innerHTML = `<div class="banner pos"><b>Saved.</b> The scope is rebuilt:</div>`;
      await loadRules();
      $('#scope-status').innerHTML = linkify(scope);
    }
  } catch (err) {
    $('#save-result').innerHTML = `<div class="banner neg">Error: ${esc(err.message)}</div>`;
  } finally {
    $('#save').disabled = false;
  }
});

/* ───────────────────────────── Proposals ───────────────────────────── */

let proposals = [];

async function loadProposals(selectPid = null) {
  try {
    proposals = (await api('proposals')).proposals;
  } catch (err) {
    $('#proposal-list').innerHTML = `<p class="muted">Error: ${esc(err.message)}</p>`;
    return;
  }
  updatePendingCount();
  $('#proposal-list').innerHTML = proposals.length ? proposals.map(p =>
    `<button class="cmd" type="button" data-pid="${esc(p.pid)}"><b>${esc(p.title)}</b>
     <span>${statusPill(p.status)} ${p.steps.length} steps · ${esc(fmtStamp(p.created))}</span></button>`).join('')
    : '<p class="muted">No proposals yet.</p>';
  document.querySelectorAll('[data-pid]').forEach(b => b.addEventListener('click', () => showProposal(b.dataset.pid)));
  const pick = selectPid || (proposals.find(p => p.status === 'pending') || proposals[0] || {}).pid;
  if (pick) showProposal(pick);
}

function updatePendingCount() {
  const n = proposals.filter(p => p.status === 'pending').length;
  $('#pending-count').hidden = !n;
  $('#pending-count').textContent = n;
}

function statusPill(s) {
  const cls = { pending: 'warn', applied: 'pos', failed: 'neg', declined: '' }[s] ?? '';
  return `<span class="pill ${cls}">${esc(s)}</span>`;
}

function fmtStamp(s) {
  return /^\d{12}$/.test(s || '') ? `${s.slice(0, 4)}-${s.slice(4, 6)}-${s.slice(6, 8)} ${s.slice(8, 10)}:${s.slice(10, 12)}` : (s || '');
}

function shellQuote(a) {
  return /^[\w@%+=:,./-]+$/.test(a) ? a : "'" + a.replace(/'/g, "'\\''") + "'";
}

function showProposal(pid) {
  const p = proposals.find(x => x.pid === pid);
  if (!p) return;
  document.querySelectorAll('[data-pid]').forEach(b => b.classList.toggle('is-active', b.dataset.pid === pid));
  const steps = p.steps.map((s, i) => `
    <div class="step">
      <div class="step-why">${i + 1}. ${esc(s.why)} ${s.name ? `<span class="pill">creates @${esc(s.name)}</span>` : ''}</div>
      <pre>${linkify(s.argv.map(shellQuote).join(' '))}</pre>
    </div>`).join('');
  const results = (p.results || []).map(r => `
    <div class="step"><div>Step ${r.step}: ${r.rc === 0 ? '<span class="pill pos">ok</span>' : `<span class="pill neg">exit ${r.rc}</span>`}
      ${r.created ? ` created ${linkify(r.created)}` : ''}</div>${r.output ? `<pre>${linkify(r.output)}</pre>` : ''}</div>`).join('');
  const pending = p.status === 'pending';
  $('#proposal-detail').innerHTML = `
    <div class="card-head"><h2>${esc(p.title)}</h2>${statusPill(p.status)}</div>
    <p class="muted">Proposed ${esc(fmtStamp(p.created))}${p.context_node ? ` for Node ${linkify(p.context_node)}` : ''}
      ${p.applied ? ` · applied ${esc(fmtStamp(p.applied))}` : ''} · <code>${esc(p.pid)}</code></p>
    ${steps}
    ${results ? `<h2 style="margin-top:12px">Results</h2>${results}` : ''}
    ${pending ? `<div class="apply-bar">
        <label><input type="checkbox" id="backup" checked> Back up first (fzbackup.py)</label>
        <button class="btn btn-primary" type="button" id="apply">Apply ${p.steps.length} steps</button>
        <button class="btn btn-ghost" type="button" id="decline">Decline</button>
      </div>` : ''}
    <pre id="job-output" class="output small" hidden></pre>`;
  if (!pending) return;
  $('#apply').addEventListener('click', () => applyProposal(p));
  $('#decline').addEventListener('click', async () => {
    if (!confirm('Mark this proposal as declined?')) return;
    await api('decline', { method: 'POST', body: { pid: p.pid } }).catch(err => alert(err.message));
    loadProposals(p.pid);
  });
}

async function applyProposal(p) {
  const backup = $('#backup').checked;
  if (!confirm(`Apply these ${p.steps.length} steps to your Formalizer${backup ? ' (after a backup)' : ' WITHOUT a backup'}?`)) return;
  $('#apply').disabled = true;
  $('#decline').disabled = true;
  const out = $('#job-output');
  out.hidden = false;
  out.textContent = 'Starting…';
  try {
    const { job } = await api('apply', { method: 'POST', body: { pid: p.pid, backup } });
    for (;;) {
      await new Promise(r => setTimeout(r, 800));
      const j = await api('job', { query: { id: job } });
      out.innerHTML = linkify(j.output || '…');
      out.scrollTop = out.scrollHeight;
      if (j.status !== 'running') break;
    }
  } catch (err) {
    out.textContent += '\nError: ' + err.message;
  }
  const keep = out.innerHTML;
  await loadProposals(p.pid);
  const again = $('#job-output');
  if (again) { again.hidden = false; again.innerHTML = keep; }
}

/* ───────────────────────────── Activity ───────────────────────────── */

function sinceStamp() {
  const days = $('#since').value;
  if (!days) return '';
  const d = new Date(Date.now() - (days - 1) * 86400000);
  return `${d.getFullYear()}${String(d.getMonth() + 1).padStart(2, '0')}${String(d.getDate()).padStart(2, '0')}`;
}

async function loadActivity() {
  const since = sinceStamp();
  const opts = since ? { since } : {};
  api('run', { method: 'POST', body: { cmd: 'disclosures', opts } })
    .then(r => { $('#disclosures').innerHTML = linkify(r.output || r.stderr); })
    .catch(err => { $('#disclosures').textContent = 'Error: ' + err.message; });
  api('run', { method: 'POST', body: { cmd: 'stats', opts } })
    .then(r => { $('#stats').textContent = r.output || r.stderr; })
    .catch(err => { $('#stats').textContent = 'Error: ' + err.message; });
  loadCalls();
  api('guard-log').then(r => renderGuard(r.records)).catch(() => {});
}
$('#since').addEventListener('change', loadActivity);
$('#who-all').addEventListener('change', loadCalls);
$('#raw-log').addEventListener('change', loadCalls);

function loadCalls() {
  const query = { limit: 500 };
  if ($('#who-all').checked) query.who = 'all';
  api('disclosure-log', { query }).then(r => renderCalls(r.records, sinceStamp())).catch(err => {
    $('#calls').innerHTML = `<tr><td class="muted">Error: ${esc(err.message)}</td></tr>`;
  });
}

function renderCalls(records, since) {
  const sinceIso = since ? `${since.slice(0, 4)}-${since.slice(4, 6)}-${since.slice(6, 8)}` : '';
  const recs = records.filter(r => !sinceIso || r.t >= sinceIso);
  const raw = $('#raw-log').checked;
  $('#calls').hidden = raw;
  $('#calls-raw').hidden = !raw;
  if (raw) {
    $('#calls-raw').innerHTML = recs.length ? linkify(recs.map(r => JSON.stringify(r)).join('\n')) : 'No entries in this period.';
    return;
  }
  const who = $('#who-all').checked;
  if (!recs.length) {
    $('#calls').innerHTML = `<tr><td class="muted">No calls${who ? '' : ' by Claude'} in this period.</td></tr>`;
    return;
  }
  $('#calls').innerHTML = `<tr><th>Time</th>${who ? '<th>By</th>' : ''}<th>Session</th><th>Command</th><th>Nodes</th><th>Log entries</th><th>~Tokens</th></tr>` +
    recs.map((r, i) => `<tr class="clickable" data-row="${i}"><td>${esc(r.t.replace('T', ' '))}</td>
      ${who ? `<td>${r.by === 'claude' ? '<span class="pill warn">Claude</span>' : '<span class="pill">you</span>'}</td>` : ''}
      <td><code title="${esc(r.session)}">${esc((r.session || '-').slice(0, 8))}</code></td>
      <td><code>fzclaude ${esc(r.argv.join(' '))}</code></td><td class="num">${r.nodes.length}</td>
      <td class="num">${r.log_entries.length}</td><td class="num">${r.est_tokens}</td></tr>`).join('');
  $('#calls').onclick = e => {
    const tr = e.target.closest('tr[data-row]');
    if (!tr || e.target.closest('a.nid')) return;
    const next = tr.nextElementSibling;
    if (next && next.classList.contains('detail')) { next.remove(); return; }
    const r = recs[+tr.dataset.row];
    const row = document.createElement('tr');
    row.className = 'detail';
    row.innerHTML = `<td colspan="7"><div><b>Nodes:</b> ${r.nodes.length ? linkify(r.nodes.join(' ')) : '-'}</div>
      <div><b>Log entries:</b> ${r.log_entries.length ? esc(r.log_entries.join(' ')) : '-'}</div></td>`;
    tr.after(row);
  };
}

function renderGuard(records) {
  if (!records.length) { $('#guard').innerHTML = '<tr><td class="muted">Nothing blocked.</td></tr>'; return; }
  $('#guard').innerHTML = '<tr><th>Time</th><th>Tool</th><th>Reason</th><th>Attempt</th></tr>' + records.map(r => {
    const what = r.input ? (r.input.command || r.input.file_path || r.input.url || JSON.stringify(r.input)) : '';
    return `<tr><td>${esc((r.t || '').replace('T', ' '))}</td><td>${esc(r.tool)}</td><td>${esc(r.reason)}</td>
      <td><code>${esc(String(what).slice(0, 300))}</code></td></tr>`;
  }).join('');
}

/* ───────────────────────────── start ───────────────────────────── */

renderCommandList();
selectCommand('today');
if (location.hash.length > 1) showView(location.hash.slice(1));
api('proposals').then(r => { proposals = r.proposals; updatePendingCount(); }).catch(() => {});
