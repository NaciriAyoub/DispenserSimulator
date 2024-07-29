using namespace net;

// HTML content
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang='en'>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>Pulser Control</title>
    <style>
        .container {
            margin-top: 50px;
        }
        .pump-controls {
            margin-bottom: 20px;
        }
        #connection-status {
            margin-bottom: 20px;
        }
        .log-sent {
            color: blue;
        }
        .log-received {
            color: green;
        }
    </style>
</head>
<body>
    <div class='container'>
        <h1>Pulser Control</h1>
        <div>
            <label for='ws-url'>Server:</label>
            <input type='text' id='ws-url' placeholder='ws://192.168.4.1:81' value='ws://192.168.4.1:81'>
            <button id='connect-btn'>Connect</button>
            <button id='disconnect-btn' disabled>Disconnect</button>
            <p id='connection-status'>Status: Disconnected</p>
        </div>
        <div id='pumps'>
            <div class='pump-controls' id='pump-1'>
                <h4>Pump 1 <button id='hook-1' onclick='toggleHook(1)'>OnHook</button><span id='relay-status-1'>&nbsp;&nbsp; Relay: Unknown</span></h4>
                <input type='number' id='pulse-count-1' placeholder='Pulse Count' value='100'>
                <input type='number' id='pulse-delay-1' placeholder='Pulse Delay (ms)' value='50'>
                <button id='start-1' onclick='startPulsing(1)'>Start</button>
                <button id='stop-1' onclick='stopPulsing(1)' disabled>Stop</button>
                <button id='continue-1' onclick='continuePulsing(1)' disabled>Continue</button>
                <button id='cancel-1' onclick='cancelPulsing(1)' disabled>Cancel</button>
            </div>
            <!-- Repeat for other pumps -->
            <div class='pump-controls' id='pump-2'>
                <h4>Pump 2 <button id='hook-2' onclick='toggleHook(2)'>OnHook</button><span id='relay-status-2'>&nbsp;&nbsp; Relay: Unknown</span></h4>
                <input type='number' id='pulse-count-2' placeholder='Pulse Count' value='100'>
                <input type='number' id='pulse-delay-2' placeholder='Pulse Delay (ms)' value='50'>
                <button id='start-2' onclick='startPulsing(2)'>Start</button>
                <button id='stop-2' onclick='stopPulsing(2)' disabled>Stop</button>
                <button id='continue-2' onclick='continuePulsing(2)' disabled>Continue</button>
                <button id='cancel-2' onclick='cancelPulsing(2)' disabled>Cancel</button>
            </div>
            <div class='pump-controls' id='pump-3'>
                <h4>Pump 3 <button id='hook-3' onclick='toggleHook(3)'>OnHook</button><span id='relay-status-3'>&nbsp;&nbsp; Relay: Unknown</span></h4>
                <input type='number' id='pulse-count-3' placeholder='Pulse Count' value='100'>
                <input type='number' id='pulse-delay-3' placeholder='Pulse Delay (ms)' value='50'>
                <button id='start-3' onclick='startPulsing(3)'>Start</button>
                <button id='stop-3' onclick='stopPulsing(3)' disabled>Stop</button>
                <button id='continue-3' onclick='continuePulsing(3)' disabled>Continue</button>
                <button id='cancel-3' onclick='cancelPulsing(3)' disabled>Cancel</button>
            </div>
            <div class='pump-controls' id='pump-4'>
                <h4>Pump 4 <button id='hook-4' onclick='toggleHook(4)'>OnHook</button><span id='relay-status-4'>&nbsp;&nbsp; Relay: Unknown</span></h4>
                <input type='number' id='pulse-count-4' placeholder='Pulse Count' value='100'>
                <input type='number' id='pulse-delay-4' placeholder='Pulse Delay (ms)' value='50'>
                <button id='start-4' onclick='startPulsing(4)'>Start</button>
                <button id='stop-4'  onclick='stopPulsing(4)' disabled>Stop</button>
                <button id='continue-4' onclick='continuePulsing(4)' disabled>Continue</button>
                <button id='cancel-4' onclick='cancelPulsing(4)' disabled>Cancel</button>
            </div>
            <div class='pump-controls' id='pump-5'>
                <h4>Pump 5 <button id='hook-5' onclick='toggleHook(5)'>OnHook</button><span id='relay-status-5'>&nbsp;&nbsp; Relay: Unknown</span></h4>
                <input type='number' id='pulse-count-5' placeholder='Pulse Count' value='100'>
                <input type='number' id='pulse-delay-5' placeholder='Pulse Delay (ms)' value='50'>
                <button id='start-5' onclick='startPulsing(5)'>Start</button>
                <button id='stop-5' onclick='stopPulsing(5)' disabled>Stop</button>
                <button id='continue-5' onclick='continuePulsing(5)' disabled>Continue</button>
                <button id='cancel-5' onclick='cancelPulsing(5)' disabled>Cancel</button>
            </div>
        </div>
        <div class='text-center'>
            <button onclick='requestPulses()'>Request Pulses</button>
            <button onclick='requestHooks()'>Request Hooks</button>
            <button onclick='requestRelays()'>Request Relays</button>
            <button onclick='clearLogs()'>Clear Logs</button>
        </div>
        <div>
            <h4>Response:</h4>
            <pre id='response' style='background-color: #f8f9fa; padding: 10px; border: 1px solid #ddd;'></pre>
            <h4>WebSocket Log:</h4>
            <pre id='ws-log' style='background-color: #f8f9fa; padding: 10px; border: 1px solid #ddd; height: 200px; overflow-y: scroll;'></pre>
        </div>
    </div>

    <script>
        document.addEventListener('DOMContentLoaded', (event) => {
            let socket;
            const connectBtn = document.getElementById('connect-btn');
            const disconnectBtn = document.getElementById('disconnect-btn');
            const connectionStatus = document.getElementById('connection-status');
            const responsePre = document.getElementById('response');
            const wsLog = document.getElementById('ws-log');

            connectBtn.addEventListener('click', () => {
                const wsUrl = document.getElementById('ws-url').value;
                connectBtn.disabled = true;
                socket = new WebSocket(wsUrl);

                socket.onopen = () => {
                    connectionStatus.textContent = 'Status: Connected';
                    connectBtn.disabled = true;
                    disconnectBtn.disabled = false;
                    logMessage('Connected', 'log-received');
                    requestHooks(); // Request hooks immediately after connecting
                    requestRelays(); // Request relays immediately after connecting
                };

                socket.onerror = () => {
                    connectionStatus.textContent = 'Status: Connection Error';
                    connectBtn.disabled = false;
                };

                socket.onclose = () => {
                    connectionStatus.textContent = 'Status: Disconnected';
                    connectBtn.disabled = false;
                    disconnectBtn.disabled = true;
                    logMessage('Disconnected', 'log-received');
                };

                socket.onmessage = (event) => {
                    const data = event.data;
                    logMessage(`Received: ${data}`, 'log-received');

                    if (data.includes('Pulser') && data.includes('ended pulsing')) {
                        const pumpNumber = data.match(/Pulser (\d+)/)[1];
                        enableButton(`start-${pumpNumber}`);
                        disableButton(`stop-${pumpNumber}`);
                        disableButton(`continue-${pumpNumber}`);
                        disableButton(`cancel-${pumpNumber}`);
                    }

                    const hookMatch = data.match(/Hook (\d+) (OnHook|OffHook)/);
                    if (hookMatch) {
                        const hookNumber = hookMatch[1];
                        const hookState = hookMatch[2];
                        const button = document.getElementById(`hook-${hookNumber}`);
                        button.textContent = hookState;
                    }

                    const relayMatch = data.match(/Relay, (\d+) (Closed|Opened)/);
                    if (relayMatch) {
                        const relayNumber = relayMatch[1];
                        const relayState = relayMatch[2];
                        const span = document.getElementById(`relay-status-${relayNumber}`);
                        span.innerHTML  = '&nbsp;&nbsp; Relay: ' + relayState;
                    }

                    // Parse hooks JSON data
                    if (data.includes('"Hooks"') || data.includes('"Relays"') || data.includes('"Pulsers"')) {
                        try {
                            const jsonData = JSON.parse(data);
                            if (jsonData.Hooks) {
                                jsonData.Hooks.forEach(hook => {
                                    const hookNumber = hook.Hook;
                                    const hookState = hook.State;
                                    const button = document.getElementById(`hook-${hookNumber}`);
                                    button.textContent = hookState;
                                });
                            } else if (jsonData.Relays) {
                                // Handle relays JSON data
                                 jsonData.Relays.forEach((relay, index) => {
                                    const relayNumber = relay.Relay;
                                    const relayState = relay.State;
                                    const span = document.getElementById(`relay-status-${relayNumber}`);
                                    span.innerHTML = `&nbsp;&nbsp; Relay: ${relayState}`;
                                });
                            } else if (jsonData.Pulsers) {
                                // Handle pulsers JSON data
                                jsonData.Pulsers.forEach(pulser => {
                                    // Process pulser data as needed
                                });
                            }
                            responsePre.textContent = JSON.stringify(jsonData, null, 2);
                        } catch (e) {
                            console.error('Failed to parse JSON:', e);
                        }
                    } else {
                        responsePre.textContent = data;
                    }
                };
            });

            disconnectBtn.addEventListener('click', () => {
                if (socket) {
                    socket.close();
                }
            });

            window.startPulsing = function(pumpNumber) {
                const count = document.getElementById(`pulse-count-${pumpNumber}`).value;
                const delay = document.getElementById(`pulse-delay-${pumpNumber}`).value;
                const message = `Pulser ${pumpNumber} Count ${count} DelayB ${delay}`;
                if (socket && socket.readyState === WebSocket.OPEN) {
                    socket.send(message);
                    logMessage(`Sent: ${message}`, 'log-sent');
                    disableButton(`start-${pumpNumber}`);
                    enableButton(`stop-${pumpNumber}`);
                    disableButton(`continue-${pumpNumber}`);
                    disableButton(`cancel-${pumpNumber}`);
                }
            };

            window.stopPulsing = function(pumpNumber) {
                const message = `Pulser ${pumpNumber} Stop`;
                if (socket && socket.readyState === WebSocket.OPEN) {
                    socket.send(message);
                    logMessage(`Sent: ${message}`, 'log-sent');
                    enableButton(`continue-${pumpNumber}`);
                    disableButton(`stop-${pumpNumber}`);
                    disableButton(`start-${pumpNumber}`);
                    enableButton(`cancel-${pumpNumber}`);
                }
            };

            window.continuePulsing = function(pumpNumber) {
                const message = `Pulser ${pumpNumber} Continue`;
                if (socket && socket.readyState === WebSocket.OPEN) {
                    socket.send(message);
                    logMessage(`Sent: ${message}`, 'log-sent');
                    disableButton(`continue-${pumpNumber}`);
                    enableButton(`stop-${pumpNumber}`);
                    disableButton(`start-${pumpNumber}`);
                    enableButton(`cancel-${pumpNumber}`);
                }
            };

            window.cancelPulsing = function(pumpNumber) {
                const message = `Pulser ${pumpNumber} Cancel`;
                if (socket && socket.readyState === WebSocket.OPEN) {
                    socket.send(message);
                    logMessage(`Sent: ${message}`, 'log-sent');
                    enableButton(`start-${pumpNumber}`);
                    disableButton(`stop-${pumpNumber}`);
                    disableButton(`continue-${pumpNumber}`);
                    disableButton(`cancel-${pumpNumber}`);
                }
            };

            window.toggleHook = function(hookNumber) {
                const button = document.getElementById(`hook-${hookNumber}`);
                const newState = button.textContent === 'OnHook' ? 'OffHook' : 'OnHook';
                const message = `Hook ${hookNumber} ${newState}`;
                if (socket && socket.readyState === WebSocket.OPEN) {
                    socket.send(message);
                    logMessage(`Sent: ${message}`, 'log-sent');
                }
            };

            window.requestPulses = function() {
                const message = 'Pulsers';
                if (socket && socket.readyState === WebSocket.OPEN) {
                    socket.send(message);
                    logMessage(`Sent: ${message}`, 'log-sent');
                }
            };

            window.requestHooks = function() {
                const message = 'Hooks';
                if (socket && socket.readyState === WebSocket.OPEN) {
                    socket.send(message);
                    logMessage(`Sent: ${message}`, 'log-sent');
                }
            };
            

            window.requestRelays = function() {
                const message = 'Relays';
                if (socket && socket.readyState === WebSocket.OPEN) {
                    socket.send(message);
                    logMessage(`Sent: ${message}`, 'log-sent');
                }
            };

            window.logMessage = function(message, type) {
                const logEntry = document.createElement('div');
                logEntry.textContent = message;
                logEntry.className = type;
                wsLog.appendChild(logEntry);
                wsLog.scrollTop = wsLog.scrollHeight;
            };

            window.enableButton = function(id) {
                document.getElementById(id).disabled = false;
            };

            window.disableButton = function(id) {
                document.getElementById(id).disabled = true;
            };

            window.clearLogs = function() {
                responsePre.textContent = '';
                wsLog.textContent = '';
            };
        });
    </script>
</body>
</html>
)rawliteral";
