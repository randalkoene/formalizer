/* Inbox Triage — local diagnostic UI.
 *
 * Reads the same cache the CLI reads, served from 127.0.0.1. Nothing here can
 * reach Gmail: the server process never constructs a Gmail client.
 *
 * Charts are hand-built inline SVG (no chart library, keeping the tool
 * dependency-free). Single-series charts use one uniform hue; only the category
 * chart uses the categorical palette, because there identity is the job.
 */

const CAT_ORDER = ['needs_reply', 'grants', 'directed', 'threads', 'onboarding',
  'bulk', 'automated', 'spam'];

const state = {
  messages: [],
  overview: null,
  byN: new Map(),
  // Rail selection is exclusive: a needs-filter, a category, or a tag.
  select: { kind: 'all', key: null },
  activeN: null,
  query: '',
  sort: 'score',
  limit: 120,
  olderLimit: 60,
  showOlder: false,
  filtered: [],
  selected: new Set(),
  mode: { armed: false, writeTokenPresent: false, remaining: 0 },
};

const $ = (sel, root = document) => root.querySelector(sel);
const $$ = (sel, root = document) => [...root.querySelectorAll(sel)];

const esc = (s) => String(s ?? '').replace(/[&<>"']/g, (c) => (
  { '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c]
));

const catVar = (cat) => `var(--cat-${cat})`;

function fmtAge(days) {
  if (days < 1) return 'today';
  if (days < 30) return `${Math.round(days)}d`;
  if (days < 365) return `${Math.round(days / 30)}mo`;
  return `${(days / 365).toFixed(1)}y`;
}

function fmtDate(ms) {
  return new Date(ms).toLocaleString(undefined, {
    year: 'numeric', month: 'short', day: 'numeric', hour: '2-digit', minute: '2-digit',
  });
}

function toast(msg, ms = 2400) {
  const el = $('#toast');
  el.classList.remove('is-busy');
  el.textContent = msg;
  el.classList.add('show');
  clearTimeout(toast._t);
  toast._t = setTimeout(() => el.classList.remove('show'), ms);
}

/* Errors need long enough to actually read. */
function errorToast(msg) { toast(msg, 7000); }

/* A sticky toast for work that takes a visible moment -- a Gmail round trip, or
   re-scoring a couple of thousand messages. Returns its own dismiss. */
function busy(msg) {
  const el = $('#toast');
  clearTimeout(toast._t);
  el.innerHTML = `<span class="spinner" aria-hidden="true"></span>${esc(msg)}`;
  el.classList.add('show', 'is-busy');
  let cleared = false;
  return () => {
    if (cleared) return;
    cleared = true;
    // Only dismiss if nothing has spoken since. toast() clears is-busy, so a
    // message shown while the work was running keeps its own full lifetime --
    // previously this cut every error down to 400ms, too fast to read.
    if (!el.classList.contains('is-busy')) return;
    el.classList.remove('is-busy');
    toast._t = setTimeout(() => el.classList.remove('show'), 400);
  };
}

/* ───────────────────────────── data ───────────────────────────── */

async function load() {
  const [overview, messages] = await Promise.all([
    fetch('/api/overview').then((r) => r.json()),
    fetch('/api/messages').then((r) => r.json()),
  ]);
  state.overview = overview;
  state.messages = messages;
  if (!overview.total) {
    $('#list').innerHTML = `<div class="empty-state">
        <div class="empty-mark">\u2601</div>
        <p>No mail cached yet.</p>
        <p class="empty-hint">Open <strong>Data</strong> and run <strong>Fetch new mail</strong>.</p>
      </div>`;
  }
  state.byN = new Map(messages.map((m) => [m.n, m]));
  renderRail();
  renderList();
  if ($('#view-dashboard').classList.contains('is-active')) renderDashboard();
  if (state.activeN == null && state.filtered.length) select(state.filtered[0].n);
  const when = overview.cacheTime ? new Date(overview.cacheTime * 1000) : null;
  $('#cache-stamp').textContent = when
    ? `cache ${when.toLocaleString(undefined, { month: 'short', day: 'numeric', hour: '2-digit', minute: '2-digit' })}`
    : '';
}

/* ───────────────────────────── rail ───────────────────────────── */

/* The one way to change what the list is showing. The rail, the dashboard
   charts and the number keys all go through here, so they cannot drift apart. */
function selectRail(kind, key) {
  state.select = { kind, key: key || null };
  state.limit = 120;
  state.showOlder = false;
  const short = { category: 'cat', needs: 'needs', tag: 'tag' }[kind];
  history.replaceState(null, '', short && state.select.key ? `#${short}=${state.select.key}` : '#triage');
  renderRail();
  renderList();
}

function renderRail() {
  const ov = state.overview;
  const need = Object.fromEntries(ov.needs.map((n) => [n.key, n.count]));

  $('#hero-reply').textContent = (need.reply || 0).toLocaleString();
  $('#hero-read').textContent = (need.read || 0).toLocaleString();
  $('#hero-sub').textContent =
    `${ov.total.toLocaleString()} unread · ${(need.none || 0).toLocaleString()} need nothing from you`
    + (ov.unscored ? ` · ${ov.unscored.toLocaleString()} unscored — press Re-score` : '');

  $('#composition').innerHTML = ov.categories
    .filter((c) => c.count > 0)
    .map((c) => `<span style="flex:${c.count};background:${catVar(c.key)}" title="${esc(c.label)}: ${c.count}"></span>`)
    .join('');

  const sel = state.select;
  const on = (kind, key) => (sel.kind === kind && sel.key === key ? 'is-active' : '');

  const row = (kind, key, chip, label, count, title) => `
    <button class="catrow ${on(kind, key)}" data-kind="${kind}" data-key="${key ?? ''}"
            style="--chip:${chip}" title="${esc(title || label)}">
      <span class="catrow-chip"></span>
      <span class="catrow-label">${esc(label)}</span>
      <span class="catrow-meta">${count.toLocaleString()}</span>
    </button>`;

  const parts = [];

  parts.push('<div class="rail-group">Needs you</div>');
  for (const n of ov.needs) {
    if (n.key === 'none') continue;
    parts.push(row('needs', n.key,
      n.key === 'reply' ? 'var(--cat-needs_reply)' : 'var(--cat-directed)',
      n.label, n.count, n.blurb));
  }

  parts.push('<div class="rail-group">By kind</div>');
  parts.push(row('all', null, 'var(--text-muted)', 'All messages', ov.total));
  for (const c of ov.categories) {
    parts.push(`
      <button class="catrow ${on('category', c.key)}" data-kind="category" data-key="${c.key}"
              style="--chip:${catVar(c.key)}" title="${esc(c.blurb)}">
        <span class="catrow-chip"></span>
        <span class="catrow-label">${esc(c.label)}</span>
        <span class="catrow-meta">${c.count.toLocaleString()}</span>
        <span class="catrow-bar"><i style="width:${c.share.toFixed(1)}%"></i></span>
      </button>`);
  }

  if (ov.tags.length) {
    parts.push(`<div class="rail-group">Worth a look inside the noise</div>`);
    for (const t of ov.tags) {
      parts.push(row('tag', t.key, 'var(--cat-threads)', t.label, t.count));
    }
  }

  // Muted mail is still here. Showing it keeps "muted" honest.
  if ((ov.mutedTags || []).length) {
    parts.push('<div class="rail-group">Muted — still filed in Bulk</div>');
    for (const t of ov.mutedTags) {
      parts.push(row('tag', t.key, 'var(--cat-automated)', t.label, t.count,
        'Muted lists are sorted to the bottom of Bulk. Nothing is hidden or removed.'));
    }
  }

  $('#rail-list').innerHTML = parts.join('');

  $$('#rail-list .catrow').forEach((btn) => {
    btn.onclick = () => selectRail(btn.dataset.kind, btn.dataset.key);
  });

  $$('.hero-jump').forEach((el) => {
    el.onclick = () => selectRail('needs', el.dataset.needs);
  });
}

/* ───────────────────────────── list ───────────────────────────── */

function applyFilters() {
  const q = state.query.trim().toLowerCase();
  let rows = state.messages;
  const sel = state.select;
  if (sel.kind === 'category') rows = rows.filter((m) => m.category === sel.key);
  else if (sel.kind === 'needs') rows = rows.filter((m) => m.needs === sel.key);
  else if (sel.kind === 'tag') rows = rows.filter((m) => m.tags.includes(sel.key));
  if (q) {
    const terms = q.split(/\s+/);
    rows = rows.filter((m) => terms.every((t) => m.q.includes(t)));
  }
  const sorters = {
    'score': (a, b) => b.score - a.score,
    'date-desc': (a, b) => b.date - a.date,
    'date-asc': (a, b) => a.date - b.date,
    'sender': (a, b) => a.fromName.localeCompare(b.fromName) || b.score - a.score,
  };
  rows = [...rows].sort(sorters[state.sort]);
  state.filtered = rows;
  return rows;
}

const NEEDS_PILL = {
  reply: ['needs-reply', 'reply'],
  read: ['needs-read', 'read'],
};

function msgRow(m, selectable) {
  const why = m.topReasons.map((r) => r.detail || r.rule).join(' · ');
  const pills = [];
  const np = NEEDS_PILL[m.needs];
  if (np) pills.push(`<span class="pill ${np[0]}">${np[1]}</span>`);
  for (const t of m.tags) {
    if (t === 'important_list') pills.push('<span class="pill pill-list">list you follow</span>');
  }
  if (m.threadCount > 1) pills.push(`<span class="pill">thread ${m.threadCount}</span>`);
  if (m.youSent > 0) pills.push('<span class="pill">you replied</span>');
  if (m.attachments) pills.push(`<span class="pill">${m.attachments} att</span>`);
  const box = selectable
    ? `<input type="checkbox" class="msg-pick" data-pick="${m.n}" ${
         state.selected.has(m.n) ? 'checked' : ''}>`
    : '';
  return `
    <div class="msgrow ${m.n === state.activeN ? 'is-active' : ''} ${selectable ? 'is-pickable' : ''}"
         data-n="${m.n}" style="--chip:${catVar(m.category)}">
      ${box}
      <div class="msg-score ${m.score < 0 ? 'is-neg' : ''}">${Math.round(m.score)}</div>
      <div class="msg-main">
        <div class="msg-from"><b>${esc(m.fromName)}</b>${pills.join('')}</div>
        <div class="msg-sub">${esc(m.subject)}</div>
        <div class="msg-why">${esc(why)}</div>
      </div>
      <div class="msg-age">${fmtAge(m.ageDays)}</div>
    </div>`;
}

function renderList() {
  const rows = applyFilters();
  const stale = state.overview ? state.overview.staleDays : 120;

  // Old mail is usually overtaken by events, but not always — so it is kept,
  // just folded away behind its own header instead of interleaved.
  const recent = rows.filter((m) => m.ageDays <= stale);
  const older = rows.filter((m) => m.ageDays > stale);

  $('#list-count').textContent = rows.length === state.messages.length
    ? `${rows.length.toLocaleString()} messages`
    : `${rows.length.toLocaleString()} of ${state.messages.length.toLocaleString()}`;

  if (!rows.length) {
    $('#list').innerHTML = '<div class="empty-state"><p>Nothing matches.</p></div>';
    $('#list-more').classList.add('hidden');
    return;
  }

  const selectable = canMultiSelect();
  $('#list').classList.toggle('has-picks', selectable);
  const shown = recent.slice(0, state.limit);
  const parts = [shown.map((m) => msgRow(m, selectable)).join('')];

  if (!recent.length) {
    parts.unshift(`<div class="age-note">Nothing newer than ${stale} days here.</div>`);
  }

  if (older.length) {
    parts.push(`
      <button class="age-split ${state.showOlder ? 'is-open' : ''}" id="toggle-older">
        <span class="age-split-caret">${state.showOlder ? '▾' : '▸'}</span>
        Older than ${stale} days
        <span class="age-split-count">${older.length.toLocaleString()}</span>
        <span class="age-split-hint">${state.showOlder ? 'hide' : 'probably stale — glance if you like'}</span>
      </button>`);
    if (state.showOlder) {
      parts.push(`<div class="older-rows">${
        older.slice(0, state.olderLimit).map((m) => msgRow(m, selectable)).join('')}</div>`);
      if (older.length > state.olderLimit) {
        parts.push(`<div class="list-more"><button class="btn" id="more-older">Show more old messages</button></div>`);
      }
    }
  }

  $('#list').innerHTML = parts.join('');

  $$('#list .msgrow').forEach((el) => {
    el.onclick = (e) => {
      if (e.target.classList.contains('msg-pick')) return;
      select(Number(el.dataset.n));
    };
  });
  $$('#list .msg-pick').forEach((box) => {
    box.onclick = (e) => {
      e.stopPropagation();
      const n = Number(box.dataset.pick);
      if (box.checked) state.selected.add(n); else state.selected.delete(n);
      renderSelectBar();
    };
  });
  renderSelectBar();
  const toggle = $('#toggle-older');
  if (toggle) toggle.onclick = () => { state.showOlder = !state.showOlder; renderList(); };
  const moreOlder = $('#more-older');
  if (moreOlder) moreOlder.onclick = () => { state.olderLimit += 120; renderList(); };

  // The footer button pages the recent list only.
  $('#list-more').classList.toggle('hidden', recent.length <= state.limit);
}

/* ───────────────────────────── detail ───────────────────────────── */

async function select(n) {
  state.activeN = n;
  $$('#list .msgrow').forEach((el) => {
    el.classList.toggle('is-active', Number(el.dataset.n) === n);
  });
  const d = await fetch(`/api/message/${n}`).then((r) => r.json());
  renderDetail(d);
  const row = $(`#list .msgrow[data-n="${n}"]`);
  if (row) row.scrollIntoView({ block: 'nearest' });
}

function renderDetail(d) {
  const cat = state.overview.categories.find((c) => c.key === d.category);
  const maxAbs = Math.max(...d.reasons.map((r) => Math.abs(r.points)), 1);

  const waterfall = d.reasons.map((r) => {
    const pct = (Math.abs(r.points) / maxAbs) * 50;
    const neg = r.points < 0;
    const style = neg
      ? `right:50%;width:${pct}%`
      : `left:50%;width:${pct}%`;
    return `
      <div class="wf-row">
        <span class="wf-pts ${neg ? 'neg' : 'pos'}">${r.points > 0 ? '+' : ''}${r.points}</span>
        <span class="wf-rule">${esc(r.rule)}</span>
        <span class="wf-track"><span class="axis" style="left:50%"></span><i class="${neg ? 'neg' : ''}" style="${style}"></i></span>
      </div>
      ${r.detail ? `<div class="wf-row"><span></span><span class="wf-detail">${esc(r.detail)}</span></div>` : ''}`;
  }).join('');

  const flagNames = Object.entries(d.flags)
    .filter(([k]) => k !== 'age_days')
    .map(([k, v]) => `<span class="flag ${v ? 'on' : ''}">${esc(k)}${v === true ? '' : v === false ? '' : `: ${esc(v)}`}</span>`)
    .join('');

  const armed = state.mode.armed;
  const viewBlocks = state.select.kind === 'all';
  const actionButtons = (d.actions || []).map((a) => `
    <button class="act-btn ${a.destructive ? 'is-destructive' : ''} ${armed && !viewBlocks ? '' : 'is-locked'}"
            data-msgact="${a.key}"
            title="${esc(a.blurb)}${armed ? '' : ' — click to arm write mode and do it'}">
      ${armed ? '' : '<span class="lock">\u25cf</span> '}${esc(a.label)}</button>`).join('');

  // ---- Teach: local, live, and deliberately distinct from the inert Gmail actions.
  const KIND_LABELS = {
    personal: 'Personal — never bulk',
    important: 'Always important',
    bulk: 'This really is bulk',
    ignore: 'Ignore this sender',
  };
  const taughtBanner = d.taught
    ? `<div class="taught-now">Currently taught: <b>${esc(d.taught.kind || d.taught.category)}</b>
         for ${esc(d.taught.scope)}${d.taught.note ? ` — ${esc(d.taught.note)}` : ''}
         <button class="link-btn" data-teach-clear="1">clear</button>
       </div>`
    : '';

  const kindButtons = Object.entries(KIND_LABELS).map(([kind, label]) => `
    <button class="teach-btn teach-${kind} ${d.taught && d.taught.kind === kind ? 'is-on' : ''}"
            data-kind="${kind}">${esc(label)}</button>`).join('');

  // Mailing lists and group addresses: the thing worth teaching is the list,
  // not whoever happened to post to it.
  const listOffer = d.listKeys.length ? `
    <div class="teach-list">
      <div class="teach-list-head">Arrived via
        <code>${esc(d.taughtList ? d.taughtList.key : d.listKeys[0])}</code>
        ${d.taughtList ? `<span class="pill pill-list">${esc(d.taughtList.kind)}</span>` : ''}
      </div>
      <div class="teach-row">
        <button class="teach-btn teach-personal ${d.taughtList && d.taughtList.kind === 'important' ? 'is-on' : ''}"
                data-list="important" data-listkey="${esc(d.listKeys[0])}">I read this list</button>
        <button class="teach-btn teach-ignore ${d.taughtList && d.taughtList.kind === 'ignore' ? 'is-on' : ''}"
                data-list="ignore" data-listkey="${esc(d.listKeys[0])}"
                title="Sorts it to the bottom of Bulk. It stays listed and counted — nothing is hidden.">Mute this list</button>
        ${d.taughtList ? `<button class="teach-btn" data-list="" data-listkey="${esc(d.taughtList.key)}">Forget</button>` : ''}
      </div>
    </div>` : '';

  const aliasOffer = d.notMeRecipients.length
    ? `<div class="teach-alias">
         Addressed to <code>${esc(d.notMeRecipients[0])}</code>, which you have not declared as yours.
         <button class="teach-btn teach-alias-btn" data-alias="${esc(d.notMeRecipients[0])}">That address is me</button>
       </div>`
    : '';

  const teachBlock = `
    <div class="dt-section dt-teach">
      <h3>Teach the classifier</h3>
      ${taughtBanner}
      <label class="teach-scope">
        Apply to
        <select id="teach-scope">
          <option value="sender">this sender — ${esc(d.fromAddr)}</option>
          <option value="domain">everyone at ${esc(d.fromDomain || '—')}</option>
          <option value="message">just this one message</option>
        </select>
      </label>
      <div class="teach-row">${kindButtons}</div>
      <div class="teach-row teach-secondary">
        <input id="teach-note" type="text" placeholder="note (why) — optional"
               value="${esc((d.taught && d.taught.note) || '')}" autocomplete="off">
        <label class="teach-pin">
          File this one message under
          <select id="teach-pin">
            <option value="">— not pinned —</option>
            ${CAT_ORDER.map((c) => `<option value="${c}" ${
              d.taught && d.taught.category === c ? 'selected' : ''}>${c}</option>`).join('')}
          </select>
        </label>
      </div>
      ${listOffer}
      ${aliasOffer}
      <div class="actions-note">
        Saved to <code>corrections.yaml</code> and re-scored immediately. This changes
        how the local cache is scored — it never touches your mailbox.
      </div>
    </div>`;

  const att = d.attachments.length
    ? `<div class="dt-section"><h3>Attachments</h3><div class="kv">${
        d.attachments.map((a) => `<div><b>${esc(a.mimeType)}</b> ${esc(a.filename)}</div>`).join('')
      }</div></div>`
    : '';

  const interesting = ['list-id', 'list-unsubscribe', 'precedence', 'auto-submitted',
                       'authentication-results', 'in-reply-to', 'delivered-to', 'return-path'];
  const rawHeaders = interesting
    .filter((h) => d.headers[h])
    .map((h) => `<div><b>${h}:</b> ${esc(d.headers[h])}</div>`).join('');

  const html = `
    <div class="dt-head" style="--chip:${catVar(d.category)}">
      <span class="dt-cat">${esc(cat ? cat.label : d.category)}</span>
      ${d.needs !== 'none'
        ? `<span class="pill ${d.needs === 'reply' ? 'needs-reply' : 'needs-read'}">${
            d.needs === 'reply' ? 'needs a reply' : 'worth reading'}</span>`
        : ''}
      ${d.tags.filter((t) => t !== 'important_list').map((t) =>
          `<span class="pill">${esc(t)}</span>`).join('')}
      <div class="dt-subject">${esc(d.subject)}</div>
      <dl class="dt-meta">
        <dt>From</dt><dd>${esc(d.fromName || '')} &lt;${esc(d.fromAddr)}&gt;</dd>
        <dt>To</dt><dd>${esc(d.to.join(', ') || '—')}</dd>
        ${d.cc.length ? `<dt>Cc</dt><dd>${esc(d.cc.join(', '))}</dd>` : ''}
        <dt>Date</dt><dd>${fmtDate(d.date)} · ${fmtAge(d.flags.age_days)} ago</dd>
        <dt>Thread</dt><dd>${d.thread.message_count || 1} message(s)${
          d.thread.i_sent_count ? `, you sent ${d.thread.i_sent_count}` : ''}</dd>
        <dt>History</dt><dd>${
          d.sender.i_wrote_to_count != null
            ? `you have written to them ${d.sender.i_wrote_to_count}× · they have sent you ${d.sender.from_count}×`
            : 'no record'}</dd>
      </dl>
    </div>

    <div class="dt-section">
      <h3>Why this score</h3>
      <div class="wf">${waterfall}</div>
      <div class="wf-total"><span>Total</span><b>${Math.round(d.score)}</b></div>
    </div>

    ${teachBlock}

    <div class="dt-section">
      <h3>Actions</h3>
      <div class="actions">
        <a class="act-btn act-primary" id="reply-link" href="${esc(d.gmailUrl)}"
           target="_blank" rel="noopener"
           title="Opens Gmail in a new tab, then asks whether to drop this message from triage.">
          Reply in Gmail ↗</a>
        ${actionButtons}
        ${d.unsubscribeUrl ? `<a class="act-btn" href="${esc(d.unsubscribeUrl)}" target="_blank"
            rel="noopener" title="Opens the sender's unsubscribe link. Nothing in Gmail changes.">Unsubscribe ↗</a>` : ''}
      </div>
      <div class="actions-note">
        ${viewBlocks
          ? 'You are in <strong>All messages</strong>, where actions are deliberately unavailable. Pick a category, a needs filter or a tag in the left rail.'
          : armed
          ? 'Write mode is on — these change your Gmail. All of them are reversible there; trashed mail stays in Trash for about 30 days.'
          : 'Read-only right now. Pressing one of these offers to arm write mode and then carry it out.'}
        <br>Replying in Gmail marks it read there, but this app cannot see that happen —
        press <strong>Sync</strong> in the top bar afterwards and it will drop out of triage.
      </div>
    </div>

    <div class="dt-section">
      <h3>Signals</h3>
      <div class="flags">${flagNames}</div>
    </div>

    ${att}

    <div class="dt-section">
      <h3>Body</h3>
      <div class="dt-body">${esc(d.body) || '<em>(no body captured)</em>'}</div>
    </div>

    ${rawHeaders ? `<div class="dt-section"><details class="raw"><summary>Raw headers</summary><div class="kv">${rawHeaders}</div></details></div>` : ''}

  `;

  const pane = $('#detail');
  const same = renderDetail.lastN === d.n;
  const keepPane = same ? pane.scrollTop : 0;
  const prevBody = $('.dt-body', pane);
  const keepBody = same && prevBody ? prevBody.scrollTop : 0;

  pane.innerHTML = html;
  renderDetail.lastN = d.n;

  // Restoring after innerHTML, so a background repaint does not yank the
  // reader back to the top of a long message.
  if (keepPane) pane.scrollTop = keepPane;
  if (keepBody) {
    const nextBody = $('.dt-body', pane);
    if (nextBody) nextBody.scrollTop = keepBody;
  }

  const scopeTarget = () => {
    const scope = $('#teach-scope').value;
    if (scope === 'domain') return { scope, target: d.fromDomain };
    if (scope === 'message') return { scope, target: d.id };
    return { scope, target: d.fromAddr };
  };

  $$('.teach-btn[data-kind]', pane).forEach((btn) => {
    btn.onclick = () => teach({
      ...scopeTarget(),
      kind: btn.dataset.kind,
      note: $('#teach-note', pane).value.trim() || undefined,
    });
  });

  // Pinning always applies to this one message, whatever the scope selector says.
  $('#teach-pin', pane).onchange = (e) => {
    const category = e.target.value;
    teach(category
      ? { scope: 'message', target: d.id, category }
      : { scope: 'message', target: d.id, clear: true });
  };
  const clearBtn = $('[data-teach-clear]', pane);
  if (clearBtn) clearBtn.onclick = () => teach({ ...scopeTarget(), clear: true });
  const aliasBtn = $('[data-alias]', pane);
  if (aliasBtn) aliasBtn.onclick = () => setAlias(aliasBtn.dataset.alias);

  const replyLink = $('#reply-link', pane);
  if (replyLink) {
    replyLink.addEventListener('click', () => {
      // The link opens Gmail itself; the modal just waits for the verdict.
      setTimeout(() => confirmHandled(d.n, d.subject), 350);
    });
  }

  $$('[data-msgact]', pane).forEach((btn) => {
    const spec = (d.actions || []).find((a) => a.key === btn.dataset.msgact);
    btn.onclick = () => runAction(btn.dataset.msgact, [d.n], { spec });
  });

  $$('[data-list]', pane).forEach((btn) => {
    btn.onclick = () => teach(btn.dataset.list
      ? { scope: 'list', target: btn.dataset.listkey, kind: btn.dataset.list }
      : { scope: 'list', target: btn.dataset.listkey, clear: true });
  });
}

async function postJSON(url, body) {
  const res = await fetch(url, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(body),
  });
  const data = await res.json();
  if (!res.ok || data.error) throw new Error(data.error || res.statusText);
  return data;
}

async function teach(payload) {
  if (teach.inFlight) return;
  teach.inFlight = true;
  const done = busy(payload.clear ? 'Clearing and re-scoring…' : 'Teaching and re-scoring…');
  try {
    await postJSON('/api/teach', payload);
    const n = state.activeN;
    await load();
    if (n != null) await select(n);
    done();
    toast(payload.clear ? 'Correction cleared — re-scored'
      : payload.category ? `Pinned to ${payload.category} — re-scored`
      : `Taught: ${payload.kind} — re-scored`);
  } catch (err) {
    errorToast(`Could not save: ${err.message}`);
  } finally {
    done();
    teach.inFlight = false;
  }
}

async function setAlias(address) {
  const done = busy('Adding address and re-scoring…');
  try {
    await postJSON('/api/alias', { address });
    const n = state.activeN;
    await load();
    if (n != null) await select(n);
    done();
    toast(`${address} now counts as you — re-scored`);
  } catch (err) {
    errorToast(`Could not save: ${err.message}`);
  } finally {
    done();
  }
}

/* ───────────────────────────── charts ───────────────────────────── */

const tip = (() => {
  const el = document.createElement('div');
  el.className = 'viz-tip';
  document.body.appendChild(el);
  return {
    show(html, x, y) {
      el.innerHTML = html;
      el.style.left = `${x + 12}px`;
      el.style.top = `${y + 12}px`;
      el.classList.add('show');
    },
    hide() { el.classList.remove('show'); },
  };
})();

/* Charts are drawn in CSS-pixel coordinates matched to the container's measured
   width, so the viewBox scale is exactly 1 and text renders at its intended size
   in a 340px card and a 1600px one alike. (A fixed 1000-unit viewBox with
   preserveAspectRatio="meet" scales the whole drawing down to ~34% in a narrow
   card, which makes the labels unreadable.) */
function chartWidth(mount) {
  const own = Math.round(mount.getBoundingClientRect().width);
  if (own > 0) return Math.max(own, 260);
  // Hidden (display:none) ancestors report 0; fall back to the card's width.
  const card = mount.closest('.card');
  const cardW = card ? Math.round(card.getBoundingClientRect().width) - 36 : 0;
  return Math.max(cardW > 0 ? cardW : 600, 260);
}

/* Horizontal bars. Thin marks, 4px rounded data-ends anchored to the baseline,
   direct value labels, per-mark hover. */
function hBar(mount, rows, opts = {}) {
  const {
    color = 'var(--seq-450)',
    colorFor = null,
    labelWidth = 128,
    rowH = 26,
    valueFmt = (v) => v.toLocaleString(),
    tipFmt = null,
    onClick = null,
  } = opts;

  const max = Math.max(...rows.map((r) => r.count), 1);
  const W = chartWidth(mount);
  const H = rows.length * rowH;
  // Long category names must not crowd out the bars in a narrow card.
  const barX = Math.min(labelWidth, Math.round(W * 0.42));
  const valueGutter = 52;
  const trackW = Math.max(W - barX - valueGutter, 40);

  const bars = rows.map((r, i) => {
    const y = i * rowH;
    const w = Math.max((r.count / max) * trackW, r.count > 0 ? 3 : 0);
    const fill = colorFor ? colorFor(r) : color;
    return `
      <g class="bar-g" data-i="${i}" ${onClick ? 'style="cursor:pointer"' : ''}>
        <rect x="0" y="${y}" width="${W}" height="${rowH}" fill="transparent"></rect>
        <text class="bar-label" x="${barX - 10}" y="${y + rowH / 2}" text-anchor="end" dominant-baseline="middle">${esc(r.name ?? r.label)}</text>
        <rect x="${barX}" y="${y + 5}" width="${w}" height="${rowH - 12}" rx="4" fill="${fill}"></rect>
        <text class="bar-value" x="${barX + w + 8}" y="${y + rowH / 2}" dominant-baseline="middle">${valueFmt(r.count)}</text>
      </g>`;
  }).join('');

  mount.innerHTML = `<svg viewBox="0 0 ${W} ${H}" width="${W}" height="${H}" role="img">${bars}</svg>`;

  $$('.bar-g', mount).forEach((g) => {
    const r = rows[Number(g.dataset.i)];
    g.addEventListener('mousemove', (e) => {
      tip.show(tipFmt ? tipFmt(r) : `${esc(r.name ?? r.label)} — <b>${r.count.toLocaleString()}</b>`, e.clientX, e.clientY);
    });
    g.addEventListener('mouseleave', () => tip.hide());
    if (onClick) g.addEventListener('click', () => onClick(r));
  });
}

/* Columns, for ordered sequences (months, score bins). */
function vBar(mount, rows, opts = {}) {
  const { color = 'var(--seq-450)', height = 150, tipFmt = null, labelEvery = 1 } = opts;
  const max = Math.max(...rows.map((r) => r.count), 1);
  const n = rows.length;
  const W = chartWidth(mount);
  const gap = 2;                   // 2px surface gap between adjacent fills
  const slot = W / n;
  const bw = Math.max(slot - gap, 1);
  const plotH = height - 22;

  const bars = rows.map((r, i) => {
    const h = (r.count / max) * plotH;
    const x = i * slot;
    const y = plotH - h;
    return `
      <g class="bar-g" data-i="${i}">
        <rect x="${x}" y="0" width="${slot}" height="${height}" fill="transparent"></rect>
        <rect x="${x}" y="${y}" width="${bw}" height="${Math.max(h, r.count > 0 ? 2 : 0)}" rx="4" fill="${color}"></rect>
        ${i % labelEvery === 0
          ? `<text class="axis-text" x="${x + bw / 2}" y="${plotH + 14}" text-anchor="middle">${esc(r.label)}</text>`
          : ''}
      </g>`;
  }).join('');

  mount.innerHTML = `<svg viewBox="0 0 ${W} ${height}" width="${W}" height="${height}" role="img">
      ${bars}
      <line class="axis-line" x1="0" y1="${plotH}" x2="${W}" y2="${plotH}"></line>
    </svg>`;

  $$('.bar-g', mount).forEach((g) => {
    const r = rows[Number(g.dataset.i)];
    g.addEventListener('mousemove', (e) => {
      tip.show(tipFmt ? tipFmt(r) : `${esc(r.label)} — <b>${r.count.toLocaleString()}</b>`, e.clientX, e.clientY);
    });
    g.addEventListener('mouseleave', () => tip.hide());
  });
}

function renderDashboard() {
  const ov = state.overview;

  // Category composition — the one chart where identity is the job, so it takes
  // the validated categorical palette. Each bar carries its own name, which is
  // what keeps identity from resting on color alone.
  hBar($('#chart-categories'), ov.categories.map((c) => ({ name: c.label, count: c.count, key: c.key, share: c.share })), {
    colorFor: (r) => catVar(r.key),
    labelWidth: 200,
    rowH: 30,
    tipFmt: (r) => `${esc(r.name)} — <b>${r.count.toLocaleString()}</b> (${r.share.toFixed(1)}%)`,
    onClick: (r) => {
      showView('triage');
      selectRail('category', r.key);
    },
  });

  hBar($('#chart-age'), ov.ageBuckets.map((b) => ({ name: b.label, count: b.count })), { labelWidth: 70 });

  vBar($('#chart-scores'), ov.scoreBins, {
    height: 160,
    labelEvery: 3,
    tipFmt: (r) => `score ${r.lo} to ${r.lo + 20} — <b>${r.count.toLocaleString()}</b>`,
  });

  vBar($('#chart-months'), ov.months, { height: 150, labelEvery: 2 });

  hBar($('#chart-senders'), ov.topSenders, {
    labelWidth: 220,
    onClick: (r) => {
      state.query = r.name;
      $('#search').value = r.name;
      showView('triage');
      selectRail('all', null);
    },
  });

  hBar($('#chart-rules'), ov.topRules, { labelWidth: 230 });

  $('#oldest-list').innerHTML = ov.oldestActionable.map((o) => `
    <div class="oldest-row" data-n="${o.n}" style="--chip:${catVar(o.category)}">
      <span class="oldest-age">${fmtAge(o.ageDays)}</span>
      <span class="oldest-main">
        <span class="oldest-sub">${esc(o.subject)}</span>
        <span class="oldest-from">${esc(o.from)}</span>
      </span>
      <span class="oldest-go">open →</span>
    </div>`).join('');

  $$('#oldest-list .oldest-row').forEach((el) => {
    el.onclick = () => {
      showView('triage');
      state.query = '';
      $('#search').value = '';
      selectRail('all', null);
      select(Number(el.dataset.n));
    };
  });
}

/* ───────────────────────────── weights ───────────────────────────── */

async function renderWeights() {
  const w = await fetch('/api/weights').then((r) => r.json());
  $('#config-path').textContent = w.configPath;

  const table = (rows, title) => `
    <div class="wt-group">${title}</div>
    <table class="wt">
      <thead><tr><th>Key</th><th style="text-align:right">Current</th><th style="text-align:right">Default</th></tr></thead>
      <tbody>${rows.map((r) => `
        <tr class="${r.current !== r.default ? 'overridden' : ''}">
          <td class="rule">${esc(r.key)}</td>
          <td class="num">${r.current}</td>
          <td class="num">${r.default}</td>
        </tr>`).join('')}
      </tbody>
    </table>`;

  $('#weights-table').innerHTML =
    table(w.weights, 'Weights') + table(w.thresholds, 'Thresholds') +
    (w.configPresent ? '' : '<p class="card-sub" style="margin-top:14px">No config.yaml yet — running on built-in defaults. Create one with <code>./triage.py seed-config</code>.</p>');
}

/* ───────────────────────────── data tab ───────────────────────────── */

let jobPoll = null;

async function renderJobs() {
  const st = await fetch('/api/job').then((r) => r.json());

  $('#job-buttons').innerHTML = st.jobs.map((j) => `
    <button class="job-btn" data-job="${esc(j.key)}" ${st.running ? 'disabled' : ''}
            title="${esc(j.blurb)}">
      <b>${esc(j.label)}</b>
      <span>${esc(j.blurb)}</span>
      ${j.destructive ? `<em class="job-warn">${esc(j.destructive)}</em>` : ''}
    </button>`).join('');

  $$('#job-buttons .job-btn').forEach((btn) => {
    btn.onclick = () => startJob(btn.dataset.job, st.jobs.find((j) => j.key === btn.dataset.job));
  });

  const panel = $('#job-panel');
  const showPanel = st.running || st.returncode !== null;
  panel.classList.toggle('hidden', !showPanel);
  if (!showPanel) return;

  const done = !st.running;
  const ok = done && st.returncode === 0 && !st.cancelled;
  panel.classList.toggle('is-done', ok);
  panel.classList.toggle('is-failed', done && !ok);

  $('#job-title').textContent =
      st.stage === 'scoring' ? `${st.label} — scoring…`
    : st.running             ? `${st.label}…`
    : st.cancelled           ? `${st.label} — stopped`
    : st.returncode === 0    ? `${st.label} — done`
    : `${st.label} — failed (exit ${st.returncode})`;
  $('#job-elapsed').textContent = `${st.elapsed}s`;
  $('#job-cancel').classList.toggle('hidden', !st.running);

  // Three distinct bar states, so "still working" never looks like "stalled".
  const prog = $('#job-progress');
  const bar = $('.job-bar', prog);
  const fill = $('.job-bar i', prog);
  const count = $('.job-count', prog);
  prog.classList.remove('hidden');
  bar.classList.toggle('is-indeterminate', st.stage === 'scoring');

  if (st.stage === 'scoring') {
    fill.style.width = '100%';
    count.textContent = 'scoring every message…';
  } else if (ok) {
    fill.style.width = '100%';
    count.textContent = 'complete';
  } else if (st.progress) {
    const pct = st.progress.total ? (100 * st.progress.done / st.progress.total) : 0;
    fill.style.width = `${pct.toFixed(1)}%`;
    count.textContent =
      `${st.progress.phase} ${st.progress.done.toLocaleString()} / ${st.progress.total.toLocaleString()}`;
  } else {
    fill.style.width = '0%';
    count.textContent = 'starting…';
  }

  // Say plainly what landed, so the end state is never ambiguous.
  const sum = $('#job-summary');
  if (ok && st.summary) {
    sum.classList.remove('hidden');
    sum.innerHTML = `<b>${st.summary.messages.toLocaleString()}</b> messages ·
      <b>${st.summary.threads.toLocaleString()}</b> threads ·
      <b>${st.summary.senders.toLocaleString()}</b> senders ·
      <b>${st.summary.actionable.toLocaleString()}</b> need your attention`;
  } else {
    sum.classList.add('hidden');
  }

  const warn = $('#job-warnings');
  if (st.warnings && st.warnings.length) {
    warn.classList.remove('hidden');
    warn.innerHTML = `<b>${st.warnings.length} warning${st.warnings.length > 1 ? 's' : ''}</b>`
      + st.warnings.slice(0, 6).map((w) => `<div>${esc(w)}</div>`).join('');
  } else {
    warn.classList.add('hidden');
  }

  const log = $('#job-log');
  const pinned = log.scrollTop + log.clientHeight >= log.scrollHeight - 8;
  log.textContent = st.lines.join('\n');
  if (pinned) log.scrollTop = log.scrollHeight;

  if (st.running && !jobPoll) {
    jobPoll = setInterval(renderJobs, 700);
  } else if (!st.running && jobPoll) {
    clearInterval(jobPoll);
    jobPoll = null;
    await load();                 // safe now: scoring finished before stage went to done
    renderCorrections();
    renderJobs();                 // repaint with the final summary
    toast(st.returncode === 0
      ? `${st.label} finished — ${(st.summary ? st.summary.messages : 0).toLocaleString()} messages scored`
      : `${st.label} did not complete`);
  }
}

async function startJob(kind, spec) {
  if (spec && spec.destructive && !confirm(`${spec.destructive}\n\nContinue?`)) return;
  try {
    await postJSON('/api/job', { kind });
    renderJobs();
  } catch (err) {
    errorToast(`Could not start: ${err.message}`);
  }
}

async function renderActionLog() {
  const { log } = await fetch('/api/actions').then((r) => r.json());
  if (!log.length) {
    $('#action-log').innerHTML =
      '<p class="card-sub">Nothing yet. This tool has not changed your mailbox.</p>';
    return;
  }
  $('#action-log').innerHTML = `
    <table class="wt">
      <thead><tr><th>When</th><th>Action</th><th style="text-align:right">Messages</th><th></th></tr></thead>
      <tbody>${log.filter((r) => !r.result).map((r) => `
        <tr>
          <td>${esc(r.at)}</td>
          <td class="rule">${esc(r.action)}</td>
          <td class="num">${(r.count || 0).toLocaleString()}</td>
          <td>${r.dry_run ? '<span class="pill">dry run</span>' : ''}</td>
        </tr>`).join('')}
      </tbody>
    </table>`;
}

async function renderCorrections() {
  const data = await fetch('/api/corrections').then((r) => r.json());
  const c = data.corrections;
  $('#corrections-path').textContent = data.path;

  const sections = [];
  for (const [scope, key, title] of [
    ['sender', 'senders', 'Senders'],
    ['domain', 'domains', 'Domains'],
    ['message', 'messages', 'Pinned messages'],
  ]) {
    const rows = Object.entries(c[key] || {});
    if (!rows.length) continue;
    sections.push(`
      <div class="wt-group">${title}</div>
      <table class="wt">
        <thead><tr><th>Target</th><th>Taught</th><th>When</th><th>Note</th><th></th></tr></thead>
        <tbody>${rows.map(([target, e]) => `
          <tr>
            <td class="rule">${esc(target)}</td>
            <td>${esc(e.kind || `category = ${e.category}`)}</td>
            <td>${esc(e.taught || '')}</td>
            <td>${esc(e.note || '')}</td>
            <td style="text-align:right">
              <button class="link-btn" data-forget="${esc(scope)}|${esc(target)}">forget</button>
            </td>
          </tr>`).join('')}
        </tbody>
      </table>`);
  }

  $('#corrections-table').innerHTML = sections.length
    ? sections.join('')
    : `<p class="card-sub">Nothing taught yet. Open a message and use <strong>Teach the
       classifier</strong>, or run <code>./triage.py teach &lt;n&gt; personal</code>.</p>`;

  $$('#corrections-table [data-forget]').forEach((btn) => {
    btn.onclick = async () => {
      const [scope, target] = btn.dataset.forget.split('|');
      await teach({ scope, target, clear: true });
      renderCorrections();
    };
  });

  // Lists: what you have ruled on, then the biggest ones you have not.
  const ov = state.overview || { taughtLists: [], listCandidates: [], aliasCandidates: [] };
  const taughtRows = (ov.taughtLists || []).map((l) => `
    <tr>
      <td class="rule">${esc(l.key)}</td>
      <td>${esc(l.kind)}</td>
      <td>${esc(l.note || '')}</td>
      <td style="text-align:right"><button class="link-btn" data-forget="list|${esc(l.key)}">forget</button></td>
    </tr>`).join('');

  const candRows = (ov.listCandidates || []).map((cand) => `
    <tr>
      <td class="rule">${esc(cand.key)}</td>
      <td class="num">${cand.count.toLocaleString()}</td>
      <td colspan="2" style="text-align:right">
        <button class="link-btn" data-listteach="important|${esc(cand.key)}">I read this</button>
        <button class="link-btn link-muted" data-listteach="ignore|${esc(cand.key)}"
                title="Sorts it to the bottom of Bulk. It stays listed and counted.">mute</button>
      </td>
    </tr>`).join('');

  $('#list-candidates').innerHTML =
    (taughtRows ? `<div class="wt-group">Lists you have ruled on</div>
       <table class="wt"><thead><tr><th>List</th><th>Verdict</th><th>Note</th><th></th></tr></thead>
       <tbody>${taughtRows}</tbody></table>` : '')
    + (candRows ? `<div class="wt-group">Biggest lists you have not ruled on</div>
       <table class="wt"><thead><tr><th>List</th><th style="text-align:right">Messages</th><th colspan="2"></th></tr></thead>
       <tbody>${candRows}</tbody></table>` : '<p class="card-sub">No untaught lists left.</p>');

  $$('#list-candidates [data-listteach]').forEach((btn) => {
    btn.onclick = async () => {
      const [kind, key] = btn.dataset.listteach.split('|');
      await teach({ scope: 'list', target: key, kind });
      renderCorrections();
    };
  });
  $$('#list-candidates [data-forget]').forEach((btn) => {
    btn.onclick = async () => {
      const [scope, target] = btn.dataset.forget.split('|');
      await teach({ scope, target, clear: true });
      renderCorrections();
    };
  });

  const aliasCands = ov.aliasCandidates || [];
  $('#alias-candidates').innerHTML = aliasCands.length ? `
    <p class="card-sub" style="margin-top:12px">
      Addresses that receive a lot of mail delivered here but are not declared yours:
    </p>
    <div class="alias-list">${aliasCands.map((a) => `
      <span class="alias-chip">${esc(a.key)}
        <span class="alias-count">${a.count.toLocaleString()}</span>
        <button class="link-btn" data-addalias="${esc(a.key)}">that's me</button>
      </span>`).join('')}</div>` : '';

  $$('#alias-candidates [data-addalias]').forEach((btn) => {
    btn.onclick = async () => {
      try {
        await postJSON('/api/alias', { address: btn.dataset.addalias });
        await load();
        renderCorrections();
        toast(`${btn.dataset.addalias} now counts as you — re-scored`);
      } catch (err) { toast(err.message); }
    };
  });

  const aliases = c.my_addresses || [];
  $('#alias-list').innerHTML = aliases.length
    ? aliases.map((a) => `
        <span class="alias-chip">${esc(a)}
          <button class="link-btn" data-unalias="${esc(a)}">remove</button>
        </span>`).join('')
    : '<p class="card-sub">None declared.</p>';

  $$('#alias-list [data-unalias]').forEach((btn) => {
    btn.onclick = async () => {
      try {
        await postJSON('/api/alias', { address: btn.dataset.unalias, remove: true });
        await load();
        renderCorrections();
        toast(`${btn.dataset.unalias} removed — re-scored`);
      } catch (err) { toast(err.message); }
    };
  });
}

/* ───────────────────────────── modal ───────────────────────────── */

function modal({ title, body, note, buttons }) {
  const el = $('#modal');
  $('#modal-title').textContent = title;
  $('#modal-body').textContent = body;
  const noteEl = $('#modal-note');
  noteEl.classList.toggle('hidden', !note);
  if (note) noteEl.textContent = note;

  $('#modal-buttons').innerHTML = buttons.map((b, i) => `
    <button class="modal-btn ${b.primary ? 'is-primary' : ''} ${b.danger ? 'is-danger' : ''}"
            data-i="${i}">${esc(b.label)}</button>`).join('');
  $$('#modal-buttons .modal-btn').forEach((btn) => {
    btn.onclick = () => {
      const choice = buttons[Number(btn.dataset.i)];
      if (!choice.keepOpen) closeModal();
      if (choice.onClick) choice.onClick();
    };
  });
  el.classList.remove('hidden');
  const first = $('#modal-buttons .is-primary') || $('#modal-buttons .modal-btn');
  if (first) first.focus();
}

function closeModal() { $('#modal').classList.add('hidden'); }

/* After replying in Gmail, settle that one message rather than re-syncing
   everything -- and ask Gmail rather than assuming. */
async function confirmHandled(n, subject) {
  modal({
    title: 'Opened in Gmail',
    body: subject,
    note: 'Reply there, then come back. Gmail marks it read when you open it; '
        + 'this app only finds out by asking.',
    buttons: [
      {
        label: 'Replied / read — remove it', primary: true, keepOpen: true,
        onClick: async () => {
          const btns = $('#modal-buttons');
          btns.innerHTML = '<button class="modal-btn" disabled>Checking Gmail…</button>';
          const doneBusy = busy('Checking Gmail…');
          try {
            const res = await postJSON('/api/sync-one', { n });
            if (res.handled) {
              closeModal();
              await load();
              toast(`Removed from triage — ${res.remaining.toLocaleString()} left`);
            } else {
              modal({
                title: 'Still unread in Gmail',
                body: subject,
                note: 'Gmail says this is still unread and in your inbox, so nothing was '
                    + 'changed. If you just replied, give it a moment and check again.',
                buttons: [
                  { label: 'Check again', primary: true, keepOpen: true,
                    onClick: () => confirmHandled(n, subject) },
                  { label: 'Leave it' },
                ],
              });
            }
          } catch (err) {
            closeModal();
            errorToast(err.message);
          } finally {
            doneBusy();
          }
        },
      },
      { label: 'Cancel — leave it here' },
    ],
  });
}

/* ─────────────────────────── write mode ─────────────────────────── */

let modeTimer = null;

function renderBadge() {
  const m = state.mode;
  const badge = $('#mode-badge');
  badge.classList.toggle('is-armed', m.armed);
  if (m.armed) {
    const mm = String(Math.floor(m.remaining / 60)).padStart(2, '0');
    const ss = String(m.remaining % 60).padStart(2, '0');
    badge.textContent = `write ${mm}:${ss}`;
    badge.title = `Write mode is ON. Actions can change your Gmail. Disarms itself in ${mm}:${ss}. Click to go back to read-only.`;
  } else {
    badge.textContent = 'read-only';
    badge.title = m.writeTokenPresent
      ? 'Read-only. Nothing can change your mailbox. Click to arm write mode.'
      : 'Read-only, and no write authorization exists yet. Click to see how to grant it.';
  }

  if (m.armed && !modeTimer) {
    modeTimer = setInterval(async () => {
      state.mode.remaining = Math.max(0, state.mode.remaining - 1);
      if (state.mode.remaining <= 0) {
        await refreshMode();          // disarming IS a change: full repaint
      } else if (state.mode.remaining % 10 === 0) {
        // The countdown is local, so it cannot see the server being restarted
        // or disarmed elsewhere. Re-check every 10s rather than let the badge
        // promise write access that is no longer there.
        await refreshMode();
      } else {
        renderBadge();                // a tick is not; touch only the badge
      }
    }, 1000);
  } else if (!m.armed && modeTimer) {
    clearInterval(modeTimer); modeTimer = null;
  }
}

/* Armed/disarmed actually changed, so the buttons and rows need repainting. */
function renderMode() {
  renderBadge();
  renderList();
  if (state.activeN != null) select(state.activeN);
}

async function refreshMode({ force = false } = {}) {
  const was = state.mode.armed;
  try {
    state.mode = await fetch('/api/mode').then((r) => r.json());
  } catch (_) {
    return;                       // server gone; leave the badge alone
  }
  // A full repaint only when the state actually changed, so revalidating does
  // not throw away the reader's scroll position.
  if (force || state.mode.armed !== was) renderMode();
  else renderBadge();
}

async function toggleMode() {
  const m = state.mode;
  if (!m.writeTokenPresent) {
    alert('No write authorization yet.\n\nRun this in a terminal:\n\n'
      + '    ./triage.py authorize-write\n\n'
      + 'It asks Google for the gmail.modify scope, which can mark read, archive '
      + 'and move to Trash — but cannot delete permanently. The grant is stored '
      + 'separately from your read-only token, and read-only mode never loads it.');
    return;
  }
  if (!m.armed) {
    const ok = confirm('Arm write mode?\n\nActions will really change your Gmail. '
      + 'Everything available is reversible — trashed mail stays in Trash for about 30 days.\n\n'
      + `Write mode switches itself off again after ${m.armMinutes} minutes.`);
    if (!ok) return;
  }
  try {
    state.mode = await postJSON('/api/mode', { armed: !m.armed });
    renderMode();
    toast(state.mode.armed ? 'Write mode armed' : 'Back to read-only');
  } catch (err) { errorToast(err.message); }
}

/* ───────────────────────────── actions ───────────────────────────── */

function viewActions() {
  const ov = state.overview;
  if (!ov || !ov.actionsFor) return [];
  const sel = state.select;
  if (sel.kind === 'category') return ov.actionsFor.categories[sel.key] || [];
  if (sel.kind === 'tag') return ov.actionsFor.tags[sel.key] || [];
  if (sel.kind === 'needs') return ov.actionsFor.needs[sel.key] || [];
  return [];   // "All messages" offers none, on purpose
}

function canMultiSelect() {
  // If a view offers actions at all, selecting several is allowed. Only the
  // catch-all "All messages" view offers none, so only it has no checkboxes.
  if (!state.overview || state.select.kind === 'all') return false;
  return viewActions().length > 0;
}

async function armFor(spec, what) {
  if (state.mode.armed) return true;
  if (!state.mode.writeTokenPresent) {
    modal({
      title: 'No write authorization yet',
      body: 'Run this once in a terminal, then reload:',
      note: './triage.py authorize-write --yes',
      buttons: [{ label: 'OK', primary: true }],
    });
    return false;
  }
  return new Promise((resolve) => {
    modal({
      title: 'Write mode is off',
      body: `${spec.label} — ${what}`,
      note: `Arming lets this app change your Gmail for ${state.mode.armMinutes} minutes, `
          + 'then it switches itself back to read-only. Everything here is reversible in Gmail.',
      buttons: [
        {
          label: `Arm and ${spec.label.toLowerCase()}`, primary: true, keepOpen: true,
          onClick: async () => {
            try {
              state.mode = await postJSON('/api/mode', { armed: true });
              renderMode();
              closeModal();
              resolve(true);
            } catch (err) { closeModal(); toast(err.message); resolve(false); }
          },
        },
        { label: 'Cancel', onClick: () => resolve(false) },
      ],
    });
  });
}

async function runAction(action, ns, { fromSelection = false, spec = null } = {}) {
  spec = spec || viewActions().find((a) => a.key === action);
  if (!spec) { toast('That action is not offered here'); return; }
  if (state.select.kind === 'all') {
    modal({
      title: 'Open a category first',
      body: 'Actions are deliberately unavailable from the All messages view.',
      note: 'Pick a category, a needs filter, or a tag in the left rail — the idea is '
          + 'that deciding what to do with something follows from having looked at it '
          + 'in context.',
      buttons: [{ label: 'OK', primary: true }],
    });
    return;
  }
  const many = ns.length > 1;
  const what = many ? `${ns.length} messages` : 'this message';

  if (!(await armFor(spec, what))) return;
  if (spec.destructive || many) {
    if (!confirm(`${spec.label} — ${what}?\n\n${spec.blurb}`)) return;
  }
  if (runAction.inFlight) return;
  const sel = state.select;
  runAction.inFlight = true;
  const done = busy(many ? `${spec.label} — ${what}…` : `${spec.label}…`);
  try {
    const res = await postJSON('/api/act', {
      action, ns,
      category: sel.kind === 'category' ? sel.key : undefined,
      tag: sel.kind === 'tag' ? sel.key : undefined,
      needs: sel.kind === 'needs' ? sel.key : undefined,
    });
    if (fromSelection) state.selected.clear();
    await load();
    renderCorrections();
    done();
    toast(`${spec.label}: ${res.succeeded.length} done`
      + (res.failed.length ? `, ${res.failed.length} failed` : ''));
  } catch (err) {
    done();
    errorToast(err.message);
    // Most likely the server disarmed under us; resync so the badge stops
    // claiming write access it does not have.
    refreshMode();
  } finally {
    done();
    runAction.inFlight = false;
  }
}

function renderSelectBar() {
  const bar = $('#select-bar');
  if (!canMultiSelect()) { bar.classList.add('hidden'); state.selected.clear(); return; }
  bar.classList.remove('hidden');
  const n = state.selected.size;
  $('#sel-count').textContent = n ? `${n.toLocaleString()} selected` : 'select all shown';
  $('#select-all').checked = n > 0;
  $('#sel-actions').innerHTML = n
    ? viewActions().map((a) => `
        <button class="sel-btn ${a.destructive ? 'is-destructive' : ''} ${state.mode.armed ? '' : 'is-locked'}"
                data-act="${a.key}"
                title="${esc(a.blurb)}${state.mode.armed ? '' : ' — click to arm write mode and do it'}">
          ${state.mode.armed ? '' : '<span class="lock">\u25cf</span> '}${esc(a.label)}</button>`).join('')
    : '';
  $$('#sel-actions .sel-btn').forEach((btn) => {
    btn.onclick = () => runAction(btn.dataset.act, [...state.selected], { fromSelection: true });
  });
}

/* ───────────────────────────── chrome ───────────────────────────── */

function showView(name, { push = true } = {}) {
  $$('.view').forEach((v) => v.classList.toggle('is-active', v.id === `view-${name}`));
  $$('.tab').forEach((t) => t.classList.toggle('is-active', t.dataset.view === name));
  if (name === 'weights') renderWeights();
  if (name === 'data') { renderJobs(); renderCorrections(); renderActionLog(); }
  if (name === 'dashboard' && state.overview) renderDashboard();
  if (push && location.hash.slice(1) !== name) {
    history.replaceState(null, '', `#${name}`);
  }
}

function applyHash() {
  const hash = location.hash.slice(1);
  const msg = /^m(\d+)$/.exec(hash);
  if (msg) { showView('triage', { push: false }); select(Number(msg[1])); return; }
  // #cat=grants, #needs=reply, #tag=travel — shareable, and what the rail sets.
  const filter = /^(cat|needs|tag)=(.+)$/.exec(hash);
  if (filter) {
    const kind = { cat: 'category', needs: 'needs', tag: 'tag' }[filter[1]];
    state.select = { kind, key: decodeURIComponent(filter[2]) };
    state.limit = 120;
    showView('triage', { push: false });
    renderRail();
    renderList();
    return;
  }
  if (['triage', 'dashboard', 'weights', 'data', 'manual'].includes(hash)) showView(hash, { push: false });
}

function moveSelection(delta) {
  const visible = $$('#list .msgrow').map((el) => Number(el.dataset.n));
  const rows = visible.map((n) => state.byN.get(n)).filter(Boolean);
  if (!rows.length) return;
  const idx = rows.findIndex((m) => m.n === state.activeN);
  const next = idx === -1 ? 0 : Math.min(Math.max(idx + delta, 0), rows.length - 1);
  select(rows[next].n);
}

function initEvents() {
  $$('.tab').forEach((t) => { t.onclick = () => showView(t.dataset.view); });

  $('#search').addEventListener('input', (e) => {
    state.query = e.target.value;
    state.limit = 120;
    renderList();
  });

  $('#sort').addEventListener('change', (e) => {
    state.sort = e.target.value;
    renderList();
  });

  $('#show-more').onclick = () => { state.limit += 240; renderList(); };

  $('#mode-badge').onclick = toggleMode;

  // Replying happens in Gmail, so the cache only finds out by asking. Sync is
  // cheap enough to sit in the top bar rather than behind a tab.
  $('#sync').onclick = async () => {
    const btn = $('#sync');
    btn.disabled = true;
    btn.textContent = 'Syncing…';
    const done = busy('Syncing with Gmail…');
    try {
      await postJSON('/api/job', { kind: 'sync' });
      let st;
      do {
        await new Promise((r) => setTimeout(r, 600));
        st = await fetch('/api/job').then((r) => r.json());
      } while (st.running);
      await load();
      renderJobs();
      const gone = (st.lines || []).find((l) => l.includes('no longer unread')) || '';
      const fresh = (st.lines || []).find((l) => /^\s*\d+ new$/.test(l)) || '';
      toast(st.returncode === 0
        ? `Synced — ${gone.trim() || 'nothing dropped'}${fresh ? `, ${fresh.trim()}` : ''}`
        : 'Sync did not complete — see Data');
    } catch (err) {
      errorToast(err.message);
    } finally {
      done();
      btn.disabled = false;
      btn.textContent = 'Sync';
    }
  };

  $('#select-all').onclick = (e) => {
    state.selected.clear();
    if (e.target.checked) {
      const stale = state.overview ? state.overview.staleDays : 120;
      const rows = applyFilters();
      const pool = state.showOlder ? rows : rows.filter((m) => m.ageDays <= stale);
      pool.slice(0, state.showOlder ? rows.length : state.limit)
          .forEach((m) => state.selected.add(m.n));
    }
    renderList();
  };

  $('#sel-clear').onclick = () => { state.selected.clear(); renderList(); };

  $('#job-cancel').onclick = async () => {
    try { await postJSON('/api/job/cancel', {}); renderJobs(); }
    catch (err) { toast(err.message); }
  };

  $('#alias-form').onsubmit = async (e) => {
    e.preventDefault();
    const address = $('#alias-input').value.trim();
    if (!address) return;
    try {
      await postJSON('/api/alias', { address });
      $('#alias-input').value = '';
      await load();
      renderCorrections();
      toast(`${address} now counts as you — re-scored`);
    } catch (err) { toast(err.message); }
  };

  $('#theme').onclick = () => {
    const cur = document.documentElement.getAttribute('data-theme');
    const next = cur === 'dark' ? 'light' : cur === 'light' ? '' : 'dark';
    if (next) document.documentElement.setAttribute('data-theme', next);
    else document.documentElement.removeAttribute('data-theme');
    try { localStorage.setItem('triage-theme', next); } catch (_) { /* private mode */ }
  };

  $('#rescore').onclick = async () => {
    const btn = $('#rescore');
    btn.disabled = true;
    btn.textContent = 'Re-scoring…';
    const done = busy('Re-scoring every message…');
    try {
      const res = await fetch('/api/rescore', { method: 'POST' }).then((r) => r.json());
      if (res.error) throw new Error(res.error);
      await load();
      toast('Re-scored from the local cache — no network call');
    } catch (err) {
      errorToast(`Re-score failed: ${err.message}`);
    } finally {
      done();
      btn.disabled = false;
      btn.textContent = 'Re-score';
    }
  };

  let resizeTimer = null;
  window.addEventListener('resize', () => {
    clearTimeout(resizeTimer);
    resizeTimer = setTimeout(() => { if (state.overview) renderDashboard(); }, 150);
  });

  document.addEventListener('keydown', (e) => {
    const typing = ['INPUT', 'SELECT', 'TEXTAREA'].includes(e.target.tagName);
    if (e.key === '/' && !typing) { e.preventDefault(); $('#search').focus(); return; }
    if (e.key === 'Escape') {
      if (!$('#modal').classList.contains('hidden')) { closeModal(); return; }
      if (typing) { e.target.blur(); return; }
    }
    if (typing) return;

    if (e.key === 'j' || e.key === 'ArrowDown') { e.preventDefault(); moveSelection(1); }
    else if (e.key === 'k' || e.key === 'ArrowUp') { e.preventDefault(); moveSelection(-1); }
    else if (e.key >= '1' && e.key <= String(CAT_ORDER.length)) {
      selectRail('category', CAT_ORDER[Number(e.key) - 1]);
    } else if (e.key === '0') {
      selectRail('all', null);
    } else if (e.key === 'g') { showView('dashboard'); }
    else if (e.key === 't') { showView('triage'); }
    else if (e.key === 'w') { showView('weights'); }
    else if (e.key === 'd') { showView('data'); }
    else if (e.key === 'm' || e.key === '?') { showView('manual'); }
    else if (e.key === 'r') { $('#rescore').click(); }
    else if (e.key === 's') { $('#sync').click(); }
  });
}

// ?theme=dark|light pins the theme for this load (handy for bookmarks and for
// screenshotting); otherwise the stored preference wins, else the OS setting.
const urlTheme = new URLSearchParams(location.search).get('theme');
if (urlTheme === 'dark' || urlTheme === 'light') {
  document.documentElement.setAttribute('data-theme', urlTheme);
} else {
  try {
    const saved = localStorage.getItem('triage-theme');
    if (saved) document.documentElement.setAttribute('data-theme', saved);
  } catch (_) { /* storage can throw in private mode */ }
}

initEvents();
refreshMode();
window.addEventListener('hashchange', applyHash);
load().then(applyHash).catch((err) => {
  document.body.innerHTML = `<div style="padding:40px;font-family:system-ui">
    <h2>Could not load the cache</h2><p>${esc(err.message)}</p>
    <p>Run <code>./triage.py fetch</code> then <code>./triage.py score</code> first.</p></div>`;
});
