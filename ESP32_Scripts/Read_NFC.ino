#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <WebSocketsClient.h>

#define RST_PIN         22
#define SS_PIN          5
#define BUZZER_PIN      2  // Buzzer connected to pin 2

const char* ssid = "Hacker";
const char* password = "mo123456";
const char* websocket_server_host = "192.168.29.177"; // Replace with your server's IP
const uint16_t websocket_server_port = 8765;

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;
WebSocketsClient webSocket;

constexpr byte sector         = 1;
constexpr byte blockAddr      = 4; // Data block
constexpr byte trailerBlock   = 7; // Trailer block (key information)

void setup() {
    Serial.begin(115200);
    while (!Serial);
    SPI.begin();
    mfrc522.PCD_Init();
    pinMode(BUZZER_PIN, OUTPUT);

    // Initialize the default key (0xFFFFFFFFFFFF)
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }

    // Connect to WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");

    // Connect to WebSocket server
    webSocket.begin(websocket_server_host, websocket_server_port, "/");
    webSocket.onEvent(webSocketEvent);
    webSocket.setReconnectInterval(5000);  // Reconnect every 5 seconds if disconnected
    webSocket.enableHeartbeat(15000, 3000, 2);  // Enable WebSocket heartbeats for stability

    Serial.println(F("NFC Reader Ready. Tap a card to read."));
}

void loop() {
    webSocket.loop();

    String nfcData = readNFC();
    if (nfcData != "") {
        nfcData = cleanString(nfcData);  // Clean the data before sending
        webSocket.sendTXT(nfcData);
        digitalWrite(BUZZER_PIN, HIGH);
        delay(100);
        digitalWrite(BUZZER_PIN, LOW);
    }
}

// Function to clean NFC data and remove non-printable characters
String cleanString(String input) {
    String result = "";
    for (int i = 0; i < input.length(); i++) {
        char c = input[i];
        if (isPrintable(c)) {  // Only add printable characters
            result += c;
        }
    }
    return result;
}

String readNFC() {
    // Look for new cards
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
        return "";
    }

    Serial.println(F("Card detected. Attempting to read..."));

    // Authenticate using key A for the trailer block
    MFRC522::StatusCode status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("Authentication failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return "";
    }

    // Read data from the target block
    byte buffer[18]; // 16 bytes data + 2 bytes CRC
    byte size = sizeof(buffer);
    status = mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("Reading failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return "";
    }

    // Convert buffer to a readable string
    String result = "";
    for (byte i = 0; i < 16; i++) {
        if (buffer[i] >= 32 && buffer[i] <= 126) { // Printable ASCII range
            result += (char)buffer[i];
        } else {
            result += "."; // Replace non-printable characters with '.'
        }
    }

    Serial.println("Data read: " + result);

    // Halt communication with the card
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();

    return result;
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.println("WebSocket Disconnected");
            break;
        case WStype_CONNECTED:
            Serial.println("WebSocket Connected");
            break;
        case WStype_TEXT:
            Serial.println("Received text from server: " + String((char*)payload));
            break;
    }
}
