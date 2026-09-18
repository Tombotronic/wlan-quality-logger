// HTML/JS for the WLAN logger web UI.
// Kept in a separate header because Arduino's .ino prototype-generator
// chokes on the word "function" inside the raw string literal.

const char* INDEX_HTML = R"HTML(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, viewport-fit=cover">
  <title>Cardputer Adv - WLAN Quality</title>

  <!-- iOS "Add to Home Screen" app behavior (fullscreen, no Safari chrome) -->
  <meta name="apple-mobile-web-app-capable" content="yes">
  <meta name="mobile-web-app-capable" content="yes">
  <meta name="apple-mobile-web-app-status-bar-style" content="default">
  <meta name="apple-mobile-web-app-title" content="WLAN Quality">
  <meta name="theme-color" content="#f2f2f6">
  <link rel="icon" href="/icon.png">
  <link rel="apple-touch-icon" href="/icon.png">

  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <style>
    :root {
      --bg: #f2f2f6;
      --card: #ffffff;
      --ink: #1c1c1e;
      --muted: #8a8a8e;
      --accent: #2f6fed;
      --accent-bg: #e8f0fe;
      --track: #eceef1;
      --good: #34c759;
      --ok: #ffcc00;
      --bad: #ff3b30;
      --border: rgba(0,0,0,0.06);
      --shadow: 0 1px 3px rgba(0,0,0,0.06), 0 1px 2px rgba(0,0,0,0.04);
    }
    html { background: var(--bg); height: 100%; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
      background: var(--bg);
      color: var(--ink);
      min-height: 100vh;
      min-height: 100dvh;
      margin: 0;
      padding: max(20px, env(safe-area-inset-top)) max(16px, env(safe-area-inset-right))
               max(20px, env(safe-area-inset-bottom)) max(16px, env(safe-area-inset-left));
      box-sizing: border-box;
      overscroll-behavior-y: none;
      -webkit-tap-highlight-color: transparent;
    }
    .wrap { max-width: 460px; margin: 0 auto; }

    .top-row { display: flex; align-items: flex-start; justify-content: space-between; }

    .badge {
      display: inline-block;
      background: var(--accent-bg);
      color: var(--accent);
      font-size: 0.72em;
      font-weight: 700;
      letter-spacing: 0.06em;
      padding: 4px 10px;
      border-radius: 8px;
      margin-bottom: 10px;
    }
    .gear-btn {
      background: var(--card);
      border: none;
      box-shadow: var(--shadow);
      width: 34px;
      height: 34px;
      border-radius: 50%;
      font-size: 1.05em;
      color: var(--muted);
      cursor: pointer;
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 0;
      flex-shrink: 0;
    }
    h1 { font-size: 1.7em; font-weight: 800; margin: 0 0 4px; letter-spacing: -0.01em; }
    .ssid-line { color: var(--ink); font-weight: 600; font-size: 0.95em; margin-bottom: 18px; }

    .settings-overlay {
      position: fixed;
      inset: 0;
      background: rgba(0,0,0,0.35);
      display: none;
      align-items: flex-end;
      justify-content: center;
      z-index: 50;
    }
    .settings-overlay.open { display: flex; }
    .settings-panel {
      background: var(--card);
      width: 100%;
      max-width: 460px;
      border-radius: 20px 20px 0 0;
      padding: 20px 20px max(20px, env(safe-area-inset-bottom));
      box-sizing: border-box;
      box-shadow: 0 -4px 24px rgba(0,0,0,0.15);
    }
    .settings-head { display: flex; align-items: center; justify-content: space-between; margin-bottom: 16px; }
    .settings-head h3 { margin: 0; font-size: 1.1em; font-weight: 800; }
    .close-btn {
      background: none;
      border: none;
      font-size: 1.5em;
      line-height: 1;
      color: var(--muted);
      cursor: pointer;
      padding: 4px;
    }

    .card {
      background: var(--card);
      border-radius: 16px;
      box-shadow: var(--shadow);
      padding: 18px 20px;
      box-sizing: border-box;
    }

    .metrics {
      display: flex;
      margin-bottom: 14px;
    }
    .metric { flex: 1; }
    .metric + .metric { padding-left: 18px; margin-left: 18px; border-left: 1px solid var(--border); }
    .metric-label { color: var(--muted); font-size: 0.75em; font-weight: 700; letter-spacing: 0.06em; text-transform: uppercase; margin-bottom: 6px; }
    .metric-value { font-size: 1.9em; font-weight: 800; letter-spacing: -0.01em; }
    .metric-value .unit { font-size: 0.5em; font-weight: 600; color: var(--muted); margin-left: 2px; }
    .status-pill {
      display: inline-block;
      font-size: 0.4em;
      font-weight: 800;
      letter-spacing: 0.02em;
      padding: 3px 9px;
      border-radius: 999px;
      margin-left: 8px;
      vertical-align: middle;
    }
    .pill-good { background: var(--good); color: #ffffff; }
    .pill-fair { background: var(--ok); color: #7a5900; }
    .pill-poor { background: var(--bad); color: #ffffff; }
    .bar-track { background: var(--track); border-radius: 999px; height: 8px; margin-top: 10px; overflow: hidden; }
    .bar-fill { height: 100%; border-radius: 999px; background: var(--good); width: 0%; transition: width 0.3s ease, background 0.3s ease; }
    .metric-ts { color: var(--muted); font-size: 0.75em; margin-top: 8px; }

    .legend {
      display: flex;
      justify-content: center;
      flex-wrap: wrap;
      gap: 4px 14px;
      background: var(--card);
      border-radius: 999px;
      box-shadow: var(--shadow);
      padding: 10px 16px;
      margin: 14px 0;
      font-size: 0.78em;
      color: var(--muted);
    }
    .legend .item { display: inline-flex; align-items: center; white-space: nowrap; }
    .dot { display: inline-block; width: 9px; height: 9px; border-radius: 50%; margin-right: 6px; }

    .section-head { display: flex; align-items: baseline; justify-content: space-between; margin: 22px 0 10px; }
    .section-head h2 { font-size: 1.05em; font-weight: 800; margin: 0; }
    .section-head .range-note { color: var(--muted); font-size: 0.8em; }

    .range-picker {
      display: flex;
      background: var(--track);
      border-radius: 999px;
      padding: 3px;
      margin-bottom: 12px;
    }
    .range-picker button {
      flex: 1;
      border: none;
      background: transparent;
      color: var(--muted);
      font-family: inherit;
      font-size: 0.8em;
      font-weight: 700;
      padding: 7px 0;
      border-radius: 999px;
      cursor: pointer;
    }
    .range-picker button.active { background: var(--card); color: var(--ink); box-shadow: var(--shadow); }

    .chart-box { position: relative; width: 100%; height: 240px; }

    #forgetBtn {
      display: block;
      width: 100%;
      background: rgba(255,59,48,0.08);
      color: var(--bad);
      border: none;
      padding: 13px 14px;
      border-radius: 14px;
      font-family: inherit;
      font-size: 0.95em;
      font-weight: 700;
      cursor: pointer;
    }

    .footer { text-align: center; color: var(--muted); font-size: 0.78em; margin: 18px 0 4px; }
  </style>
</head>
<body>
  <div class="wrap">
    <div class="top-row">
      <span class="badge">CARDPUTER ADV</span>
      <button class="gear-btn" id="settingsBtn" aria-label="Settings">&#9881;</button>
    </div>
    <h1>Wi-Fi Signal Quality</h1>
    <div class="ssid-line" id="ssid">-</div>

    <div class="card">
      <div class="metrics">
        <div class="metric">
          <div class="metric-label">RSSI</div>
          <div class="metric-value"><span id="rssi">-</span><span class="unit">dBm</span><span class="status-pill" id="rssiStatus">-</span></div>
          <div class="bar-track"><div class="bar-fill" id="rssiBar"></div></div>
          <div class="metric-ts" id="ts">-</div>
        </div>
        <div class="metric">
          <div class="metric-label">Battery</div>
          <div class="metric-value"><span id="batt">-</span><span class="unit">%</span></div>
          <div class="bar-track"><div class="bar-fill" id="battBar"></div></div>
        </div>
      </div>
    </div>

    <div class="legend">
      <span class="item"><span class="dot" style="background:#34c759"></span>Good (&ge; -60 dBm)</span>
      <span class="item"><span class="dot" style="background:#ffcc00"></span>Fair (-60 to -75 dBm)</span>
      <span class="item"><span class="dot" style="background:#ff3b30"></span>Poor (&lt; -75 dBm)</span>
    </div>

    <div class="section-head">
      <h2>Signal history</h2>
      <span class="range-note" id="rangeLabel">last 1h</span>
    </div>
    <div class="range-picker">
      <button data-h="1" class="active">1H</button>
      <button data-h="3">3H</button>
      <button data-h="6">6H</button>
      <button data-h="12">12H</button>
      <button data-h="24">24H</button>
      <button data-h="all">All</button>
    </div>
    <div class="card chart-box"><canvas id="chart"></canvas></div>

    <div class="footer">Cardputer Adv &middot; ESP32-S3 &middot; readings every ~60s</div>
  </div>

  <div class="settings-overlay" id="settingsOverlay">
    <div class="settings-panel">
      <div class="settings-head">
        <h3>Settings</h3>
        <button class="close-btn" id="closeSettings" aria-label="Close">&times;</button>
      </div>
      <button id="forgetBtn">Forget Wi-Fi</button>
    </div>
  </div>
  <script>
    // Draws shaded good/ok/weak bands behind the line plus dashed threshold
    // lines at -60 and -75 dBm, so the zones are visible at a glance instead
    // of only being explained in the legend above the chart.
    const thresholdBands = {
      id: 'thresholdBands',
      beforeDatasetsDraw(chart) {
        const { ctx, chartArea, scales: { y } } = chart;
        if (!chartArea) return;
        const { left, right, top, bottom } = chartArea;
        const goodY = y.getPixelForValue(-60);
        const okY = y.getPixelForValue(-75);

        ctx.save();
        ctx.fillStyle = 'rgba(52,199,89,0.08)';
        ctx.fillRect(left, top, right - left, goodY - top);
        ctx.fillStyle = 'rgba(255,204,0,0.10)';
        ctx.fillRect(left, goodY, right - left, okY - goodY);
        ctx.fillStyle = 'rgba(255,59,48,0.08)';
        ctx.fillRect(left, okY, right - left, bottom - okY);

        ctx.setLineDash([4, 4]);
        ctx.lineWidth = 1;
        ctx.strokeStyle = 'rgba(52,199,89,0.6)';
        ctx.beginPath(); ctx.moveTo(left, goodY); ctx.lineTo(right, goodY); ctx.stroke();
        ctx.strokeStyle = 'rgba(255,59,48,0.6)';
        ctx.beginPath(); ctx.moveTo(left, okY); ctx.lineTo(right, okY); ctx.stroke();
        ctx.setLineDash([]);

        ctx.font = '10px -apple-system, sans-serif';
        ctx.textAlign = 'right';
        ctx.fillStyle = '#248a3d';
        ctx.fillText('-60 dBm', right - 4, goodY - 4);
        ctx.fillStyle = '#d70015';
        ctx.fillText('-75 dBm', right - 4, okY + 12);
        ctx.restore();
      }
    };

    const ctx = document.getElementById('chart').getContext('2d');
    const chart = new Chart(ctx, {
      type: 'line',
      data: { labels: [], datasets: [{ label: 'RSSI (dBm)', data: [], borderColor: '#2f6fed', borderWidth: 1, tension: 0.2, pointRadius: 0 }] },
      options: {
        responsive: true,
        maintainAspectRatio: false,
        animation: false,
        scales: {
          x: { ticks: { color: '#8a8a8e', maxTicksLimit: 6, font: { size: 9 } }, grid: { color: 'rgba(0,0,0,0.05)' } },
          y: { min: -100, max: -20, ticks: { color: '#8a8a8e', stepSize: 20 }, grid: { color: 'rgba(0,0,0,0.05)' } }
        },
        plugins: { legend: { display: false } }
      },
      plugins: [thresholdBands]
    });

    function colorForRssi(rssi) {
      return rssi >= -60 ? '#34c759' : rssi >= -75 ? '#ffcc00' : '#ff3b30';
    }
    function rssiStatus(rssi) {
      return rssi >= -60 ? { label: 'Good', cls: 'pill-good' }
           : rssi >= -75 ? { label: 'Fair', cls: 'pill-fair' }
           : { label: 'Poor', cls: 'pill-poor' };
    }
    function colorForBattery(pct) {
      return pct >= 50 ? '#34c759' : pct >= 20 ? '#ffcc00' : '#ff3b30';
    }

    // "YYYY-MM-DD HH:MM:SS" -> "dd.mm.yyyy, HH:MM:SS"
    function formatTs(ts) {
      const m = /^(\d{4})-(\d{2})-(\d{2})[ T](\d{2}:\d{2}:\d{2})$/.exec(ts);
      if (!m) return ts;
      return `${m[3]}.${m[2]}.${m[1]}, ${m[4]}`;
    }

    async function pollNow() {
      try {
        const r = await fetch('/data');
        const d = await r.json();
        document.getElementById('rssi').textContent = d.rssi;
        document.getElementById('ts').textContent = formatTs(d.ts);
        document.getElementById('batt').textContent = d.batt;
        document.getElementById('ssid').textContent = d.ssid;

        const rssiPct = Math.max(0, Math.min(100, (d.rssi + 100) / 80 * 100));
        const rssiBar = document.getElementById('rssiBar');
        rssiBar.style.width = rssiPct + '%';
        rssiBar.style.background = colorForRssi(d.rssi);

        const status = rssiStatus(d.rssi);
        const pill = document.getElementById('rssiStatus');
        pill.textContent = status.label;
        pill.className = 'status-pill ' + status.cls;

        const battBar = document.getElementById('battBar');
        battBar.style.width = Math.max(0, Math.min(100, d.batt)) + '%';
        battBar.style.background = colorForBattery(d.batt);
      } catch (e) {}
      setTimeout(pollNow, 2000);
    }

    let historyRaw = []; // [{t: Date, v: number}]
    let rangeHours = 1;

    function fmtTime(d) {
      const time = d.toTimeString().slice(0, 5);
      const date = `${String(d.getDate()).padStart(2, '0')}.${String(d.getMonth() + 1).padStart(2, '0')}`;
      return [time, date];
    }

    function applyRange() {
      if (!historyRaw.length) {
        chart.data.labels = [];
        chart.data.datasets[0].data = [];
        chart.update();
        return;
      }
      const lastT = historyRaw[historyRaw.length - 1].t;
      const cutoff = rangeHours === 'all' ? null : new Date(lastT.getTime() - rangeHours * 3600 * 1000);
      const filtered = cutoff ? historyRaw.filter(p => p.t >= cutoff) : historyRaw;
      chart.data.labels = filtered.map(p => fmtTime(p.t));
      chart.data.datasets[0].data = filtered.map(p => p.v);
      chart.update();
    }

    async function loadHistory() {
      try {
        const r = await fetch('/history');
        const text = await r.text();
        const lines = text.trim().split('\n').filter(l => l.length > 0);
        historyRaw = lines.map(l => {
          const parts = l.split(',');
          return { t: new Date(parts[0].replace(' ', 'T')), v: parseInt(parts[1]) };
        }).filter(p => !isNaN(p.t.getTime()) && !isNaN(p.v));
        applyRange();
      } catch (e) {}
      setTimeout(loadHistory, 30000);
    }

    document.querySelectorAll('.range-picker button').forEach(btn => {
      btn.onclick = () => {
        document.querySelectorAll('.range-picker button').forEach(b => b.classList.remove('active'));
        btn.classList.add('active');
        rangeHours = btn.dataset.h === 'all' ? 'all' : parseInt(btn.dataset.h);
        document.getElementById('rangeLabel').textContent = btn.dataset.h === 'all' ? 'all time' : `last ${btn.dataset.h}h`;
        applyRange();
      };
    });

    pollNow();
    loadHistory();

    const settingsOverlay = document.getElementById('settingsOverlay');
    document.getElementById('settingsBtn').onclick = () => settingsOverlay.classList.add('open');
    document.getElementById('closeSettings').onclick = () => settingsOverlay.classList.remove('open');
    settingsOverlay.onclick = (e) => { if (e.target === settingsOverlay) settingsOverlay.classList.remove('open'); };

    document.getElementById('forgetBtn').onclick = async () => {
      if (!confirm('Forget saved WiFi credentials and restart into setup mode?')) return;
      await fetch('/forget', { method: 'POST' });
      document.body.innerHTML = '<h2 style="font-family:-apple-system,sans-serif;color:#1c1c1e;padding:40px">Restarting into WiFi setup mode...</h2>';
    };
  </script>
</body>
</html>
)HTML";
