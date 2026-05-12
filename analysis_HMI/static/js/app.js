(() => {
  const COLORS = {
    bg:         '#11151c',
    grid:       '#1f2533',
    text:       '#ece9de',
    textMuted:  '#7a8290',
    leather:    '#d04848',
    cream:      '#e8b04b',
    sky:        '#5fb3d4',
    refPurple:  '#b87cff',
  };

  const FONT_MONO = "'JetBrains Mono', 'SF Mono', Menlo, monospace";

  // chartId -> { kind: 'angle'|'accel', imu: 'wrist'|'shoulder' }
  const CHARTS = {
    'chart-angle-wrist':    { kind: 'angle', imu: 'wrist'    },
    'chart-angle-shoulder': { kind: 'angle', imu: 'shoulder' },
    'chart-accel-wrist':    { kind: 'accel', imu: 'wrist'    },
    'chart-accel-shoulder': { kind: 'accel', imu: 'shoulder' },
  };

  // chartId -> 'components' | 'magnitude'
  const viewState = Object.fromEntries(
    Object.keys(CHARTS).map(id => [id, 'components'])
  );

  let lastData = null;

  const fileInput  = document.getElementById('file-input');
  const uploadBtn  = document.getElementById('upload-btn');
  const statusDot  = document.getElementById('status-dot');
  const statusLbl  = document.getElementById('status-label');
  const inputAng   = document.getElementById('input-angle');
  const inputAcc   = document.getElementById('input-accel');

  uploadBtn.addEventListener('click', () => fileInput.click());
  fileInput.addEventListener('change', onFilePicked);
  inputAng.addEventListener('input', updateReferenceLines);
  inputAcc.addEventListener('input', updateReferenceLines);

  document.querySelectorAll('.view-toggle').forEach(toggle => {
    const chartId = toggle.dataset.chart;
    toggle.querySelectorAll('.view-toggle__btn').forEach(btn => {
      btn.addEventListener('click', () => {
        const view = btn.dataset.view;
        if (viewState[chartId] === view) return;
        viewState[chartId] = view;
        toggle.querySelectorAll('.view-toggle__btn').forEach(b =>
          b.classList.toggle('is-active', b.dataset.view === view)
        );
        if (lastData) renderChart(chartId);
      });
    });
  });

  function setStatus(state, label) {
    statusDot.className = 'status-dot' + (state ? ' is-' + state : '');
    statusLbl.textContent = label;
  }

  function setPlaceholderState(loading) {
    document.querySelectorAll('.placeholder').forEach(el => {
      el.classList.toggle('is-loading', loading);
      el.textContent = loading ? 'LOADING' : 'AWAITING DATA';
    });
  }

  async function onFilePicked(e) {
    const file = e.target.files[0];
    if (!file) return;

    setStatus('loading', 'PARSING ' + file.name.toUpperCase());
    setPlaceholderState(true);

    const fd = new FormData();
    fd.append('file', file);

    try {
      const res = await fetch('/upload', { method: 'POST', body: fd });
      const json = await res.json();

      if (!res.ok) {
        setStatus(null, 'ERROR · ' + (json.error || 'unknown').toUpperCase());
        setPlaceholderState(false);
        return;
      }

      lastData = json;
      Object.keys(CHARTS).forEach(renderChart);
      setStatus('ready', 'LOADED · ' + file.name.toUpperCase());
    } catch (err) {
      setStatus(null, 'NETWORK ERROR');
      setPlaceholderState(false);
      console.error(err);
    } finally {
      fileInput.value = '';
    }
  }

  function magnitude(a, b, c) {
    const out = new Array(a.length);
    for (let i = 0; i < a.length; i++) {
      out[i] = Math.sqrt(a[i] * a[i] + b[i] * b[i] + c[i] * c[i]);
    }
    return out;
  }

  function lineTrace(name, x, y, color) {
    return {
      type: 'scatter',
      mode: 'lines',
      name,
      x, y,
      line: { color, width: 1.6 },
      hovertemplate: name + ' · %{y:.3f}<extra></extra>',
    };
  }

  function buildTraces(chartId) {
    const { kind, imu } = CHARTS[chartId];
    const data = lastData[imu];
    const view = viewState[chartId];

    if (kind === 'angle') {
      if (view === 'magnitude') {
        const mag = magnitude(data.gx, data.gy, data.gz);
        return [lineTrace('|g|', data.time_s, mag, COLORS.leather)];
      }
      return [
        lineTrace('gx', data.time_s, data.gx, COLORS.leather),
        lineTrace('gy', data.time_s, data.gy, COLORS.cream),
        lineTrace('gz', data.time_s, data.gz, COLORS.sky),
      ];
    }

    // accel
    if (view === 'magnitude') {
      const mag = magnitude(data.ax, data.ay, data.az);
      return [lineTrace('|a|', data.time_s, mag, COLORS.leather)];
    }
    return [
      lineTrace('ax', data.time_s, data.ax, COLORS.leather),
      lineTrace('ay', data.time_s, data.ay, COLORS.cream),
      lineTrace('az', data.time_s, data.az, COLORS.sky),
    ];
  }

  function makeLayout(yTitle) {
    return {
      paper_bgcolor: COLORS.bg,
      plot_bgcolor:  COLORS.bg,
      font: { family: FONT_MONO, size: 11, color: COLORS.textMuted },
      margin: { l: 60, r: 24, t: 16, b: 44 },
      xaxis: {
        title: { text: 'Time', font: { size: 10, color: COLORS.textMuted } },
        gridcolor: COLORS.grid,
        zerolinecolor: COLORS.grid,
        linecolor: COLORS.grid,
        tickfont: { size: 10, color: COLORS.textMuted },
      },
      yaxis: {
        title: { text: yTitle, font: { size: 10, color: COLORS.textMuted } },
        gridcolor: COLORS.grid,
        zerolinecolor: COLORS.grid,
        linecolor: COLORS.grid,
        tickfont: { size: 10, color: COLORS.textMuted },
      },
      legend: {
        orientation: 'h',
        x: 1,
        xanchor: 'right',
        y: 1.08,
        font: { size: 10, color: COLORS.textMuted },
        bgcolor: 'rgba(0,0,0,0)',
      },
      shapes: [],
      hoverlabel: {
        bgcolor: '#0a0d12',
        bordercolor: COLORS.grid,
        font: { family: FONT_MONO, size: 11, color: COLORS.text },
      },
    };
  }

  const CONFIG = {
    displayModeBar: false,
    responsive: true,
  };

  function renderChart(chartId) {
    if (!lastData) return;
    const { kind } = CHARTS[chartId];
    const yTitle = kind === 'angle' ? 'Angle Magnitude' : 'Acceleration Magnitude';
    Plotly.newPlot(chartId, buildTraces(chartId), makeLayout(yTitle), CONFIG);
    hidePlaceholder(chartId);
    applyRefLine(chartId);
  }

  function hidePlaceholder(divId) {
    const ph = document.querySelector('#' + divId + ' .placeholder');
    if (ph) ph.style.display = 'none';
  }

  function refLineShape(yValue) {
    return {
      type: 'line',
      xref: 'paper',
      x0: 0, x1: 1,
      y0: yValue, y1: yValue,
      line: { color: COLORS.refPurple, width: 1.6, dash: 'dash' },
      layer: 'above',
    };
  }

  function applyRefLine(chartId) {
    const { kind } = CHARTS[chartId];
    const raw = kind === 'angle' ? inputAng.value : inputAcc.value;
    const v = parseFloat(raw);
    const shapes = Number.isFinite(v) ? [refLineShape(v)] : [];
    Plotly.relayout(chartId, { shapes });
  }

  function updateReferenceLines() {
    if (!lastData) return;
    Object.keys(CHARTS).forEach(applyRefLine);
  }
})();
