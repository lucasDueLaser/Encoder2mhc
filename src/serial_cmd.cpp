#include "serial_cmd.h"
#include "encoder.h"
#include "relay.h"

void serialPrintStatus() {
    Serial.println("------------------------------------------");
    Serial.println("STATUS");
    Serial.println("------------------------------------------");
    encoderPrintStatus();
    Serial.print("  Relay:        "); Serial.println(sys.relayOn ? "ON" : "OFF");
    Serial.print("  Mov. bruto:   "); Serial.println(sys.moving ? "SIM" : "NAO");
    Serial.print("  Relay ok:     "); Serial.println(sys.relayEligible ? "SIM" : "NAO");
    Serial.print("  Relay score:  "); Serial.println(sys.relayScore);
    Serial.print("  Pulso ON:     "); Serial.print(cfg.pulseOnMs); Serial.println(" ms");
    Serial.print("  Intervalo:    "); Serial.print(cfg.pulseIntervalMs); Serial.println(" ms");
    Serial.print("  Min bordas:   "); Serial.println(cfg.minEdges);
    Serial.print("  Timeout:      "); Serial.print(cfg.stopTimeoutMs); Serial.println(" ms");
    Serial.print("  Relay ON/OFF: "); Serial.print(cfg.relayScoreOn); Serial.print(" / "); Serial.println(cfg.relayScoreOff);
    Serial.print("  Relay T evid: "); Serial.print(cfg.relayEvidenceTimeoutMs); Serial.println(" ms");
    Serial.println("------------------------------------------");
}

void serialPrintMenu() {
    Serial.println("\n==========================================");
    Serial.println("COMANDOS");
    Serial.println("==========================================");
    Serial.println("  !      = Debug rapido analogico (A/B)");
    Serial.println("  S      = Status completo");
    Serial.println("  Z      = Zerar contagem");
    Serial.println("  ON/OFF = Forcar relay");
    Serial.println("  I=XXX  = Intervalo pulso (ms) ex:I=500");
    Serial.println("  P=XXX  = Duracao pulso ON  ex:P=100");
    Serial.println("  E=X    = Min bordas detect. ex:E=3");
    Serial.println("  W=XXX  = Timeout parada ms ex:W=500");
    Serial.println("  RON=X  = Score para liberar relay ex:RON=10");
    Serial.println("  ROFF=X = Score para cortar relay ex:ROFF=4");
    Serial.println("  RT=XXX = Timeout evidencia relay ms ex:RT=1200");
    Serial.println("==========================================\n");
}

void serialCmdUpdate() {
    if (!Serial.available()) return;

    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input == "!") {
        // Debug rapido focado na leitura analogica do encoder
        encoderPrintStatus();
        return;
    }

    input.toUpperCase();

    if (input == "S") {
        serialPrintStatus();
    } else if (input == "M") {
        serialPrintMenu();
    } else if (input == "ON") {
        relayForceOn();
    } else if (input == "OFF") {
        relayForceOff();
    } else if (input == "Z") {
        encoderReset();
    } else if (input.startsWith("I=")) {
        uint32_t v = (uint32_t)input.substring(2).toInt();
        if (v >= 50) {
            cfg.pulseIntervalMs = v;
            if (cfg.pulseOnMs >= v) cfg.pulseOnMs = v - 10;
            Serial.print("Intervalo: "); Serial.print(v); Serial.println(" ms");
        } else {
            Serial.println("Minimo 50ms. Ex: I=500");
        }
    } else if (input.startsWith("P=")) {
        uint32_t v = (uint32_t)input.substring(2).toInt();
        if (v >= 10 && v < cfg.pulseIntervalMs) {
            cfg.pulseOnMs = v;
            Serial.print("Pulso ON: "); Serial.print(v); Serial.println(" ms");
        } else {
            Serial.println("Entre 10ms e intervalo-1. Ex: P=100");
        }
    } else if (input.startsWith("E=")) {
        uint8_t v = (uint8_t)input.substring(2).toInt();
        if (v >= 1 && v <= 50) {
            cfg.minEdges = v;
            Serial.print("Min bordas: "); Serial.println(v);
        } else {
            Serial.println("Entre 1 e 50. Ex: E=3");
        }
    } else if (input.startsWith("W=")) {
        uint32_t v = (uint32_t)input.substring(2).toInt();
        if (v >= 100) {
            cfg.stopTimeoutMs = v;
            Serial.print("Timeout parada: "); Serial.print(v); Serial.println(" ms");
        } else {
            Serial.println("Minimo 100ms. Ex: W=500");
        }
    } else if (input.startsWith("RON=")) {
        uint8_t v = (uint8_t)input.substring(4).toInt();
        if (v >= 2 && v <= 40) {
            cfg.relayScoreOn = v;
            if (cfg.relayScoreOff >= cfg.relayScoreOn) {
                cfg.relayScoreOff = (cfg.relayScoreOn > 1) ? (uint8_t)(cfg.relayScoreOn - 1) : 1;
            }
            Serial.print("Relay score ON: "); Serial.println(cfg.relayScoreOn);
        } else {
            Serial.println("Entre 2 e 40. Ex: RON=10");
        }
    } else if (input.startsWith("ROFF=")) {
        uint8_t v = (uint8_t)input.substring(5).toInt();
        if (v >= 1 && v <= 39) {
            cfg.relayScoreOff = v;
            if (cfg.relayScoreOff >= cfg.relayScoreOn) {
                cfg.relayScoreOff = (cfg.relayScoreOn > 1) ? (uint8_t)(cfg.relayScoreOn - 1) : 1;
            }
            Serial.print("Relay score OFF: "); Serial.println(cfg.relayScoreOff);
        } else {
            Serial.println("Entre 1 e 39. Ex: ROFF=4");
        }
    } else if (input.startsWith("RT=")) {
        uint32_t v = (uint32_t)input.substring(3).toInt();
        if (v >= 200) {
            cfg.relayEvidenceTimeoutMs = v;
            Serial.print("Relay timeout evidencia: "); Serial.print(v); Serial.println(" ms");
        } else {
            Serial.println("Minimo 200ms. Ex: RT=1200");
        }
    } else {
        serialPrintMenu();
    }
}
