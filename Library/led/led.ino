#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <WebSocketsServer.h>
#include "data.h"

const char *ssid = "ESP32";
const char *password = "12345678";
const int ws_port = 1337;
const int led_pin = 22;

AsyncWebServer server(80);
WebSocketsServer webSocket(ws_port);

// Store LED brightness
volatile int led_brightness = 0;
char msg_buf[10];

void IRAM_ATTR setLEDBrightness(int brightness) {
    led_brightness = brightness;
    analogWrite(led_pin, led_brightness);
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
            // Send initial LED state to the client
            sprintf(msg_buf, "%d", led_brightness);
            webSocket.sendTXT(client_num, msg_buf);
            break;
        }
        case WStype_TEXT:
            if (strncmp((char *)payload, "setBrightness:", 14) == 0) {
                int brightness = atoi((char *)payload + 14);
                setLEDBrightness(brightness);
                sprintf(msg_buf, "%d", led_brightness);
                webSocket.sendTXT(client_num, msg_buf);
            } else if (strcmp((char *)payload, "getLEDState") == 0) {
                sprintf(msg_buf, "%d", led_brightness);
                webSocket.sendTXT(client_num, msg_buf);
            } else if (strncmp((char *)payload, "joystick:", 9) == 0) {
                // Parse the joystick values
                float x, y;
                sscanf((char *)payload + 9, "%f,%f", &x, &y);
                Serial.printf("Joystick X: %.2f, Y: %.2f\n", x, y);
            } else {
                Serial.printf("[%u] Message not recognized\n", client_num);
            }
            break;

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
    pinMode(led_pin, OUTPUT);
    digitalWrite(led_pin, LOW);
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