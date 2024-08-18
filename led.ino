#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <WebSocketsServer.h>
#include "data.h"

const char *ssid = "ESP32";
const char *password = "12345678";
const int ws_port = 1337;

// Constants
const float R = 120.0;
const float r = 30.0;
const float L = 90.0;
const float l = 250.0;

AsyncWebServer server(80);
WebSocketsServer webSocket(ws_port);

// Store joystick and angle values
volatile float joystickX = 0.0;
volatile float joystickY = 0.0;
volatile int sliderZ = 0;
volatile float theta1 = 0.0;
volatile float theta2 = 0.0;
volatile float theta3 = 0.0;

char msg_buf[150];

float IKinemTh(float x0, float y0, float z0) {
    float y1 = -R;
    y0 = y0 - r;

    float a = (x0 * x0 + y0 * y0 + z0 * z0 + L * L - l * l - y1 * y1) / (2 * z0);
    float b = (y1 - y0) / z0;
    float D = -(a + b * y1) * (a + b * y1) + L * L * (b * b + 1);

    if (D < 0) {
        return NAN; // Return NaN if no solution
    } else {
        float yj = (y1 - a * b - sqrt(D)) / (b * b + 1);
        float zj = a + b * yj;
        float theta = atan2(-zj, (y1 - yj));
        if (yj > y1) {
            theta += M_PI;
        }
        return theta * 180.0 / M_PI;
    }
}


void IKinem(float X, float Y, float Z, float &theta1, float &theta2, float &theta3) {
    // Calculate theta1
    theta1 = IKinemTh(X, Y, Z);


    // Calculate x0_2, y0_2 for theta2
    float x0_2 = X * cos(2 * M_PI / 3) + Y * sin(2 * M_PI / 3);
    float y0_2 = Y * cos(2 * M_PI / 3) - X * sin(2 * M_PI / 3);
    theta2 = IKinemTh(x0_2, y0_2, Z);


    // Calculate x0_3, y0_3 for theta3
    float x0_3 = X * cos(2 * M_PI / 3) - Y * sin(2 * M_PI / 3);
    float y0_3 = Y * cos(2 * M_PI / 3) + X * sin(2 * M_PI / 3);
    theta3 = IKinemTh(x0_3, y0_3, Z);
}


// Callback: receiving any WebSocket message
void onWebSocketEvent(uint8_t client_num, WStype_t type, uint8_t *payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            Serial.printf("[%u] Disconnected!\n", client_num);
            break;

        case WStype_CONNECTED: {
            IPAddress ip = webSocket.remoteIP(client_num);
            Serial.printf("[%u] Connection from ", client_num);
            Serial.println(ip.toString());
            break;
        }
        case WStype_TEXT: {
            String message = String((char *)payload);
            if (message.startsWith("{")) {
                // Parse the received message to extract x, y, z
                sscanf(message.c_str(), "{\"x\":%f,\"y\":%f,\"z\":%d}", &joystickX, &joystickY, &sliderZ);

                // Calculate theta1, theta2, theta3 based on x, y, z
                IKinem(joystickX, joystickY, sliderZ, theta1, theta2, theta3);

                // Create a new message with the calculated angles
                char msg_buf[150];
                sprintf(msg_buf, "{\"x\":%.2f,\"y\":%.2f,\"z\":%d,\"theta1\":%.2f,\"theta2\":%.2f,\"theta3\":%.2f}",
                        joystickX, joystickY, sliderZ, theta1, theta2, theta3);

                // Broadcast the updated message
                webSocket.broadcastTXT(msg_buf);

                // Log the values
                Serial.printf("X: %.2f, Y: %.2f, Z: %d, θ1: %.2f, θ2: %.2f, θ3: %.2f\n", joystickX, joystickY, sliderZ, theta1, theta2, theta3);
            } else {
                Serial.printf("[%u] Message not recognized\n", client_num);
            }
            break;
        }

        default:
            break;
    }
}

// Callback: send homepage
void onIndexRequest(AsyncWebServerRequest *request) {
    IPAddress remote_ip = request->client()->remoteIP();
    Serial.println("[" + remote_ip.toString() + "] HTTP GET request of " + request->url());
    request->send_P(200, "text/html", index_html);
}

// Callback: send 404 if requested file does not exist
void onPageNotFound(AsyncWebServerRequest *request) {
    IPAddress remote_ip = request->client()->remoteIP();
    Serial.println("[" + remote_ip.toString() + "] HTTP GET request of " + request->url());
    request->send(404, "text/plain", "Not found");
}

/***********************************************************
 * Main
 */

void setup() {
    // Start Serial port
    Serial.begin(115200);
    // Start access point
    WiFi.softAP(ssid, password);
    // Print our IP address
    Serial.println("AP running");
    Serial.printf("My IP address: %s\n", WiFi.softAPIP().toString().c_str());

    // On HTTP request for root, provide index.html file
    server.on("/", HTTP_GET, onIndexRequest);
    // Handle requests for pages that do not exist
    server.onNotFound(onPageNotFound);
    // Start web server
    server.begin();
    // Start WebSocket server and assign callback
    webSocket.begin();
    webSocket.onEvent(onWebSocketEvent);
}

void loop() {
    webSocket.loop();
}