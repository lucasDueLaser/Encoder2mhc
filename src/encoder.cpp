#include "encoder.h"

// ============================================================
//  encoder.cpp - OVW2 05 2MHC
//  RPM por delta de pulsos em janela fixa (SPEED_CHECK_MS)
//  ISR minima em RISING para sustentar RPM mais alto
// ============================================================

volatile int32_t _totalCount = 0;
volatile unsigned long _lastEdgeUs = 0;
volatile bool _hasSignal = false;

static int32_t _lastSampleCount = 0;
static unsigned long _lastSampleMs = 0;

void IRAM_ATTR isrA() {
    _totalCount++;
    _lastEdgeUs = micros();
    _hasSignal = true;
}

bool encoderInit() {
    pinMode(ENCODER_A_PIN, INPUT_PULLUP);
    pinMode(ENCODER_B_PIN, INPUT_PULLUP);

    // FALLING costuma ser mais robusto com saida coletor aberto + pull-up.
    attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), isrA, FALLING);

    Serial.println("Encoder OVW2 05 2MHC iniciado (media 8 periodos)");
    Serial.print("  A=GPIO"); Serial.print(ENCODER_A_PIN);
    Serial.print("  B=GPIO"); Serial.println(ENCODER_B_PIN);
    Serial.print("  Bordas/volta: "); Serial.println(PPR);

    return true;
}

float encoderUpdateRPM() {
    unsigned long nowMs = millis();

    noInterrupts();
    bool has = _hasSignal;
    unsigned long lastEdgeUs = _lastEdgeUs;
    int32_t totalCount = _totalCount;
    interrupts();

    if (_lastSampleMs == 0) {
        _lastSampleMs = nowMs;
        _lastSampleCount = totalCount;
        return sys.rpm;
    }

    unsigned long dtMs = nowMs - _lastSampleMs;
    if (dtMs == 0) return sys.rpm;

    int32_t dCount = totalCount - _lastSampleCount;
    _lastSampleCount = totalCount;
    _lastSampleMs = nowMs;

    unsigned long timeoutUs = (unsigned long)cfg.stopTimeoutMs * 1000UL;
    if (!has || (micros() - lastEdgeUs) > timeoutUs) {
        sys.rpm = 0.0f;
        sys.lastMoveMs = 0;
        return 0.0f;
    }

    if (dCount <= 0) {
        // Nao zera instantaneamente: mantem ultima RPM enquanto houve borda recente.
        return sys.rpm;
    }

    sys.lastMoveMs = nowMs;

    float edgesPerSecond = ((float)dCount * 1000.0f) / (float)dtMs;
    sys.rpm = (edgesPerSecond * 60.0f) / (float)PPR;

    return sys.rpm;
}

void encoderReset() {
    noInterrupts();
    _totalCount = 0;
    _hasSignal = false;
    _lastEdgeUs = 0;
    interrupts();

    _lastSampleCount = 0;
    _lastSampleMs = 0;

    sys.rpm = 0.0f;
    sys.lastMoveMs = 0;
    Serial.println("Encoder zerado");
}

int32_t encoderGetCount() {
    noInterrupts();
    int32_t c = _totalCount;
    interrupts();
    return c;
}

int32_t encoderGetSampleCount() { return 0; }

void encoderPrintStatus() {
    noInterrupts();
    int32_t c = _totalCount;
    unsigned long edgeUs = _lastEdgeUs;
    interrupts();

    Serial.println("  OVW2 05 2MHC:");
    Serial.print("    Contagem bordas: "); Serial.println(c);
    Serial.print("    RPM:             "); Serial.println(sys.rpm, 2);
    Serial.print("    Ultima borda:    "); Serial.print(edgeUs); Serial.println(" us");
    Serial.print("    Ultimo mov.:     ");
    Serial.print(millis() - sys.lastMoveMs); Serial.println(" ms atras");
}
