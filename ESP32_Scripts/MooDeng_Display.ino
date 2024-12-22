#include <WiFi.h>
#include <WebSocketsClient.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include <math.h>

// WiFi credentials
const char* ssid = "Your_SSID";
const char* password = "Your_PASSWORD";
const char* websocketServer = "Server_IP";
const int websocketPort = 8765;

// WebSocket client
WebSocketsClient webSocket;

// TFT display setup
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);

// Screen and face dimensions for 1.77" display (128x160)
const int SCREEN_WIDTH = 128;
const int SCREEN_HEIGHT = 160;

int eyeWidth;
int eyeHeight;
int eyeRoundness;
int pupilWidth;
int pupilHeight;
int eyeOffsetX;
int eyeOffsetY;

// Other parameters
char currentState = 'H';
int pupilOffsetX = 0;
unsigned long lastBlink = 0;
unsigned long blinkInterval = 3000;
int blinkDuration = 200;
int angle = 0;
unsigned long lastSpinnerUpdate = 0;
int spinnerUpdateInterval = 200;
int spinnerArcDegrees = 90;
int spinnerThickness = 4;
int spinnerRadius = 8;

// WebSocket event handler
void webSocketEvent(WStype_t type, uint8_t *payload, size_t length) {
    switch (type) {
        case WStype_TEXT:
            if (length > 0) {
                char command = payload[0];
                if (command == 'H' || command == 'B' || command == 'L') {
                    currentState = command;
                    drawFace(currentState);
                    Serial.printf("Face set to %c\n", command);
                }
            }
            break;
    }
}

void setup() {
    Serial.begin(115200);

    // Initialize WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");

    // Initialize WebSocket
    webSocket.begin(websocketServer, websocketPort, "/");
    webSocket.onEvent(webSocketEvent);

    // Initialize TFT display
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);

    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    spr.createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);

    // Adjust eye dimensions
    eyeWidth = SCREEN_WIDTH / 2.2;
    eyeHeight = SCREEN_HEIGHT / 5;
    eyeRoundness = eyeHeight / 4;
    pupilWidth = eyeWidth / 4;
    pupilHeight = eyeHeight / 2;

    // Adjust eye positioning
    eyeOffsetX = eyeWidth / 2 + 5;
    eyeOffsetY = -SCREEN_HEIGHT / 8;

    spinnerRadius = (pupilWidth + pupilHeight) / 3;

    drawFace(currentState);
}

void loop() {
    webSocket.loop();
    unsigned long currentMillis = millis();

    // Handle blinking
    if (currentState != 'L' && currentMillis - lastBlink > blinkInterval) {
        blinkEyes();
        lastBlink = currentMillis;
    }

    // Handle spinner animation
    if (currentState == 'L' && currentMillis - lastSpinnerUpdate > spinnerUpdateInterval) {
        angle = (angle + 10) % 360;
        drawFace(currentState);
        lastSpinnerUpdate = currentMillis;
    }

    delay(50);
}

void drawFace(char state) {
    spr.fillSprite(TFT_BLACK);
    drawEyes(state);
    spr.pushSprite(0, 0);
}

void drawEyes(char state) {
    int faceCenterX = SCREEN_WIDTH / 2;
    int faceCenterY = SCREEN_HEIGHT / 2;

    int leftEyeX = faceCenterX - eyeOffsetX - (eyeWidth / 2);
    int leftEyeY = faceCenterY - (eyeHeight / 2) + eyeOffsetY;
    int rightEyeX = faceCenterX + eyeOffsetX - (eyeWidth / 2);
    int rightEyeY = faceCenterY - (eyeHeight / 2) + eyeOffsetY;

    spr.fillRoundRect(leftEyeX, leftEyeY, eyeWidth, eyeHeight, eyeRoundness, TFT_CYAN);
    spr.fillRoundRect(rightEyeX, rightEyeY, eyeWidth, eyeHeight, eyeRoundness, TFT_CYAN);

    if (state == 'H') {
        drawPupils(leftEyeX, leftEyeY, rightEyeX, rightEyeY, 0, -5);
        addPupilHighlights(leftEyeX, leftEyeY, rightEyeX, rightEyeY, 0, -5);
    } else if (state == 'B') {
        drawPupils(leftEyeX, leftEyeY, rightEyeX, rightEyeY, pupilOffsetX, 0);
        addPupilHighlights(leftEyeX, leftEyeY, rightEyeX, rightEyeY, pupilOffsetX, 0);
        spr.fillRect(leftEyeX, leftEyeY, eyeWidth, eyeHeight / 2, TFT_BLACK);
        spr.fillRect(rightEyeX, rightEyeY, eyeWidth, eyeHeight / 2, TFT_BLACK);
    } else if (state == 'L') {
        drawLoadingSpinner(leftEyeX, leftEyeY);
        drawLoadingSpinner(rightEyeX, rightEyeY);
    }
}

void drawPupils(int leftEyeX, int leftEyeY, int rightEyeX, int rightEyeY, int offset, int verticalPupilLift) {
    int leftPupilX = leftEyeX + (eyeWidth / 2) - (pupilWidth / 2) + offset;
    int leftPupilY = leftEyeY + (eyeHeight / 2) - (pupilHeight / 2) + verticalPupilLift;
    int rightPupilX = rightEyeX + (eyeWidth / 2) - (pupilWidth / 2) + offset;
    int rightPupilY = rightEyeY + (eyeHeight / 2) - (pupilHeight / 2) + verticalPupilLift;

    spr.fillRect(leftPupilX, leftPupilY, pupilWidth, pupilHeight, TFT_BLACK);
    spr.fillRect(rightPupilX, rightPupilY, pupilWidth, pupilHeight, TFT_BLACK);
}

void addPupilHighlights(int leftEyeX, int leftEyeY, int rightEyeX, int rightEyeY, int offset, int verticalPupilLift) {
    int highlightSize = pupilWidth / 8;
    spr.fillCircle(leftEyeX + pupilWidth / 4 + offset, leftEyeY + pupilHeight / 4 + verticalPupilLift, highlightSize, TFT_WHITE);
    spr.fillCircle(rightEyeX + pupilWidth / 4 + offset, rightEyeY + pupilHeight / 4 + verticalPupilLift, highlightSize, TFT_WHITE);
}

void drawLoadingSpinner(int eyeX, int eyeY) {
    int centerX = eyeX + eyeWidth / 2;
    int centerY = eyeY + eyeHeight / 2;

    spr.drawRect(centerX - pupilWidth / 2, centerY - pupilHeight / 2, pupilWidth, pupilHeight, TFT_CYAN);

    for (int thick = 0; thick < spinnerThickness; thick++) {
        drawArc(centerX, centerY, spinnerRadius - thick, angle, angle + spinnerArcDegrees, TFT_BLACK);
    }
}

void drawArc(int cx, int cy, int r, int startAngle, int endAngle, uint16_t color) {
    float step = 5.0;
    for (float a = startAngle; a < endAngle; a += step) {
        float rad1 = a * M_PI / 180.0;
        float rad2 = (a + step) * M_PI / 180.0;
        int x1 = cx + (int)(r * cos(rad1));
        int y1 = cy + (int)(r * sin(rad1));
        int x2 = cx + (int)(r * cos(rad2));
        int y2 = cy + (int)(r * sin(rad2));
        spr.drawLine(x1, y1, x2, y2, color);
    }
}

void blinkEyes() {
    if (currentState == 'L') {
        return;
    }
    closeEyes();
    delay(blinkDuration);
    drawFace(currentState);
}

void closeEyes() {
    int faceCenterX = SCREEN_WIDTH / 2;
    int faceCenterY = SCREEN_HEIGHT / 2;

    int leftEyeX = faceCenterX - eyeOffsetX - (eyeWidth / 2);
    int leftEyeY = faceCenterY - (eyeHeight / 2) + eyeOffsetY;
    int rightEyeX = faceCenterX + eyeOffsetX - (eyeWidth / 2);
    int rightEyeY = faceCenterY - (eyeHeight / 2) + eyeOffsetY;

    spr.fillRect(leftEyeX, leftEyeY, eyeWidth, eyeHeight, TFT_BLACK);
    spr.fillRect(rightEyeX, rightEyeY, eyeWidth, eyeHeight, TFT_BLACK);

    spr.pushSprite(0, 0);
}
