#include "ap_html.h"

// Store HTML in PROGMEM to save RAM
const char SETUP_PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 Water System Setup</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Arial, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            padding: 20px;
        }
        .container {
            background: white;
            border-radius: 12px;
            box-shadow: 0 10px 40px rgba(0,0,0,0.2);
            max-width: 600px;
            margin: 0 auto;
            overflow: hidden;
        }
        .header {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 30px 20px;
            text-align: center;
        }
        .header h1 {
            font-size: 24px;
            margin-bottom: 8px;
        }
        .header p {
            font-size: 14px;
            opacity: 0.9;
        }
        .warning {
            background: #fff3cd;
            border-left: 4px solid #ffc107;
            padding: 15px;
            margin: 20px;
            border-radius: 4px;
        }
        .warning strong {
            color: #856404;
            display: block;
            margin-bottom: 5px;
        }
        .warning p {
            color: #856404;
            font-size: 13px;
            line-height: 1.5;
        }
        .content {
            padding: 30px 20px;
        }
        .form-group {
            margin-bottom: 20px;
        }
        .form-group label {
            display: block;
            color: #333;
            font-weight: 600;
            margin-bottom: 8px;
            font-size: 14px;
        }
        .form-group input,
        .form-group select {
            width: 100%;
            padding: 12px;
            border: 2px solid #ddd;
            border-radius: 8px;
            font-size: 14px;
            transition: border-color 0.3s;
        }
        .form-group input:focus,
        .form-group select:focus {
            outline: none;
            border-color: #667eea;
        }
        .form-group small {
            display: block;
            color: #666;
            font-size: 12px;
            margin-top: 5px;
        }
        .error {
            color: #d32f2f;
            font-size: 12px;
            margin-top: 5px;
            display: none;
        }
        .form-group.has-error input,
        .form-group.has-error select {
            border-color: #d32f2f;
        }
        .form-group.has-error .error {
            display: block;
        }
        .btn {
            width: 100%;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            padding: 15px;
            font-size: 16px;
            font-weight: 600;
            border-radius: 8px;
            cursor: pointer;
            transition: transform 0.2s;
        }
        .btn:hover {
            transform: translateY(-2px);
        }
        .btn:disabled {
            opacity: 0.6;
            cursor: not-allowed;
            transform: none;
        }
        .btn-secondary {
            background: #6c757d;
            margin-bottom: 10px;
        }
        .spinner {
            display: inline-block;
            width: 14px;
            height: 14px;
            border: 2px solid rgba(255,255,255,0.3);
            border-top-color: white;
            border-radius: 50%;
            animation: spin 0.8s linear infinite;
        }
        @keyframes spin {
            to { transform: rotate(360deg); }
        }
        .hidden {
            display: none;
        }
        .wifi-list {
            max-height: 200px;
            overflow-y: auto;
            border: 2px solid #ddd;
            border-radius: 8px;
            margin-bottom: 10px;
        }
        .wifi-item {
            padding: 12px;
            border-bottom: 1px solid #eee;
            cursor: pointer;
            display: flex;
            justify-content: space-between;
            align-items: center;
            transition: background 0.2s;
        }
        .wifi-item:last-child {
            border-bottom: none;
        }
        .wifi-item:hover {
            background: #f5f5f5;
        }
        .wifi-item.selected {
            background: #e3f2fd;
            border-left: 4px solid #2196f3;
        }
        .wifi-name {
            font-weight: 600;
            color: #333;
        }
        .wifi-signal {
            font-size: 12px;
            color: #666;
        }
        .footer {
            padding: 20px;
            text-align: center;
            color: #999;
            font-size: 12px;
            border-top: 1px solid #eee;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>Top Off Water System</h1>
            <p>Device Configuration</p>
        </div>
        
        <div class="warning">
            <strong>⚠️ Security Notice</strong>
            <p>This connection is not encrypted. Only configure in a trusted environment.</p>
        </div>
        
        <div class="content">
            <form id="configForm">
                <!-- Device Name -->
                <div class="form-group">
                    <label for="device_name">Device Name *</label>
                    <input type="text" id="device_name" name="device_name" 
                           placeholder="device" 
                           pattern="[a-zA-Z0-9_-]{3,32}"
                           required>
                    <small>3-32 characters: letters, numbers, underscore, hyphen</small>
                    <span class="error">Invalid device name format</span>
                </div>
                
                <!-- WiFi Network Selection -->
                <div class="form-group">
                    <label>WiFi Network *</label>
                    <button type="button" class="btn btn-secondary" id="scanBtn" onclick="scanNetworks()">
                        <span id="scanBtnText">Scan for Networks</span>
                        <span class="spinner hidden" id="scanSpinner"></span>
                    </button>
                    <div class="wifi-list hidden" id="wifiList"></div>
                    <input type="text" id="wifi_ssid" name="wifi_ssid" 
                           placeholder="or enter SSID manually" 
                           required>
                    <span class="error">WiFi SSID is required</span>
                </div>
                
                <!-- WiFi Password -->
                <div class="form-group">
                    <label for="wifi_password">WiFi Password *</label>
                    <input type="password" id="wifi_password" name="wifi_password" 
                           placeholder="wifi password"
                           minlength="8"
                           required>
                    <small>Minimum 8 characters</small>
                    <span class="error">Password must be at least 8 characters</span>
                </div>
                
                <!-- Admin Password -->
                <div class="form-group">
                    <label for="admin_password">Admin Dashboard Password *</label>
                    <input type="password" id="admin_password" name="admin_password"
                           placeholder="admin password"
                           minlength="8"
                           required>
                    <small>For accessing device dashboard. Minimum 8 characters.</small>
                    <span class="error">Password must be at least 8 characters</span>
                </div>
                <!-- Edit Lock PIN -->
                <div class="form-group">
                    <label for="lock_pin">Edit Lock PIN</label>
                    <input type="text" id="lock_pin" name="lock_pin"
                           placeholder="e.g. 1234"
                           inputmode="numeric"
                           pattern="[0-9]{4,8}"
                           maxlength="8"
                           value="1234">
                    <small>4-8 digit PIN to unlock dashboard editing. Default: 1234</small>
                    <span class="error">PIN must be 4-8 digits</span>
                </div>
                <button type="submit" class="btn" id="submitBtn">
                    <span id="submitBtnText">Save Configuration</span>
                    <span class="spinner hidden" id="submitSpinner"></span>
                </button>
            </form>
        </div>
        
        <div class="footer">
            ESP32-C3 Water System v1.0 | Provisioning Mode
        </div>
    </div>
    
    <script>
        let selectedSSID = '';
        
        async function scanNetworks() {
            const btn = document.getElementById('scanBtn');
            const btnText = document.getElementById('scanBtnText');
            const spinner = document.getElementById('scanSpinner');
            const wifiList = document.getElementById('wifiList');
            
            btn.disabled = true;
            btnText.textContent = 'Scanning...';
            spinner.classList.remove('hidden');
            wifiList.innerHTML = '';
            wifiList.classList.add('hidden');
            
            try {
                const response = await fetch('/api/scan');
                const data = await response.json();
                
                if (data.networks && data.networks.length > 0) {
                    wifiList.classList.remove('hidden');
                    
                    data.networks.forEach(network => {
                        const item = document.createElement('div');
                        item.className = 'wifi-item';
                        item.onclick = () => selectNetwork(network.ssid);
                        
                        const signalBars = getSignalBars(network.quality);
                        
                        item.innerHTML = `
                            <div>
                                <div class="wifi-name">${network.ssid}</div>
                                <div class="wifi-signal">${network.encryptionName} | Channel ${network.channel}</div>
                            </div>
                            <div style="font-size: 18px">${signalBars}</div>
                        `;
                        
                        wifiList.appendChild(item);
                    });
                } else {
                    alert('No networks found. Try scanning again.');
                }
            } catch (error) {
                console.error('Scan error:', error);
                alert('Failed to scan networks. Please try again.');
            }
            
            btn.disabled = false;
            btnText.textContent = 'Scan for Networks';
            spinner.classList.add('hidden');
        }
        
        function selectNetwork(ssid) {
            selectedSSID = ssid;
            document.getElementById('wifi_ssid').value = ssid;
            
            // Update visual selection
            document.querySelectorAll('.wifi-item').forEach(item => {
                item.classList.remove('selected');
            });
            event.currentTarget.classList.add('selected');
        }
        
        function getSignalBars(quality) {
            if (quality >= 80) return '📶';
            if (quality >= 60) return '📶';
            if (quality >= 40) return '📶';
            if (quality >= 20) return '📶';
            return '📶';
        }
        
        // Form validation
        document.getElementById('configForm').addEventListener('submit', async function(e) {
            e.preventDefault();
            
            // Clear previous errors
            document.querySelectorAll('.form-group').forEach(group => {
                group.classList.remove('has-error');
            });
            
            // Get form data
            const formData = {
                device_name: document.getElementById('device_name').value,
                wifi_ssid: document.getElementById('wifi_ssid').value,
                wifi_password: document.getElementById('wifi_password').value,
                admin_password: document.getElementById('admin_password').value,

            };
            
            // Validate
            let hasErrors = false;
            
            // Device name
            if (!/^[a-zA-Z0-9_-]{3,32}$/.test(formData.device_name)) {
                document.getElementById('device_name').parentElement.classList.add('has-error');
                hasErrors = true;
            }
            
            // WiFi SSID
            if (formData.wifi_ssid.length === 0) {
                document.getElementById('wifi_ssid').parentElement.classList.add('has-error');
                hasErrors = true;
            }
            
            // WiFi Password
            if (formData.wifi_password.length < 8) {
                document.getElementById('wifi_password').parentElement.classList.add('has-error');
                hasErrors = true;
            }
            
            // Admin Password
            if (formData.admin_password.length < 8) {
                document.getElementById('admin_password').parentElement.classList.add('has-error');
                hasErrors = true;
            }
            
            if (hasErrors) {
                alert('Please fix the errors in the form');
                return;
            }
            
            // Disable submit button
            const submitBtn = document.getElementById('submitBtn');
            const submitBtnText = document.getElementById('submitBtnText');
            const submitSpinner = document.getElementById('submitSpinner');
            
            submitBtn.disabled = true;
            submitBtnText.textContent = 'Saving...';
            submitSpinner.classList.remove('hidden');
            
            // Submit to API
            try {
                const response = await fetch('/api/configure', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json'
                    },
                    body: JSON.stringify(formData)
                });
                
                const result = await response.json();
                
                if (result.success) {
                    // Success - show instructions
                    alert('✓ Configuration Saved Successfully!\n\n' + 
                          'Please restart the device:\n' +
                          '1. Disconnect power\n' +
                          '2. Wait 5 seconds\n' +
                          '3. Reconnect power\n\n' +
                          'Device will boot in production mode.');
                    
                    // Disable form
                    document.getElementById('configForm').style.display = 'none';
                    
                    // Show success message
                    const content = document.querySelector('.content');
                    content.innerHTML = `
                        <div style="text-align: center; padding: 40px;">
                            <div style="font-size: 80px; margin-bottom: 20px;">✓</div>
                            <h2 style="color: #2e7d32; margin-bottom: 20px;">Configuration Saved!</h2>
                            <p style="color: #666; margin-bottom: 30px;">Please restart your device manually.</p>
                            <div style="background: #f5f5f5; border-radius: 8px; padding: 20px; text-align: left;">
                                <strong>Next Steps:</strong>
                                <ol style="margin: 10px 0 0 20px;">
                                    <li style="margin: 10px 0;">Disconnect power from device</li>
                                    <li style="margin: 10px 0;">Wait 5 seconds</li>
                                    <li style="margin: 10px 0;">Reconnect power</li>
                                    <li style="margin: 10px 0;">Device will boot in production mode</li>
                                </ol>
                            </div>
                        </div>
                    `;
                } else {
                    // Error from server
                    alert('❌ Configuration Failed\n\n' + (result.error || 'Unknown error'));
                    
                    // Re-enable button
                    submitBtn.disabled = false;
                    submitBtnText.textContent = 'Save Configuration';
                    submitSpinner.classList.add('hidden');
                    
                    // Highlight error field if provided
                    if (result.field) {
                        const fieldElement = document.getElementById(result.field);
                        if (fieldElement) {
                            fieldElement.parentElement.classList.add('has-error');
                            fieldElement.focus();
                        }
                    }
                }
            } catch (error) {
                console.error('Submit error:', error);
                alert('❌ Failed to save configuration\n\n' + 
                      'Please check your connection and try again.');
                
                // Re-enable button
                submitBtn.disabled = false;
                submitBtnText.textContent = 'Save Configuration';
                submitSpinner.classList.add('hidden');
            }
        });
    </script>
</body>
</html>
)rawliteral";

const char SUCCESS_PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Configuration Saved</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Arial, sans-serif;
            background: linear-gradient(135deg, #11998e 0%, #38ef7d 100%);
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            padding: 20px;
        }
        .container {
            background: white;
            border-radius: 12px;
            box-shadow: 0 10px 40px rgba(0,0,0,0.2);
            max-width: 500px;
            width: 100%;
            padding: 40px 20px;
            text-align: center;
        }
        .success-icon {
            font-size: 80px;
            margin-bottom: 20px;
        }
        h1 {
            color: #2e7d32;
            font-size: 28px;
            margin-bottom: 15px;
        }
        p {
            color: #666;
            font-size: 16px;
            line-height: 1.6;
            margin-bottom: 20px;
        }
        .instructions {
            background: #f5f5f5;
            border-radius: 8px;
            padding: 20px;
            margin: 20px 0;
            text-align: left;
        }
        .instructions ol {
            margin-left: 20px;
        }
        .instructions li {
            color: #333;
            font-size: 14px;
            margin: 10px 0;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="success-icon">✓</div>
        <h1>Configuration Saved!</h1>
        <p>Your device has been configured successfully.</p>
        
        <div class="instructions">
            <strong>Next Steps:</strong>
            <ol>
                <li>Disconnect power from the device</li>
                <li>Wait 5 seconds</li>
                <li>Reconnect power</li>
                <li>Device will boot in production mode</li>
            </ol>
        </div>
        
        <p style="font-size: 14px; color: #999;">
            This page will remain active until restart.
        </p>
    </div>
</body>
</html>
)rawliteral";

const char* getSetupPageHTML() {
    return SETUP_PAGE_HTML;
}

const char* getSuccessPageHTML() {
    return SUCCESS_PAGE_HTML;
}