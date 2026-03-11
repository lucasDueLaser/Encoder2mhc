#include "relay.h"

// ============================================================
//  relay.cpp — pulsos periódicos
//  Estado do timer NÃO reseta ao desativar/reativar — evita
//  relay travado off em sistemas que param e arrancam rápido
// ============================================================

typedef enum { WAITING, IN_PULSE } RelayState;
static RelayState    _state   = WAITING;
static unsigned long _lastMs  = 0;

void relayInit() {
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, RELAY_IDLE);
    sys.relayOn = false;
    Serial.print("✓ Relay iniciado — GPIO"); Serial.println(RELAY_PIN);
}

static void _relayOn() {
    digitalWrite(RELAY_PIN, RELAY_ACTIVE);
    sys.relayOn = true;
}

static void _relayOff() {
    digitalWrite(RELAY_PIN, RELAY_IDLE);
    sys.relayOn = false;
}

void relayStopAll() {
    _relayOff();
    // NÃO reseta _state nem _lastMs — mantém posição no ciclo
}

void relayUpdate() {
    unsigned long now = millis();

    // Verifica parada
    bool stopped = (sys.lastMoveMs == 0)
                || (now - sys.lastMoveMs >= cfg.stopTimeoutMs)
                || (sys.rpm < cfg.stopRPM && sys.rpm > 0.0f);

    if (stopped) {
        if (sys.active) {
            sys.active = false;
            _relayOff();
            Serial.println("⭕ PARADO — velocidade zerou");
        }
        return;
    }

    // Ativa sistema
    if (!sys.active && sys.rpm >= cfg.thresholdRPM) {
        sys.active = true;
        // Dispara pulso imediatamente na primeira ativação
        if (_state == WAITING) {
            _state  = IN_PULSE;
            _lastMs = now;
            _relayOn();
        }
        Serial.print("🟢 ATIVO — ");
        Serial.print(sys.rpm, 1);
        Serial.println(" RPM → disparos periódicos iniciados");
        return;
    }

    if (!sys.active) return;

    // Máquina de estados dos pulsos
    switch (_state) {
        case IN_PULSE:
            if (now - _lastMs >= cfg.pulseOnMs) {
                _relayOff();
                _lastMs = now;
                _state  = WAITING;
            }
            break;

        case WAITING:
            if (now - _lastMs >= (cfg.pulseIntervalMs - cfg.pulseOnMs)) {
                _lastMs = now;
                _state  = IN_PULSE;
                _relayOn();
            }
            break;
    }
}

void relayForceOn() {
    sys.active = true;
    _state     = IN_PULSE;
    _lastMs    = millis();
    _relayOn();
    Serial.println("🔴 Relay forçado ON");
}

void relayForceOff() {
    sys.active = false;
    _relayOff();
    Serial.println("⭕ Relay forçado OFF");
}