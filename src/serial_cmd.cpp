#include "serial_cmd.h"
#include "encoder.h"
#include "relay.h"

// ============================================================
//  serial_cmd.cpp — parser de comandos e impressão de status
// ============================================================

// ---- helpers -----------------------------------------------

static void _printSeparator() {
    Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
}

// ---- API pública -------------------------------------------

void serialCmdUpdate() {
    if (!Serial.available()) return;

    String input = Serial.readStringUntil('\n');
    input.trim();
    input.toUpperCase();

    if (input == "S") {
        serialPrintStatus();

    } else if (input == "M") {
        serialPrintMenu();

    } else if (input == "ON") {
        relayForceOn();

    } else if (input == "OFF") {
        relayForceOff();

    } else if (input.startsWith("T=")) {
        // Threshold RPM — ex: T=8.5
        float v = input.substring(2).toFloat();
        if (v > 0.0f) {
            cfg.thresholdRPM = v;
            Serial.print("✓ Threshold: "); Serial.print(v, 1); Serial.println(" RPM");
        } else {
            Serial.println("❌ Valor inválido. Ex: T=5.0");
        }

    } else if (input.startsWith("I=")) {
        // Intervalo entre pulsos (ms) — ex: I=500
        uint32_t v = (uint32_t)input.substring(2).toInt();
        if (v >= 50) {
            cfg.pulseIntervalMs = v;
            // Garante que pulseOnMs continua menor que o intervalo
            if (cfg.pulseOnMs >= v) cfg.pulseOnMs = v - 10;
            Serial.print("✓ Intervalo: "); Serial.print(v); Serial.println(" ms");
        } else {
            Serial.println("❌ Mínimo 50 ms. Ex: I=500");
        }

    } else if (input.startsWith("P=")) {
        // Duração do pulso ON (ms) — ex: P=100
        uint32_t v = (uint32_t)input.substring(2).toInt();
        if (v >= 10 && v < cfg.pulseIntervalMs) {
            cfg.pulseOnMs = v;
            Serial.print("✓ Pulso ON: "); Serial.print(v); Serial.println(" ms");
        } else {
            Serial.print("❌ Entre 10 ms e "); Serial.print(cfg.pulseIntervalMs - 1);
            Serial.println(" ms. Ex: P=100");
        }

    } else {
        serialPrintMenu();
    }
}

void serialPrintStatus() {
    _printSeparator();
    Serial.println("STATUS DO SISTEMA");
    _printSeparator();

    encoderPrintStatus();

    Serial.println();
    Serial.print("  RPM atual:   "); Serial.println(sys.rpm, 2);
    Serial.print("  Threshold:   "); Serial.print(cfg.thresholdRPM, 1); Serial.println(" RPM");
    Serial.print("  Limite stop: "); Serial.print(cfg.stopRPM, 1);      Serial.println(" RPM");
    Serial.print("  Sistema:     "); Serial.println(sys.active  ? "ATIVO 🟢"  : "PARADO ⭕");
    Serial.print("  Relay:       "); Serial.println(sys.relayOn ? "ON 🔴"     : "OFF ⭕");
    Serial.println();
    Serial.print("  Pulso ON:    "); Serial.print(cfg.pulseOnMs);       Serial.println(" ms");
    Serial.print("  Intervalo:   "); Serial.print(cfg.pulseIntervalMs); Serial.println(" ms");
    Serial.print("  Stop timeout:"); Serial.print(cfg.stopTimeoutMs);   Serial.println(" ms");
    _printSeparator();
    Serial.println();
}

void serialPrintMenu() {
    Serial.println("\n╔══════════════════════════════════════════╗");
    Serial.println("║          COMANDOS DISPONÍVEIS            ║");
    Serial.println("╠══════════════════════════════════════════╣");
    Serial.println("║  S        = Status completo              ║");
    Serial.println("║  T=XX.X   = Threshold velocidade (RPM)   ║");
    Serial.println("║             ex: T=5.0                    ║");
    Serial.println("║  I=XXXX   = Intervalo entre pulsos (ms)  ║");
    Serial.println("║             ex: I=500                    ║");
    Serial.println("║  P=XXX    = Duração do pulso ON (ms)     ║");
    Serial.println("║             ex: P=100                    ║");
    Serial.println("║  ON       = Forçar relay LIGADO (teste)  ║");
    Serial.println("║  OFF      = Parar tudo (teste)           ║");
    Serial.println("║  M        = Mostrar este menu            ║");
    Serial.println("╠══════════════════════════════════════════╣");
    Serial.print  ("║  Threshold: "); Serial.print(cfg.thresholdRPM, 1);
    Serial.println(" RPM                      ║");
    Serial.print  ("║  Intervalo: "); Serial.print(cfg.pulseIntervalMs);
    Serial.println(" ms                         ║");
    Serial.print  ("║  Pulso ON:  "); Serial.print(cfg.pulseOnMs);
    Serial.println(" ms                         ║");
    Serial.println("╚══════════════════════════════════════════╝\n");
}
