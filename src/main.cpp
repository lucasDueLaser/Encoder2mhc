#include "main.h"
#include "encoder.h"
#include "relay.h"
#include "serial_cmd.h"

Config      cfg;
SystemState sys;

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n╔══════════════════════════════════════════╗");
    Serial.println("║   ENCODER OVW2 + RELAY PERIODICO         ║");
    Serial.println("║   ESP32 WROOM-32E                        ║");
    Serial.println("╚══════════════════════════════════════════╝\n");

    relayInit();
    encoderInit();
    serialPrintMenu();
}

void loop() {
    serialCmdUpdate();

    static unsigned long lastCheck = 0;
    if (millis() - lastCheck >= SPEED_CHECK_MS) {
        lastCheck = millis();
        encoderUpdateRPM();
        relayUpdate();
    }

    static unsigned long lastPrint = 0;
    if (millis() - lastPrint >= SERIAL_PRINT_MS) {
        lastPrint = millis();

        Serial.print("RPM: ");
        Serial.print(sys.rpm, 1);
        Serial.print("  |  ");
        Serial.print(sys.active  ? "ATIVO 🟢 " : "PARADO ⭕");
        Serial.print("  |  Relay: ");
        Serial.print(sys.relayOn ? "ON 🔴 " : "off   ");
        Serial.print("  |  T=");
        Serial.print(cfg.thresholdRPM, 1);
        Serial.print("rpm  P=");
        Serial.print(cfg.pulseOnMs);
        Serial.print("ms  I=");
        Serial.print(cfg.pulseIntervalMs);
        Serial.println("ms");

                Serial.print("  Z="); Serial.println(digitalRead(14));
        Serial.print("  A="); Serial.println(digitalRead(32));
        Serial.print("  B="); Serial.println(digitalRead(33));
    }
}