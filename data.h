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
      body {
        font-family: "Gill Sans", "Gill Sans MT", Calibri, "Trebuchet MS",
          sans-serif;
        color: rgb(128, 128, 128);
        font-size: 1.5rem;
        margin: 0;
        width: 100%;
        height: 100%;
        display: flex;
        flex-direction: column;
        justify-content: center;
        align-items: center;
      }

      h1 {
        text-align: center;
        margin: 0.5rem;
      }

      p {
        text-align: center;
        margin: 0.5rem;
      }

      #canvas {
        display: block;
        border: 1px solid #ddd;
        touch-action: none; /* Prevent scrolling on touch devices while interacting with the canvas */
      }

      #slider {
        width: 80%;
        margin: 1rem;
      }
    </style>
  </head>
  <body>
    <h1>DELTA BOT</h1>
    <p>
      X: <span id="x_coordinate">0</span> Y: <span id="y_coordinate">0</span> Z:
      <span id="z_value">0</span>
    </p>
    <p>
      θ1: <span id="theta1">0</span> θ2: <span id="theta2">0</span> θ3:
      <span id="theta3">0</span>
    </p>

    <input type="range" id="slider" min="0" max="100" value="50" />
    <canvas id="canvas"></canvas>

    <script type="text/javascript">
      var canvas, ctx;
      var x_orig, y_orig, outerRadius, innerRadius;
      var paint = false;
      var coord = { x: 0, y: 0 };
      var websocket;

      const minValues = { x: -124.02, y: -134.32, z: -323.24 };
      const maxValues = { x: 124.02, y: 139.53, z: -173.49 };

      async function init() {
        // Get WebSocket URL from window location href
        var url = "ws://" + window.location.hostname + ":1337/";
        console.log("Connecting to WebSocket at " + url);

        // Connect to WebSocket server
        await wsConnect(url);
      }

      async function wsConnect(url) {
        // Connect to WebSocket server
        websocket = new WebSocket(url);

        // Assign callbacks
        websocket.onopen = function (evt) {
          onOpen(evt);
        };
        websocket.onclose = function (evt) {
          onClose(evt);
        };
        websocket.onmessage = function (evt) {
          onMessage(evt);
        };
        websocket.onerror = function (evt) {
          onError(evt);
        };
      }

      function onOpen(evt) {
        // Log connection state
        console.log("Connected to WebSocket");
      }

      function onClose(evt) {
        // Try to reconnect after a few seconds
        setTimeout(function () {
          wsConnect(websocket.url);
        }, 2000);
      }

      function onMessage(evt) {
        // Print out our received message
        console.log("Received: " + evt.data);
        let data = JSON.parse(evt.data);
        document.getElementById("x_coordinate").innerText = data.x;
        document.getElementById("y_coordinate").innerText = data.y;
        document.getElementById("z_value").innerText = data.z;
        document.getElementById("theta1").innerText = data.theta1.toFixed(2);
        document.getElementById("theta2").innerText = data.theta2.toFixed(2);
        document.getElementById("theta3").innerText = data.theta3.toFixed(2);
        coord.x = data.coord_x;
        coord.y = data.coord_y;
        ctx.clearRect(0, 0, canvas.width, canvas.height);
        background();
        joystick(data.coord_x, data.coord_y);
      }

      function onError(evt) {
        console.log("WebSocket error: " + evt.data);
      }

      function doSend(message) {
        websocket.send(JSON.stringify(message));
      }

      window.addEventListener("load", () => {
        init();
        canvas = document.getElementById("canvas");
        ctx = canvas.getContext("2d");
        resize();
        document.addEventListener("mousedown", startDrawing);
        document.addEventListener("mouseup", stopDrawing);
        document.addEventListener("mousemove", draw);
        document.addEventListener("touchstart", startDrawing);
        document.addEventListener("touchend", stopDrawing);
        document.addEventListener("touchcancel", stopDrawing);
        document.addEventListener("touchmove", draw);
        window.addEventListener("resize", resize);

        var slider = document.getElementById("slider");
        slider.addEventListener("input", updateSlider);
      });

      function resize() {
        var minDimension = Math.min(window.innerWidth, window.innerHeight);
        outerRadius = minDimension / 4;
        innerRadius = outerRadius / 2;
        canvas.width = window.innerWidth;
        canvas.height = window.innerHeight * 0.8; // Increased height to center properly
        background();
        joystick(canvas.width / 2, canvas.height / 2); // Centered joystick
      }

      function background() {
        x_orig = canvas.width / 2;
        y_orig = canvas.height / 2; // Centered joystick
        ctx.beginPath();
        ctx.arc(x_orig, y_orig, outerRadius - 10, 0, Math.PI * 2, true);
        ctx.fillStyle = "#ECE5E5";
        ctx.fill();
      }

      function joystick(x, y) {
        // Outer circle
        ctx.beginPath();
        ctx.arc(x, y, outerRadius - 50, 0, Math.PI * 2, true);
        ctx.fillStyle = "#F08080";
        ctx.fill();
        ctx.strokeStyle = "#F6ABAB";
        ctx.lineWidth = 8;
        ctx.stroke();

        // Inner circle
        ctx.beginPath();
        ctx.arc(x, y, innerRadius - 20, 0, Math.PI * 2, true);
        ctx.fillStyle = "#333";
        ctx.fill();
      }

      function getPosition(event) {
        var mouse_x = event.clientX || event.touches[0].clientX;
        var mouse_y = event.clientY || event.touches[0].clientY;
        coord.x = mouse_x - canvas.offsetLeft;
        coord.y = mouse_y - canvas.offsetTop;
      }

      function isItInTheCircle() {
        var current_radius = Math.sqrt(
          Math.pow(coord.x - x_orig, 2) + Math.pow(coord.y - y_orig, 2)
        );
        return outerRadius >= current_radius;
      }

      function startDrawing(event) {
        paint = true;
        getPosition(event);
        if (isItInTheCircle()) {
          ctx.clearRect(0, 0, canvas.width, canvas.height);
          background();
          joystick(coord.x, coord.y);
          draw();
        }
      }

      async function draw(event) {
        if (paint) {
          ctx.clearRect(0, 0, canvas.width, canvas.height);
          background();
          getPosition(event);
          var angle = Math.atan2(coord.y - y_orig, coord.x - x_orig);
          var angle_in_degrees =
            Math.sign(angle) === -1
              ? Math.round((-angle * 180) / Math.PI)
              : Math.round(360 - (angle * 180) / Math.PI);
          if (isItInTheCircle()) {
            joystick(coord.x, coord.y);
            x = coord.x;
            y = coord.y;
          } else {
            x = outerRadius * Math.cos(angle) + x_orig;
            y = outerRadius * Math.sin(angle) + y_orig;
            joystick(x, y);
          }

          var x_relative = Math.round(
            mapValue(
              x - x_orig,
              -outerRadius,
              outerRadius,
              minValues.x,
              maxValues.x
            )
          );
          var y_relative = Math.round(
            mapValue(
              y - y_orig,
              -outerRadius,
              outerRadius,
              minValues.y,
              maxValues.y
            )
          );
          document.getElementById("x_coordinate").innerText = x_relative;
          document.getElementById("y_coordinate").innerText = y_relative;
          await send(x_relative, y_relative);
        }
      }

      function stopDrawing() {
        paint = false;
      }

      function mapValue(value, inMin, inMax, outMin, outMax) {
        return ((value - inMin) * (outMax - outMin)) / (inMax - inMin) + outMin;
      }

      async function send(x, y) {
        var zValue = document.getElementById("slider").value;
        var z = Math.round(mapValue(zValue, 0, 100, minValues.z, maxValues.z));
        document.getElementById("z_value").innerText = z;

        var { theta1, theta2, theta3, fl } = IKinem(x, y, z);
        if (fl === 0) {
          document.getElementById("theta1").innerText = theta1.toFixed(2);
          document.getElementById("theta2").innerText = theta2.toFixed(2);
          document.getElementById("theta3").innerText = theta3.toFixed(2);

          // Send data to WebSocket server
          doSend({
            x: x,
            y: y,
            z: z,
            theta1: theta1,
            theta2: theta2,
            theta3: theta3,
            coord_x: coord.x,
            coord_y: coord.y,
          });
        } else {
          document.getElementById("theta1").innerText = "N/A";
          document.getElementById("theta2").innerText = "N/A";
          document.getElementById("theta3").innerText = "N/A";
        }
      }

      function updateSlider(event) {
        var sliderValue = event.target.value;
        var z = Math.round(
          mapValue(sliderValue, 0, 100, minValues.z, maxValues.z)
        );
        document.getElementById("z_value").innerText = z;
        // Send updated Z value along with current X and Y
        send(coord.x, coord.y);
      }

      function IKinem(X, Y, Z) {
        const R = 120;
        const r = 30;
        const L = 90;
        const l = 250;

        function IKinemTh(x0, y0, z0) {
          const y1 = -R;
          y0 = y0 - r; // shift center to edge

          const a =
            (x0 * x0 + y0 * y0 + z0 * z0 + L * L - l * l - y1 * y1) / (2 * z0);
          const b = (y1 - y0) / z0;
          const D = -(a + b * y1) * (a + b * y1) + L * L * (b * b + 1);

          if (D < 0) {
            return NaN; // non-existing
          } else {
            const yj = (y1 - a * b - Math.sqrt(D)) / (b * b + 1);
            const zj = a + b * yj;
            let theta = Math.atan(-zj / (y1 - yj));
            if (yj > y1) {
              theta += Math.PI;
            }
            theta = (theta * 180) / Math.PI; // Convert to degrees
            return theta;
          }
        }

        const theta1 = IKinemTh(X, Y, Z);
        const x0_2 =
          X * Math.cos((2 * Math.PI) / 3) + Y * Math.sin((2 * Math.PI) / 3);
        const y0_2 =
          Y * Math.cos((2 * Math.PI) / 3) - X * Math.sin((2 * Math.PI) / 3);
        const theta2 = IKinemTh(x0_2, y0_2, Z);

        const x0_3 =
          X * Math.cos((2 * Math.PI) / 3) - Y * Math.sin((2 * Math.PI) / 3);
        const y0_3 =
          Y * Math.cos((2 * Math.PI) / 3) + X * Math.sin((2 * Math.PI) / 3);
        const theta3 = IKinemTh(x0_3, y0_3, Z);

        const fl = !isNaN(theta1) && !isNaN(theta2) && !isNaN(theta3) ? 0 : -1;
        return { theta1, theta2, theta3, fl };
      }
    </script>
  </body>
</html>
)rawliteral";
#endif // DATA_H