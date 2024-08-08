#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <WebSocketsServer.h>
#include "data.h"

const char *ssid = "ESP32";
const char *password = "12345678";
const int ws_port = 1337;

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
                sscanf(message.c_str(), "{\"x\":%f,\"y\":%f,\"z\":%d,\"theta1\":%f,\"theta2\":%f,\"theta3\":%f,\"coord_x\":%f,\"coord_y\":%f}", &joystickX, &joystickY, &sliderZ, &theta1, &theta2, &theta3, &joystickX, &joystickY);
                webSocket.broadcastTXT(message);
                
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