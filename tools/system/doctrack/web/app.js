// Doc Track — local UI for the doctrack catalog.
// Views are hash routes: #/queue, #/catalog?..., #/work/<n>, #/aside, #/tags, #/sent.

'use strict';

const $ = (sel, root = document) => root.querySelector(sel);
const $$ = (sel, root = document) => [...root.querySelectorAll(sel)];
const main = $('#main');

const esc = s => String(s ?? '').replace(/[&<>"']/g,
  c => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c]));

// Search snippets arrive with \x01 ... \x02 around each hit.
const snippet = s => esc(s).replace(/\x01/g, '<mark>').replace(/\x02/g, '</mark>');

const STATE_LABEL = { new: 'waiting', approved: 'approved', ignored: 'ignored', summarized: 'summarized',
  aside: 'set aside' };

async function api(path, body) {
  const opts = body === undefined ? {} : {
    method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(body),
  };
  const res = await fetch(path, opts);
  const data = await res.json().catch(() => ({}));
  if (!res.ok) throw Object.assign(new Error(data.error || res.statusText), { status: res.status, data });
  return data;
}

let toastTimer;
function toast(msg, error = false) {
  const t = $('#toast');
  t.textContent = msg;
  t.className = 'toast' + (error ? ' error' : '');
  t.hidden = false;
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => { t.hidden = true; }, error ? 6000 : 2500);
}

function stateChip(w) {
  if (w.excluded) return `<span class="chip st-excluded" title="${esc(w.excluded)}">excluded</span>`;
  if (!w.head) return `<span class="chip st-version">older version</span>`;
  return `<span class="chip st-${w.state}">${STATE_LABEL[w.state]}</span>`;
}

function tagLinks(tags) {
  return tags.map(t => `<a class="tag" href="#/catalog?tag=${encodeURIComponent(t)}">${esc(t)}</a>`).join('');
}

function areaLinks(areas) {
  return areas.map(a => `<a class="tag area" href="#/catalog?area=${encodeURIComponent(a)}">${esc(a)}</a>`).join('');
}

function titleCell(w, extra = '') {
  return `<td class="title"><a href="#/work/${w.n}">${esc(w.title)}</a>`
    + `<span class="path">${esc(w.path)}</span>${extra}</td>`;
}

function versionsCell(w) {
  return `<td class="num hide-sm">${w.versions ? '+' + w.versions : ''}</td>`;
}

// The PDF for a LaTeX work, the Google Drive page for a Drive work.
function openLink(w, primary = false) {
  const cls = primary ? 'btn btn-primary' : 'btn btn-sm';
  if (w.webUrl) return `<a class="${cls}" href="${esc(w.webUrl)}" target="_blank" rel="noopener">${primary ? 'Open in Drive' : 'Drive'}</a>`;
  return `<a class="${cls}" href="/pdf/${w.n}" target="_blank" rel="noopener">${primary ? 'Open PDF' : 'PDF'}</a>`;
}

// ───────────────────────── header ─────────────────────────

async function refreshHeader() {
  try {
    const ov = await api('/api/overview');
    $('#count-queue').textContent = ov.states.new || '';
    $('#count-aside').textContent = ov.states.aside || '';
    $('#scan-stamp').textContent = ov.lastScan ? 'scanned ' + ov.lastScan.replace('T', ' ').slice(0, 16) : '';
    return ov;
  } catch (e) { return null; }
}

// ───────────────────────── queue ─────────────────────────

async function viewQueue() {
  const [ov, q] = await Promise.all([api('/api/overview'), api('/api/queue')]);
  const waiting = q.folders.reduce((n, f) => n + f.works.length, 0);
  let html = `
    <div class="view-head">
      <h2>Review queue</h2>
      <span class="lead">Approve what Claude may summarize. Ignored works stay searchable.</span>
      <span class="actions">
        ${waiting ? `<button class="btn btn-approve" id="approve-all">Approve all ${waiting}</button>` : ''}
      </span>
    </div>
    <div class="counts">
      <div class="count"><b class="st-new">${ov.states.new}</b><span>waiting</span></div>
      <div class="count"><b class="st-approved">${ov.states.approved}</b><span>approved</span></div>
      <div class="count"><b class="st-summarized">${ov.states.summarized}</b><span>summarized</span></div>
      <div class="count"><b class="st-ignored">${ov.states.ignored}</b><span>ignored</span></div>
      <div class="count"><b class="st-aside">${ov.states.aside}</b><span>set aside</span></div>
    </div>`;
  if (q.approvedWaiting) {
    html += `<div class="hint">${q.approvedWaiting} approved works wait for a summary. Summaries are written
      in a Claude Code session: <code>doctrack.py batch</code>, then <code>doctrack.py summarize</code>.</div>`;
  }
  if (!waiting) html += `<div class="panel"><div class="empty">Nothing waiting for review.</div></div>`;
  for (const f of q.folders) {
    html += `
      <section class="panel" data-folder="${esc(f.prefix)}">
        <div class="panel-head">
          <h3>${esc(f.label)}</h3><span class="muted small">${f.works.length}</span>
          <span class="actions">
            <button class="btn btn-sm btn-approve" data-folder-decide="approved"
              title="Approve every work in ${esc(f.label)}, and anything found there later">Approve folder</button>
            <button class="btn btn-sm" data-folder-decide="ignored"
              title="Ignore every work in ${esc(f.label)}, and anything found there later">Ignore folder</button>
            <button class="btn btn-sm" data-folder-decide="aside"
              title="Set aside every work in ${esc(f.label)}, and anything found there later">Set aside folder</button>
          </span>
        </div>
        <table class="works"><tbody>
          ${f.works.map(w => `
            <tr data-n="${w.n}">
              <td class="num nowrap muted">${w.n}</td>
              <td class="nowrap">${esc(w.date.slice(0, 7))}</td>
              <td class="nowrap hide-sm">${esc(w.kind)}</td>
              ${titleCell(w, w.koene ? '' : '<span class="snippet">"Koene" not found on the first pages</span>')}
              <td class="num nowrap hide-sm">${w.pages}p</td>
              ${versionsCell(w)}
              <td class="row-actions">
                ${openLink(w)}
                <button class="btn btn-sm btn-approve" data-decide="approved">Approve</button>
                <button class="btn btn-sm" data-decide="ignored">Ignore</button>
                <button class="btn btn-sm" data-decide="aside"
                  title="Not your writing, but worth attention later (e.g. CCF library)">Set aside</button>
              </td>
            </tr>`).join('')}
        </tbody></table>
      </section>`;
  }
  if (q.rules.length) {
    html += `
      <section class="panel">
        <div class="panel-head"><h3>Folder rules</h3>
          <span class="muted small">applied to works found later in these folders</span></div>
        <table class="works"><tbody>
          ${q.rules.map(r => `
            <tr><td class="nowrap"><span class="chip st-${esc(r.decision)}">${esc(r.decision)}</span></td>
              <td class="mono">${esc(r.label)}</td>
              <td class="muted small nowrap">${esc((r.at || '').slice(0, 10))}</td>
              <td class="row-actions"><button class="btn btn-sm" data-forget="${esc(r.prefix)}"
                title="Remove the rule and put its works back in the queue">Forget</button></td></tr>`).join('')}
        </tbody></table>
      </section>`;
  }
  main.innerHTML = html;

  $$('[data-decide]').forEach(b => b.onclick = async () => {
    const tr = b.closest('tr');
    await decide({ state: b.dataset.decide, works: [Number(tr.dataset.n)] });
  });
  $$('[data-folder-decide]').forEach(b => b.onclick = async () => {
    const folder = b.closest('section').dataset.folder;
    const state = b.dataset.folderDecide;
    const verb = { approved: 'Approve', ignored: 'Ignore', aside: 'Set aside' }[state];
    if (!confirm(`${verb} everything in ${folder}, including works found there later?`)) return;
    await decide({ state, folder });
  });
  $$('[data-forget]').forEach(b => b.onclick = async () => {
    if (!confirm('Remove this rule and put its works back in the queue?')) return;
    await decide({ state: 'new', folder: b.dataset.forget });
  });
  const all = $('#approve-all');
  if (all) all.onclick = async () => {
    if (!confirm(`Approve all ${waiting} waiting works for summarizing by Claude?`)) return;
    await decide({ state: 'approved', allNew: true });
  };
}

async function decide(body) {
  try {
    const r = await api('/api/decide', body);
    toast(`${r.count} ${r.count === 1 ? 'work' : 'works'} ${body.state === 'new' ? 'back in the queue' : STATE_LABEL[body.state]}`);
    await route();
  } catch (e) { toast(e.message, true); }
}

// ───────────────────────── catalog ─────────────────────────

function catalogParams() {
  const qs = location.hash.split('?')[1] || '';
  return new URLSearchParams(qs);
}

async function viewCatalog() {
  const p = catalogParams();
  const [ov, { tags }] = await Promise.all([api('/api/overview'), api('/api/tags')]);
  const opt = (vals, cur, all) => `<option value="">${all}</option>` + vals.map(v =>
    `<option value="${esc(v)}"${v === cur ? ' selected' : ''}>${esc(v)}</option>`).join('');
  main.innerHTML = `
    <div class="view-head">
      <h2>Catalog</h2>
      <span class="lead">${ov.families} works (${ov.works} files with versions), ${ov.excluded} excluded</span>
    </div>
    <form class="filters" id="filters">
      <input type="search" name="q" placeholder='Search the full text, e.g. substrate  or  "whole brain"' value="${esc(p.get('q') || '')}">
      <select name="state">${opt(['new', 'approved', 'ignored', 'summarized', 'aside'], p.get('state'), 'any state')}</select>
      <select name="kind">${opt(Object.keys(ov.kinds).sort(), p.get('kind'), 'any kind')}</select>
      <select name="decade">${opt(Object.keys(ov.decades).sort(), p.get('decade'), 'any decade')}</select>
      <select name="area">${opt(Object.keys(ov.areas), p.get('area'), 'any area')}</select>
      <select name="tag">${opt(tags.filter(t => !t.merged_into).map(t => t.name), p.get('tag'), 'any tag')}</select>
      <label><input type="checkbox" name="versions" value="1"${p.get('versions') ? ' checked' : ''}> versions</label>
      <label><input type="checkbox" name="excluded" value="1"${p.get('excluded') ? ' checked' : ''}> excluded</label>
    </form>
    <div id="results"><div class="empty">Loading…</div></div>`;

  const form = $('#filters');
  const apply = () => {
    const np = new URLSearchParams();
    for (const [k, v] of new FormData(form)) if (v) np.set(k, v);
    const h = '#/catalog' + (np.toString() ? '?' + np : '');
    if (h !== location.hash) history.replaceState(null, '', h);
    loadResults(np);
  };
  let timer;
  form.oninput = e => { clearTimeout(timer); timer = setTimeout(apply, e.target.type === 'search' ? 350 : 0); };
  form.onsubmit = e => { e.preventDefault(); apply(); };
  loadResults(p);
}

async function loadResults(p) {
  const box = $('#results');
  if (!box) return;
  let rows;
  try { rows = await api('/api/works?' + p); }
  catch (e) { box.innerHTML = `<div class="empty">${esc(e.message)}</div>`; return; }
  if (!rows.length) { box.innerHTML = '<div class="empty">No works match.</div>'; return; }
  box.innerHTML = `
    <div class="muted small" style="margin-bottom:6px">${rows.length} shown${p.get('q') ? ', best match first' : ''}</div>
    <div class="panel"><table class="works">
      <thead><tr><th class="num">#</th><th>Date</th><th class="hide-sm">Kind</th><th>Title</th>
        <th class="num hide-sm">Pages</th><th class="num hide-sm">Ver.</th><th>State</th></tr></thead>
      <tbody>${rows.map(w => `
        <tr>
          <td class="num nowrap muted">${w.n}</td>
          <td class="nowrap">${esc(w.date.slice(0, 7))}</td>
          <td class="nowrap hide-sm">${esc(w.kind)}</td>
          ${titleCell(w, (w.snippet ? `<span class="snippet">${snippet(w.snippet)}</span>` : '')
            + (w.areas.length || w.tags.length
              ? `<span class="snippet">${areaLinks(w.areas)}${tagLinks(w.tags)}</span>` : ''))}
          <td class="num nowrap hide-sm">${w.pages}</td>
          ${versionsCell(w)}
          <td class="nowrap">${stateChip(w)}</td>
        </tr>`).join('')}
      </tbody></table></div>`;
}

// ───────────────────────── work ─────────────────────────

async function viewWork(n) {
  let w;
  try { w = await api('/api/work/' + n); }
  catch (e) { main.innerHTML = `<div class="empty">${esc(e.message)}</div>`; return; }
  const canDecide = !w.excluded;
  const headLink = w.head ? '' : `<a href="#/work/${w.headN}">newest version (${w.headN})</a>`;
  const actions = [];
  if (w.hasPdf || w.webUrl) actions.push(openLink(w, true));
  if (canDecide) {
    const st = w.headState;
    if (st !== 'approved' && st !== 'summarized') actions.push(`<button class="btn btn-approve" data-decide="approved">Approve</button>`);
    if (st !== 'ignored') actions.push(`<button class="btn" data-decide="ignored">Ignore</button>`);
    if (st !== 'aside') actions.push(`<button class="btn" data-decide="aside" title="Not your writing, but worth attention later (e.g. CCF library)">Set aside</button>`);
    if (st !== 'new') actions.push(`<button class="btn" data-decide="new" title="Back to the review queue">Back to queue</button>`);
  }
  const s = w.summary;
  main.innerHTML = `
    <div class="work-title"><h2>${esc(w.title)}</h2></div>
    <div class="work-meta">
      <span class="muted">#${w.n}</span>
      <span>${esc(w.kind)}</span>
      <span>${esc(w.date)} <span class="muted small">(${esc(w.dateSource)})</span></span>
      <span>${w.pages} pages, ${w.words.toLocaleString()} words</span>
      ${stateChip(w.head ? { ...w, state: w.headState } : w)}
      ${headLink}
    </div>
    <div class="work-actions">${actions.join('')}</div>

    <section class="panel">
      <div class="panel-head"><h3>Summary</h3>
        ${s ? `<span class="muted small">${esc(s.dateEstimate)} · by ${esc(s.by)}, ${esc(s.at.slice(0, 10))}</span>` : ''}</div>
      <div class="panel-body">
        ${s ? `<ul class="key-points">${s.keyPoints.map(p => `<li>${esc(p)}</li>`).join('')}</ul>
               <div style="margin-top:10px">${areaLinks(w.areas)}${tagLinks(w.tags)}</div>`
            : `<span class="muted">${w.excluded ? 'Excluded works are not summarized.'
                : w.headState === 'approved' ? 'Approved; waiting for a summary in a Claude Code session (doctrack.py batch).'
                : w.headState === 'ignored' ? 'Ignored: searchable, but not summarized.'
                : w.headState === 'aside' ? 'Set aside for later attention: kept out of the catalog and never summarized.'
                : 'Not reviewed yet. Approve it to have it summarized.'}</span>`}
      </div>
    </section>

    ${w.family.length > 1 ? `
    <section class="panel">
      <div class="panel-head"><h3>Versions</h3><span class="muted small">near-identical text; the newest one stands for the family</span></div>
      <table class="works"><tbody>${w.family.map(v => `
        <tr${v.n === w.n ? ' style="font-weight:600"' : ''}>
          <td class="num nowrap muted">${v.n}</td>
          <td class="nowrap">${esc(v.date)}</td>
          ${titleCell(v)}
          <td class="num nowrap">${v.pages}p</td>
          <td class="row-actions">${openLink(v)}</td>
        </tr>`).join('')}</tbody></table>
    </section>` : ''}

    <section class="panel">
      <div class="panel-head"><h3>Details</h3></div>
      <div class="panel-body"><dl class="facts">
        <dt>id</dt><dd class="mono">${esc(w.id)}</dd>
        ${w.foundTitle !== w.title ? `<dt>title as found</dt><dd>${esc(w.foundTitle)}</dd>` : ''}
        ${w.excluded ? `<dt>excluded</dt><dd>${esc(w.excluded)}</dd>` : ''}
        ${w.meta.docclass ? `<dt>document class</dt><dd>${esc(w.meta.docclass)}</dd>` : ""}
        ${w.meta.owners ? `<dt>owners</dt><dd>${esc(w.meta.owners)}</dd>` : ""}
        ${w.meta.modified ? `<dt>last modified</dt><dd>${esc(w.meta.modified.replace("T", " ").slice(0, 16))}</dd>` : ""}
        ${w.meta.tex_author ? `<dt>author (tex)</dt><dd>${esc(w.meta.tex_author)}</dd>` : ''}
        ${w.meta.tex_date ? `<dt>\\date</dt><dd>${esc(w.meta.tex_date)}</dd>` : ''}
        ${w.meta.pdf_author ? `<dt>author (pdf)</dt><dd>${esc(w.meta.pdf_author)}</dd>` : ''}
        <dt>"Koene" on first pages</dt><dd>${w.koene ? 'yes' : 'no'}</dd>
        ${w.files.map(f => `<dt>${esc(f.role)} file</dt><dd class="mono">${esc(f.path)}</dd>`).join('')}
      </dl></div>
    </section>`;

  $$('[data-decide]').forEach(b => b.onclick = async () => {
    try {
      const st = b.dataset.decide;
      await api('/api/decide', { state: st, works: [w.headN] });
      toast(st === 'new' ? 'Back in the queue' : STATE_LABEL[st][0].toUpperCase() + STATE_LABEL[st].slice(1));
      refreshHeader();
      viewWork(n);
    } catch (e) { toast(e.message, true); }
  });
}

// ───────────────────────── set aside ─────────────────────────

async function viewAside() {
  const { folders } = await api('/api/aside');
  const count = folders.reduce((n, f) => n + f.works.length, 0);
  let html = `
    <div class="view-head">
      <h2>Set aside</h2>
      <span class="lead">${count} ${count === 1 ? 'work' : 'works'} kept apart from the catalog for later
        attention, such as CCF library items. Not summarized, not in catalog search.</span>
    </div>`;
  if (!count) html += `<div class="panel"><div class="empty">Nothing set aside. Use Set aside in the queue.</div></div>`;
  for (const f of folders) {
    html += `
      <section class="panel">
        <div class="panel-head"><h3>${esc(f.label)}</h3><span class="muted small">${f.works.length}</span></div>
        <table class="works"><tbody>
          ${f.works.map(w => `
            <tr data-n="${w.n}">
              <td class="num nowrap muted">${w.n}</td>
              <td class="nowrap">${esc(w.date.slice(0, 7))}</td>
              <td class="nowrap hide-sm">${esc(w.kind)}</td>
              ${titleCell(w)}
              <td class="num nowrap hide-sm">${w.pages}p</td>
              <td class="row-actions">
                ${openLink(w)}
                <button class="btn btn-sm" data-decide="new" title="Back to the review queue">Back to queue</button>
              </td>
            </tr>`).join('')}
        </tbody></table>
      </section>`;
  }
  main.innerHTML = html;
  $$('[data-decide]').forEach(b => b.onclick = async () => {
    await decide({ state: 'new', works: [Number(b.closest('tr').dataset.n)] });
  });
}

// ───────────────────────── tags ─────────────────────────

async function viewTags() {
  const { tags, areas } = await api('/api/tags');
  const live = tags.filter(t => !t.merged_into);
  const merged = tags.filter(t => t.merged_into);
  main.innerHTML = `
    <div class="view-head"><h2>Tags</h2>
      <span class="lead">Every summarized work gets one or two broad areas and a few specific tags.</span></div>
    <section class="panel">
      <div class="panel-head"><h3>Areas</h3><span class="muted small">fixed list, set in store.py</span></div>
      <div class="panel-body"><div class="tag-cloud">${Object.entries(areas).map(([a, n]) =>
        `<a class="tag area" href="#/catalog?area=${encodeURIComponent(a)}">${esc(a)}<span class="n">${n}</span></a>`).join('')}</div>
    </div></section>
    <section class="panel">
      <div class="panel-head"><h3>Specific tags</h3><span class="muted small">${live.length}; proposed by Claude, merge near-duplicates below</span></div>
      <div class="panel-body">
      ${live.length ? `<div class="tag-cloud">${live.map(t =>
        `<a class="tag" href="#/catalog?tag=${encodeURIComponent(t.name)}">${esc(t.name)}<span class="n">${t.n}</span></a>`).join('')}</div>`
        : '<div class="empty">No tags yet. They arrive with the first summaries.</div>'}
    </div></section>
    ${live.length > 1 ? `
    <section class="panel">
      <div class="panel-head"><h3>Merge</h3><span class="muted small">every work tagged with the first gets the second instead</span></div>
      <div class="panel-body"><form class="merge" id="merge">
        <select name="old">${live.map(t => `<option>${esc(t.name)}</option>`).join('')}</select>
        <span>into</span>
        <input name="new" list="tag-names" placeholder="tag" required>
        <datalist id="tag-names">${live.map(t => `<option value="${esc(t.name)}">`).join('')}</datalist>
        <button class="btn">Merge</button>
      </form></div>
    </section>` : ''}
    ${merged.length ? `
    <section class="panel">
      <div class="panel-head"><h3>Merged</h3></div>
      <div class="panel-body small">${merged.map(t => `${esc(t.name)} → ${esc(t.merged_into)}`).join('<br>')}</div>
    </section>` : ''}`;
  const form = $('#merge');
  if (form) form.onsubmit = async e => {
    e.preventDefault();
    const fd = new FormData(form);
    try {
      await api('/api/tag-merge', { old: fd.get('old'), new: fd.get('new') });
      toast(`${fd.get('old')} merged into ${fd.get('new')}`);
      viewTags();
    } catch (err) { toast(err.message, true); }
  };
}

// ───────────────────────── sent ─────────────────────────

async function viewSent() {
  const sends = await api('/api/sent');
  const total = sends.reduce((n, s) => n + s.chars, 0);
  main.innerHTML = `
    <div class="view-head"><h2>Sent to Claude</h2>
      <span class="lead">Every time <code>doctrack.py batch</code> handed out document text:
        ${sends.length} batches, ${total.toLocaleString()} characters.</span></div>
    ${sends.length ? sends.map(s => `
      <section class="panel">
        <div class="panel-head"><h3>${esc(s.at.replace('T', ' '))}</h3>
          <span class="muted small">${esc(s.purpose)} · ${s.chars.toLocaleString()} characters · ${s.works.length} works</span></div>
        <div class="panel-body small">${s.works.map(w => w.n
          ? `<a href="#/work/${w.n}">${esc(w.title)}</a>` : esc(w.title)).join('<br>')}</div>
      </section>`).join('')
      : '<div class="panel"><div class="empty">Nothing has been sent yet.</div></div>'}`;
}

// ───────────────────────── routing ─────────────────────────

async function route() {
  const h = location.hash || '#/queue';
  const [, view, arg] = h.split('?')[0].split('/');
  $$('.tab').forEach(t => t.classList.toggle('is-active', t.dataset.view === (view === 'work' ? '' : view)));
  refreshHeader();
  try {
    if (view === 'catalog') await viewCatalog();
    else if (view === 'work') await viewWork(arg);
    else if (view === 'aside') await viewAside();
    else if (view === 'tags') await viewTags();
    else if (view === 'sent') await viewSent();
    else await viewQueue();
  } catch (e) {
    main.innerHTML = `<div class="empty">Could not load: ${esc(e.message)}</div>`;
  }
}

let lastHash = '';
window.addEventListener('hashchange', () => {
  // Typing in the catalog search rewrites the hash with replaceState, which
  // fires no hashchange; real navigation does.
  if (location.hash !== lastHash) { lastHash = location.hash; route(); }
});

// ───────────────────────── scan, theme, stop ─────────────────────────

async function pollJob() {
  const box = $('#job');
  const st = await api('/api/job').catch(() => null);
  if (!st) return;
  if (st.running || (st.ended && Date.now() / 1000 - st.ended < 8)) {
    box.hidden = false;
    box.textContent = (st.running ? 'Scanning…\n' : 'Scan finished.\n') + st.tail.join('\n');
  } else {
    box.hidden = true;
  }
  $('#scan').disabled = st.running;
  if (st.running) setTimeout(pollJob, 1000);
  else if (st.ended && Date.now() / 1000 - st.ended < 2) route();
  if (!st.running && st.ended && Date.now() / 1000 - st.ended < 8) setTimeout(pollJob, 2000);
}

$('#scan').onclick = async () => {
  try { await api('/api/scan', {}); pollJob(); }
  catch (e) { toast(e.message, true); }
};

const THEME_KEY = 'doctrack-theme';
function applyTheme(t) {
  if (t) document.documentElement.dataset.theme = t;
  else delete document.documentElement.dataset.theme;
}
try { applyTheme(localStorage.getItem(THEME_KEY)); } catch (e) { /* storage blocked */ }
$('#theme').onclick = () => {
  const dark = document.documentElement.dataset.theme
    ? document.documentElement.dataset.theme === 'dark'
    : matchMedia('(prefers-color-scheme: dark)').matches;
  const next = dark ? 'light' : 'dark';
  applyTheme(next);
  try { localStorage.setItem(THEME_KEY, next); } catch (e) { /* storage blocked */ }
};

$('#stop-server').onclick = async () => {
  if (!confirm('Stop the doctrack server? The dashboard button starts it again.')) return;
  try {
    let res = await fetch('/api/shutdown', { method: 'POST' });
    if (res.status === 409) {
      if (!confirm('A scan is still running. Stop the server anyway?')) return;
      res = await fetch('/api/shutdown', { method: 'POST', headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ force: true }) });
    }
  } catch (e) { /* the server is going away */ }
  document.body.innerHTML = `<div class="stopped-page"><h2>Doc Track stopped</h2>
    <p>Nothing is lost. Start it again from the Formalizer dashboard, or with <code>./serve.py</code>.</p></div>`;
};

lastHash = location.hash;
route();
pollJob();
