#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN         22  // RST pin connected to GPIO 22
#define SS_PIN          5   // SDA pin connected to GPIO 5

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance
MFRC522::MIFARE_Key key;

void setup() {
    Serial.begin(115200); // Initialize serial communications
    while (!Serial);      // Wait for Serial Monitor
    SPI.begin(18, 19, 23, 5);  // Initialize SPI bus
    mfrc522.PCD_Init();   // Initialize MFRC522 reader
    mfrc522.PCD_DumpVersionToSerial(); // Output firmware version for debugging

    // Initialize key (default factory key: FFFFFFFFFFFFh)
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;
    }

    Serial.println(F("Ready to write to the RFID card."));
    Serial.println(F("Type a message in the Serial Monitor and press Enter."));
}

void loop() {
    // Wait for a new card to be present
    if (!mfrc522.PICC_IsNewCardPresent()) return;
    if (!mfrc522.PICC_ReadCardSerial()) return;

    // Card detected, prompt for input
    Serial.println(F("Card detected. Please type your message (up to 16 characters):"));
    while (!Serial.available());
    String input = Serial.readStringUntil('\n');
    input.trim();

    // Prepare data block (max 16 bytes)
    byte dataBlock[16] = {0};
    for (byte i = 0; i < input.length() && i < 16; i++) {
        dataBlock[i] = input[i];
    }

    // Set target block and trailer block for authentication
    byte blockAddr = 4;       // Data block to write
    byte trailerBlock = 7;    // Trailer block for authentication
    MFRC522::StatusCode status;

    // Authenticate with key A
    Serial.println(F("Authenticating..."));
    status = (MFRC522::StatusCode)mfrc522.PCD_Authenticate(
        MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid)
    );
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("Authentication failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }

    // Write data to the specified block
    Serial.println(F("Writing data to card..."));
    status = (MFRC522::StatusCode)mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);
    if (status != MFRC522::STATUS_OK) {
        Serial.print(F("Write failed: "));
        Serial.println(mfrc522.GetStatusCodeName(status));
        return;
    }

    // Confirm success
    Serial.println(F("Data written successfully!"));

    // Halt communication with the card
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
}
