#include "main.h"
#include "encoder.h"
#include "relay.h"
#include "serial_cmd.h"

Config cfg;
SystemState sys;

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n==========================================");
    Serial.println("ENCODER OVW2 + RELAY PERIODICO");
    Serial.println("ESP32 WROOM-32E");
    Serial.println("==========================================\n");

    relayInit();
    encoderInit();
    serialPrintMenu();
}

void loop() {
    serialCmdUpdate();

    static unsigned long lastSample = 0;
    if (millis() - lastSample >= SAMPLE_MS) {
        lastSample = millis();
        encoderUpdate();
        relayUpdate();
    }

    static unsigned long lastPrint = 0;
    if (millis() - lastPrint >= SERIAL_PRINT_MS) {
        lastPrint = millis();
        encoderPrintStatus();
        Serial.print("  Relay: ");
        Serial.print(sys.relayOn ? "ON" : "OFF");
        Serial.print(" | Estado: ");
        Serial.print(sys.moving ? "MOVENDO" : "PARADO");
        Serial.print(" | P=");
        Serial.print(cfg.pulseOnMs);
        Serial.print("ms I=");
        Serial.print(cfg.pulseIntervalMs);
        Serial.println("ms");
    }
}
