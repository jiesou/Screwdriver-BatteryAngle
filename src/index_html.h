#ifndef INDEX_HTML_H
#define INDEX_HTML_H

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>AP Mode Selection</title>
  <style>
    .input-group { display: none; margin: 10px 0; }
    .input-group label { display: block; }
    #status { margin-top: 20px; font-weight: bold; color: red; }
  </style>
  <script>
    function toggleInput(mode) {
      document.getElementById("otaAP").style.display = mode === 'ota' ? 'block' : 'none';
      document.getElementById("workAP").style.display = mode === 'work' ? 'block' : 'none';
    }

    function loadConfig() {
      fetch('/get_config')
        .then(response => response.json())
        .then(data => {
          if (data.ssid) document.getElementById("otaSSID").value = data.ssid;
          if (data.password) document.getElementById("otaPassword").value = data.password;
        })
        .catch(error => console.error("Error loading config:", error));
    }

    function submitForm() {
      const selectedMode = document.querySelector('input[name="apMode"]:checked').value;
      const ssid = document.getElementById(`${selectedMode}SSID`).value;
      const password = document.getElementById(`${selectedMode}Password`).value;

      fetch('/set_ap', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ mode: selectedMode, ssid, password })
      })
      .then(response => response.json())
      .then(data => alert(data.message))
      .catch(error => console.error("Error:", error));
    }

    async function readStatus() {
      const response = await fetch('/status')
      const reader = response.body.getReader();
      const decoder = new TextDecoder();
      let buffer = '';

      while (true) {
        const { done, value } = await reader.read();
        if (done) break;
        buffer += decoder.decode(value, { stream: true });
        const lines = buffer.split('\n');
        buffer = lines.pop();

        for (const line of lines) {
          try {
            const data = JSON.parse(line.replace(/^data: /, ''));
            const status = document.getElementById("status");
            let message = "Waiting for STA connection...";
            let color = "orange";
            
            if (data.sta_connected) {
            message = `STA connected! IP: ${data.ip}`;
            color = "green";
            } else if (data.sta_error) {
            message = "STA connection failed!";
            color = "red";
            }
  
            message += ` (频率: ${data.frequency})`;
            message += ` (${data.btn_pressed ? "按钮按下" : "按钮未按下"})`;
            
            status.textContent = message;
            status.style.color = color;
          } catch (error) {
            console.error("Error parsing status:", error);
          }
        }
      }
    }

    window.onload = loadConfig; // 页面开启就去读上一个正确配置
    readStatus(); // 页面开启就去读状态
  </script>
</head>
<body>
  <h2>Select AP Mode</h2>
  <div>
    <input type="radio" id="otaMode" name="apMode" value="ota" onchange="toggleInput('ota')">
    <label for="otaMode">通用AP (OTA烧录)</label>
    <div id="otaAP" class="input-group">
      <label for="otaSSID">SSID:</label>
      <input type="text" id="otaSSID">
      <label for="otaPassword">Password:</label>
      <input type="password" id="otaPassword">
    </div>

    <input type="radio" id="workMode" name="apMode" value="work" onchange="toggleInput('work')">
    <label for="workMode">工作AP (联网)</label>
    <div id="workAP" class="input-group">
      <label for="workSSID">SSID:</label>
      <input type="text" id="workSSID">
      <label for="workPassword">Password:</label>
      <input type="password" id="workPassword">
    </div>

    <button onclick="submitForm()">提交</button>
  </div>
  <div id="status">Waiting for STA connection...</div>
</body>
</html>
)rawliteral";

#endif // INDEX_HTML_H
