/* HodgeLedger atlas — 2D/3D Hodge embedding, glowing harmonic 1-cycles. */
(() => {
  const FIELDS = {
    cs: "#6ea8fe",
    stat: "#e6b35a",
    math: "#d48cff",
    physics: "#ff8b6b",
    qbio: "#7dffb3",
    "q-bio": "#7dffb3",
    eess: "#7ee0e8",
    econ: "#f0a3c2",
    qfin: "#c9d46a",
  };

  const state = {
    embedding: null,
    harmonic: null,
    bridges: null,
    meta: null,
    ledger: null,
    timeline: null,
    dim: 3,
    onlyCycles: false,
    faces: false,
    selected: null,
    selectedCycle: 0,
    hover: null,
    query: "",
    yearIdx: -1,
    rot: 0.62,
    tilt: 0.48,
    panX: 0,
    panY: 0,
    scale: 1,
    drag: null,
    auto: true,
    knn: [],
    stars: [],
    layoutCache: null,
    adj: [],
    idIndex: new Map(),
    flowPhase: 0,
  };

  const $ = (id) => document.getElementById(id);
  const canvas = $("atlas");
  const ctx = canvas.getContext("2d", { alpha: false });

  async function load() {
    const [emb, harm, br, meta, led, tl] = await Promise.all([
      fetch("data/embedding.json").then((r) => r.json()),
      fetch("data/harmonic.json").then((r) => (r.ok ? r.json() : { cycles: [] })),
      fetch("data/bridges.json").then((r) => (r.ok ? r.json() : { bridges: [] })),
      fetch("data/graph_meta.json").then((r) => r.json()),
      fetch("data/ledger.json").then((r) => r.json()),
      fetch("data/timeline.json")
        .then((r) => (r.ok ? r.json() : { slices: [] }))
        .catch(() => ({ slices: [] })),
    ]);
    state.embedding = emb;
    state.harmonic = harm;
    state.bridges = br;
    state.meta = meta;
    state.ledger = led;
    state.timeline = tl;
    (emb.nodes || []).forEach((n, i) => state.idIndex.set(n.id, i));
    buildAdj(meta.edges || [], emb.nodes.length);
    buildKnn(emb.nodes, 3);
    seedStars(260);
    applySlice(-1);
    renderLegend();
    renderCycles();
    paintDetail(null);
    setupTimeline();
    resize();
    loop();
  }

  function buildAdj(edges, n) {
    const adj = Array.from({ length: n }, () => []);
    for (const e of edges) {
      const a = e[0];
      const b = e[1];
      if (a == null || b == null) continue;
      adj[a].push(b);
      adj[b].push(a);
    }
    state.adj = adj;
  }

  function seedStars(n) {
    const stars = [];
    for (let i = 0; i < n; i++) {
      stars.push({
        x: Math.random(),
        y: Math.random(),
        r: Math.random() * 1.2 + 0.2,
        a: 0.06 + Math.random() * 0.2,
      });
    }
    state.stars = stars;
  }

  function buildKnn(nodes, k) {
    const coords = nodes.map((n) => n.x || [0, 0]);
    const knn = nodes.map(() => []);
    const N = nodes.length;
    const cap = N > 400 ? 180 : N;
    for (let i = 0; i < N; i++) {
      const di = [];
      const a = coords[i];
      const step = N > cap ? Math.max(1, Math.floor(N / cap)) : 1;
      for (let j = i % step; j < N; j += step) {
        if (i === j) continue;
        const b = coords[j];
        let s = 0;
        const d = Math.min(a.length, b.length, 3);
        for (let t = 0; t < d; t++) {
          const u = (a[t] || 0) - (b[t] || 0);
          s += u * u;
        }
        di.push([s, j]);
      }
      di.sort((p, q) => p[0] - q[0]);
      knn[i] = di.slice(0, k).map((p) => p[1]);
    }
    state.knn = knn;
  }

  function currentSlice() {
    const slices = state.timeline?.slices || [];
    if (!slices.length) return null;
    if (state.yearIdx < 0 || state.yearIdx >= slices.length) return slices[slices.length - 1];
    return slices[state.yearIdx];
  }

  function currentYear() {
    const sl = currentSlice();
    if (sl) return sl.year;
    const years = (state.embedding?.nodes || []).map((n) => n.year).filter((y) => y > 0);
    return years.length ? Math.max(...years) : 9999;
  }

  function applySlice(idx) {
    const slices = state.timeline?.slices || [];
    state.yearIdx = slices.length ? (idx < 0 ? slices.length - 1 : idx) : -1;
    const sl = currentSlice();
    const full = state.meta || {};
    $("stat-n").textContent = sl ? sl.n : full.n;
    $("stat-m").textContent = sl ? sl.m : full.undirected_edges;
    $("stat-t").textContent = sl ? sl.triangles : full.triangles;
    $("stat-b0").textContent = sl ? sl.components : full.betti0;
    $("stat-b1").textContent = sl ? sl.betti1 : full.betti1;
    $("stat-l2").textContent = Number(sl ? sl.lambda2 : full.algebraic_connectivity_hodge_L0).toFixed(4);
    $("stat-l1").textContent = Number(sl ? sl.lambda_min_l1 : full.lambda_min_L1).toExponential(2);
    $("stat-year").textContent = sl ? sl.year : "—";
    if ($("year-label")) $("year-label").textContent = sl ? sl.year : "—";
    if ($("year-cycle")) {
      $("year-cycle").textContent = sl && sl.betti1 ? `β₁ ${sl.betti1}` : "";
    }
  }

  function setupTimeline() {
    const slices = state.timeline?.slices || [];
    const box = $("timebox");
    const sl = $("year");
    if (!slices.length) {
      box.hidden = true;
      return;
    }
    box.hidden = false;
    sl.min = 0;
    sl.max = slices.length - 1;
    sl.value = slices.length - 1;
    sl.addEventListener("input", () => {
      applySlice(Number(sl.value));
      state.layoutCache = null;
    });
  }

  function renderLegend() {
    const box = $("legend");
    box.innerHTML = "";
    const seen = new Map();
    for (const n of state.embedding.nodes) {
      seen.set(n.field, (seen.get(n.field) || 0) + 1);
    }
    for (const [field, count] of [...seen.entries()].sort()) {
      const el = document.createElement("span");
      el.innerHTML = `<i class="sw" style="background:${FIELDS[field] || "#aaa"}"></i> ${field} ${count}`;
      box.appendChild(el);
    }
    const cy = document.createElement("span");
    cy.innerHTML = `<i class="sw" style="background:#7ef0ff;box-shadow:0 0 8px #7ef0ff"></i> harmonic`;
    box.appendChild(cy);
  }

  function renderCycles() {
    const ul = $("cycles");
    ul.innerHTML = "";
    const cycles = state.harmonic?.cycles || [];
    $("cycle-meta").textContent =
      `Lanczos on L₁ · β₁≈${state.harmonic?.betti1 ?? "—"} · λmin ${Number(state.harmonic?.lambda_min || 0).toExponential(2)}. ` +
      `[ ] cycles the ranked 1-chains.`;
    cycles.slice(0, 12).forEach((c, i) => {
      const li = document.createElement("li");
      li.dataset.idx = String(i);
      const harm = c.harmonic || c.lambda < 1e-6;
      li.innerHTML = `<span class="rk">#${c.rank}</span> ${harm ? "harmonic" : "near"} λ ${Number(c.lambda).toExponential(2)}
        <small>${c.pair_a || "?"} ↔ ${c.pair_b || "?"} · |E| ${c.edges?.length || 0} · cross ${Number(c.cross_field_mass || 0).toFixed(3)}</small>`;
      li.addEventListener("click", () => selectCycle(i));
      ul.appendChild(li);
    });
    highlightCycleList();
  }

  function highlightCycleList() {
    for (const li of document.querySelectorAll(".cycle-list li")) {
      li.classList.toggle("on", Number(li.dataset.idx) === state.selectedCycle);
    }
  }

  function accountsFor(paperId) {
    const accs = state.ledger?.accounts || [];
    return {
      z: accs.find((a) => a.paper_id === paperId && a.dim === 0) || null,
      o: accs.find((a) => a.paper_id === paperId && a.dim === 1) || null,
    };
  }

  function nodeById(id) {
    const i = state.idIndex.get(id);
    return i == null ? null : state.embedding.nodes[i];
  }

  function currentCycle() {
    return (state.harmonic?.cycles || [])[state.selectedCycle] || null;
  }

  function paintSpark(id) {
    const spark = $("spark");
    const { z, o } = accountsFor(id);
    if (!z && !o) {
      spark.hidden = true;
      return;
    }
    spark.hidden = false;
    const cap = Math.max(
      1,
      ...(state.ledger?.accounts || []).map((a) => Math.max(a.debit_cents || 0, a.credit_cents || 0))
    );
    const set = (el, lab, v) => {
      el.style.setProperty("--w", `${Math.min(100, (100 * (v || 0)) / cap)}%`);
      lab.textContent = v || 0;
    };
    set($("bar-dr0"), $("lab-dr0"), z?.debit_cents);
    set($("bar-cr0"), $("lab-cr0"), z?.credit_cents);
    set($("bar-dr1"), $("lab-dr1"), o?.debit_cents);
    set($("bar-cr1"), $("lab-cr1"), o?.credit_cents);
    const node = nodeById(id);
    const role0 = !z ? "" : z.net_cents > 0 ? "0-FLOW CREDITOR" : z.net_cents < 0 ? "0-FLOW DEBTOR" : "0-FLOW FLAT";
    const role1 = !o ? "no 1-flow" : Math.abs(o.net_cents) < 2 ? "1-FLOW CIRCULATION (div-free)" : `1-FLOW NET ${o.net_cents}`;
    $("spark-peer").textContent = `${role0} · ${role1} · field ${node?.field || "?"}`;
  }

  function paintDetail(id) {
    const pre = $("detail");
    const C = currentCycle();
    const head =
      "HODGELEDGER TRIAL BALANCE\n" +
      `BOOK ${state.ledger?.balanced ? "BALANCED" : "OUT OF BALANCE"} ` +
      `0-DR ${state.ledger?.total_debit_0}  1-DR ${state.ledger?.total_debit_1}\n`;
    if (!id && !C) {
      pre.textContent = head + "SELECT A PAPER OR A HARMONIC CYCLE.";
      $("spark").hidden = true;
      return;
    }
    const lines = [head];
    if (C) {
      lines.push(
        `CYCLE #${C.rank}  λ ${Number(C.lambda).toExponential(3)}  ${C.harmonic || C.lambda < 1e-6 ? "HARMONIC" : "NEAR"}`,
        `PAIR ${C.pair_a} ↔ ${C.pair_b}  CROSS ${Number(C.cross_field_mass || 0).toFixed(4)}  H ${Number(C.participation_entropy || 0).toFixed(3)}`,
        `LOOP ${(C.loop || []).slice(0, 12).join(" → ")}${(C.loop || []).length > 12 ? " …" : ""}`,
        `1-FLOW EDGES ${C.edges?.length || 0}`
      );
    }
    if (id) {
      const node = nodeById(id);
      const { z, o } = accountsFor(id);
      lines.push(
        `ACCOUNT ${id}`,
        node ? `TITLE ${node.title}` : "",
        node ? `FIELD ${node.field} / ${node.category} YEAR ${node.year}` : "",
        node ? `DEGREE ${node.degree} CLUSTER ${node.cluster}` : "",
        z ? `CHART0 DR ${z.debit_cents} CR ${z.credit_cents} NET ${z.net_cents}` : "CHART0 none",
        o ? `CHART1 DR ${o.debit_cents} CR ${o.credit_cents} NET ${o.net_cents}` : "CHART1 none"
      );
      paintSpark(id);
    } else {
      $("spark").hidden = true;
    }
    pre.textContent = lines.filter(Boolean).join("\n");
    highlightCycleList();
  }

  function select(id) {
    state.selected = id;
    paintDetail(id);
  }

  function selectCycle(i) {
    state.selectedCycle = i;
    const C = currentCycle();
    const pick = C?.loop?.[0] || C?.edges?.[0]?.u || null;
    if (pick) state.selected = pick;
    paintDetail(state.selected);
  }

  function resize() {
    const dpr = Math.min(window.devicePixelRatio || 1, 2);
    const r = canvas.getBoundingClientRect();
    canvas.width = Math.max(1, Math.floor(r.width * dpr));
    canvas.height = Math.max(1, Math.floor(r.height * dpr));
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
    state.layoutCache = null;
  }

  function project(node) {
    const x = node.x || [0, 0, 0];
    let X = x[0] || 0;
    let Y = x[1] || 0;
    let Z = node.z != null ? node.z : x[2] || 0;
    if (state.dim === 3) {
      const cy = Math.cos(state.rot);
      const sy = Math.sin(state.rot);
      const cx = Math.cos(state.tilt);
      const sx = Math.sin(state.tilt);
      const x1 = X * cy + Z * sy;
      const z1 = -X * sy + Z * cy;
      const y1 = Y * cx - z1 * sx;
      const z2 = Y * sx + z1 * cx;
      const focal = 1.85;
      const p = focal / (focal + z2 + 0.55);
      return { X: x1 * p, Y: y1 * p, depth: z2 };
    }
    return { X, Y, depth: 0 };
  }

  function layout() {
    const nodes = state.embedding?.nodes || [];
    if (!nodes.length) return { pts: [], w: 1, h: 1 };
    const pts = nodes.map((n, i) => ({ n, i, ...project(n) }));
    let minX = Infinity,
      maxX = -Infinity,
      minY = Infinity,
      maxY = -Infinity;
    for (const p of pts) {
      minX = Math.min(minX, p.X);
      maxX = Math.max(maxX, p.X);
      minY = Math.min(minY, p.Y);
      maxY = Math.max(maxY, p.Y);
    }
    const r = canvas.getBoundingClientRect();
    const pad = 48;
    const sx = (r.width - pad * 2) / Math.max(1e-9, maxX - minX);
    const sy = (r.height - pad * 2) / Math.max(1e-9, maxY - minY);
    const s = Math.min(sx, sy) * state.scale;
    const ox = pad + (r.width - pad * 2 - (maxX - minX) * s) / 2 + state.panX;
    const oy = pad + (r.height - pad * 2 - (maxY - minY) * s) / 2 + state.panY;
    for (const p of pts) {
      p.px = ox + (p.X - minX) * s;
      p.py = oy + (maxY - p.Y) * s;
    }
    pts.sort((a, b) => a.depth - b.depth);
    return { pts, w: r.width, h: r.height };
  }

  function qmatch(n) {
    if (!state.query) return 0;
    const q = state.query;
    if (n.id.toLowerCase().includes(q)) return 2;
    if ((n.title || "").toLowerCase().includes(q)) return 1;
    return 0;
  }

  function drawArrow(x1, y1, x2, y2, phase) {
    const dx = x2 - x1;
    const dy = y2 - y1;
    const len = Math.hypot(dx, dy) || 1;
    const ux = dx / len;
    const uy = dy / len;
    const t = ((phase % 1) + 1) % 1;
    const mx = x1 + dx * t;
    const my = y1 + dy * t;
    ctx.beginPath();
    ctx.moveTo(mx - uy * 4 - ux * 6, my + ux * 4 - uy * 6);
    ctx.lineTo(mx, my);
    ctx.lineTo(mx + uy * 4 - ux * 6, my - ux * 4 - uy * 6);
    ctx.stroke();
  }

  function draw() {
    const r = canvas.getBoundingClientRect();
    ctx.fillStyle = "#07060f";
    ctx.fillRect(0, 0, r.width, r.height);
    const g = ctx.createRadialGradient(r.width * 0.5, r.height * 0.42, 20, r.width * 0.5, r.height * 0.42, r.width * 0.7);
    g.addColorStop(0, "#16122a");
    g.addColorStop(1, "#07060f");
    ctx.fillStyle = g;
    ctx.fillRect(0, 0, r.width, r.height);

    for (const s of state.stars) {
      ctx.globalAlpha = s.a;
      ctx.fillStyle = "#d8d2c4";
      ctx.fillRect(s.x * r.width, s.y * r.height, s.r, s.r);
    }
    ctx.globalAlpha = 1;
    if (!state.embedding) return;

    const { pts } = layout();
    state.layoutCache = pts;
    const year = currentYear();
    const searching = !!state.query;
    const C = currentCycle();
    const cycleVerts = new Set();
    for (const e of C?.edges || []) {
      cycleVerts.add(e.u);
      cycleVerts.add(e.v);
    }

    const byIndex = new Array(state.embedding.nodes.length);
    for (const p of pts) byIndex[p.i] = p;

    ctx.lineWidth = 0.7;
    for (let i = 0; i < byIndex.length; i++) {
      const a = byIndex[i];
      if (!a) continue;
      if ((a.n.year || 0) > year) continue;
      if (state.onlyCycles && !cycleVerts.has(a.n.id)) continue;
      for (const j of state.knn[i] || []) {
        const b = byIndex[j];
        if (!b) continue;
        if ((b.n.year || 0) > year) continue;
        ctx.globalAlpha = 0.12;
        ctx.beginPath();
        ctx.moveTo(a.px, a.py);
        ctx.lineTo(b.px, b.py);
        ctx.strokeStyle = FIELDS[a.n.field] || "#888";
        ctx.stroke();
      }
    }

    if (state.faces && C) {
      const used = new Set();
      for (const e of C.edges || []) {
        const ui = state.idIndex.get(e.u);
        const vi = state.idIndex.get(e.v);
        if (ui == null || vi == null) continue;
        const nbr = new Set(state.adj[ui] || []);
        for (const w of state.adj[vi] || []) {
          if (!nbr.has(w)) continue;
          const key = [ui, vi, w].sort((x, y) => x - y).join("|");
          if (used.has(key)) continue;
          used.add(key);
          const pa = byIndex[ui];
          const pb = byIndex[vi];
          const pc = byIndex[w];
          if (!pa || !pb || !pc) continue;
          if ((pa.n.year || 0) > year || (pb.n.year || 0) > year || (pc.n.year || 0) > year) continue;
          ctx.globalAlpha = 0.08;
          ctx.beginPath();
          ctx.moveTo(pa.px, pa.py);
          ctx.lineTo(pb.px, pb.py);
          ctx.lineTo(pc.px, pc.py);
          ctx.closePath();
          ctx.fillStyle = "#7ef0ff";
          ctx.fill();
        }
      }
    }

    if (C) {
      ctx.lineJoin = "round";
      for (const e of C.edges || []) {
        const ui = state.idIndex.get(e.u);
        const vi = state.idIndex.get(e.v);
        const a = ui == null ? null : byIndex[ui];
        const b = vi == null ? null : byIndex[vi];
        if (!a || !b) continue;
        if ((a.n.year || 0) > year && (b.n.year || 0) > year) continue;
        const w = Math.abs(e.weight || 0);
        ctx.globalAlpha = 0.35 + Math.min(0.55, w * 2);
        ctx.strokeStyle = "#7ef0ff";
        ctx.shadowColor = "#7ef0ff";
        ctx.shadowBlur = 14;
        ctx.lineWidth = 1.6 + Math.min(4, w * 8);
        ctx.beginPath();
        ctx.moveTo(a.px, a.py);
        ctx.lineTo(b.px, b.py);
        ctx.stroke();
        ctx.shadowBlur = 0;
        ctx.lineWidth = 1.2;
        const src = (e.weight || 0) >= 0 ? a : b;
        const dst = (e.weight || 0) >= 0 ? b : a;
        drawArrow(src.px, src.py, dst.px, dst.py, state.flowPhase);
      }
      ctx.shadowBlur = 0;
      ctx.globalAlpha = 1;
    }

    if (state.selected != null) {
      const si = state.idIndex.get(state.selected);
      const sp = si == null ? null : byIndex[si];
      if (sp) {
        for (const j of state.adj[si] || []) {
          const b = byIndex[j];
          if (!b) continue;
          ctx.globalAlpha = 0.35;
          ctx.strokeStyle = "#e8e0d4";
          ctx.lineWidth = 1;
          ctx.beginPath();
          ctx.moveTo(sp.px, sp.py);
          ctx.lineTo(b.px, b.py);
          ctx.stroke();
        }
      }
    }

    const centroids = new Map();
    for (const p of pts) {
      if ((p.n.year || 0) > year) continue;
      const c = centroids.get(p.n.field) || { x: 0, y: 0, n: 0 };
      c.x += p.px;
      c.y += p.py;
      c.n += 1;
      centroids.set(p.n.field, c);
    }
    ctx.globalAlpha = 0.45;
    ctx.fillStyle = "#8b8496";
    ctx.font = "12px IBM Plex Mono, monospace";
    for (const [field, c] of centroids) {
      ctx.fillText(field, c.x / c.n - 10, c.y / c.n);
    }

    for (const p of pts) {
      const alive = (p.n.year || 0) <= year;
      const onC = cycleVerts.has(p.n.id);
      if (state.onlyCycles && !onC) continue;
      const hit = qmatch(p.n);
      if (searching && !hit && !onC && state.selected !== p.n.id) {
        ctx.globalAlpha = 0.08;
      } else {
        ctx.globalAlpha = alive ? 1 : 0.12;
      }
      const col = FIELDS[p.n.field] || "#c8c2b6";
      const sel = state.selected === p.n.id;
      const rad = (onC ? 4.6 : 2.0 + Math.min(3, (p.n.degree || 0) / 18)) * (alive ? 1 : 0.7);
      if ((onC || sel || hit) && alive) {
        ctx.beginPath();
        ctx.arc(p.px, p.py, rad + (sel ? 9 : 6), 0, Math.PI * 2);
        ctx.fillStyle = sel ? "rgba(126,240,255,0.28)" : hit ? "rgba(255,255,255,0.16)" : "rgba(126,240,255,0.14)";
        ctx.fill();
      }
      ctx.beginPath();
      ctx.arc(p.px, p.py, sel ? rad + 1.4 : rad, 0, Math.PI * 2);
      ctx.fillStyle = onC || sel ? "#7ef0ff" : col;
      ctx.fill();
    }
    ctx.globalAlpha = 1;
  }

  function hit(mx, my) {
    const pts = state.layoutCache || layout().pts;
    let best = null;
    let bestD = 14;
    const C = currentCycle();
    const cycleVerts = new Set();
    for (const e of C?.edges || []) {
      cycleVerts.add(e.u);
      cycleVerts.add(e.v);
    }
    for (const p of pts) {
      if (state.onlyCycles && !cycleVerts.has(p.n.id)) continue;
      const d = Math.hypot(p.px - mx, p.py - my);
      if (d < bestD) {
        bestD = d;
        best = p;
      }
    }
    return best;
  }

  function showHover(p) {
    const el = $("hover");
    if (!p) {
      el.hidden = true;
      return;
    }
    el.hidden = false;
    el.innerHTML = `<b>${p.n.id}</b> ${p.n.year || ""}<br>${p.n.title || ""}`;
    const hud = canvas.getBoundingClientRect();
    el.style.left = `${Math.min(hud.width - 240, p.px + 12)}px`;
    el.style.top = `${Math.max(8, p.py - 28)}px`;
  }

  function loop() {
    if (state.dim === 3 && state.auto && !state.drag) {
      state.rot += 0.0034;
    }
    state.flowPhase = (state.flowPhase + 0.012) % 1;
    draw();
    requestAnimationFrame(loop);
  }

  canvas.addEventListener("click", (e) => {
    if (state.drag && state.drag.moved) return;
    const r = canvas.getBoundingClientRect();
    const p = hit(e.clientX - r.left, e.clientY - r.top);
    if (p) select(p.n.id);
  });

  canvas.addEventListener("pointerdown", (e) => {
    state.drag = {
      x: e.clientX,
      y: e.clientY,
      rot: state.rot,
      tilt: state.tilt,
      panX: state.panX,
      panY: state.panY,
      pan: e.shiftKey || e.button === 1 || state.dim === 2,
      moved: false,
    };
    state.auto = false;
    canvas.setPointerCapture(e.pointerId);
  });
  canvas.addEventListener("pointermove", (e) => {
    const r = canvas.getBoundingClientRect();
    if (state.drag) {
      const dx = e.clientX - state.drag.x;
      const dy = e.clientY - state.drag.y;
      if (Math.hypot(dx, dy) > 3) state.drag.moved = true;
      if (state.drag.pan) {
        state.panX = state.drag.panX + dx;
        state.panY = state.drag.panY + dy;
      } else if (state.dim === 3) {
        state.rot = state.drag.rot + dx * 0.008;
        state.tilt = Math.max(-1.2, Math.min(1.2, state.drag.tilt + dy * 0.006));
      }
    } else {
      const p = hit(e.clientX - r.left, e.clientY - r.top);
      state.hover = p ? p.n.id : null;
      showHover(p);
    }
  });
  canvas.addEventListener("pointerup", () => {
    state.drag = null;
  });
  canvas.addEventListener("pointerleave", () => showHover(null));
  canvas.addEventListener(
    "wheel",
    (e) => {
      e.preventDefault();
      const f = Math.exp(-e.deltaY * 0.0012);
      state.scale = Math.max(0.35, Math.min(4.5, state.scale * f));
    },
    { passive: false }
  );

  $("btn-2d").addEventListener("click", () => {
    state.dim = 2;
    $("btn-2d").classList.add("active");
    $("btn-3d").classList.remove("active");
  });
  $("btn-3d").addEventListener("click", () => {
    state.dim = 3;
    state.auto = true;
    $("btn-3d").classList.add("active");
    $("btn-2d").classList.remove("active");
  });
  $("btn-cycles").addEventListener("click", (e) => {
    state.onlyCycles = !state.onlyCycles;
    e.currentTarget.classList.toggle("active", state.onlyCycles);
  });
  $("btn-faces").addEventListener("click", (e) => {
    state.faces = !state.faces;
    e.currentTarget.classList.toggle("active", state.faces);
  });
  $("search").addEventListener("input", (e) => {
    state.query = e.target.value.trim().toLowerCase();
    if (state.query) {
      const hitN = (state.embedding?.nodes || []).find((n) => qmatch(n));
      if (hitN) select(hitN.id);
    }
  });

  function cycleHarmonic(dir) {
    const list = state.harmonic?.cycles || [];
    if (!list.length) return;
    state.selectedCycle = (state.selectedCycle + dir + list.length) % list.length;
    selectCycle(state.selectedCycle);
  }

  document.addEventListener("keydown", (e) => {
    const typing = document.activeElement === $("search");
    if (e.key === "/" && !typing) {
      e.preventDefault();
      $("search").focus();
      $("search").select();
      return;
    }
    if (e.key === "Escape") {
      $("sheet").hidden = true;
      if (typing) {
        $("search").blur();
        $("search").value = "";
        state.query = "";
      }
      return;
    }
    if (e.key === "?" && !typing) {
      e.preventDefault();
      $("sheet").hidden = !$("sheet").hidden;
      return;
    }
    if (typing) return;
    if (e.key === "[") cycleHarmonic(-1);
    if (e.key === "]") cycleHarmonic(1);
    if (e.key === "2") $("btn-2d").click();
    if (e.key === "3") $("btn-3d").click();
    if (e.key === "c") $("btn-cycles").click();
    if (e.key === "f") $("btn-faces").click();
  });

  $("sheet").addEventListener("click", (e) => {
    if (e.target.id === "sheet") $("sheet").hidden = true;
  });

  window.addEventListener("resize", resize);
  load().catch((err) => {
    $("detail").textContent =
      "FAILED TO LOAD docs/data/*.json\n" +
      err +
      "\nServe via Pages or: python3 -m http.server --directory docs 8000";
  });
})();
