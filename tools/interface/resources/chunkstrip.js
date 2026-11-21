// chunkstrip.js
//
// A draggable time strip for the Log chunk modify page: shows the previous chunk, the
// chunk being modified and the next chunk, and lets the start and end of the chunk be
// dragged in 5-minute steps. Dragging a shared boundary can move the neighbouring
// chunk's edge along (default), or leave a gap. The fzlog calls needed are planned so
// that every intermediate step obeys fzlog's rules.
//
// Times are handled as "wall-clock minutes": Formalizer time stamps (YYYYmmddHHMM) are
// local times without a time zone, so they are converted with Date.UTC() and formatted
// back with the UTC getters, which avoids any daylight-saving shifts in the browser.
//
// The logic part (ChunkTimes) has no browser dependencies. Its test, chunkstrip_test.html
// (next to this file), runs in any browser: open it directly from the source directory.
//
// Used by the Log chunk modify page (fzlog-cgi.py?action=modify&id=...). Deployed to the web
// root with `make executables` (TOPLEVEL in the top Makefile).

const ChunkTimes = (function () {

    function parseStamp(s) { // 'YYYYmmddHHMM' -> minutes
        return Date.UTC(+s.substr(0, 4), +s.substr(4, 2) - 1, +s.substr(6, 2), +s.substr(8, 2), +s.substr(10, 2)) / 60000;
    }
    function pad2(n) { return (n < 10 ? '0' : '') + n; }
    function fmtStamp(m) {
        const d = new Date(m * 60000);
        return d.getUTCFullYear() + pad2(d.getUTCMonth() + 1) + pad2(d.getUTCDate()) + pad2(d.getUTCHours()) + pad2(d.getUTCMinutes());
    }
    function fmtTime(m) { const d = new Date(m * 60000); return pad2(d.getUTCHours()) + ':' + pad2(d.getUTCMinutes()); }
    function fmtDate(m) { const d = new Date(m * 60000); return d.getUTCFullYear() + '-' + pad2(d.getUTCMonth() + 1) + '-' + pad2(d.getUTCDate()); }
    function fmtDuration(mins) { return (mins >= 60 ? Math.floor(mins / 60) + 'h ' : '') + (mins % 60) + 'm'; }

    // Model: { now, prev, cur, next }, each chunk { open, close } in minutes, close === null
    // for an open chunk; prev and next may be null. Only the most recent chunk can be open.
    // Targets: { open, close } for the current chunk, plus link flags for each side.

    // Range for the current chunk's open time.
    function limitsOpen(m, t, linkPrev, linkNext) {
        let hi = m.now, hiWhy = 'now';
        if ((t.close !== null) && (t.close < hi)) { hi = t.close; hiWhy = 'its own end'; }
        // If the next chunk's start moves along with this end, this chunk must keep at least
        // 1 minute, since the next chunk has to start after this one starts.
        if (m.next && linkNext && (t.close !== null) && (t.close - 1 < hi)) { hi = t.close - 1; hiWhy = 'its own end (-1 min)'; }
        // Two chunks cannot have the same ID (start time): stay before the next chunk's start.
        if (m.next && !linkNext && (m.next.open - 1 < hi)) { hi = m.next.open - 1; hiWhy = 'next chunk start (-1 min)'; }
        let lo = -Infinity, loWhy = 'start of the Log';
        if (m.prev) {
            if (linkPrev) {
                lo = m.prev.open + 1; loWhy = 'previous chunk start (+1 min)';
            } else {
                lo = Math.max(m.prev.close, m.prev.open + 1);
                loWhy = (lo === m.prev.close) ? 'previous chunk end' : 'previous chunk start (+1 min)';
            }
        }
        return { lo, hi, loWhy, hiWhy };
    }

    // Range for the current chunk's close time (only when it is closed).
    function limitsClose(m, t, linkNext) {
        let lo = t.open, loWhy = 'its own start';
        let hi = m.now, hiWhy = 'now';
        if (m.next) {
            if (linkNext) {
                // The next chunk's start moves to this end, and must stay after this start.
                lo = t.open + 1; loWhy = 'its own start (+1 min)';
                const nextEnd = (m.next.close === null) ? m.now : m.next.close;
                if (nextEnd < hi) { hi = nextEnd; hiWhy = 'next chunk end'; }
            } else {
                hi = m.next.open; hiWhy = 'next chunk start';
            }
        }
        return { lo, hi, loWhy, hiWhy };
    }

    // The neighbours' edges that follow from the targets.
    function derive(m, t, linkPrev, linkNext) {
        const r = { open: t.open, close: t.close, prevClose: null, nextOpen: null };
        if (m.prev) {
            const adjacent = (m.prev.close === m.cur.open);
            r.prevClose = (linkPrev && (adjacent || t.open < m.prev.close)) ? t.open : m.prev.close;
        }
        if (m.next && (t.close !== null)) {
            const adjacent = (m.next.open === m.cur.close);
            r.nextOpen = (linkNext && (adjacent || t.close > m.next.open)) ? t.close : m.next.open;
        }
        return r;
    }

    // Nearest allowed time: 5-minute marks within [lo, hi], or lo, hi or the original time.
    function snap(raw, lo, hi, orig, step) {
        step = step || 5;
        const c = Math.min(Math.max(raw, lo), hi);
        const candidates = [lo, hi];
        if ((orig >= lo) && (orig <= hi)) candidates.push(orig);
        const mark = Math.round(c / step) * step;
        if ((mark >= lo) && (mark <= hi)) candidates.push(mark);
        let best = candidates[0];
        for (const x of candidates) if (Math.abs(x - c) < Math.abs(best - c)) best = x;
        return best;
    }

    // fzlog's rules for one change, applied to a working copy of the three chunks.
    function checkOpen(st, who, to, now) {
        const ch = st[who];
        const before = (who === 'cur') ? st.prev : ((who === 'next') ? st.cur : null);
        if ((ch.close !== null) && (to > ch.close)) return 'would start after its own end';
        if (before && (to < before.close)) return 'would start before the previous chunk ends';
        if (before && (to <= before.open)) return 'would not start after the previous chunk starts';
        const after = (who === 'prev') ? st.cur : ((who === 'cur') ? st.next : null);
        if (after && (to >= after.open)) return 'would start when or after the next chunk starts';
        if (to > now) return 'would start in the future';
        return '';
    }
    function checkClose(st, who, to, now) {
        const ch = st[who];
        const after = (who === 'prev') ? st.cur : ((who === 'cur') ? st.next : null);
        if (to < ch.open) return 'would end before it starts';
        if (after && (to > after.open)) return 'would end after the next chunk starts';
        if (to > now) return 'would end in the future';
        return '';
    }

    function permutations(a) {
        if (a.length <= 1) return [a.slice()];
        const out = [];
        a.forEach((x, i) => {
            for (const p of permutations(a.slice(0, i).concat(a.slice(i + 1)))) out.push([x].concat(p));
        });
        return out;
    }

    const whoName = { prev: 'Previous chunk', cur: 'This chunk', next: 'Next chunk' };

    // The fzlog calls (at most 4), in an order in which every step obeys the rules.
    function plan(m, d) {
        const ops = [];
        if (m.prev && (d.prevClose !== m.prev.close)) ops.push({ who: 'prev', field: 'close', to: d.prevClose });
        if (d.open !== m.cur.open) ops.push({ who: 'cur', field: 'open', to: d.open });
        if ((m.cur.close !== null) && (d.close !== m.cur.close)) ops.push({ who: 'cur', field: 'close', to: d.close });
        if (m.next && (d.nextOpen !== null) && (d.nextOpen !== m.next.open)) ops.push({ who: 'next', field: 'open', to: d.nextOpen });
        if (ops.length === 0) return { ok: true, steps: [] };
        let lastProblem = '';
        for (const order of permutations(ops)) {
            const st = {
                prev: m.prev ? { open: m.prev.open, close: m.prev.close } : null,
                cur: { open: m.cur.open, close: m.cur.close },
                next: m.next ? { open: m.next.open, close: m.next.close } : null,
            };
            const steps = [];
            let ok = true;
            for (const op of order) {
                const problem = (op.field === 'open') ? checkOpen(st, op.who, op.to, m.now) : checkClose(st, op.who, op.to, m.now);
                if (problem) { ok = false; lastProblem = whoName[op.who] + ' ' + problem; break; }
                const id = fmtStamp(st[op.who].open); // the chunk's ID at this step
                steps.push({
                    id: id, flag: (op.field === 'open') ? '1' : '2', to: fmtStamp(op.to), // for the request to fzlog-cgi.py
                    cmd: 'fzlog -m ' + id + ' ' + ((op.field === 'open') ? '-1' : '-2') + ' ' + fmtStamp(op.to),
                    what: whoName[op.who] + ' (' + id + '): ' + ((op.field === 'open') ? 'start ' : 'end ') +
                          fmtTime(st[op.who][op.field]) + ' → ' + fmtTime(op.to),
                });
                st[op.who][op.field] = op.to;
            }
            if (ok) return { ok: true, steps };
        }
        return { ok: false, steps: [], problem: lastProblem };
    }

    return { parseStamp, fmtStamp, fmtTime, fmtDate, fmtDuration, limitsOpen, limitsClose, derive, snap, plan };
})();

if (typeof module !== 'undefined') module.exports = ChunkTimes;

// ---------------------------------------------------------------------------
// Browser part: renders the strip into a container element and handles dragging.
// data: { now: stamp, prev, cur, next } with chunks { open: stamp, close: stamp|null,
// node: id, desc: text }. onApply(steps, derived) is called by the Apply button.

function ChunkStrip(container, data, onApply) {
    const T = ChunkTimes;
    const conv = (c) => c ? { open: T.parseStamp(c.open), close: (c.close ? T.parseStamp(c.close) : null), node: c.node, desc: c.desc || '' } : null;
    const m = { now: T.parseStamp(data.now), prev: conv(data.prev), cur: conv(data.cur), next: conv(data.next) };
    const t = { open: m.cur.open, close: m.cur.close };
    let linkPrev = true, linkNext = true;

    // Visible range: previous chunk start to next chunk end (or now). If the chunk is the
    // most recent closed one, show up to now, but not more than a few hours.
    const rs = m.prev ? m.prev.open : m.cur.open - Math.max(30, Math.round(((m.cur.close || m.now) - m.cur.open) / 2));
    let re;
    if (m.next) re = (m.next.close === null) ? m.now : m.next.close;
    else if (m.cur.close === null) re = m.now;
    else re = Math.min(m.now, m.cur.close + Math.max(60, m.cur.close - m.cur.open));
    const span = Math.max(re - rs, 1);
    const pct = (x) => (2 + 96 * (x - rs) / span); // 2% margin on both sides

    container.innerHTML = `
      <div class="cs-strip" role="group" aria-label="Log chunk time strip">
        <div class="cs-ghost cs-ghost-cur"></div>
        <div class="cs-block cs-prev"></div>
        <div class="cs-block cs-cur"></div>
        <div class="cs-block cs-next"></div>
        <div class="cs-now"></div>
        <div class="cs-handle cs-h-open" tabindex="0" role="slider" aria-label="Start of this chunk"><span class="cs-tip"></span></div>
        <div class="cs-handle cs-h-close" tabindex="0" role="slider" aria-label="End of this chunk"><span class="cs-tip"></span></div>
      </div>
      <div class="cs-ticks"></div>
      <div class="cs-controls">
        <label class="cs-link-prev"><input type="checkbox" checked> Move the previous chunk's end along (unchecked: leave a gap)</label><br>
        <label class="cs-link-next"><input type="checkbox" checked> Move the next chunk's start along (unchecked: leave a gap)</label>
      </div>
      <table class="cs-table"></table>
      <div class="cs-plan"></div>
      <p><button class="button button1 cs-apply" disabled>Apply</button> <button class="button button2 cs-reset">Reset</button></p>`;

    const q = (s) => container.querySelector(s);
    const hOpen = q('.cs-h-open'), hClose = q('.cs-h-close');
    if (!m.prev) q('.cs-link-prev').style.display = 'none';
    if (!m.next || m.cur.close === null) q('.cs-link-next').style.display = 'none';
    if (m.cur.close === null) hClose.style.display = 'none';

    q('.cs-link-prev input').addEventListener('change', (e) => { linkPrev = e.target.checked; clampTargets(); render(); });
    q('.cs-link-next input').addEventListener('change', (e) => { linkNext = e.target.checked; clampTargets(); render(); });
    q('.cs-reset').addEventListener('click', () => { t.open = m.cur.open; t.close = m.cur.close; render(); });
    q('.cs-apply').addEventListener('click', () => {
        const d = T.derive(m, t, linkPrev, linkNext);
        const p = T.plan(m, d);
        if (p.ok && p.steps.length) onApply(p.steps, d);
    });

    // When a toggle changes, the targets may be outside the new limits.
    function clampTargets() {
        const lo = T.limitsOpen(m, t, linkPrev, linkNext);
        t.open = Math.min(Math.max(t.open, lo.lo), lo.hi);
        if (t.close !== null) {
            const lc = T.limitsClose(m, t, linkNext);
            t.close = Math.min(Math.max(t.close, lc.lo), lc.hi);
        }
    }

    function ticks() {
        const steps = [5, 10, 15, 30, 60, 120, 180, 360, 720];
        let step = steps.find((s) => span / s <= 12) || 1440;
        let html = '';
        for (let x = Math.ceil(rs / step) * step; x <= re; x += step) {
            const label = (T.fmtTime(x) === '00:00') ? T.fmtDate(x).substr(5) : T.fmtTime(x);
            html += `<span class="cs-tick" style="left:${pct(x)}%">${label}</span>`;
        }
        q('.cs-ticks').innerHTML = html;
    }

    function place(el, a, b) {
        el.style.left = pct(a) + '%';
        el.style.width = Math.max(pct(b) - pct(a), 0.3) + '%';
    }

    function blockLabel(c, a, b) {
        return `${c.node} ${c.desc}`.trim() + ` (${T.fmtTime(a)}–${b === null ? 'open' : T.fmtTime(b)})`;
    }

    function render() {
        const d = T.derive(m, t, linkPrev, linkNext);
        const curEnd = (t.close === null) ? m.now : t.close;
        place(q('.cs-ghost-cur'), m.cur.open, (m.cur.close === null) ? m.now : m.cur.close);
        if (m.prev) {
            place(q('.cs-prev'), m.prev.open, d.prevClose);
            q('.cs-prev').title = q('.cs-prev').textContent = blockLabel(m.prev, m.prev.open, d.prevClose);
        } else q('.cs-prev').style.display = 'none';
        place(q('.cs-cur'), t.open, curEnd);
        q('.cs-cur').title = q('.cs-cur').textContent = blockLabel(m.cur, t.open, t.close);
        q('.cs-cur').classList.toggle('cs-open', t.close === null);
        if (m.next) {
            const nextEnd = (m.next.close === null) ? m.now : m.next.close;
            const nOpen = (d.nextOpen !== null) ? d.nextOpen : m.next.open;
            place(q('.cs-next'), nOpen, nextEnd);
            q('.cs-next').title = q('.cs-next').textContent = blockLabel(m.next, nOpen, m.next.close);
            q('.cs-next').classList.toggle('cs-open', m.next.close === null);
        } else q('.cs-next').style.display = 'none';
        q('.cs-now').style.left = pct(Math.min(m.now, re)) + '%';
        q('.cs-now').style.display = (m.now <= re) ? '' : 'none';

        hOpen.style.left = pct(t.open) + '%';
        hOpen.querySelector('.cs-tip').textContent = T.fmtTime(t.open);
        hOpen.setAttribute('aria-valuetext', T.fmtTime(t.open));
        if (t.close !== null) {
            hClose.style.left = pct(t.close) + '%';
            hClose.querySelector('.cs-tip').textContent = T.fmtTime(t.close);
            hClose.setAttribute('aria-valuetext', T.fmtTime(t.close));
        }

        // Table of old and new times.
        const row = (label, a, b) => {
            const changed = (a !== b);
            const fa = (a === null) ? 'open' : T.fmtTime(a), fb = (b === null) ? 'open' : T.fmtTime(b);
            return `<tr${changed ? ' class="cs-changed"' : ''}><td>${label}</td><td>${fa}</td><td>${changed ? '→ ' + fb : ''}</td></tr>`;
        };
        const dur = (a, b) => (b === null) ? 'open' : T.fmtDuration(b - a);
        let rows = '<tr><th></th><th>currently</th><th>new</th></tr>';
        if (m.prev) rows += row('Previous chunk end', m.prev.close, d.prevClose);
        rows += row('This chunk start (its ID)', m.cur.open, t.open);
        rows += row('This chunk end', m.cur.close, t.close);
        if (m.next && (t.close !== null)) rows += row('Next chunk start (its ID)', m.next.open, d.nextOpen);
        rows += `<tr class="cs-dur"><td>Length of this chunk</td><td>${dur(m.cur.open, m.cur.close)}</td><td>${(t.open !== m.cur.open || t.close !== m.cur.close) ? '→ ' + dur(t.open, t.close) : ''}</td></tr>`;
        q('.cs-table').innerHTML = rows;

        // Planned fzlog calls.
        const p = T.plan(m, d);
        let html;
        if (!p.ok) html = `<p class="cs-problem">These times cannot be reached: ${p.problem}.</p>`;
        else if (p.steps.length === 0) html = '<p class="cs-none">Drag the start or end of this chunk (the green block). Times move in 5-minute steps, and stop at the limits.</p>';
        else html = '<p>Apply will make these changes, in this order:</p><ol>' +
            p.steps.map((s) => `<li>${s.what}<br><code>${s.cmd}</code></li>`).join('') + '</ol>';
        q('.cs-plan').innerHTML = html;
        q('.cs-apply').disabled = !(p.ok && p.steps.length);
    }

    // Dragging (mouse, touch and pen through pointer events) and arrow keys.
    function attach(handle, which) {
        let dragging = false;
        const fromX = (clientX) => {
            const r = q('.cs-strip').getBoundingClientRect();
            return rs + ((clientX - r.left) / r.width * 100 - 2) / 96 * span;
        };
        const move = (raw) => {
            if (which === 'open') {
                const l = T.limitsOpen(m, t, linkPrev, linkNext);
                t.open = T.snap(raw, Math.max(l.lo, rs), l.hi, m.cur.open);
                handle.dataset.why = (t.open === l.lo) ? l.loWhy : ((t.open === l.hi) ? l.hiWhy : '');
            } else {
                const l = T.limitsClose(m, t, linkNext);
                t.close = T.snap(raw, l.lo, Math.min(l.hi, re), m.cur.close);
                handle.dataset.why = (t.close === l.lo) ? l.loWhy : ((t.close === l.hi) ? l.hiWhy : '');
            }
            render();
            const why = handle.dataset.why;
            if (why) handle.querySelector('.cs-tip').textContent += ' (limit: ' + why + ')';
        };
        handle.addEventListener('pointerdown', (e) => { dragging = true; handle.setPointerCapture(e.pointerId); handle.classList.add('cs-active'); e.preventDefault(); });
        handle.addEventListener('pointermove', (e) => { if (dragging) move(fromX(e.clientX)); });
        const end = () => { dragging = false; handle.classList.remove('cs-active'); };
        handle.addEventListener('pointerup', end);
        handle.addEventListener('pointercancel', end);
        handle.addEventListener('keydown', (e) => {
            const cur = (which === 'open') ? t.open : t.close;
            if (e.key === 'ArrowLeft') { move(cur - 5); e.preventDefault(); }
            if (e.key === 'ArrowRight') { move(cur + 5); e.preventDefault(); }
        });
    }
    attach(hOpen, 'open');
    if (m.cur.close !== null) attach(hClose, 'close');

    ticks();
    render();
}
