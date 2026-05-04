(() => {
  const COLORS = {
    bg:         '#11151c',
    grid:       '#1f2533',
    text:       '#ece9de',
    textMuted:  '#7a8290',
    leather:    '#d04848',
    cream:      '#e8b04b',
    sky:        '#5fb3d4',
  };

  const FONT_MONO = "'JetBrains Mono', 'SF Mono', Menlo, monospace";

  const CHARTS = {
    angleWrist:    { id: 'chart-angle-wrist',    kind: 'angle' },
    angleShoulder: { id: 'chart-angle-shoulder', kind: 'angle' },
    accelWrist:    { id: 'chart-accel-wrist',    kind: 'accel' },
    accelShoulder: { id: 'chart-accel-shoulder', kind: 'accel' },
  };

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
      renderAll(json);
      setStatus('ready', 'LOADED · ' + file.name.toUpperCase());
    } catch (err) {
      setStatus(null, 'NETWORK ERROR');
      setPlaceholderState(false);
      console.error(err);
    } finally {
      fileInput.value = '';
    }
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

  function renderAngle(divId, imuData) {
    const traces = [
      lineTrace('gx', imuData.time_s, imuData.gx, COLORS.leather),
      lineTrace('gy', imuData.time_s, imuData.gy, COLORS.cream),
      lineTrace('gz', imuData.time_s, imuData.gz, COLORS.sky),
    ];
    Plotly.newPlot(divId, traces, makeLayout('Angle Magnitude'), CONFIG);
    hidePlaceholder(divId);
  }

  function renderAccel(divId, imuData) {
    const traces = [
      lineTrace('ax', imuData.time_s, imuData.ax, COLORS.leather),
      lineTrace('ay', imuData.time_s, imuData.ay, COLORS.cream),
      lineTrace('az', imuData.time_s, imuData.az, COLORS.sky),
    ];
    Plotly.newPlot(divId, traces, makeLayout('Acceleration Magnitude'), CONFIG);
    hidePlaceholder(divId);
  }

  function hidePlaceholder(divId) {
    const ph = document.querySelector('#' + divId + ' .placeholder');
    if (ph) ph.style.display = 'none';
  }

  function renderAll(data) {
    renderAngle(CHARTS.angleWrist.id,    data.wrist);
    renderAngle(CHARTS.angleShoulder.id, data.shoulder);
    renderAccel(CHARTS.accelWrist.id,    data.wrist);
    renderAccel(CHARTS.accelShoulder.id, data.shoulder);
    updateReferenceLines();
  }

  function refLineShape(yValue) {
    return {
      type: 'line',
      xref: 'paper',
      x0: 0, x1: 1,
      y0: yValue, y1: yValue,
      line: { color: COLORS.leather, width: 1.4, dash: 'dash' },
      layer: 'above',
    };
  }

  function updateReferenceLines() {
    if (!lastData) return;

    const angVal = parseFloat(inputAng.value);
    const accVal = parseFloat(inputAcc.value);

    const angShapes = Number.isFinite(angVal) ? [refLineShape(angVal)] : [];
    const accShapes = Number.isFinite(accVal) ? [refLineShape(accVal)] : [];

    Plotly.relayout(CHARTS.angleWrist.id,    { shapes: angShapes });
    Plotly.relayout(CHARTS.angleShoulder.id, { shapes: angShapes });
    Plotly.relayout(CHARTS.accelWrist.id,    { shapes: accShapes });
    Plotly.relayout(CHARTS.accelShoulder.id, { shapes: accShapes });
  }
})();
