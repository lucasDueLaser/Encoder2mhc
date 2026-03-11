#include "relay.h"

// ============================================================
// relay.cpp - delayed ON, steady ON while movement is present
// ============================================================

static bool          _active = false;
static unsigned long _movingSinceMs = 0;
static bool          _moveQualified = false;

void relayInit() {
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, RELAY_IDLE);
    sys.relayOn = false;
    Serial.print("Relay iniciado - GPIO"); Serial.println(RELAY_PIN);
}

static void _on()  { digitalWrite(RELAY_PIN, RELAY_ACTIVE); sys.relayOn = true; }
static void _off() { digitalWrite(RELAY_PIN, RELAY_IDLE);   sys.relayOn = false; }

void relayUpdate() {
    unsigned long now = millis();

    // Encoder stopped: drop relay and reset qualification.
    if (!sys.moving) {
        _movingSinceMs = 0;
        _moveQualified = false;

        if (_active) {
            _active = false;
            _off();
            Serial.println("PARADO - relay off");
        }
        return;
    }

    // Require continuous movement for a minimum time before relay activation.
    if (_movingSinceMs == 0) {
        _movingSinceMs = now;
    }
    if (!_moveQualified) {
        if (now - _movingSinceMs < RELAY_START_DELAY_MS) {
            return;
        }
        _moveQualified = true;
    }

    // Qualified movement: keep relay continuously ON.
    if (!_active) {
        _active = true;
        _on();
        Serial.println("MOVENDO - relay iniciado");
    }
}

void relayForceOn() {
    _active = true;
    _on();
    Serial.println("Relay forcado ON");
}

void relayForceOff() {
    _active = false;
    _movingSinceMs = 0;
    _moveQualified = false;
    _off();
    Serial.println("Relay forcado OFF");
}
