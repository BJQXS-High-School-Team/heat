#pragma once

namespace heatrisk {

// TODO(Phase 3): 替换为真实Dashboard HTML（轮询 /api/health,/api/host,/api/devices）
static constexpr const char kDashboardHtml[] = R"HTML(
<!doctype html>
<html>
  <head>
    <meta charset="utf-8" />
    <meta name="viewport" content="width=device-width,initial-scale=1" />
    <title>Heat Risk Gateway Dashboard</title>
  </head>
  <body>
    <h1>Heat Risk Gateway Dashboard (Phase 1 Placeholder)</h1>
    <p>Gateway Ambient and Wearable Status UI will be implemented in Phase 3.</p>
  </body>
</html>
)HTML";

}  // namespace heatrisk
