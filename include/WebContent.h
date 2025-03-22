#pragma once

const char SETUP_PAGE_HTML[] PROGMEM = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
        <title>WiFi Setup</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <style>
            /* Theme variables */
            :root {
                /* Light theme (default) */
                --bg-color: #f5f5f5;
                --card-bg: white;
                --text-color: #333;
                --heading-color: #1a73e8;
                --subheading-color: #5f6368;
                --border-color: #eee;
                --input-bg: #f8f9fa;
                --input-hover: #e8f0fe;
                --button-color: #1a73e8;
                --button-hover: #1557b0;
                --success-bg: #e6f4ea;
                --success-text: #1e8e3e;
                --error-bg: #fce8e6;
                --error-text: #d93025;
                --network-item-bg: #f8f9fa;
                --network-item-hover: #e8f0fe;
            }
    
            [data-theme="dark"] {
                --bg-color: #121212;
                --card-bg: #1e1e1e;
                --text-color: #e0e0e0;
                --heading-color: #90caf9;
                --subheading-color: #b0bec5;
                --border-color: #333;
                --input-bg: #2d2d2d;
                --input-hover: #3d3d3d;
                --button-color: #64b5f6;
                --button-hover: #42a5f5;
                --success-bg: #1b5e20;
                --success-text: #a5d6a7;
                --error-bg: #b71c1c;
                --error-text: #ef9a9a;
                --network-item-bg: #2d2d2d;
                --network-item-hover: #3d3d3d;
            }
    
            /* Base styles */
            body {
                font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Arial, sans-serif;
                margin: 0 auto;
                max-width: 600px;
                padding: 20px;
                background-color: var(--bg-color);
                color: var(--text-color);
                line-height: 1.6;
                transition: background-color 0.3s ease, color 0.3s ease;
            }
    
            .card {
                background: var(--card-bg);
                padding: 24px;
                border-radius: 12px;
                box-shadow: 0 2px 4px rgba(0,0,0,0.1);
                margin-bottom: 20px;
                position: relative;
                transition: background-color 0.3s ease;
            }
    
            .section {
                margin-bottom: 24px;
                padding-bottom: 16px;
                border-bottom: 1px solid var(--border-color);
            }
    
            .section:last-child {
                border-bottom: none;
            }
    
            h1 {
                font-size: 24px;
                font-weight: 600;
                margin: 0 0 24px 0;
                color: var(--heading-color);
            }
    
            h2 {
                font-size: 18px;
                font-weight: 500;
                margin: 0 0 16px 0;
                color: var(--subheading-color);
            }
    
            .form-group {
                margin-bottom: 16px;
            }
    
            .form-group label {
                display: block;
                margin-bottom: 8px;
                font-weight: 500;
            }
    
            .form-control {
                width: 100%;
                padding: 10px;
                border: 1px solid var(--border-color);
                border-radius: 8px;
                background-color: var(--input-bg);
                color: var(--text-color);
                font-size: 16px;
                transition: all 0.2s ease;
            }
    
            .form-control:focus {
                outline: none;
                border-color: var(--button-color);
                background-color: var(--input-hover);
            }
    
            .password-container {
                position: relative;
            }
    
            .toggle-password {
                position: absolute;
                right: 12px;
                top: 12px;
                cursor: pointer;
                color: var(--subheading-color);
            }
    
            .connect-button {
                background: var(--button-color);
                color: white;
                border: none;
                padding: 12px 24px;
                border-radius: 8px;
                font-size: 16px;
                font-weight: 500;
                cursor: pointer;
                width: 100%;
                transition: background-color 0.2s;
            }
    
            .connect-button:hover {
                background: var(--button-hover);
            }
    
            .connect-button:disabled {
                background: var(--border-color);
                cursor: not-allowed;
            }
    
            .status {
                display: none;
                padding: 12px;
                border-radius: 8px;
                margin-top: 16px;
                font-size: 14px;
            }
    
            .status.success {
                background: var(--success-bg);
                color: var(--success-text);
            }
    
            .status.error {
                background: var(--error-bg);
                color: var(--error-text);
            }
    
            /* WiFi Networks List */
            .networks-container {
                margin-top: 20px;
                max-height: 300px;
                overflow-y: auto;
                border: 1px solid var(--border-color);
                border-radius: 8px;
            }
    
            .network-list {
                list-style: none;
                padding: 0;
                margin: 0;
            }
    
            .network-item {
                padding: 12px 16px;
                border-bottom: 1px solid var(--border-color);
                cursor: pointer;
                background-color: var(--network-item-bg);
                transition: background-color 0.2s;
                display: flex;
                justify-content: space-between;
                align-items: center;
            }
    
            .network-item:last-child {
                border-bottom: none;
            }
    
            .network-item:hover {
                background-color: var(--network-item-hover);
            }
    
            .network-name {
                font-weight: 500;
            }
    
            .network-security {
                display: flex;
                align-items: center;
                color: var(--subheading-color);
            }
    
            .network-security svg {
                margin-right: 6px;
            }
    
            .signal-strength {
                display: flex;
                align-items: center;
                margin-left: 8px;
            }
    
            .loading {
                text-align: center;
                padding: 20px;
                color: var(--subheading-color);
            }
    
            .spinner {
                border: 3px solid rgba(0, 0, 0, 0.1);
                border-radius: 50%;
                border-top: 3px solid var(--button-color);
                width: 20px;
                height: 20px;
                animation: spin 1s linear infinite;
                margin: 0 auto 8px;
            }
    
            @keyframes spin {
                0% { transform: rotate(0deg); }
                100% { transform: rotate(360deg); }
            }
    
            [data-theme="dark"] .spinner {
                border: 3px solid rgba(255, 255, 255, 0.1);
                border-top: 3px solid var(--button-color);
            }
    
            .refresh-button {
                background: none;
                border: none;
                color: var(--button-color);
                cursor: pointer;
                font-size: 14px;
                padding: 8px 12px;
                display: flex;
                align-items: center;
                margin: 0 auto;
            }
    
            .refresh-button svg {
                margin-right: 6px;
            }
    
            .device-name {
                font-size: 16px;
                color: var(--subheading-color);
                margin-top: -16px;
                margin-bottom: 24px;
                text-align: center;
            }
    
            /* Theme toggle button */
            .theme-toggle {
                position: absolute;
                top: 24px;
                right: 24px;
            }
    
            #theme-toggle-btn {
                background: none;
                border: none;
                cursor: pointer;
                color: var(--heading-color);
                padding: 5px;
                border-radius: 50%;
                display: flex;
                align-items: center;
                justify-content: center;
                transition: background-color 0.3s ease;
            }
    
            #theme-toggle-btn:hover {
                background-color: var(--input-hover);
            }
    
            @media (max-width: 600px) {
                .theme-toggle {
                    top: 20px;
                    right: 20px;
                }
            }
        </style>
    </head>
    <body>
        <div class="card">
            <h1>WiFi Setup</h1>
            <div class="device-name">Chaotivolt Aux Display</div>
            
            <!-- Theme Toggle Button -->
            <div class="theme-toggle">
                <button id="theme-toggle-btn" aria-label="Toggle dark/light mode">
                    <svg id="sun-icon" xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                        <circle cx="12" cy="12" r="5"></circle>
                        <line x1="12" y1="1" x2="12" y2="3"></line>
                        <line x1="12" y1="21" x2="12" y2="23"></line>
                        <line x1="4.22" y1="4.22" x2="5.64" y2="5.64"></line>
                        <line x1="18.36" y1="18.36" x2="19.78" y2="19.78"></line>
                        <line x1="1" y1="12" x2="3" y2="12"></line>
                        <line x1="21" y1="12" x2="23" y2="12"></line>
                        <line x1="4.22" y1="19.78" x2="5.64" y2="18.36"></line>
                        <line x1="18.36" y1="5.64" x2="19.78" y2="4.22"></line>
                    </svg>
                    <svg id="moon-icon" xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" style="display: none;">
                        <path d="M21 12.79A9 9 0 1 1 11.21 3 7 7 0 0 0 21 12.79z"></path>
                    </svg>
                </button>
            </div>
    
            <div class="section">
                <h2>Available Networks</h2>
                <div class="networks-container">
                    <div id="networks-loading" class="loading">
                        <div class="spinner"></div>
                        <div>Scanning for networks...</div>
                    </div>
                    <ul id="network-list" class="network-list" style="display: none;">
                        <!-- Networks will be populated here by JavaScript -->
                    </ul>
                </div>
                <button id="refresh-networks" class="refresh-button">
                    <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                        <path d="M21.5 2v6h-6M2.5 22v-6h6M2 11.5a10 10 0 0 1 18.8-4.3M22 12.5a10 10 0 0 1-18.8 4.2"/>
                    </svg>
                    Refresh Networks
                </button>
            </div>
    
            <div class="section">
                <h2>Connect to Network</h2>
                <form id="wifi-form">
                    <div class="form-group">
                        <label for="ssid">Network Name (SSID)</label>
                        <input type="text" id="ssid" name="ssid" class="form-control" required>
                    </div>
                    <div class="form-group">
                        <label for="password">Password</label>
                        <div class="password-container">
                            <input type="password" id="password" name="password" class="form-control">
                            <span class="toggle-password" onclick="togglePasswordVisibility()">
                                <svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" id="password-toggle-icon">
                                    <path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path>
                                    <circle cx="12" cy="12" r="3"></circle>
                                </svg>
                            </span>
                        </div>
                    </div>
                    <button type="submit" id="connect-button" class="connect-button">Connect</button>
                </form>
                <div id="status" class="status"></div>
            </div>
        </div>
    
        <script>
            // Network signal strength icons
            const signalStrengthIcons = {
                excellent: '<svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="currentColor"><path d="M1 9l2 2c4.97-4.97 13.03-4.97 18 0l2-2C16.93 2.93 7.08 2.93 1 9zm8 8l3 3 3-3c-1.65-1.66-4.34-1.66-6 0zm-4-4l2 2c2.76-2.76 7.24-2.76 10 0l2-2C15.14 9.14 8.87 9.14 5 13z"/></svg>',
                good: '<svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="currentColor"><path d="M5 13l2 2c2.76-2.76 7.24-2.76 10 0l2-2C15.14 9.14 8.87 9.14 5 13z"/><path d="M9 17l3 3 3-3c-1.65-1.66-4.34-1.66-6 0z"/></svg>',
                fair: '<svg xmlns="http://www.w3.org/2000/svg" width="18" height="18" viewBox="0 0 24 24" fill="currentColor"><path d="M9 17l3 3 3-3c-1.65-1.66-4.34-1.66-6 0z"/></svg>',
            };
    
            // Lock icon for secured networks
            const lockIcon = '<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="11" width="18" height="11" rx="2" ry="2"></rect><path d="M7 11V7a5 5 0 0 1 10 0v4"></path></svg>';
    
            // Theme toggle functionality
            function setupThemeToggle() {
                const themeToggleBtn = document.getElementById('theme-toggle-btn');
                const sunIcon = document.getElementById('sun-icon');
                const moonIcon = document.getElementById('moon-icon');
                
                if (!themeToggleBtn || !sunIcon || !moonIcon) return;
                
                // Check for saved theme preference or use system preference
                const savedTheme = localStorage.getItem('theme');
                const prefersDark = window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches;
                
                // Set initial theme
                if (savedTheme === 'dark' || (!savedTheme && prefersDark)) {
                    document.documentElement.setAttribute('data-theme', 'dark');
                    sunIcon.style.display = 'none';
                    moonIcon.style.display = 'block';
                }
                
                // Toggle theme
                themeToggleBtn.addEventListener('click', () => {
                    const currentTheme = document.documentElement.getAttribute('data-theme');
                    if (currentTheme === 'dark') {
                        document.documentElement.setAttribute('data-theme', 'light');
                        localStorage.setItem('theme', 'light');
                        sunIcon.style.display = 'block';
                        moonIcon.style.display = 'none';
                    } else {
                        document.documentElement.setAttribute('data-theme', 'dark');
                        localStorage.setItem('theme', 'dark');
                        sunIcon.style.display = 'none';
                        moonIcon.style.display = 'block';
                    }
                });
            }
    
            // Function to toggle password visibility
            function togglePasswordVisibility() {
                const passwordInput = document.getElementById('password');
                const icon = document.getElementById('password-toggle-icon');
                
                if (passwordInput.type === 'password') {
                    passwordInput.type = 'text';
                    icon.innerHTML = '<path d="M17.94 17.94A10.07 10.07 0 0 1 12 20c-7 0-11-8-11-8a18.45 18.45 0 0 1 5.06-5.94M9.9 4.24A9.12 9.12 0 0 1 12 4c7 0 11 8 11 8a18.5 18.5 0 0 1-2.16 3.19m-6.72-1.07a3 3 0 1 1-4.24-4.24"></path><line x1="1" y1="1" x2="23" y2="23"></line>';
                } else {
                    passwordInput.type = 'password';
                    icon.innerHTML = '<path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path><circle cx="12" cy="12" r="3"></circle>';
                }
            }
    
            // Show status message
            function showStatus(message, isError = false) {
                const statusDiv = document.getElementById('status');
                statusDiv.textContent = message;
                statusDiv.style.display = 'block';
                statusDiv.className = 'status ' + (isError ? 'error' : 'success');
                
                if (!isError) {
                    setTimeout(() => {
                        statusDiv.style.display = 'none';
                    }, 3000);
                }
            }
    
            // Get signal strength icon based on RSSI
            function getSignalStrengthIcon(rssi) {
                if (rssi >= -50) return signalStrengthIcons.excellent;
                if (rssi >= -70) return signalStrengthIcons.good;
                return signalStrengthIcons.fair;
            }
    
            // Scan for WiFi networks
            async function scanNetworks() {
                const loadingDiv = document.getElementById('networks-loading');
                const networkList = document.getElementById('network-list');
                
                loadingDiv.style.display = 'block';
                networkList.style.display = 'none';
                
                try {
                    const response = await fetch('/scan');
                    if (!response.ok) {
                        throw new Error(`HTTP error: ${response.status}`);
                    }
                    
                    const networks = await response.json();
                    displayNetworks(networks);
                } catch (error) {
                    console.error('Error scanning networks:', error);
                    loadingDiv.innerHTML = 'Error scanning networks. Please try again.';
                }
            }
    
            // Display networks in the list
            function displayNetworks(networks) {
                const loadingDiv = document.getElementById('networks-loading');
                const networkList = document.getElementById('network-list');
                
                // Sort networks by signal strength
                networks.sort((a, b) => b.rssi - a.rssi);
                
                networkList.innerHTML = '';
                
                if (networks.length === 0) {
                    loadingDiv.innerHTML = 'No networks found. Please try again.';
                    return;
                }
                
                networks.forEach(network => {
                    const li = document.createElement('li');
                    li.className = 'network-item';
                    li.onclick = () => selectNetwork(network.ssid, network.encrypted);
                    
                    const nameDiv = document.createElement('div');
                    nameDiv.className = 'network-name';
                    nameDiv.textContent = network.ssid;
                    
                    const infoDiv = document.createElement('div');
                    infoDiv.className = 'network-security';
                    
                    if (network.encrypted) {
                        infoDiv.innerHTML = lockIcon;
                    }
                    
                    const signalDiv = document.createElement('div');
                    signalDiv.className = 'signal-strength';
                    signalDiv.innerHTML = getSignalStrengthIcon(network.rssi);
                    
                    infoDiv.appendChild(signalDiv);
                    li.appendChild(nameDiv);
                    li.appendChild(infoDiv);
                    networkList.appendChild(li);
                });
                
                loadingDiv.style.display = 'none';
                networkList.style.display = 'block';
            }
    
            // Select a network from the list
            function selectNetwork(ssid, isEncrypted) {
                const ssidInput = document.getElementById('ssid');
                const passwordInput = document.getElementById('password');
                const passwordGroup = passwordInput.parentNode.parentNode;
                
                ssidInput.value = ssid;
                
                if (isEncrypted) {
                    passwordGroup.style.display = 'block';
                    passwordInput.required = true;
                    passwordInput.focus();
                } else {
                    passwordGroup.style.display = 'none';
                    passwordInput.required = false;
                    passwordInput.value = '';
                }
            }
    
            // Connect to the selected WiFi
            async function connectToWiFi(event) {
                event.preventDefault();
                
                const ssid = document.getElementById('ssid').value;
                const password = document.getElementById('password').value;
                const connectButton = document.getElementById('connect-button');
                
                if (!ssid) {
                    showStatus('Please enter a network name or select one from the list', true);
                    return;
                }
                
                connectButton.disabled = true;
                connectButton.textContent = 'Connecting...';
                
                try {
                    const response = await fetch('/connect', {
                        method: 'POST',
                        headers: {
                            'Content-Type': 'application/x-www-form-urlencoded',
                        },
                        body: `ssid=${encodeURIComponent(ssid)}&password=${encodeURIComponent(password)}`
                    });
                    
                    if (response.ok) {
                        showStatus('Successfully connected! Device will restart.');
                        setTimeout(() => {
                            window.location.href = '/';
                        }, 5000);
                    } else {
                        const text = await response.text();
                        throw new Error(text || 'Failed to connect');
                    }
                } catch (error) {
                    showStatus(`Connection failed: ${error.message}`, true);
                    connectButton.disabled = false;
                    connectButton.textContent = 'Connect';
                }
            }
    
            document.addEventListener('DOMContentLoaded', function() {
                setupThemeToggle();
                
                // Scan for networks on page load
                scanNetworks();
                
                // Set up refresh button
                document.getElementById('refresh-networks').addEventListener('click', scanNetworks);
                
                // Set up form submission
                document.getElementById('wifi-form').addEventListener('submit', connectToWiFi);
                
                // Initialize password field visibility
                const passwordGroup = document.getElementById('password').parentNode.parentNode;
                passwordGroup.style.display = 'block';
            });
        </script>
    </body>
    </html>
    )rawliteral";

const char* const WIFI_CONFIG_TITLE PROGMEM = "WiFi Configuration";
const char* const SCAN_NETWORKS_MSG PROGMEM = "Scanning for networks...";

const char PREFERENCES_PAGE_HTML[] PROGMEM = R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head>
        <title>Display Preferences</title>
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <style>
            /* Theme variables */
            :root {
                /* Light theme (default) */
                --bg-color: #f5f5f5;
                --card-bg: white;
                --text-color: #333;
                --heading-color: #1a73e8;
                --subheading-color: #5f6368;
                --border-color: #eee;
                --input-bg: #f8f9fa;
                --input-hover: #e8f0fe;
                --slider-bg: #e0e0e0;
                --value-bg: #e8f0fe;
                --button-color: #1a73e8;
                --button-hover: #1557b0;
                --status-bg: #f8f9fa;
                --status-text: #5f6368;
                --success-bg: #e6f4ea;
                --success-text: #1e8e3e;
                --error-bg: #fce8e6;
                --error-text: #d93025;
                --on-color: #1e8e3e;
                --off-color: #d93025;
            }
    
            [data-theme="dark"] {
                --bg-color: #121212;
                --card-bg: #1e1e1e;
                --text-color: #e0e0e0;
                --heading-color: #90caf9;
                --subheading-color: #b0bec5;
                --border-color: #333;
                --input-bg: #2d2d2d;
                --input-hover: #3d3d3d;
                --slider-bg: #424242;
                --value-bg: #3d3d3d;
                --button-color: #64b5f6;
                --button-hover: #42a5f5;
                --status-bg: #2d2d2d;
                --status-text: #b0bec5;
                --success-bg: #1b5e20;
                --success-text: #a5d6a7;
                --error-bg: #b71c1c;
                --error-text: #ef9a9a;
                --on-color: #66bb6a;
                --off-color: #ef5350;
            }
    
            /* Base styles */
            body {
                font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
                margin: 0 auto;
                max-width: 600px;
                padding: 20px;
                background-color: var(--bg-color);
                color: var(--text-color);
                line-height: 1.6;
                transition: background-color 0.3s ease, color 0.3s ease;
            }
    
            /* Card container */
            .card {
                background: var(--card-bg);
                padding: 24px;
                border-radius: 12px;
                box-shadow: 0 2px 4px rgba(0,0,0,0.1);
                margin-bottom: 20px;
                position: relative;
                transition: background-color 0.3s ease;
            }
    
            /* Section styling */
            .section {
                margin-bottom: 24px;
                padding-bottom: 16px;
                border-bottom: 1px solid var(--border-color);
            }
    
            .section:last-child {
                border-bottom: none;
            }
    
            h1 {
                font-size: 24px;
                font-weight: 600;
                margin: 0 0 24px 0;
                color: var(--heading-color);
            }
    
            h2 {
                font-size: 18px;
                font-weight: 500;
                margin: 0 0 16px 0;
                color: var(--subheading-color);
            }
    
            h3 {
                font-size: 16px;
                font-weight: 500;
                margin: 0 0 12px 0;
                color: var(--subheading-color);
            }
    
            /* Form controls */
            .form-group {
                margin-bottom: 16px;
            }
    
            .form-group label {
                display: block;
                margin-bottom: 8px;
                color: var(--subheading-color);
            }
    
            /* Custom radio button styling */
            .radio-group {
                display: flex;
                gap: 20px;
                margin: 10px 0;
            }
    
            .radio-label {
                display: flex;
                align-items: center;
                cursor: pointer;
                padding: 8px 16px;
                background: var(--input-bg);
                border-radius: 8px;
                transition: all 0.2s ease;
            }
    
            .radio-label:hover {
                background: var(--input-hover);
            }
    
            .radio-label input[type="radio"] {
                margin-right: 8px;
            }
    
            /* Slider styling */
            .slider-container {
                margin: 20px 0;
            }
    
            .slider-container label {
                display: flex;
                justify-content: space-between;
                align-items: center;
                margin-bottom: 8px;
            }
    
            .slider-value {
                background: var(--value-bg);
                padding: 2px 8px;
                border-radius: 4px;
                min-width: 30px;
                text-align: center;
            }
    
            input[type="range"] {
                width: 100%;
                height: 6px;
                background: var(--slider-bg);
                border-radius: 3px;
                outline: none;
                -webkit-appearance: none;
            }
    
            input[type="range"]::-webkit-slider-thumb {
                -webkit-appearance: none;
                width: 20px;
                height: 20px;
                background: var(--button-color);
                border-radius: 50%;
                cursor: pointer;
                transition: background .15s ease-in-out;
            }
    
            input[type="range"]::-webkit-slider-thumb:hover {
                background: var(--button-hover);
            }
    
            /* Time picker styling */
            .time-picker-container {
                display: flex;
                gap: 20px;
                margin: 20px 0;
            }
    
            .time-picker {
                flex: 1;
            }
    
            .time-picker select {
                width: 100%;
                padding: 8px;
                border: 1px solid var(--border-color);
                border-radius: 8px;
                background: var(--input-bg);
                font-size: 16px;
                color: var(--text-color);
            }
    
            /* Save button */
            .save-button {
                background: var(--button-color);
                color: white;
                border: none;
                padding: 12px 24px;
                border-radius: 8px;
                font-size: 16px;
                font-weight: 500;
                cursor: pointer;
                width: 100%;
                transition: background-color 0.2s;
            }
    
            .save-button:hover {
                background: var(--button-hover);
            }
    
            .save-button:active {
                background: var(--button-hover);
            }
    
            /* Status message */
            .status {
                display: none;
                padding: 12px;
                border-radius: 8px;
                margin-top: 16px;
                font-size: 14px;
            }
    
            .status.success {
                background: var(--success-bg);
                color: var(--success-text);
            }
    
            .status.error {
                background: var(--error-bg);
                color: var(--error-text);
            }
    
            .status-info {
                margin-top: 16px;
                padding: 12px;
                background: var(--status-bg);
                border-radius: 8px;
                display: flex;
                justify-content: space-between;
            }
    
            .status-info span {
                color: var(--status-text);
            }
    
            #relay0-current-state.on, #relay1-current-state.on {
                color: var(--on-color);
                font-weight: 600;
            }
    
            #relay0-current-state.off, #relay1-current-state.off {
                color: var(--off-color);
                font-weight: 600;
            }
    
            .relay-control {
                margin-bottom: 24px;
            }
    
            .relay-control:last-child {
                margin-bottom: 0;
            }
    
            /* Theme toggle button */
            .theme-toggle {
                position: absolute;
                top: 24px;
                right: 24px;
            }
    
            #theme-toggle-btn {
                background: none;
                border: none;
                cursor: pointer;
                color: var(--heading-color);
                padding: 5px;
                border-radius: 50%;
                display: flex;
                align-items: center;
                justify-content: center;
                transition: background-color 0.3s ease;
            }
    
            #theme-toggle-btn:hover {
                background-color: var(--input-hover);
            }
    
            @media (max-width: 600px) {
                .theme-toggle {
                    top: 20px;
                    right: 20px;
                }
            }
            
            /* Password field styling */
            .password-container {
                position: relative;
            }
            
            .toggle-password {
                position: absolute;
                right: 12px;
                top: 12px;
                cursor: pointer;
                color: var(--subheading-color);
            }
            
            .form-control {
                width: 100%;
                padding: 10px;
                border: 1px solid var(--border-color);
                border-radius: 8px;
                background-color: var(--input-bg);
                color: var(--text-color);
                font-size: 16px;
                transition: all 0.2s ease;
            }
            
            .form-control:focus {
                outline: none;
                border-color: var(--button-color);
                background-color: var(--input-hover);
            }
        </style>
    </head>
    <body>
        <div class="card">
            <h1>ESP32 Control Panel</h1>
            
            <!-- Theme Toggle Button -->
            <div class="theme-toggle">
                <button id="theme-toggle-btn" aria-label="Toggle dark/light mode">
                    <svg id="sun-icon" xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                        <circle cx="12" cy="12" r="5"></circle>
                        <line x1="12" y1="1" x2="12" y2="3"></line>
                        <line x1="12" y1="21" x2="12" y2="23"></line>
                        <line x1="4.22" y1="4.22" x2="5.64" y2="5.64"></line>
                        <line x1="18.36" y1="18.36" x2="19.78" y2="19.78"></line>
                        <line x1="1" y1="12" x2="3" y2="12"></line>
                        <line x1="21" y1="12" x2="23" y2="12"></line>
                        <line x1="4.22" y1="19.78" x2="5.64" y2="18.36"></line>
                        <line x1="18.36" y1="5.64" x2="19.78" y2="4.22"></line>
                    </svg>
                    <svg id="moon-icon" xmlns="http://www.w3.org/2000/svg" width="24" height="24" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" style="display: none;">
                        <path d="M21 12.79A9 9 0 1 1 11.21 3 7 7 0 0 0 21 12.79z"></path>
                    </svg>
                </button>
            </div>
            
            <!-- Relay Control Section -->
            <div class="section">
                <h2>Relay Control</h2>
                
                <!-- Relay 1 -->
                <div class="relay-control">
                    <h3>Relay 1</h3>
                    <div class="form-group">
                        <div class="radio-group">
                            <label class="radio-label">
                                <input type="radio" name="relay0State" value="ON">
                                ON
                            </label>
                            <label class="radio-label">
                                <input type="radio" name="relay0State" value="OFF">
                                OFF
                            </label>
                        </div>
                        <div class="status-info">
                            <span>Current State: <span id="relay0-current-state">Loading...</span></span>
                            <span>Override: <span id="relay0-override-state">Loading...</span></span>
                        </div>
                    </div>
                </div>
    
                <!-- Relay 2 -->
                <div class="relay-control">
                    <h3>Relay 2</h3>
                    <div class="form-group">
                        <div class="radio-group">
                            <label class="radio-label">
                                <input type="radio" name="relay1State" value="ON">
                                ON
                            </label>
                            <label class="radio-label">
                                <input type="radio" name="relay1State" value="OFF">
                                OFF
                            </label>
                        </div>
                        <div class="status-info">
                            <span>Current State: <span id="relay1-current-state">Loading...</span></span>
                            <span>Override: <span id="relay1-override-state">Loading...</span></span>
                        </div>
                    </div>
                </div>
            </div>
            
            <form id="preferences-form">
                <div class="section">
                    <h2>Night Mode Dimming</h2>
                    <div class="form-group">
                        <div class="radio-group">
                            <label class="radio-label">
                                <input type="radio" name="nightDimming" value="enabled" required>
                                Enabled
                            </label>
                            <label class="radio-label">
                                <input type="radio" name="nightDimming" value="disabled" required>
                                Disabled
                            </label>
                        </div>
                    </div>
                </div>
    
                <div class="section">
                    <h2>Brightness Settings</h2>
                    <div class="slider-container">
                        <label>
                            Day Brightness
                            <span class="slider-value" id="day-brightness-value">10</span>
                        </label>
                        <input type="range" id="day-brightness" name="dayBrightness" 
                               min="1" max="25" value="10" 
                               oninput="document.getElementById('day-brightness-value').textContent = this.value">
                    </div>
    
                    <div class="slider-container">
                        <label>
                            Night Brightness
                            <span class="slider-value" id="night-brightness-value">5</span>
                        </label>
                        <input type="range" id="night-brightness" name="nightBrightness" 
                               min="1" max="25" value="5" 
                               oninput="document.getElementById('night-brightness-value').textContent = this.value">
                    </div>
                </div>
    
                <div class="section">
                    <h2>Night Mode Hours</h2>
                    <div class="time-picker-container">
                        <div class="time-picker">
                            <label>Start Time</label>
                            <select name="nightStartHour" id="night-start">
                                <!-- Options filled by JavaScript -->
                            </select>
                        </div>
                        <div class="time-picker">
                            <label>End Time</label>
                            <select name="nightEndHour" id="night-end">
                                <!-- Options filled by JavaScript -->
                            </select>
                        </div>
                    </div>
                </div>
                <div class="section">
                    <h2>Remote Temperature Sensor</h2>
                    <div class="form-group">
                        <div class="radio-group">
                            <label class="radio-label">
                                <input type="radio" name="useSensorhub" value="enabled" required>
                                Enabled
                            </label>
                            <label class="radio-label">
                                <input type="radio" name="useSensorhub" value="disabled" required>
                                Disabled
                            </label>
                        </div>
                    </div>
                    
                    <div id="sensorhub-settings" style="display: none;">
                        <div class="form-group">
                            <label for="sensorhub-username">Username</label>
                            <input type="text" id="sensorhub-username" name="sensorhubUsername" class="form-control">
                        </div>
                        <div class="form-group">
                            <label for="sensorhub-password">Password</label>
                            <div class="password-container">
                                <input type="password" id="sensorhub-password" name="sensorhubPassword" class="form-control">
                                <span class="toggle-password" onclick="toggleSensorhubPasswordVisibility()">
                                    <svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" id="sensorhub-password-toggle-icon">
                                        <path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path>
                                        <circle cx="12" cy="12" r="3"></circle>
                                    </svg>
                                </span>
                            </div>
                        </div>
                    </div>
                </div>

                <div class="section">
                    <h2>MQTT Data Publishing</h2>
                    <div class="form-group">
                        <div class="radio-group">
                            <label class="radio-label">
                                <input type="radio" name="mqttPublishEnabled" value="enabled" required>
                                Enabled
                            </label>
                            <label class="radio-label">
                                <input type="radio" name="mqttPublishEnabled" value="disabled" required>
                                Disabled
                            </label>
                        </div>
                    </div>
                    
                    <div id="mqtt-settings" style="display: none;">
                        <div class="form-group">
                            <label for="mqtt-broker">MQTT Broker Address</label>
                            <input type="text" id="mqtt-broker" name="mqttBrokerAddress" class="form-control" placeholder="mqtt.example.com">
                        </div>
                        <div class="form-group">
                            <label for="mqtt-username">MQTT Username</label>
                            <input type="text" id="mqtt-username" name="mqttUsername" class="form-control">
                        </div>
                        <div class="form-group">
                            <label for="mqtt-password">MQTT Password</label>
                            <div class="password-container">
                                <input type="password" id="mqtt-password" name="mqttPassword" class="form-control">
                                <span class="toggle-password" onclick="toggleMqttPasswordVisibility()">
                                    <svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" id="mqtt-password-toggle-icon">
                                        <path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path>
                                        <circle cx="12" cy="12" r="3"></circle>
                                    </svg>
                                </span>
                            </div>
                        </div>
                        <div class="form-group">
                            <label for="mqtt-interval">Publish Interval (seconds)</label>
                            <input type="number" id="mqtt-interval" name="mqttPublishInterval" class="form-control" min="10" max="3600" value="60">
                            <small class="form-text" style="color: var(--subheading-color);">Values between 10 and 3600 seconds</small>
                        </div>
                    </div>
                </div>
                    <div id="sensorhub-settings" style="display: none;">
                        <div class="form-group">
                            <label for="sensorhub-username">Username</label>
                            <input type="text" id="sensorhub-username" name="sensorhubUsername" class="form-control">
                        </div>
                        <div class="form-group">
                            <label for="sensorhub-password">Password</label>
                            <div class="password-container">
                                <input type="password" id="sensorhub-password" name="sensorhubPassword" class="form-control">
                                <span class="toggle-password" onclick="toggleSensorhubPasswordVisibility()">
                                    <svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" id="sensorhub-password-toggle-icon">
                                        <path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path>
                                        <circle cx="12" cy="12" r="3"></circle>
                                    </svg>
                                </span>
                            </div>
                        </div>
                    </div>
                </div>
    
                <button type="submit" class="save-button">Save Preferences</button>
            </form>
    
            <div id="status" class="status"></div>
        </div>
    
    <script>
        // Function to toggle sensorhub settings visibility
        function toggleSensorhubSettings() {
            const useSensorhub = document.querySelector('input[name="useSensorhub"]:checked')?.value === 'enabled';
            const settingsDiv = document.getElementById('sensorhub-settings');
            
            if (settingsDiv) {
                settingsDiv.style.display = useSensorhub ? 'block' : 'none';
            }
        }
        
        // Function to toggle sensorhub password visibility
        function toggleSensorhubPasswordVisibility() {
            const passwordInput = document.getElementById('sensorhub-password');
            const icon = document.getElementById('sensorhub-password-toggle-icon');
            
            if (passwordInput.type === 'password') {
                passwordInput.type = 'text';
                icon.innerHTML = '<path d="M17.94 17.94A10.07 10.07 0 0 1 12 20c-7 0-11-8-11-8a18.45 18.45 0 0 1 5.06-5.94M9.9 4.24A9.12 9.12 0 0 1 12 4c7 0 11 8 11 8a18.5 18.5 0 0 1-2.16 3.19m-6.72-1.07a3 3 0 1 1-4.24-4.24"></path><line x1="1" y1="1" x2="23" y2="23"></line>';
            } else {
                passwordInput.type = 'password';
                icon.innerHTML = '<path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path><circle cx="12" cy="12" r="3"></circle>';
            }
        }
    
        // Populate time select options
        function populateTimeOptions() {
            const startSelect = document.getElementById('night-start');
            const endSelect = document.getElementById('night-end');
            
            if (!startSelect || !endSelect) return;
            
            for (let i = 0; i < 24; i++) {
                const hour = i.toString().padStart(2, '0');
                const timeString = `${hour}:00`;
                
                const startOption = new Option(timeString, i);
                const endOption = new Option(timeString, i);
                
                startSelect.add(startOption);
                endSelect.add(endOption);
            }
        }
    
        // Theme toggle functionality
        function setupThemeToggle() {
            const themeToggleBtn = document.getElementById('theme-toggle-btn');
            const sunIcon = document.getElementById('sun-icon');
            const moonIcon = document.getElementById('moon-icon');
            
            if (!themeToggleBtn || !sunIcon || !moonIcon) return;
            
            // Check for saved theme preference or use system preference
            const savedTheme = localStorage.getItem('theme');
            const prefersDark = window.matchMedia && window.matchMedia('(prefers-color-scheme: dark)').matches;
            
            // Set initial theme
            if (savedTheme === 'dark' || (!savedTheme && prefersDark)) {
                document.documentElement.setAttribute('data-theme', 'dark');
                sunIcon.style.display = 'none';
                moonIcon.style.display = 'block';
            }
            
            // Toggle theme
            themeToggleBtn.addEventListener('click', () => {
                const currentTheme = document.documentElement.getAttribute('data-theme');
                if (currentTheme === 'dark') {
                    document.documentElement.setAttribute('data-theme', 'light');
                    localStorage.setItem('theme', 'light');
                    sunIcon.style.display = 'block';
                    moonIcon.style.display = 'none';
                } else {
                    document.documentElement.setAttribute('data-theme', 'dark');
                    localStorage.setItem('theme', 'dark');
                    sunIcon.style.display = 'none';
                    moonIcon.style.display = 'block';
                }
            });
        }
    
        // Function to show status message
        function showStatus(message, isError = false) {
            const statusDiv = document.getElementById('status');
            if (!statusDiv) return;
            
            statusDiv.textContent = message;
            statusDiv.style.display = 'block';
            statusDiv.className = 'status ' + (isError ? 'error' : 'success');
            
            setTimeout(() => {
                statusDiv.style.display = 'none';
            }, 3000);
        }
    
        // Function to apply default values for immediate UI responsiveness
        function applyDefaultValues() {
            // Set default values before actual data loads
            const dayBrightness = document.getElementById('day-brightness');
            const nightBrightness = document.getElementById('night-brightness');
            const dayValue = document.getElementById('day-brightness-value');
            const nightValue = document.getElementById('night-brightness-value');
            
            // Get previously stored values from localStorage or use defaults
            const savedDayBrightness = localStorage.getItem('dayBrightness') || 10;
            const savedNightBrightness = localStorage.getItem('nightBrightness') || 5;
            const savedNightStartHour = localStorage.getItem('nightStartHour') || 22;
            const savedNightEndHour = localStorage.getItem('nightEndHour') || 6;
            const savedUseSensorhub = localStorage.getItem('useSensorhub') || 'disabled';
            const savedSensorhubUsername = localStorage.getItem('sensorhubUsername') || '';
            
            // Apply values to UI
            if (dayBrightness && dayValue) {
                dayBrightness.value = savedDayBrightness;
                dayValue.textContent = savedDayBrightness;
            }
            
            if (nightBrightness && nightValue) {
                nightBrightness.value = savedNightBrightness;
                nightValue.textContent = savedNightBrightness;
            }
            
            // Set time selects
            const startSelect = document.getElementById('night-start');
            const endSelect = document.getElementById('night-end');
            
            if (startSelect) startSelect.value = savedNightStartHour;
            if (endSelect) endSelect.value = savedNightEndHour;
            
            // Set night dimming radio based on localStorage
            const savedNightDimming = localStorage.getItem('nightDimming');
            if (savedNightDimming !== null) {
                const radioButtons = document.getElementsByName('nightDimming');
                for (let radio of radioButtons) {
                    if (radio.value === savedNightDimming) {
                        radio.checked = true;
                    }
                }
            }
            
            // Set sensorhub radio based on localStorage
            const sensorhubRadios = document.getElementsByName('useSensorhub');
            for (let radio of sensorhubRadios) {
                if (radio.value === savedUseSensorhub) {
                    radio.checked = true;
                }
            }
            
            // Set sensorhub username if saved
            const usernameField = document.getElementById('sensorhub-username');
            if (usernameField && savedSensorhubUsername) {
                usernameField.value = savedSensorhubUsername;
            }
            
            // Update visibility of sensorhub settings
            toggleSensorhubSettings();
        }
    
        // Fetch with timeout to prevent hanging
        async function fetchWithTimeout(url, options, timeout = 5000) {
            return Promise.race([
                fetch(url, options),
                new Promise((_, reject) => 
                    setTimeout(() => reject(new Error('Request timed out')), timeout)
                )
            ]);
        }

        // Load current preferences with performance optimizations
        async function loadPreferences() {
            // Apply default values immediately for responsive UI
            applyDefaultValues();
            applyMqttDefaults();
            
            try {
                const response = await fetchWithTimeout('/api/preferences', {
                    method: 'GET',
                    headers: {
                        'Accept': 'application/json',
                        'Cache-Control': 'max-age=10' // Use cache if available
                    }
                }, 3000); // 3 second timeout
                
                if (!response.ok) {
                    throw new Error(`HTTP error! status: ${response.status}`);
                }
                
                const result = await response.json();
                
                if (!result.success) {
                    throw new Error(result.error || 'Failed to load preferences');
                }
                
                const data = result.data;
                
                // Save values to localStorage for future use
                localStorage.setItem('dayBrightness', data.dayBrightness);
                localStorage.setItem('nightBrightness', data.nightBrightness);
                localStorage.setItem('nightStartHour', data.nightStartHour);
                localStorage.setItem('nightEndHour', data.nightEndHour);
                localStorage.setItem('nightDimming', data.nightDimming ? 'enabled' : 'disabled');
                localStorage.setItem('useSensorhub', data.useSensorhub ? 'enabled' : 'disabled');
                if (data.sensorhubUsername) {
                    localStorage.setItem('sensorhubUsername', data.sensorhubUsername);
                }
                
                // Save MQTT values to localStorage
                localStorage.setItem('mqttPublishEnabled', data.mqttPublishEnabled ? 'enabled' : 'disabled');
                localStorage.setItem('mqttBrokerAddress', data.mqttBrokerAddress || '');
                localStorage.setItem('mqttUsername', data.mqttUsername || '');
                localStorage.setItem('mqttPublishInterval', data.mqttPublishInterval || '60');
                
                // Set night dimming radio
                const radioButtons = document.getElementsByName('nightDimming');
                for (let radio of radioButtons) {
                    if (radio.value === (data.nightDimming ? 'enabled' : 'disabled')) {
                        radio.checked = true;
                    }
                }
                
                // Set sensorhub radio
                const sensorhubRadios = document.getElementsByName('useSensorhub');
                for (let radio of sensorhubRadios) {
                    if (radio.value === (data.useSensorhub ? 'enabled' : 'disabled')) {
                        radio.checked = true;
                    }
                }
                
                // Set MQTT radio
                const mqttRadios = document.getElementsByName('mqttPublishEnabled');
                for (let radio of mqttRadios) {
                    if (radio.value === (data.mqttPublishEnabled ? 'enabled' : 'disabled')) {
                        radio.checked = true;
                    }
                }
                
                // Set sensorhub username
                const usernameField = document.getElementById('sensorhub-username');
                if (usernameField && data.sensorhubUsername) {
                    usernameField.value = data.sensorhubUsername;
                }
                
                // Update password field placeholder if a password exists
                const passwordField = document.getElementById('sensorhub-password');
                if (passwordField && data.hasSensorhubPassword) {
                    passwordField.placeholder = '••••••••'; // Show password dots as placeholder
                }
                
                // Set MQTT form fields
                const mqttBrokerField = document.getElementById('mqtt-broker');
                if (mqttBrokerField && data.mqttBrokerAddress) {
                    mqttBrokerField.value = data.mqttBrokerAddress;
                }
                
                const mqttUsernameField = document.getElementById('mqtt-username');
                if (mqttUsernameField && data.mqttUsername) {
                    mqttUsernameField.value = data.mqttUsername;
                }
                
                // Update MQTT password field placeholder if a password exists
                const mqttPasswordField = document.getElementById('mqtt-password');
                if (mqttPasswordField && data.hasMqttPassword) {
                    mqttPasswordField.placeholder = '••••••••'; // Show password dots as placeholder
                }
                
                const mqttIntervalField = document.getElementById('mqtt-interval');
                if (mqttIntervalField && data.mqttPublishInterval) {
                    mqttIntervalField.value = data.mqttPublishInterval;
                }
                
                // Update settings visibility
                toggleSensorhubSettings();
                toggleMqttSettings();
                
                // Set brightness sliders
                const dayBrightness = document.getElementById('day-brightness');
                const nightBrightness = document.getElementById('night-brightness');
                const dayValue = document.getElementById('day-brightness-value');
                const nightValue = document.getElementById('night-brightness-value');
                
                if (dayBrightness && dayValue) {
                    dayBrightness.value = data.dayBrightness;
                    dayValue.textContent = data.dayBrightness;
                }
                
                if (nightBrightness && nightValue) {
                    nightBrightness.value = data.nightBrightness;
                    nightValue.textContent = data.nightBrightness;
                }
                
                // Set time selects
                const startSelect = document.getElementById('night-start');
                const endSelect = document.getElementById('night-end');
                
                if (startSelect) startSelect.value = data.nightStartHour;
                if (endSelect) endSelect.value = data.nightEndHour;
                
            } catch (error) {
                console.error('Error loading preferences:', error);
                // Don't show error to user, just use default values that were already applied
            }
        }
    
        // Relay status update function
        async function updateRelayStatus() {
            try {
                const response = await fetchWithTimeout('/api/relay', {
                    method: 'GET',
                    headers: {
                        'Accept': 'application/json',
                        'Cache-Control': 'max-age=3' // Short cache
                    }
                }, 3000);
                
                if (!response.ok) throw new Error(`HTTP error! status: ${response.status}`);
                
                const relays = await response.json();
                
                // Update both relays
                for (let i = 0; i < 2; i++) {
                    const relay = relays[i];
                    const stateElement = document.getElementById(`relay${i}-current-state`);
                    const overrideElement = document.getElementById(`relay${i}-override-state`);
                    const radioButtons = document.getElementsByName(`relay${i}State`);
                    
                    if (stateElement) {
                        stateElement.textContent = relay.state;
                        stateElement.className = relay.state.toLowerCase();
                    }
                    
                    if (overrideElement) {
                        overrideElement.textContent = relay.override ? 'Yes' : 'No';
                    }
                    
                    for (let radio of radioButtons) {
                        radio.checked = radio.value === relay.state;
                    }
                }
            } catch (error) {
                console.error('Error updating relay status:', error);
                // Silently fail for relay status
            }
        }
    
        function setupRelayControls() {
            for (let i = 0; i < 2; i++) {
                const relayRadios = document.getElementsByName(`relay${i}State`);
                for (let radio of relayRadios) {
                    radio.addEventListener('change', async function() {
                        try {
                            // Optimistically update UI first
                            const stateElement = document.getElementById(`relay${i}-current-state`);
                            if (stateElement) {
                                stateElement.textContent = this.value;
                                stateElement.className = this.value.toLowerCase();
                            }
                            
                            const response = await fetchWithTimeout('/api/relay', {
                                method: 'POST',
                                headers: {
                                    'Content-Type': 'application/json',
                                },
                                body: JSON.stringify({
                                    relay_id: i,
                                    state: this.value
                                })
                            }, 3000);
    
                            const result = await response.json();
                            if (response.ok && result.success) {
                                showStatus(`Relay ${i + 1} ${this.value} command sent successfully`);
                            } else {
                                throw new Error(result.error || 'Failed to control relay');
                            }
                        } catch (error) {
                            console.error('Error controlling relay:', error);
                            showStatus('Failed to control relay: ' + error.message, true);
                            // Refresh relay status to show actual state
                            updateRelayStatus();
                        }
                    });
                }
            }
        }
        // Function to toggle mqtt password visibility
        function toggleMqttPasswordVisibility() {
            const passwordInput = document.getElementById('mqtt-password');
            const icon = document.getElementById('mqtt-password-toggle-icon');
            
            if (passwordInput.type === 'password') {
                passwordInput.type = 'text';
                icon.innerHTML = '<path d="M17.94 17.94A10.07 10.07 0 0 1 12 20c-7 0-11-8-11-8a18.45 18.45 0 0 1 5.06-5.94M9.9 4.24A9.12 9.12 0 0 1 12 4c7 0 11 8 11 8a18.5 18.5 0 0 1-2.16 3.19m-6.72-1.07a3 3 0 1 1-4.24-4.24"></path><line x1="1" y1="1" x2="23" y2="23"></line>';
            } else {
                passwordInput.type = 'password';
                icon.innerHTML = '<path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path><circle cx="12" cy="12" r="3"></circle>';
            }
        }

        // Function to toggle MQTT settings visibility
        function toggleMqttSettings() {
            const mqttEnabled = document.querySelector('input[name="mqttPublishEnabled"]:checked')?.value === 'enabled';
            const settingsDiv = document.getElementById('mqtt-settings');
            
            if (settingsDiv) {
                settingsDiv.style.display = mqttEnabled ? 'block' : 'none';
            }
        }

        // Apply default values for MQTT settings
        function applyMqttDefaults() {
            // Set MQTT default values from localStorage or use defaults
            const savedMqttEnabled = localStorage.getItem('mqttPublishEnabled') || 'disabled';
            const savedMqttBroker = localStorage.getItem('mqttBrokerAddress') || '';
            const savedMqttUsername = localStorage.getItem('mqttUsername') || '';
            const savedMqttInterval = localStorage.getItem('mqttPublishInterval') || '60';
            
            // Set MQTT radio buttons
            const mqttRadios = document.getElementsByName('mqttPublishEnabled');
            for (let radio of mqttRadios) {
                if (radio.value === savedMqttEnabled) {
                    radio.checked = true;
                }
            }
            
            // Set MQTT form fields
            const brokerField = document.getElementById('mqtt-broker');
            if (brokerField && savedMqttBroker) {
                brokerField.value = savedMqttBroker;
            }
            
            const usernameField = document.getElementById('mqtt-username');
            if (usernameField && savedMqttUsername) {
                usernameField.value = savedMqttUsername;
            }
            
            const intervalField = document.getElementById('mqtt-interval');
            if (intervalField && savedMqttInterval) {
                intervalField.value = savedMqttInterval;
            }
            
            // Update visibility of MQTT settings
            toggleMqttSettings();
        }

        // Load MQTT settings from server response
        function loadMqttSettings(data) {
            // Save values to localStorage for future use
            localStorage.setItem('mqttPublishEnabled', data.mqttPublishEnabled ? 'enabled' : 'disabled');
            localStorage.setItem('mqttBrokerAddress', data.mqttBrokerAddress || '');
            localStorage.setItem('mqttUsername', data.mqttUsername || '');
            localStorage.setItem('mqttPublishInterval', data.mqttPublishInterval || '60');
            
            // Set MQTT radio buttons
            const mqttRadios = document.getElementsByName('mqttPublishEnabled');
            for (let radio of mqttRadios) {
                if (radio.value === (data.mqttPublishEnabled ? 'enabled' : 'disabled')) {
                    radio.checked = true;
                }
            }
            
            // Set MQTT form fields
            const brokerField = document.getElementById('mqtt-broker');
            if (brokerField && data.mqttBrokerAddress) {
                brokerField.value = data.mqttBrokerAddress;
            }
            
            const usernameField = document.getElementById('mqtt-username');
            if (usernameField && data.mqttUsername) {
                usernameField.value = data.mqttUsername;
            }
            
            // Update password field placeholder if a password exists
            const passwordField = document.getElementById('mqtt-password');
            if (passwordField && data.hasMqttPassword) {
                passwordField.placeholder = '••••••••'; // Show password dots as placeholder
            }
            
            const intervalField = document.getElementById('mqtt-interval');
            if (intervalField && data.mqttPublishInterval) {
                intervalField.value = data.mqttPublishInterval;
            }
            
            // Update visibility of MQTT settings
            toggleMqttSettings();
        }

    document.addEventListener('DOMContentLoaded', function() {
        // Initialize theme toggle and UI
        setupThemeToggle();
        populateTimeOptions();
        
        // Apply defaults immediately
        applyDefaultValues();
        applyMqttDefaults();
        
        // Set up radio buttons for sensorhub
        const sensorhubRadios = document.getElementsByName('useSensorhub');
        for (let radio of sensorhubRadios) {
            radio.addEventListener('change', toggleSensorhubSettings);
        }
        
        // Set up radio buttons for MQTT
        const mqttRadios = document.getElementsByName('mqttPublishEnabled');
        for (let radio of mqttRadios) {
            radio.addEventListener('change', toggleMqttSettings);
        }
        
        // Add validation for MQTT publish interval
        const intervalField = document.getElementById('mqtt-interval');
        if (intervalField) {
            intervalField.addEventListener('change', function() {
                const value = parseInt(this.value);
                if (isNaN(value) || value < 10) {
                    this.value = 10;
                } else if (value > 3600) {
                    this.value = 3600;
                }
            });
        }
        
        // Setup relay controls
        setupRelayControls();
        
        // Load data asynchronously
        setTimeout(() => {
            loadPreferences();
            updateRelayStatus();
        }, 100);
        
        // Set up periodic updates
        const relayUpdateInterval = setInterval(updateRelayStatus, 10000);
        
        // Handle form submission with optimistic UI update
        const form = document.getElementById('preferences-form');
        if (form) {
            form.onsubmit = async function(e) {
                e.preventDefault();
                
                try {
                    const formData = new FormData(this);
                    const preferences = {
                        nightDimming: formData.get('nightDimming') === 'enabled',
                        dayBrightness: parseInt(formData.get('dayBrightness')),
                        nightBrightness: parseInt(formData.get('nightBrightness')),
                        nightStartHour: parseInt(formData.get('nightStartHour')),
                        nightEndHour: parseInt(formData.get('nightEndHour')),
                        
                        // Sensorhub settings
                        useSensorhub: formData.get('useSensorhub') === 'enabled',
                        sensorhubUsername: formData.get('sensorhubUsername'),
                        
                        // MQTT settings
                        mqttPublishEnabled: formData.get('mqttPublishEnabled') === 'enabled',
                        mqttBrokerAddress: formData.get('mqttBrokerAddress'),
                        mqttUsername: formData.get('mqttUsername'),
                        mqttPublishInterval: parseInt(formData.get('mqttPublishInterval'))
                    };
                    
                    // Only include passwords if provided (don't clear existing passwords)
                    const sensorhubPassword = formData.get('sensorhubPassword');
                    if (sensorhubPassword && sensorhubPassword.length > 0) {
                        preferences.sensorhubPassword = sensorhubPassword;
                    }
                    
                    const mqttPassword = formData.get('mqttPassword');
                    if (mqttPassword && mqttPassword.length > 0) {
                        preferences.mqttPassword = mqttPassword;
                    }
                    
                    // Update localStorage immediately for responsive UI
                    localStorage.setItem('dayBrightness', preferences.dayBrightness);
                    localStorage.setItem('nightBrightness', preferences.nightBrightness);
                    localStorage.setItem('nightStartHour', preferences.nightStartHour);
                    localStorage.setItem('nightEndHour', preferences.nightEndHour);
                    localStorage.setItem('nightDimming', preferences.nightDimming ? 'enabled' : 'disabled');
                    
                    // Sensorhub localStorage
                    localStorage.setItem('useSensorhub', preferences.useSensorhub ? 'enabled' : 'disabled');
                    if (preferences.sensorhubUsername) {
                        localStorage.setItem('sensorhubUsername', preferences.sensorhubUsername);
                    }
                    
                    // MQTT localStorage
                    localStorage.setItem('mqttPublishEnabled', preferences.mqttPublishEnabled ? 'enabled' : 'disabled');
                    localStorage.setItem('mqttBrokerAddress', preferences.mqttBrokerAddress);
                    localStorage.setItem('mqttUsername', preferences.mqttUsername);
                    localStorage.setItem('mqttPublishInterval', preferences.mqttPublishInterval);

                    // Show immediate feedback
                    showStatus('Saving preferences...');

                    const response = await fetchWithTimeout('/api/preferences', {
                        method: 'POST',
                        headers: {
                            'Content-Type': 'application/json',
                            'Accept': 'application/json'
                        },
                        body: JSON.stringify(preferences)
                    }, 5000);

                    const result = await response.json();
                    if (response.ok && result.success) {
                        showStatus('Preferences saved successfully');
                    } else {
                        throw new Error(result.error || 'Failed to save preferences');
                    }
                } catch (error) {
                    console.error('Error saving preferences:', error);
                    showStatus('Failed to save preferences: ' + error.message, true);
                }
            };
        }
        
        // Clean up intervals when page is unloaded
        window.addEventListener('unload', function() {
            if (relayUpdateInterval) {
                clearInterval(relayUpdateInterval);
            }
        });
    });
    </script>
    </body>
    </html>
    )rawliteral";