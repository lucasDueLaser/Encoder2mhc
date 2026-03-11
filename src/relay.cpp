#include "relay.h"

// ============================================================
//  relay.cpp - pulsos periodicos com histerese/debounce de estado
// ============================================================

typedef enum { WAITING, IN_PULSE } RelayState;
static RelayState _state = WAITING;
static unsigned long _lastMs = 0;
static unsigned long _aboveThresholdSinceMs = 0;

#define ACTIVATE_DEBOUNCE_MS 120UL

void relayInit() {
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, RELAY_IDLE);
    sys.relayOn = false;
    Serial.print("Relay iniciado - GPIO"); Serial.println(RELAY_PIN);
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
}

void relayUpdate() {
    unsigned long now = millis();

    bool timeoutStopped = (sys.lastMoveMs == 0)
                       || (now - sys.lastMoveMs >= cfg.stopTimeoutMs);
    if (timeoutStopped) {
        if (sys.active) {
            sys.active = false;
            _relayOff();
            Serial.println("PARADO - timeout sem pulso");
        }
        _aboveThresholdSinceMs = 0;
        return;
    }

    if (!sys.active && sys.rpm >= cfg.thresholdRPM) {
        if (_aboveThresholdSinceMs == 0) _aboveThresholdSinceMs = now;
        if (now - _aboveThresholdSinceMs < ACTIVATE_DEBOUNCE_MS) return;

        sys.active = true;
        if (_state == WAITING) {
            _state = IN_PULSE;
            _lastMs = now;
            _relayOn();
        }

        Serial.print("ATIVO - ");
        Serial.print(sys.rpm, 1);
        Serial.println(" RPM");
        _aboveThresholdSinceMs = 0;
        return;
    }

    if (!sys.active && sys.rpm < cfg.thresholdRPM) {
        _aboveThresholdSinceMs = 0;
        return;
    }

    switch (_state) {
        case IN_PULSE:
            if (now - _lastMs >= cfg.pulseOnMs) {
                _relayOff();
                _lastMs = now;
                _state = WAITING;
            }
            break;

        case WAITING:
            if (now - _lastMs >= (cfg.pulseIntervalMs - cfg.pulseOnMs)) {
                _lastMs = now;
                _state = IN_PULSE;
                _relayOn();
            }
            break;
    }
}

void relayForceOn() {
    sys.active = true;
    _state = IN_PULSE;
    _lastMs = millis();
    _aboveThresholdSinceMs = 0;
    _relayOn();
    Serial.println("Relay forcado ON");
}

void relayForceOff() {
    sys.active = false;
    _aboveThresholdSinceMs = 0;
    _relayOff();
    Serial.println("Relay forcado OFF");
}
