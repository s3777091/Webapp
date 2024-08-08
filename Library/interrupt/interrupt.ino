#include <WiFi.h>
#include <WiFiClient.h>

const char* ssid = "Kasda 7DCF KW6516";
const char* password = "12345678";

const char* host = "192.168.0.107";
const uint16_t port = 3000;

WiFiClient client;
const int LED_PIN = 22;

bool isConnected = false;
bool messageReceived = false;

const int BUTTON_PIN = 18;
bool buttonPressed = false;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;

void IRAM_ATTR isr() {
    unsigned long currentMillis = millis();
    if ((currentMillis - lastDebounceTime) > debounceDelay) {
        buttonPressed = true;
        lastDebounceTime = currentMillis;
    }
}

void setup() {
    Serial.begin(9600);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    attachInterrupt(BUTTON_PIN, isr, FALLING);

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    WiFi.begin(ssid, password);
  
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
  
    Serial.println("Connected to WiFi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
}

void connectToWebSocket() {
    if (client.connect(host, port)) {
        Serial.println("Connected to WebSocket server");
        
        // Send WebSocket handshake request
        client.println("GET / HTTP/1.1");
        client.println("Host: " + String(host) + ":" + String(port));
        client.println("Upgrade: websocket");
        client.println("Connection: Upgrade");
        client.println("Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==");
        client.println("Sec-WebSocket-Version: 13");
        client.println();

        unsigned long timeout = millis() + 5000; // 5 seconds timeout
        while (client.connected() && !client.available() && millis() < timeout) {
            delay(100);
        }
        
        if (client.available()) {
            while (client.available()) {
                String line = client.readStringUntil('\r');
                Serial.print(line);
            }
            Serial.println("WebSocket handshake complete");
            isConnected = true;
        } else {
            Serial.println("WebSocket handshake failed");
            client.stop();
        }
    } else {
        Serial.println("Connection to WebSocket server failed");
    }
}

void disconnectFromWebSocket() {
    client.stop();
    isConnected = false;
    Serial.println("Disconnected from WebSocket server");
}

void handleClientMessages() {
    if (client.connected()) {
        // Check for incoming messages
        while (client.available()) {
            String message = client.readStringUntil('\n');
            Serial.println("Received message: " + message);
            
            // Process the incoming message
            if (message.substring(2) == "on") {
                digitalWrite(LED_PIN, HIGH);
                Serial.println("LED turned ON");
            } else if (message.substring(2) == "off") {
                digitalWrite(LED_PIN, LOW);
                Serial.println("LED turned OFF");
            }
            messageReceived = true; // Set the flag indicating a message was received
        }
    }
}

void loop() {
    if (buttonPressed) {
        if (isConnected) {
            disconnectFromWebSocket();
        } else {
            connectToWebSocket();
        }
        buttonPressed = false;
    }

    handleClientMessages(); // Handle client messages in the main loop
}