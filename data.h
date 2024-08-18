#ifndef DATA_H
#define DATA_H
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <title>Mousebot</title>
    <meta
      name="viewport"
      content="width=device-width, initial-scale=1.0, user-scalable=no"
    />
    <style>
      html,
      body {
        overflow: hidden;
        height: 100%;
      }
      body {
        font-family: "Gill Sans", "Gill Sans MT", Calibri, "Trebuchet MS",
          sans-serif;
        color: rgb(128, 128, 128);
        font-size: 1.5rem;
        margin: 0;
        width: 100%;
        display: flex;
        flex-direction: column;
        justify-content: center;
        align-items: center;
        position: fixed;
      }
      h1,
      p {
        text-align: center;
        margin: 0.5rem;
      }
      #canvas {
        display: block;
        border: 1px solid #ddd;
        touch-action: none;
      }
      .slider {
        -webkit-appearance: none;
        width: 80%;
        height: 10px;
        border-radius: 5px;
        background-color: #4158d0;
        background-image: linear-gradient(
          43deg,
          #4158d0 0%,
          #c850c0 46%,
          #ffcc70 100%
        );
        outline: none;
        opacity: 0.7;
        -webkit-transition: 0.2s;
        transition: opacity 0.2s;
        margin: 1rem;
      }
      .slider::-webkit-slider-thumb {
        -webkit-appearance: none;
        appearance: none;
        width: 20px;
        height: 20px;
        border-radius: 50%;
        background-color: #4c00ff;
        background-image: linear-gradient(160deg, #4900f5 0%, #80d0c7 100%);
        cursor: pointer;
      }
      .slider::-moz-range-thumb {
        width: 20px;
        height: 20px;
        border-radius: 50%;
        background-color: #0093e9;
        background-image: linear-gradient(160deg, #0093e9 0%, #80d0c7 100%);
        cursor: pointer;
      }
    </style>
  </head>
  <body>
    <h1>DELTA BOT</h1>
    <p>
      X: <span id="x_coordinate">0</span> Y: <span id="y_coordinate">0</span> Z:
      <span id="z_value">0</span>
    </p>
    <input
      type="range"
      id="slider"
      class="slider"
      min="0"
      max="100"
      value="50"
    />
    <canvas id="canvas"></canvas>

    <script type="text/javascript">
      var canvas,
        ctx,
        paint = false,
        coord = { x: 0, y: 0 };
      z = 0;
      const minValues = { x: -124, y: -134, z: -322 };
      const maxValues = { x: 124, y: 139, z: -173 };
      var x_orig, y_orig, outerRadius;

      async function init() {
        var url = "ws://" + window.location.hostname + ":1337/";
        await wsConnect(url);
      }

      async function wsConnect(url) {
        websocket = new WebSocket(url);
        websocket.onopen = function (evt) {
          console.log("Connected to WebSocket");
        };
        websocket.onclose = function (evt) {
          setTimeout(function () {
            wsConnect(websocket.url);
          }, 2000);
        };
        websocket.onmessage = function (evt) {
          onMessage(evt);
        };
        websocket.onerror = function (evt) {
          console.log("WebSocket error: " + evt.data);
        };
      }

      function onMessage(evt) {
        let data = JSON.parse(evt.data);
        document.getElementById("x_coordinate").innerText = data.x;
        document.getElementById("y_coordinate").innerText = data.y;
        document.getElementById("z_value").innerText = data.z;
      }

      function doSend(message) {
        websocket.send(JSON.stringify(message));
      }

      async function sendMessage(x, y, z) {
        const message = { x, y, z };
        await doSend(message);
      }

      window.addEventListener("load", () => {
        init();
        canvas = document.getElementById("canvas");
        ctx = canvas.getContext("2d");
        resizeCanvas();

        document.addEventListener("mousedown", startDrawing);
        document.addEventListener("mouseup", stopDrawing);
        document.addEventListener("mousemove", draw);
        document.addEventListener("touchstart", startDrawing);
        document.addEventListener("touchend", stopDrawing);
        document.addEventListener("touchmove", draw);
        window.addEventListener("resize", resizeCanvas);

        // Add event listener for the slider
        var slider = document.getElementById("slider");
        slider.addEventListener("input", updateSlider);
      });

      function resizeCanvas() {
        var minDimension = Math.min(window.innerWidth, window.innerHeight);
        outerRadius = minDimension / 4;
        canvas.width = window.innerWidth;
        canvas.height = window.innerHeight * 0.8;
        drawBackground();
        drawJoystick(canvas.width / 2, canvas.height / 2);
      }

      function drawBackground() {
        x_orig = canvas.width / 2;
        y_orig = canvas.height / 2;
        ctx.beginPath();
        ctx.arc(x_orig, y_orig, outerRadius, 0, Math.PI * 2, true);
        ctx.fillStyle = "#BED7DC";
        ctx.fill();
      }

      function drawJoystick(x, y) {
        ctx.beginPath();
        ctx.arc(x, y, outerRadius - 30, 0, Math.PI * 2, true);
        ctx.fillStyle = "#000000";
        ctx.fill();
        ctx.strokeStyle = "#E5DDC5";
        ctx.lineWidth = 10;
        ctx.stroke();
      }

      function getPosition(event) {
        var mouse_x = event.clientX || event.touches[0].clientX;
        var mouse_y = event.clientY || event.touches[0].clientY;
        coord.x = mouse_x - canvas.offsetLeft;
        coord.y = mouse_y - canvas.offsetTop;
      }

      function isInCircle() {
        var current_radius = Math.sqrt(
          Math.pow(coord.x - x_orig, 2) + Math.pow(coord.y - y_orig, 2)
        );
        return outerRadius >= current_radius;
      }

      function startDrawing(event) {
        getPosition(event);
        if (isInCircle()) {
          paint = true;
          ctx.clearRect(0, 0, canvas.width, canvas.height);
          drawBackground();
          drawJoystick(coord.x, coord.y);
          draw(event);
        }
      }

      async function draw(event) {
        if (paint) {
          getPosition(event);
          var angle = Math.atan2(coord.y - y_orig, coord.x - x_orig);
          if (isInCircle()) {
            ctx.clearRect(0, 0, canvas.width, canvas.height);
            drawBackground();
            drawJoystick(coord.x, coord.y);
          } else {
            coord.x = outerRadius * Math.cos(angle) + x_orig;
            coord.y = outerRadius * Math.sin(angle) + y_orig;
            ctx.clearRect(0, 0, canvas.width, canvas.height);
            drawBackground();
            drawJoystick(coord.x, coord.y);
          }

          var x_relative = Math.round(
            mapValue(
              coord.x - x_orig,
              -outerRadius,
              outerRadius,
              minValues.x,
              maxValues.x
            )
          );
          var y_relative = Math.round(
            mapValue(
              coord.y - y_orig,
              -outerRadius,
              outerRadius,
              minValues.y,
              maxValues.y
            )
          );

          document.getElementById("x_coordinate").innerText = x_relative;
          document.getElementById("y_coordinate").innerText = y_relative;

          sendMessage(x_relative, y_relative, z);
        }
      }

      function stopDrawing() {
        paint = false;
      }

      function mapValue(value, inMin, inMax, outMin, outMax) {
        return ((value - inMin) * (outMax - outMin)) / (inMax - inMin) + outMin;
      }

      function updateSlider(event) {
        var sliderValue = event.target.value;
        z = Math.round(mapValue(sliderValue, 0, 100, minValues.z, maxValues.z));
        document.getElementById("z_value").innerText = z;

        // Send current x, y, and updated z
        const x_relative = parseInt(
          document.getElementById("x_coordinate").innerText
        );
        const y_relative = parseInt(
          document.getElementById("y_coordinate").innerText
        );

        // Send the message with current x, y, and updated z
        sendMessage(x_relative, y_relative, z);
      }
    </script>
  </body>
</html>
)rawliteral";
#endif // DATA_H