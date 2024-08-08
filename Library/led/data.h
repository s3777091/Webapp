#ifndef DATA_H
#define DATA_H

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="utf-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>WebSocket LED Control</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            text-align: center;
            background-color: #f0f0f0;
            margin: 0;
            padding: 0;
        }
        h2 {
            color: #333;
        }
        .container {
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            height: 100vh;
        }
        canvas {
            border: 1px solid #333;
            border-radius: 50%;
            margin: 10px;
        }
        input[type=range] {
            width: 300px;
            margin-top: 20px;
        }
        #joystick {
            width: 100px;
            height: 100px;
            border: 2px solid #333;
            border-radius: 50%;
            position: relative;
            margin-top: 20px;
            touch-action: none;
        }
        #knob {
            width: 40px;
            height: 40px;
            background-color: #333;
            border-radius: 50%;
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            touch-action: none;
        }
    </style>
    <script type="text/javascript">
        var brightnessSlider;
        var canvas;
        var context;
        var websocket;
        var knob;
        var joystick;

        // This is called when the page finishes loading
        function init() {
            // Get WebSocket URL from window location href
            var url = "ws://" + window.location.hostname + ":1337/";
            console.log("Connecting to WebSocket at " + url);

            // Assign page elements to variables
            brightnessSlider = document.getElementById("brightnessSlider");
            canvas = document.getElementById("led");
            joystick = document.getElementById("joystick");
            knob = document.getElementById("knob");

            // Draw circle in canvas
            context = canvas.getContext("2d");
            context.arc(25, 25, 15, 0, Math.PI * 2, false);
            context.lineWidth = 3;
            context.strokeStyle = "black";
            context.stroke();
            context.fillStyle = "black";
            context.fill();

            // Connect to WebSocket server
            wsConnect(url);

            // Initialize joystick
            joystick.addEventListener('touchstart', onJoystickMove, false);
            joystick.addEventListener('touchmove', onJoystickMove, false);
            joystick.addEventListener('touchend', onJoystickEnd, false);
        }

        // Call this to connect to the WebSocket server
        function wsConnect(url) {
            // Connect to WebSocket server
            websocket = new WebSocket(url);

            // Assign callbacks
            websocket.onopen = function(evt) { onOpen(evt) };
            websocket.onclose = function(evt) { onClose(evt) };
            websocket.onmessage = function(evt) { onMessage(evt) };
            websocket.onerror = function(evt) { onError(evt) };
        }

        // Called when a WebSocket connection is established with the server
        function onOpen(evt) {
            // Log connection state
            console.log("Connected to WebSocket");

            // Enable slider
            brightnessSlider.disabled = false;

            // Get the current state of the LED
            doSend("getLEDState");
        }

        // Called when the WebSocket connection is closed
        function onClose(evt) {
            // Log disconnection state
            console.log("Disconnected from WebSocket");

            // Disable slider
            brightnessSlider.disabled = true;

            // Try to reconnect after a few seconds
            setTimeout(function() { wsConnect(websocket.url) }, 2000);
        }

        // Called when a message is received from the server
        function onMessage(evt) {
            // Print out our received message
            console.log("Received: " + evt.data);

            // Update circle graphic with LED state
            var ledBrightness = evt.data;

            // Update the slider position
            brightnessSlider.value = ledBrightness;

            // Update the LED circle color
            var brightness = parseInt(ledBrightness);
            context.fillStyle = `rgb(${brightness},0,0)`;
            context.fill();
        }

        // Called when a WebSocket error occurs
        function onError(evt) {
            console.log("WebSocket error: " + evt.data);
        }

        // Sends a message to the server (and prints it to the console)
        function doSend(message) {
            console.log("Sending: " + message);
            websocket.send(message);
        }

        // Called whenever the slider value changes
        function onSliderChange() {
            var brightness = brightnessSlider.value;
            doSend("setBrightness:" + brightness);
        }

        // Called when the joystick moves
        function onJoystickMove(evt) {
            evt.preventDefault();
            var touch = evt.targetTouches[0];
            var rect = joystick.getBoundingClientRect();
            var x = touch.clientX - rect.left - rect.width / 2;
            var y = touch.clientY - rect.top - rect.height / 2;
            knob.style.transform = `translate(${x - knob.offsetWidth / 2}px, ${y - knob.offsetHeight / 2}px)`;
            doSend(`joystick:${x.toFixed(2)},${y.toFixed(2)}`);
        }

        // Called when the joystick is released
        function onJoystickEnd(evt) {
            knob.style.transform = 'translate(-50%, -50%)';
            doSend('joystick:0,0');
        }

        // Call the init function as soon as the page loads
        window.addEventListener("load", init, false);
    </script>
</head>
<body>
    <div class="container">
        <h2>LED Control</h2>
        <canvas id="led" width="50" height="50"></canvas>
        <input type="range" id="brightnessSlider" min="0" max="255" value="0" oninput="onSliderChange()" disabled>
        <div id="joystick">
            <div id="knob"></div>
        </div>
    </div>
</body>
</html>
)rawliteral";

#endif // DATA_H