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
  <meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
  <meta name="apple-mobile-web-app-title" content="WLAN Quality">
  <meta name="theme-color" content="#111111">
  <link rel="icon" href="/icon.png">
  <link rel="apple-touch-icon" href="/icon.png">

  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <style>
    html { background: #111; height: 100%; }
    body {
      font-family: monospace;
      background: #111;
      color: #eee;
      min-height: 100vh;
      min-height: 100dvh;
      margin: 0;
      padding: max(20px, env(safe-area-inset-top)) max(20px, env(safe-area-inset-right))
               max(20px, env(safe-area-inset-bottom)) max(20px, env(safe-area-inset-left));
      box-sizing: border-box;
      overscroll-behavior-y: none;
      -webkit-tap-highlight-color: transparent;
    }
    h1 { font-size: 1.2em; }
    .now { font-size: 1.4em; margin: 10px 0 20px; }
    .now span { font-weight: bold; }
    .chart-box { position: relative; width: 100%; max-width: 500px; height: 250px; background: #1a1a1a; border-radius: 8px; }
    .dot { display: inline-block; width: 14px; height: 14px; border-radius: 50%; margin-right: 8px; background: #555; vertical-align: middle; }
    .legend { font-size: 0.85em; color: #888; margin-bottom: 20px; }
    .legend .dot { width: 10px; height: 10px; margin-right: 4px; }
    @media (max-width: 400px) {
      .now { font-size: 1.2em; }
    }
  </style>
</head>
<body>
  <h1>Cardputer Adv - WLAN Signal Quality</h1>
  <div style="font-size: 1.2em; margin-bottom: 10px;">Device IP: <span id="ip">-</span></div>
  <div class="now"><span class="dot" id="dot"></span>RSSI: <span id="rssi">-</span> dBm (<span id="ts">-</span>)</div>
  <div class="now"><span class="dot" id="battDot"></span>Battery: <span id="batt">-</span> %</div>
  <div class="legend">
    <span class="dot" style="background:#2ecc40"></span>&ge; -60 dBm &nbsp;
    <span class="dot" style="background:#ffdc00"></span>-60 to -75 dBm &nbsp;
    <span class="dot" style="background:#ff4136"></span>&lt; -75 dBm
  </div>
  <div class="chart-box"><canvas id="chart"></canvas></div>
  <div class="now" style="margin-top: 20px;">
    <button id="forgetBtn" style="background:#331111; color:#ff4136; border:1px solid #662222; padding:8px 14px; border-radius:6px; font-family:monospace; cursor:pointer;">Forget WiFi</button>
  </div>
  <script>
    // Draws shaded good/ok/weak bands behind the line plus dashed threshold
    // lines at -60 and -75 dBm, so the zones are visible at a glance instead
    // of only being explained in the legend below the chart.
    const thresholdBands = {
      id: 'thresholdBands',
      beforeDatasetsDraw(chart) {
        const { ctx, chartArea, scales: { y } } = chart;
        if (!chartArea) return;
        const { left, right, top, bottom } = chartArea;
        const goodY = y.getPixelForValue(-60);
        const okY = y.getPixelForValue(-75);

        ctx.save();
        ctx.fillStyle = 'rgba(46,204,64,0.10)';
        ctx.fillRect(left, top, right - left, goodY - top);
        ctx.fillStyle = 'rgba(255,220,0,0.08)';
        ctx.fillRect(left, goodY, right - left, okY - goodY);
        ctx.fillStyle = 'rgba(255,65,54,0.08)';
        ctx.fillRect(left, okY, right - left, bottom - okY);

        ctx.setLineDash([4, 4]);
        ctx.lineWidth = 1;
        ctx.strokeStyle = 'rgba(46,204,64,0.7)';
        ctx.beginPath(); ctx.moveTo(left, goodY); ctx.lineTo(right, goodY); ctx.stroke();
        ctx.strokeStyle = 'rgba(255,65,54,0.7)';
        ctx.beginPath(); ctx.moveTo(left, okY); ctx.lineTo(right, okY); ctx.stroke();
        ctx.setLineDash([]);

        ctx.font = '10px monospace';
        ctx.textAlign = 'right';
        ctx.fillStyle = '#2ecc40';
        ctx.fillText('-60 dBm', right - 4, goodY - 4);
        ctx.fillStyle = '#ff4136';
        ctx.fillText('-75 dBm', right - 4, okY + 12);
        ctx.restore();
      }
    };

    const ctx = document.getElementById('chart').getContext('2d');
    const chart = new Chart(ctx, {
      type: 'line',
      data: { labels: [], datasets: [{ label: 'RSSI (dBm)', data: [], borderColor: '#4ea1ff', borderWidth: 2, tension: 0.2, pointRadius: 0 }] },
      options: {
        responsive: true,
        maintainAspectRatio: false,
        animation: false,
        scales: {
          x: { ticks: { color: '#888', maxTicksLimit: 8 } },
          y: { min: -100, max: -20, ticks: { color: '#888', stepSize: 10 } }
        },
        plugins: { legend: { labels: { color: '#ccc' } } }
      },
      plugins: [thresholdBands]
    });

    async function pollNow() {
      try {
        const r = await fetch('/data');
        const d = await r.json();
        document.getElementById('rssi').textContent = d.rssi;
        document.getElementById('ts').textContent = d.ts;
        document.getElementById('batt').textContent = d.batt;
        document.getElementById('ip').textContent = d.ip;
        const dot = document.getElementById('dot');
        dot.style.background = d.rssi >= -60 ? '#2ecc40' : d.rssi >= -75 ? '#ffdc00' : '#ff4136';
        const battDot = document.getElementById('battDot');
        battDot.style.background = d.batt >= 50 ? '#2ecc40' : d.batt >= 20 ? '#ffdc00' : '#ff4136';
      } catch (e) {}
      setTimeout(pollNow, 2000);
    }

    async function loadHistory() {
      try {
        const r = await fetch('/history');
        const text = await r.text();
        const lines = text.trim().split('\n').filter(l => l.length > 0);
        chart.data.labels = lines.map(l => l.split(',')[0].slice(11)); // time only
        chart.data.datasets[0].data = lines.map(l => parseInt(l.split(',')[1]));
        chart.update();
      } catch (e) {}
      setTimeout(loadHistory, 30000);
    }

    pollNow();
    loadHistory();

    document.getElementById('forgetBtn').onclick = async () => {
      if (!confirm('Forget saved WiFi credentials and restart into setup mode?')) return;
      await fetch('/forget', { method: 'POST' });
      document.body.innerHTML = '<h2 style="font-family:monospace;color:#eee;padding:40px">Restarting into WiFi setup mode...</h2>';
    };
  </script>
</body>
</html>
)HTML";
