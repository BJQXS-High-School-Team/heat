#pragma once

namespace heatrisk {

static constexpr const char kDashboardHtml[] = R"HTML(
<!doctype html>
<html>
  <head>
    <meta charset="utf-8" />
    <meta name="viewport" content="width=device-width,initial-scale=1" />
    <title>Heat Risk Gateway Dashboard</title>
    <style>
      :root {
        --normal: #1f9d55;
        --warning: #d69e2e;
        --danger: #c53030;
        --offline: #718096;
        --bg: #f7fafc;
        --panel: #ffffff;
        --text: #1a202c;
      }
      body { margin: 0; padding: 16px; background: var(--bg); color: var(--text); font-family: "Segoe UI", Arial, sans-serif; }
      h1 { margin: 0 0 12px 0; font-size: 22px; }
      h2 { margin: 0 0 8px 0; font-size: 16px; }
      .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(280px, 1fr)); gap: 12px; }
      .panel { background: var(--panel); border-radius: 10px; padding: 12px; box-shadow: 0 1px 2px rgba(0,0,0,0.08); }
      .muted { color: #4a5568; font-size: 12px; }
      .devices { margin-top: 12px; display: grid; gap: 10px; }
      .device { border-radius: 8px; padding: 10px; border-left: 6px solid var(--offline); background: #fff; }
      .state-normal { border-left-color: var(--normal); }
      .state-warning { border-left-color: var(--warning); }
      .state-danger { border-left-color: var(--danger); }
      .state-offline { border-left-color: var(--offline); opacity: 0.9; }
      .row { display: flex; justify-content: space-between; gap: 8px; flex-wrap: wrap; font-size: 13px; }
      .kv { margin: 2px 0; }
    </style>
  </head>
  <body>
    <h1>Extreme-Heat Gateway Dashboard</h1>
    <div class="grid">
      <div class="panel">
        <h2>Gateway Status</h2>
        <div id="health" class="muted">loading...</div>
      </div>
      <div class="panel">
        <h2>Gateway Ambient (Host-Side Only)</h2>
        <div id="ambient" class="muted">loading...</div>
      </div>
    </div>
    <div class="panel" style="margin-top:12px;">
      <h2>Wearables</h2>
      <div class="muted">Note: Gateway ambient data is not wearable ambient data.</div>
      <div id="devices" class="devices"></div>
    </div>
    <script>
      function esc(v) { return (v === undefined || v === null) ? "" : String(v); }
      function stateClass(d) {
        if (!d.online) return "state-offline";
        const s = esc(d.status && d.status.effective_state);
        if (s === "danger") return "state-danger";
        if (s === "warning") return "state-warning";
        return "state-normal";
      }
      async function refresh() {
        try {
          const [h, host, dev] = await Promise.all([
            fetch("/api/health").then(r => r.json()),
            fetch("/api/host").then(r => r.json()),
            fetch("/api/devices").then(r => r.json())
          ]);
          document.getElementById("health").innerHTML =
            "WiFi: " + (h.wifi_connected ? "connected" : "disconnected") +
            " | MQTT: " + (h.mqtt_connected ? "connected" : "disconnected") +
            " | Uptime(ms): " + esc(h.uptime_ms) +
            " | Devices: " + esc(h.device_count);

          const a = host.gateway_ambient || {};
          document.getElementById("ambient").innerHTML =
            "Temperature: " + esc(a.temperature_c) + " C<br/>" +
            "Humidity: " + esc(a.humidity_pct) + " %<br/>" +
            "Light: " + esc(a.light_lux) + " lux";

          const list = (dev.devices || []);
          const root = document.getElementById("devices");
          if (!list.length) {
            root.innerHTML = '<div class="muted">No wearable uploads yet.</div>';
            return;
          }
          root.innerHTML = list.map(d => {
            const gps = d.gps || {};
            const p = d.physiology || {};
            const m = d.motion || {};
            const st = d.status || {};
            return `
              <div class="device ${stateClass(d)}">
                <div class="row"><strong>${esc(d.device_id)}</strong><span>${d.online ? "online" : "offline"}</span></div>
                <div class="kv">last_seen_ms_ago: ${esc(d.last_seen_ms_ago)}</div>
                <div class="kv">HR: ${esc(p.heart_rate_bpm)} bpm | SpO2: ${esc(p.spo2_pct)} % | Temp: ${esc(p.body_temp_c)} C</div>
                <div class="kv">Activity: ${esc(m.activity_score)} (${esc(m.activity_level)})</div>
                <div class="kv">GPS: ${gps.has_fix ? "fix" : "no-fix"}, sat=${esc(gps.satellites)}, lat=${esc(gps.lat)}, lng=${esc(gps.lng)}</div>
                <div class="kv">local=${esc(st.local_state)} | effective=${esc(st.effective_state)} | alarm=${esc(st.alarm_active)}</div>
              </div>`;
          }).join("");
        } catch (e) {
          console.error(e);
        }
      }
      refresh();
      setInterval(refresh, 1500);
    </script>
  </body>
</html>
)HTML";

}  // namespace heatrisk
