#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
//  dashboard_html.h  –  read-only web dashboard (desktop/mobile)
//  Static markup lives in flash (PROGMEM); all data comes from /api/astro,
//  polled at data.refreshIntervalMs (mirrors API_REFRESH_INTERVAL_MS on-device).
//  Served as-is by WebDashboard.cpp — no templating on the device side.
// ═══════════════════════════════════════════════════════════════════════════════

static const char DASHBOARD_HTML[] PROGMEM = R"HTMLPAGE(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Astronomy Micro Station</title>
<style>
  :root{
    --bg:            #0a0f0c;
    --panel:         #131c17;
    --panel-line:    #23342b;
    --ink:           #e7f2ec;
    --ink-dim:       #7c9186;
    --ink-faint:     #4c5c53;
    --green:         #22e07e;
    --green-dim:     #0f6b3d;
    --cyan:          #33c7ff;
    --amber:         #ffb020;
    --silver:        #c7d1cb;
    --night-band:    #060a08;
    --astro-band:    #0d1c22;
    --naut-band:     #0f2a38;
    --civil-band:    #164a5a;
    --day-band:      #ffb020;
    --focus:         #33c7ff;
  }
  *{ box-sizing:border-box; }
  html,body{ margin:0; padding:0; }
  body{
    background: radial-gradient(120% 140% at 15% -10%, #0d1712 0%, var(--bg) 55%);
    color: var(--ink);
    font-family: -apple-system, "Segoe UI", "Helvetica Neue", system-ui, sans-serif;
    min-height: 100vh;
    padding: 32px 20px 60px;
  }
  @media (max-width: 520px){ body{ padding: 20px 14px 40px; } }
  a{ color:var(--cyan); }
  .mono{
    font-family: ui-monospace, "SF Mono", "Cascadia Code", "JetBrains Mono", Menlo, Consolas, monospace;
    font-variant-numeric: tabular-nums;
  }
  .page{ max-width: 1180px; margin: 0 auto; }

  #redmode{ position:absolute; opacity:0; pointer-events:none; }
  body:has(#redmode:checked){
    --green:#ff5a3c; --green-dim:#5a1f14; --cyan:#ff8a5c; --amber:#ff6a3c; --silver:#ffb89a;
    --ink:#ffd9c4; --ink-dim:#8a5a44; --ink-faint:#4a2e22;
    --panel:#170e09; --panel-line:#3a1f12; --bg:#0c0603;
    --day-band:#ff6a3c; --civil-band:#5a2c14; --naut-band:#3a1a0c; --astro-band:#1e0f08; --night-band:#0a0503;
  }
  body:has(#redmode:checked){ background: radial-gradient(120% 140% at 15% -10%, #170d08 0%, var(--bg) 55%); }

  .toggle-row{ display:flex; align-items:center; gap:10px; font-size:11px; letter-spacing:.08em; text-transform:uppercase; color:var(--ink-dim); }
  .toggle{ position:relative; width:38px; height:20px; border-radius:999px; background:var(--panel); border:1px solid var(--panel-line); cursor:pointer; transition:background .15s ease; }
  .toggle::after{ content:""; position:absolute; top:2px; left:2px; width:14px; height:14px; border-radius:50%; background:var(--ink-dim); transition:transform .15s ease, background .15s ease; }
  body:has(#redmode:checked) .toggle::after{ transform:translateX(18px); background:var(--green); }
  .toggle:has(+ #redmode:focus-visible){ outline:2px solid var(--focus); outline-offset:2px; }
  @media (max-width: 520px){
    .toggle{ width:44px; height:24px; }
    .toggle::after{ width:18px; height:18px; }
    body:has(#redmode:checked) .toggle::after{ transform:translateX(20px); }
  }

  button.ghost{
    font: inherit; font-size:11px; letter-spacing:.08em; text-transform:uppercase;
    color:var(--ink-dim); background:var(--panel); border:1px solid var(--panel-line);
    border-radius:6px; padding:6px 10px; cursor:pointer;
  }
  button.ghost:hover{ color:var(--ink); border-color:var(--green-dim); }
  button.ghost:focus-visible{ outline:2px solid var(--focus); outline-offset:2px; }

  header{ display:flex; align-items:center; justify-content:space-between; gap:16px; padding-bottom:18px; margin-bottom:26px; border-bottom:1px solid var(--panel-line); flex-wrap:wrap; }
  .brand{ display:flex; align-items:center; gap:12px; }
  .brand svg{ width:26px; height:26px; flex:none; }
  .brand-text{ display:flex; flex-direction:column; line-height:1.15; }
  .brand-name{ font-size:15px; font-weight:700; letter-spacing:.06em; color:var(--green); text-transform:uppercase; }
  .brand-sub{ font-size:11px; color:var(--ink-faint); letter-spacing:.04em; }
  .header-right{ display:flex; align-items:center; gap:22px; flex-wrap:wrap; }
  .location{ text-align:right; }
  .location-name{ font-size:13px; font-weight:600; color:var(--silver); }
  .location-time{ font-size:11px; color:var(--ink-faint); }
  @media (max-width: 520px){
    header{ flex-direction:column; align-items:flex-start; gap:14px; }
    .header-right{ width:100%; justify-content:space-between; }
    .location{ text-align:left; }
    .brand-sub{ display:none; }
  }

  .hero{ display:grid; grid-template-columns: 1.1fr 1fr 1fr 1fr; gap:14px; margin-bottom:16px; }
  @media (max-width: 880px){ .hero{ grid-template-columns: 1fr 1fr; } }
  @media (max-width: 520px){ .hero{ grid-template-columns: 1fr; gap:10px; } }

  .card{ background:var(--panel); border:1px solid var(--panel-line); border-radius:10px; padding:18px 20px; }
  @media (max-width: 520px){ .card{ padding:14px 16px; } .stat .card-value{ font-size:26px; } }
  .card-label{ font-size:10.5px; letter-spacing:.1em; text-transform:uppercase; color:var(--ink-dim); margin:0 0 10px; }

  .moon-card{ display:flex; align-items:center; gap:16px; }
  .moon-disc{ width:100px; height:100px; flex:none; }
  .moon-disc svg{ display:block; width:100%; height:100%; filter: grayscale(1); }
  body:has(#redmode:checked) .moon-disc svg{ filter: grayscale(1) sepia(1) hue-rotate(-50deg) saturate(6) brightness(.85); }
  @media (max-width: 520px){ .moon-card{ gap:12px; } .moon-disc{ width:80px; height:80px; } }
  .moon-info .card-value{ font-size:18px; font-weight:700; color:var(--ink); margin-bottom:2px; }
  .moon-info .card-sub{ font-size:12.5px; color:var(--cyan); }

  .stat .card-value{ font-size:30px; font-weight:700; color:var(--ink); letter-spacing:-.01em; }
  .stat .card-value small{ font-size:14px; color:var(--ink-dim); font-weight:500; margin-left:2px; }
  .stat .card-sub{ font-size:12px; color:var(--ink-dim); margin-top:4px; }
  .stat.amber .card-value{ color:var(--amber); }
  .stat.cyan .card-value{ color:var(--cyan); }

  .sky{ margin-bottom:16px; }
  .sky-strip{ position:relative; height:74px; border-radius:8px; margin-top:10px; background: linear-gradient(180deg, transparent 0%, transparent 62%, var(--panel-line) 62%, var(--panel-line) 63%, transparent 63%); border-bottom:1px solid var(--panel-line); }
  @media (max-width: 520px){ .sky-strip{ height:64px; } }
  .sky-tick{ position:absolute; bottom:-18px; transform:translateX(-50%); font-size:10px; color:var(--ink-faint); letter-spacing:.06em; }
  .sky-body{ position:absolute; transform:translate(-50%, 50%); display:flex; flex-direction:column; align-items:center; gap:3px; transition:left .3s ease, bottom .3s ease; }
  .sky-dot{ width:11px; height:11px; border-radius:50%; box-shadow:0 0 10px 2px currentColor; }
  .sky-dot.sun{ background:var(--amber); color:var(--amber); }
  .sky-dot.moon{ background:var(--cyan); color:var(--cyan); }
  .sky-body-label{ font-size:10px; color:var(--ink-dim); white-space:nowrap; }
  @media (max-width: 520px){ .sky-body-label{ font-size:9px; } }

  .timeline{ margin-bottom:16px; }
  .timeline-bar{ position:relative; height:34px; border-radius:6px; overflow:hidden; border:1px solid var(--panel-line); margin-top:14px; }
  .timeline-labels{ position:relative; height:34px; margin-top:6px; }
  .timeline-label{ position:absolute; transform:translateX(-50%); text-align:center; font-size:10.5px; color:var(--ink-dim); white-space:nowrap; }
  .timeline-label .t-time{ display:block; color:var(--ink); font-size:11.5px; margin-top:1px; }
  @media (max-width: 520px){ .timeline-label{ font-size:9.5px; } .timeline-label .t-time{ font-size:10.5px; } }
  .timeline-legend{ display:flex; gap:18px; margin-top:30px; flex-wrap:wrap; font-size:11px; color:var(--ink-dim); }
  .legend-swatch{ display:inline-block; width:10px; height:10px; border-radius:2px; margin-right:6px; vertical-align:-1px; }

  .details{ display:grid; grid-template-columns:1fr 1fr; gap:14px; margin-bottom:16px; }
  @media (max-width: 780px){ .details{ grid-template-columns:1fr; } }
  .panel-title{ display:flex; align-items:center; justify-content:space-between; font-size:11px; letter-spacing:.1em; text-transform:uppercase; color:var(--ink-dim); margin:0 0 12px; padding-bottom:10px; border-bottom:1px solid var(--panel-line); }
  .panel-title .dot{ width:6px; height:6px; border-radius:50%; }
  .row-list{ display:flex; flex-direction:column; }
  .row-list .r{ display:flex; justify-content:space-between; align-items:baseline; padding:7px 0; border-bottom:1px dashed var(--panel-line); font-size:13px; }
  .row-list .r:last-child{ border-bottom:none; }
  .row-list .r .k{ color:var(--ink-dim); }
  .row-list .r .v{ color:var(--ink); font-weight:600; }
  .row-list .r .v.cyan{ color:var(--cyan); }
  .row-list .r .v.amber{ color:var(--amber); }
  @media (max-width: 520px){ .row-list .r{ font-size:12.5px; } }

  .twilight-block{ margin-top:14px; }
  .twilight-block h4{ margin:0 0 6px; font-size:10.5px; letter-spacing:.08em; text-transform:uppercase; color:var(--ink-faint); font-weight:600; }

  footer{ margin-top:26px; padding-top:16px; border-top:1px solid var(--panel-line); display:flex; justify-content:space-between; align-items:center; flex-wrap:wrap; gap:10px; font-size:11px; color:var(--ink-faint); }
  @media (max-width: 520px){ footer{ flex-direction:column; align-items:flex-start; } }

  .location{ margin-bottom:16px; }
  .location-form{ display:flex; gap:14px; align-items:flex-end; flex-wrap:wrap; margin-top:12px; }
  .loc-field{ display:flex; flex-direction:column; gap:6px; font-size:10.5px; letter-spacing:.08em; text-transform:uppercase; color:var(--ink-dim); }
  .loc-field input{
    font: inherit; font-family: ui-monospace, "SF Mono", "Cascadia Code", "JetBrains Mono", Menlo, Consolas, monospace;
    font-size:14px; color:var(--ink); background:var(--bg); border:1px solid var(--panel-line);
    border-radius:6px; padding:8px 10px; width:150px;
  }
  .loc-field input:focus-visible{ outline:2px solid var(--focus); outline-offset:1px; border-color:var(--cyan); }
  button.ghost.primary{ color:var(--bg); background:var(--green); border-color:var(--green); font-weight:700; }
  button.ghost.primary:hover{ filter:brightness(1.1); color:var(--bg); }
  .location-status{ margin:10px 0 0; font-size:12px; color:var(--ink-dim); min-height:1em; }
  .location-status.ok{ color:var(--green); }
  .location-status.err{ color:var(--amber); }
  .location-hint{ margin:8px 0 0; font-size:11px; color:var(--ink-faint); }
  @media (max-width: 520px){ .loc-field input{ width:130px; } }
</style>
</head>
<body>
<div class="page">

  <header>
    <div class="brand">
      <svg viewBox="0 0 24 24" fill="none" aria-hidden="true">
        <ellipse cx="12" cy="12" rx="10" ry="4" stroke="var(--green)" stroke-width="1.4" transform="rotate(28 12 12)"/>
        <circle cx="12" cy="12" r="2.6" fill="var(--green)"/>
      </svg>
      <div class="brand-text">
        <div class="brand-name">Astronomy Micro Station</div>
        <div class="brand-sub mono" id="hostLabel">desktop view</div>
      </div>
    </div>
    <div class="header-right">
      <label class="toggle-row" for="redmode">
        Night vision
        <input type="checkbox" id="redmode">
        <span class="toggle" tabindex="0"></span>
      </label>
      <div class="location">
        <div class="location-name" id="locationName">—</div>
        <div class="location-time mono" id="dateTime">—</div>
      </div>
    </div>
  </header>

  <section class="hero">
    <div class="card moon-card">
      <div class="moon-disc" id="moonDisc" aria-hidden="true"></div>
      <div class="moon-info">
        <p class="card-label">Moon</p>
        <div class="card-value" id="moonPhaseName">—</div>
        <div class="card-sub mono" id="moonIllumTop">—</div>
      </div>
    </div>
    <div class="card stat cyan">
      <p class="card-label">Sun altitude</p>
      <div class="card-value mono"><span id="sunAltValue">—</span><small>°</small></div>
      <div class="card-sub" id="sunAltSub">—</div>
    </div>
    <div class="card stat">
      <p class="card-label">Day length</p>
      <div class="card-value mono" id="dayLengthValue">—</div>
      <div class="card-sub" id="solarNoonSub">—</div>
    </div>
    <div class="card stat amber">
      <p class="card-label">Sunset</p>
      <div class="card-value mono" id="sunsetValue">—</div>
      <div class="card-sub" id="goldenHourSub">—</div>
    </div>
  </section>

  <section class="card sky">
    <p class="card-label">Sky position &middot; azimuth / altitude</p>
    <div class="sky-strip">
      <span class="sky-tick" style="left:0%">N</span>
      <span class="sky-tick" style="left:25%">E</span>
      <span class="sky-tick" style="left:50%">S</span>
      <span class="sky-tick" style="left:75%">W</span>
      <span class="sky-tick" style="left:100%">N</span>
      <div class="sky-body" id="skyMoonBody" style="left:50%; bottom:0%;">
        <span class="sky-dot moon"></span>
        <span class="sky-body-label mono" id="skyMoonLabel">moon —</span>
      </div>
      <div class="sky-body" id="skySunBody" style="left:50%; bottom:0%;">
        <span class="sky-dot sun"></span>
        <span class="sky-body-label mono" id="skySunLabel">sun —</span>
      </div>
    </div>
  </section>

  <section class="card timeline">
    <p class="card-label">Today's light &middot; astronomical &rarr; nautical &rarr; civil &rarr; daylight</p>
    <div class="timeline-bar" id="timelineBar"></div>
    <div class="timeline-labels" id="timelineLabelWrap"></div>
    <div class="timeline-legend">
      <span><span class="legend-swatch" style="background:var(--night-band)"></span>Night</span>
      <span><span class="legend-swatch" style="background:var(--astro-band)"></span>Astronomical</span>
      <span><span class="legend-swatch" style="background:var(--naut-band)"></span>Nautical</span>
      <span><span class="legend-swatch" style="background:var(--civil-band)"></span>Civil</span>
      <span><span class="legend-swatch" style="background:var(--day-band)"></span>Daylight</span>
    </div>
  </section>

  <section class="card location">
    <p class="card-label">Observer location &middot; saved to the device</p>
    <form class="location-form" id="locationForm">
      <label class="loc-field">
        <span>Latitude</span>
        <input type="number" id="latInput" step="0.000001" min="-90" max="90" inputmode="decimal" required>
      </label>
      <label class="loc-field">
        <span>Longitude</span>
        <input type="number" id="lonInput" step="0.000001" min="-180" max="180" inputmode="decimal" required>
      </label>
      <button class="ghost" type="button" id="homeBtn">Use home location</button>
      <button class="ghost primary" type="submit">Save &amp; refresh</button>
    </form>
    <p class="location-status" id="locationStatus"></p>
    <p class="location-hint">The city/province shown top-right is looked up automatically from these coordinates — no need to type it.</p>
  </section>

  <section class="details">
    <div class="card">
      <p class="panel-title"><span>Sun</span><span class="dot" style="background:var(--amber)"></span></p>
      <div class="row-list mono">
        <div class="r"><span class="k">Sunrise</span><span class="v" id="d-sunrise">—</span></div>
        <div class="r"><span class="k">Sunset</span><span class="v" id="d-sunset">—</span></div>
        <div class="r"><span class="k">Solar noon</span><span class="v" id="d-solarnoon">—</span></div>
        <div class="r"><span class="k">Solar midnight</span><span class="v" id="d-midnight">—</span></div>
        <div class="r"><span class="k">Day length</span><span class="v" id="d-daylength">—</span></div>
        <div class="r"><span class="k">Altitude (now)</span><span class="v amber" id="d-sunalt">—</span></div>
        <div class="r"><span class="k">Azimuth (now)</span><span class="v" id="d-sunaz">—</span></div>
        <div class="r"><span class="k">Distance</span><span class="v" id="d-sundist">—</span></div>
      </div>
      <div class="twilight-block">
        <h4>Morning twilight</h4>
        <div class="row-list mono">
          <div class="r"><span class="k">Astronomical</span><span class="v" id="d-m-astro">—</span></div>
          <div class="r"><span class="k">Nautical</span><span class="v" id="d-m-naut">—</span></div>
          <div class="r"><span class="k">Civil</span><span class="v" id="d-m-civil">—</span></div>
          <div class="r"><span class="k">Blue hour</span><span class="v" id="d-m-blue">—</span></div>
          <div class="r"><span class="k">Golden hour</span><span class="v" id="d-m-golden">—</span></div>
        </div>
      </div>
      <div class="twilight-block">
        <h4>Evening twilight</h4>
        <div class="row-list mono">
          <div class="r"><span class="k">Golden hour</span><span class="v" id="d-e-golden">—</span></div>
          <div class="r"><span class="k">Civil</span><span class="v" id="d-e-civil">—</span></div>
          <div class="r"><span class="k">Blue hour</span><span class="v" id="d-e-blue">—</span></div>
          <div class="r"><span class="k">Nautical</span><span class="v" id="d-e-naut">—</span></div>
          <div class="r"><span class="k">Astronomical</span><span class="v" id="d-e-astro">—</span></div>
        </div>
      </div>
    </div>

    <div class="card">
      <p class="panel-title"><span>Moon</span><span class="dot" style="background:var(--cyan)"></span></p>
      <div class="row-list mono">
        <div class="r"><span class="k">Phase</span><span class="v" id="d-phase">—</span></div>
        <div class="r"><span class="k">Illumination</span><span class="v cyan" id="d-illum">—</span></div>
        <div class="r"><span class="k">Moonrise</span><span class="v" id="d-moonrise">—</span></div>
        <div class="r"><span class="k">Moonset</span><span class="v" id="d-moonset">—</span></div>
        <div class="r"><span class="k">Altitude (now)</span><span class="v cyan" id="d-moonalt">—</span></div>
        <div class="r"><span class="k">Azimuth (now)</span><span class="v" id="d-moonaz">—</span></div>
        <div class="r"><span class="k">Moon angle</span><span class="v" id="d-moonangle">—</span></div>
        <div class="r"><span class="k">Distance</span><span class="v" id="d-moondist">—</span></div>
      </div>
      <div class="twilight-block">
        <h4>Night window</h4>
        <div class="row-list mono">
          <div class="r"><span class="k">Night begins</span><span class="v" id="d-nightbegin">—</span></div>
          <div class="r"><span class="k">Night ends</span><span class="v" id="d-nightend">—</span></div>
          <div class="r"><span class="k">Duration</span><span class="v" id="d-nightdur">—</span></div>
        </div>
      </div>
    </div>
  </section>

  <footer>
    <span id="footerStatus">Loading…</span>
    <button class="ghost" id="refreshBtn" type="button">Refresh now</button>
  </footer>

</div>

<script>
(function(){
  var $ = function(id){ return document.getElementById(id); };

  // Same 8 source icons the device renders (assets/moon-phases/*.svg),
  // keyed by the exact phase string the API returns — same mapping as
  // moonPhaseIndex() in include/moon_icons.h. Colour is neutralised via
  // CSS filter (.moon-disc svg) to match the device's greyscale rendering.
  var MOON_SVG = {
    NEW_MOON: '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 512 512"><path fill="none" stroke="#e5e7eb" stroke-dasharray="16.9 56.2" stroke-linecap="round" stroke-linejoin="round" stroke-width="17.4" d="M384 256a128 128 0 00-128-128c-169.8 6.7-169.7 249.3 0 256a128 128 0 00128-128Z"/></svg>',
    WAXING_CRESCENT: '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 512 512"><defs><linearGradient id="a" x1="211.4" x2="330.3" y1="166.7" y2="372.6" gradientUnits="userSpaceOnUse"><stop offset="0" stop-color="#86c3db"/><stop offset=".5" stop-color="#86c3db"/><stop offset="1" stop-color="#5eafcf"/></linearGradient></defs><path fill="none" stroke="#e5e7eb" stroke-dasharray="16.9 56.2" stroke-linecap="round" stroke-linejoin="round" stroke-width="17.4" d="M384 256a128 128 0 00-128-128c-169.8 6.7-169.7 249.3 0 256a128 128 0 00128-128Z"/><path fill="url(#a)" stroke="#72b9d5" stroke-linecap="round" stroke-linejoin="round" stroke-width="6" d="M310.3 127.5a142.3 142.3 0 00-19-7l.7.6h0c92.2 96.7 21 256.7-112.6 252.8l-.8-.1a141.4 141.4 0 0017.8 9.5 140 140 0 10114-255.8Z"/></svg>',
    FIRST_QUARTER: '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 512 512"><defs><linearGradient id="a" x1="193.7" x2="325.5" y1="147.7" y2="376" gradientUnits="userSpaceOnUse"><stop offset="0" stop-color="#86c3db"/><stop offset=".5" stop-color="#86c3db"/><stop offset="1" stop-color="#5eafcf"/></linearGradient></defs><path fill="none" stroke="#e5e7eb" stroke-dasharray="16.9 56.2" stroke-linecap="round" stroke-linejoin="round" stroke-width="17.4" d="M384 256a128 128 0 00-128-128c-169.8 6.7-169.7 249.3 0 256a128 128 0 00128-128Z"/><path fill="url(#a)" stroke="#72b9d5" stroke-linecap="round" stroke-linejoin="round" stroke-width="6" d="M310 126.1a140.4 140.4 0 00-57-12.6 140 140 0 0126 152.1A140 140 0 01148.7 348c14 17.3 25.6 24.1 47.5 33.9 72 32 156 .8 187.4-69.8s-1.5-153.9-73.6-186Z"/></svg>',
    WAXING_GIBBOUS: '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 512 512"><defs><linearGradient id="a" x1="193.9" x2="327.1" y1="143.9" y2="374.7" gradientUnits="userSpaceOnUse"><stop offset="0" stop-color="#86c3db"/><stop offset=".5" stop-color="#86c3db"/><stop offset="1" stop-color="#5eafcf"/></linearGradient></defs><path fill="none" stroke="#e5e7eb" stroke-dasharray="16.9 56.2" stroke-linecap="round" stroke-linejoin="round" stroke-width="17.4" d="M384 256a128 128 0 00-128-128c-169.8 6.7-169.7 249.3 0 256a128 128 0 00128-128Z"/><path fill="url(#a)" stroke="#72b9d5" stroke-linecap="round" stroke-linejoin="round" stroke-width="6" d="M391.4 214.8a134.1 134.1 0 00-155.6-95.1 135.2 135.2 0 0113.8 31.9c20.7 73.2-22 151-95.4 173.8a145.4 145.4 0 01-14.8 3.6c31 52.5 94.7 78.7 156.6 59.6a142.6 142.6 0 0095.4-173.8Z"/></svg>',
    FULL_MOON: '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 512 512"><defs><linearGradient id="a" x1="186" x2="326" y1="134.7" y2="377.3" gradientUnits="userSpaceOnUse"><stop offset="0" stop-color="#86c3db"/><stop offset=".5" stop-color="#86c3db"/><stop offset="1" stop-color="#5eafcf"/></linearGradient></defs><circle cx="256" cy="256" r="140" fill="url(#a)" stroke="#72b9d5" stroke-linecap="round" stroke-linejoin="round" stroke-width="6"/></svg>',
    WANING_GIBBOUS: '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 512 512"><defs><linearGradient id="a" x1="12993.6" x2="13126.8" y1="143.9" y2="374.7" gradientTransform="matrix(-1 0 0 1 13312.32 0)" gradientUnits="userSpaceOnUse"><stop offset="0" stop-color="#86c3db"/><stop offset=".5" stop-color="#86c3db"/><stop offset="1" stop-color="#5eafcf"/></linearGradient></defs><path fill="none" stroke="#e5e7eb" stroke-dasharray="16.9 56.2" stroke-linecap="round" stroke-linejoin="round" stroke-width="17.4" d="M384 256a128 128 0 00-128-128c-169.8 6.7-169.7 249.3 0 256a128 128 0 00128-128Z"/><path fill="url(#a)" stroke="#72b9d5" stroke-linecap="round" stroke-linejoin="round" stroke-width="6" d="M121.2 214.8a134.1 134.1 0 01155.6-95.1 135.6 135.6 0 00-13.8 31.9c-20.7 73.2 22 151 95.4 173.8a145.4 145.4 0 0014.9 3.6 134.6 134.6 0 01-156.7 59.6 142.6 142.6 0 01-95.4-173.8Z"/></svg>',
    LAST_QUARTER: '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 512 512"><defs><linearGradient id="a" x1="12482" x2="12613.8" y1="147.7" y2="376" gradientTransform="matrix(-1 0 0 1 12799.71 0)" gradientUnits="userSpaceOnUse"><stop offset="0" stop-color="#86c3db"/><stop offset=".5" stop-color="#86c3db"/><stop offset="1" stop-color="#5eafcf"/></linearGradient></defs><path fill="none" stroke="#e5e7eb" stroke-dasharray="16.9 56.2" stroke-linecap="round" stroke-linejoin="round" stroke-width="17.4" d="M384 256a128 128 0 00-128-128c-169.8 6.7-169.7 249.3 0 256a128 128 0 00128-128Z"/><path fill="url(#a)" stroke="#72b9d5" stroke-linecap="round" stroke-linejoin="round" stroke-width="6" d="M201.5 126.1a140.3 140.3 0 0157-12.6 140 140 0 00-26.2 152.1A140 140 0 00363 348c-14 17.3-25.7 24.1-47.5 33.9-72.1 32-156 .8-187.5-69.8s1.5-153.9 73.6-186Z"/></svg>',
    WANING_CRESCENT: '<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 512 512"><defs><linearGradient id="a" x1="11988.7" x2="12107.6" y1="166.7" y2="372.6" gradientTransform="matrix(-1 0 0 1 12286.71 0)" gradientUnits="userSpaceOnUse"><stop offset="0" stop-color="#86c3db"/><stop offset=".5" stop-color="#86c3db"/><stop offset="1" stop-color="#5eafcf"/></linearGradient></defs><path fill="none" stroke="#e5e7eb" stroke-dasharray="16.9 56.2" stroke-linecap="round" stroke-linejoin="round" stroke-width="17.4" d="M384 256a128 128 0 00-128-128c-169.8 6.7-169.7 249.3 0 256a128 128 0 00128-128Z"/><path fill="url(#a)" stroke="#72b9d5" stroke-linecap="round" stroke-linejoin="round" stroke-width="6" d="M199 127.5a142.4 142.4 0 0119.2-7l-.8.6h0c-92.2 96.7-21 256.7 112.6 252.8l.8-.1a140 140 0 11-131.7-246.3Z"/></svg>'
  };

  function toMinutes(t){
    if(!t || t === '--:--' || t === '-:-') return null;
    var parts = t.split(':');
    if(parts.length < 2) return null;
    var h = parseInt(parts[0],10), m = parseInt(parts[1],10);
    if(isNaN(h) || isNaN(m)) return null;
    return h*60+m;
  }
  function pct(mins){ return (mins/1440*100).toFixed(2); }
  function fmtKm(n){ return (typeof n === 'number') ? (Math.round(n).toLocaleString('en-US') + ' km') : '—'; }
  function fmtDeg(n){ return (typeof n === 'number') ? (n.toFixed(1) + '°') : '—'; }
  function fmtPct(n){ return (typeof n === 'number') ? (n.toFixed(1) + '%') : '—'; }
  function rangeText(a,b){ if((!a||a==='--:--') && (!b||b==='--:--')) return '—'; return (a||'--:--') + ' – ' + (b||'--:--'); }
  function durationBetween(a,b){
    var s = toMinutes(a), e = toMinutes(b);
    if(s===null || e===null) return '—';
    if(e < s) e += 1440;
    var d = e - s;
    return Math.floor(d/60) + 'h ' + (d%60) + 'm';
  }
  function setText(id, val){ var el = $(id); if(el) el.textContent = val; }

  var pollTimer = null;

  function placeSkyBody(bodyId, labelId, name, az, alt){
    var body = $(bodyId);
    if(!body || typeof az !== 'number' || typeof alt !== 'number') return;
    var left = ((az % 360) + 360) % 360 / 360 * 100;
    var clampedAlt = Math.max(0, Math.min(90, alt));
    var bottom = (clampedAlt / 90) * 80;
    body.style.left = left.toFixed(1) + '%';
    body.style.bottom = bottom.toFixed(1) + '%';
    body.style.opacity = alt < 0 ? '0.35' : '1';
    setText(labelId, name + ' ' + Math.round(az) + '°' + (alt < 0 ? ' (below horizon)' : ''));
  }

  function renderTimeline(sun){
    var bar = $('timelineBar');
    if(!bar) return;

    var sunrise = toMinutes(sun.sunrise);
    var sunset  = toMinutes(sun.sunset);
    var solarNoon = toMinutes(sun.solarNoon);
    if(sunrise === null || sunset === null) return;

    var raw = [
      toMinutes(sun.morning.astronomicalBegin),
      toMinutes(sun.morning.nauticalBegin),
      toMinutes(sun.morning.civilBegin),
      sunrise, sunset,
      toMinutes(sun.evening.civilEnd),
      toMinutes(sun.evening.nauticalEnd),
      toMinutes(sun.evening.astronomicalEnd)
    ];
    // Near midsummer some twilight boundaries never occur ("--:--"); collapse
    // those bands into the daylight edge rather than leave a gap.
    var marks = raw.map(function(m, i){ return m === null ? (i < 4 ? sunrise : sunset) : m; });

    var bands = ['var(--night-band)','var(--astro-band)','var(--naut-band)','var(--civil-band)',
                 'var(--day-band)','var(--civil-band)','var(--naut-band)','var(--astro-band)','var(--night-band)'];
    var stops = [0].concat(marks.map(pct)).concat([100]);
    var css = 'linear-gradient(90deg';
    for(var i=0;i<bands.length;i++){
      css += ', ' + bands[i] + ' ' + stops[i] + '%, ' + bands[i] + ' ' + stops[i+1] + '%';
    }
    css += ')';
    bar.style.background = css;

    var labelWrap = $('timelineLabelWrap');
    if(labelWrap){
      labelWrap.innerHTML = '';
      var items = [['Sunrise', sunrise, sun.sunrise], ['Solar noon', solarNoon, sun.solarNoon], ['Sunset', sunset, sun.sunset]];
      items.forEach(function(item){
        if(item[1] === null) return;
        var d = document.createElement('div');
        d.className = 'timeline-label';
        d.style.left = pct(item[1]) + '%';
        d.innerHTML = item[0] + '<span class="t-time mono">' + item[2] + '</span>';
        labelWrap.appendChild(d);
      });
    }
  }

  function render(data){
    if(!data || data.valid === false){
      setText('footerStatus', 'Waiting for the device to complete its first astronomy fetch…');
      return;
    }
    var sun = data.sun, moon = data.moon;

    setText('hostLabel', 'device ' + location.host + ' · desktop view');
    setText('dateTime', (data.fetchDate || '') + ' · ' + (data.currentTime || ''));
    if(data.locationName) setText('locationName', data.locationName);

    setText('moonPhaseName', moon.phaseName || moon.phase || '—');
    setText('moonIllumTop', fmtPct(moon.illumination) + ' illuminated');
    var disc = $('moonDisc');
    if(disc) disc.innerHTML = MOON_SVG[moon.phase] || MOON_SVG.NEW_MOON;

    setText('sunAltValue', typeof sun.altitude === 'number' ? sun.altitude.toFixed(1) : '—');
    setText('sunAltSub', 'azimuth ' + fmtDeg(sun.azimuth) + ' · ' + (sun.altitude >= 0 ? 'above horizon' : 'below horizon'));

    setText('dayLengthValue', sun.dayLength || '—');
    setText('solarNoonSub', 'solar noon ' + (sun.solarNoon || '—'));

    setText('sunsetValue', sun.sunset || '—');
    setText('goldenHourSub', 'golden hour began ' + (sun.evening && sun.evening.goldenHourBegin || '—'));

    placeSkyBody('skyMoonBody', 'skyMoonLabel', 'moon', moon.azimuth, moon.altitude);
    placeSkyBody('skySunBody', 'skySunLabel', 'sun', sun.azimuth, sun.altitude);

    renderTimeline(sun);

    setText('d-sunrise', sun.sunrise || '—');
    setText('d-sunset', sun.sunset || '—');
    setText('d-solarnoon', sun.solarNoon || '—');
    setText('d-midnight', sun.midNight || '—');
    setText('d-daylength', sun.dayLength || '—');
    setText('d-sunalt', fmtDeg(sun.altitude));
    setText('d-sunaz', fmtDeg(sun.azimuth));
    setText('d-sundist', fmtKm(sun.distanceKm));

    setText('d-m-astro', rangeText(sun.morning.astronomicalBegin, sun.morning.astronomicalEnd));
    setText('d-m-naut', rangeText(sun.morning.nauticalBegin, sun.morning.nauticalEnd));
    setText('d-m-civil', rangeText(sun.morning.civilBegin, sun.morning.civilEnd));
    setText('d-m-blue', rangeText(sun.morning.blueHourBegin, sun.morning.blueHourEnd));
    setText('d-m-golden', rangeText(sun.morning.goldenHourBegin, sun.morning.goldenHourEnd));

    setText('d-e-golden', rangeText(sun.evening.goldenHourBegin, sun.evening.goldenHourEnd));
    setText('d-e-civil', rangeText(sun.evening.civilBegin, sun.evening.civilEnd));
    setText('d-e-blue', rangeText(sun.evening.blueHourBegin, sun.evening.blueHourEnd));
    setText('d-e-naut', rangeText(sun.evening.nauticalBegin, sun.evening.nauticalEnd));
    setText('d-e-astro', rangeText(sun.evening.astronomicalBegin, sun.evening.astronomicalEnd));

    setText('d-phase', moon.phaseName || moon.phase || '—');
    setText('d-illum', fmtPct(moon.illumination));
    setText('d-moonrise', moon.moonrise || '—');
    setText('d-moonset', moon.moonset || '—');
    setText('d-moonalt', fmtDeg(moon.altitude));
    setText('d-moonaz', fmtDeg(moon.azimuth));
    setText('d-moonangle', fmtDeg(moon.angle));
    setText('d-moondist', fmtKm(moon.distanceKm));

    setText('d-nightbegin', sun.nightBegin || '—');
    setText('d-nightend', sun.nightEnd || '—');
    setText('d-nightdur', durationBetween(sun.nightBegin, sun.nightEnd));

    var mins = Math.round((data.refreshIntervalMs || 900000) / 60000);
    setText('footerStatus', 'Last device fetch ' + (data.fetchDate || '') + ' ' + (data.currentTime || '') +
      ' · page polls every ' + mins + ' min, synced to the device’s own refresh');
  }

  function poll(){
    fetch('/api/astro', {cache:'no-store'})
      .then(function(r){ return r.json(); })
      .then(function(data){
        render(data);
        var interval = data.refreshIntervalMs || 900000;
        clearTimeout(pollTimer);
        pollTimer = setTimeout(poll, interval);
      })
      .catch(function(){
        setText('footerStatus', 'Could not reach the device — retrying…');
        clearTimeout(pollTimer);
        pollTimer = setTimeout(poll, 30000);
      });
  }

  var btn = $('refreshBtn');
  if(btn) btn.addEventListener('click', poll);

  function loadLocation(){
    fetch('/api/location', {cache:'no-store'})
      .then(function(r){ return r.json(); })
      .then(function(loc){
        var lat = $('latInput'), lon = $('lonInput');
        if(lat && document.activeElement !== lat) lat.value = loc.lat;
        if(lon && document.activeElement !== lon) lon.value = loc.lon;
      })
      .catch(function(){});
  }

  var locForm = $('locationForm');
  if(locForm){
    locForm.addEventListener('submit', function(e){
      e.preventDefault();
      var lat = parseFloat($('latInput').value);
      var lon = parseFloat($('lonInput').value);
      var status = $('locationStatus');
      if(isNaN(lat) || isNaN(lon) || lat < -90 || lat > 90 || lon < -180 || lon > 180){
        status.textContent = 'Latitude must be -90..90, longitude -180..180.';
        status.className = 'location-status err';
        return;
      }
      status.textContent = 'Saving…';
      status.className = 'location-status';
      fetch('/api/location', {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify({lat: lat, lon: lon})
      })
        .then(function(r){ return r.json().then(function(body){ return {ok: r.ok, body: body}; }); })
        .then(function(res){
          if(!res.ok){
            status.textContent = (res.body && res.body.error) || 'Save failed.';
            status.className = 'location-status err';
            return;
          }
          status.textContent = 'Saved — refreshing astronomy data for the new location…';
          status.className = 'location-status ok';
          poll();
        })
        .catch(function(){
          status.textContent = 'Could not reach the device.';
          status.className = 'location-status err';
        });
    });
  }

  var homeBtn = $('homeBtn');
  if(homeBtn){
    homeBtn.addEventListener('click', function(){
      var status = $('locationStatus');
      status.textContent = 'Setting home location…';
      status.className = 'location-status';
      fetch('/api/location/home', {method: 'POST'})
        .then(function(r){ return r.json().then(function(body){ return {ok: r.ok, body: body}; }); })
        .then(function(res){
          if(!res.ok){
            status.textContent = (res.body && res.body.error) || 'Failed to set home location.';
            status.className = 'location-status err';
            return;
          }
          status.textContent = 'Home location set — refreshing astronomy data…';
          status.className = 'location-status ok';
          loadLocation();
          poll();
        })
        .catch(function(){
          status.textContent = 'Could not reach the device.';
          status.className = 'location-status err';
        });
    });
  }

  loadLocation();
  poll();
})();
</script>
</body>
</html>
)HTMLPAGE";
