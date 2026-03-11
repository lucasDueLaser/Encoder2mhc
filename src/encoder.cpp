#include "encoder.h"
#include "driver/pcnt.h"

// ============================================================
//  encoder.cpp - OVW2 05 2MHC
//  Leitura por PCNT (hardware) para alta rotacao sem perda por ISR
// ============================================================

#define ENC_PCNT_UNIT PCNT_UNIT_0
#define ENC_PCNT_CH PCNT_CHANNEL_0
#define ENC_FILTER_TICKS 80U // ~1us @ 80MHz APB

#define EDGES_PER_REV (PPR * 2)
#define RPM_IIR_ALPHA 0.25f
#define RPM_DECAY_PER_CYCLE 0.90f
#define STUCK_DETECT_MS 1200UL

static int32_t _totalCount = 0;
static unsigned long _lastEdgeUs = 0;
static bool _hasSignal = false;
static unsigned long _lastSampleMs = 0;
static float _rpmFiltered = 0.0f;

static int _lastA = -1;
static int _lastB = -1;
static unsigned long _aStableMs = 0;
static unsigned long _bStableMs = 0;
static bool _faultAStuckLow = false;
static bool _faultAStuckHigh = false;
static bool _faultBStuckLow = false;
static bool _faultBStuckHigh = false;

static bool _pcntConfigured = false;

static void updatePinDiagnostics(unsigned long dtMs, bool movingRecently) {
    int a = digitalRead(ENCODER_A_PIN);
    int b = digitalRead(ENCODER_B_PIN);

    if (_lastA < 0) _lastA = a;
    if (_lastB < 0) _lastB = b;

    if (a == _lastA) {
        _aStableMs += dtMs;
    } else {
        _aStableMs = 0;
        _lastA = a;
    }

    if (b == _lastB) {
        _bStableMs += dtMs;
    } else {
        _bStableMs = 0;
        _lastB = b;
    }

    _faultAStuckLow = false;
    _faultAStuckHigh = false;
    _faultBStuckLow = false;
    _faultBStuckHigh = false;

    if (movingRecently) {
        if (_aStableMs >= STUCK_DETECT_MS) {
            if (a == LOW) _faultAStuckLow = true;
            else _faultAStuckHigh = true;
        }
        if (_bStableMs >= STUCK_DETECT_MS) {
            if (b == LOW) _faultBStuckLow = true;
            else _faultBStuckHigh = true;
        }
    }
}

static bool encoderPcntInit() {
    pcnt_config_t cfgPcnt = {};
    cfgPcnt.pulse_gpio_num = ENCODER_A_PIN;
    cfgPcnt.ctrl_gpio_num = PCNT_PIN_NOT_USED;
    cfgPcnt.channel = ENC_PCNT_CH;
    cfgPcnt.unit = ENC_PCNT_UNIT;
    cfgPcnt.pos_mode = PCNT_COUNT_INC;
    cfgPcnt.neg_mode = PCNT_COUNT_INC;
    cfgPcnt.lctrl_mode = PCNT_MODE_KEEP;
    cfgPcnt.hctrl_mode = PCNT_MODE_KEEP;
    cfgPcnt.counter_h_lim = 32767;
    cfgPcnt.counter_l_lim = -32768;

    if (pcnt_unit_config(&cfgPcnt) != ESP_OK) return false;
    if (pcnt_set_filter_value(ENC_PCNT_UNIT, ENC_FILTER_TICKS) != ESP_OK) return false;
    if (pcnt_filter_enable(ENC_PCNT_UNIT) != ESP_OK) return false;
    if (pcnt_counter_pause(ENC_PCNT_UNIT) != ESP_OK) return false;
    if (pcnt_counter_clear(ENC_PCNT_UNIT) != ESP_OK) return false;
    if (pcnt_counter_resume(ENC_PCNT_UNIT) != ESP_OK) return false;

    return true;
}

bool encoderInit() {
    pinMode(ENCODER_A_PIN, INPUT_PULLUP);
    pinMode(ENCODER_B_PIN, INPUT_PULLUP);

    _pcntConfigured = encoderPcntInit();

    Serial.println("Encoder OVW2 05 2MHC iniciado (PCNT)");
    Serial.print("  A=GPIO"); Serial.print(ENCODER_A_PIN);
    Serial.print("  B=GPIO"); Serial.println(ENCODER_B_PIN);
    Serial.print("  Bordas/volta: "); Serial.println(EDGES_PER_REV);
    Serial.print("  PCNT: "); Serial.println(_pcntConfigured ? "OK" : "FALHOU");

    return _pcntConfigured;
}

float encoderUpdateRPM() {
    if (!_pcntConfigured) {
        sys.rpm = 0.0f;
        sys.lastMoveMs = 0;
        return 0.0f;
    }

    unsigned long nowMs = millis();
    if (_lastSampleMs == 0) {
        _lastSampleMs = nowMs;
        return sys.rpm;
    }

    unsigned long dtMs = nowMs - _lastSampleMs;
    if (dtMs == 0) return sys.rpm;
    _lastSampleMs = nowMs;

    int16_t pulseDelta = 0;
    if (pcnt_get_counter_value(ENC_PCNT_UNIT, &pulseDelta) != ESP_OK) {
        return sys.rpm;
    }
    pcnt_counter_clear(ENC_PCNT_UNIT);

    unsigned long nowUs = micros();
    unsigned long timeoutUs = (unsigned long)cfg.stopTimeoutMs * 1000UL;

    if (pulseDelta > 0) {
        _totalCount += pulseDelta;
        _lastEdgeUs = nowUs;
        _hasSignal = true;
        sys.lastMoveMs = nowMs;

        float edgesPerSecond = ((float)pulseDelta * 1000.0f) / (float)dtMs;
        float rpmInstant = (edgesPerSecond * 60.0f) / (float)EDGES_PER_REV;

        if (_rpmFiltered <= 0.01f) _rpmFiltered = rpmInstant;
        else _rpmFiltered += RPM_IIR_ALPHA * (rpmInstant - _rpmFiltered);

        sys.rpm = _rpmFiltered;

        updatePinDiagnostics(dtMs, true);
        return sys.rpm;
    }

    bool movingRecently = _hasSignal && ((nowUs - _lastEdgeUs) <= timeoutUs);
    updatePinDiagnostics(dtMs, movingRecently);

    if (!movingRecently) {
        _rpmFiltered = 0.0f;
        sys.rpm = 0.0f;
        sys.lastMoveMs = 0;
        return 0.0f;
    }

    _rpmFiltered *= RPM_DECAY_PER_CYCLE;
    if (_rpmFiltered < 0.05f) _rpmFiltered = 0.0f;
    sys.rpm = _rpmFiltered;
    return sys.rpm;
}

void encoderReset() {
    _totalCount = 0;
    _hasSignal = false;
    _lastEdgeUs = 0;
    _lastSampleMs = 0;
    _rpmFiltered = 0.0f;

    _lastA = -1;
    _lastB = -1;
    _aStableMs = 0;
    _bStableMs = 0;
    _faultAStuckLow = false;
    _faultAStuckHigh = false;
    _faultBStuckLow = false;
    _faultBStuckHigh = false;

    if (_pcntConfigured) {
        pcnt_counter_pause(ENC_PCNT_UNIT);
        pcnt_counter_clear(ENC_PCNT_UNIT);
        pcnt_counter_resume(ENC_PCNT_UNIT);
    }

    sys.rpm = 0.0f;
    sys.lastMoveMs = 0;
    Serial.println("Encoder zerado");
}

int32_t encoderGetCount() {
    return _totalCount;
}

int32_t encoderGetSampleCount() { return 0; }

void encoderPrintStatus() {
    Serial.println("  OVW2 05 2MHC:");
    Serial.print("    Contagem bordas: "); Serial.println(_totalCount);
    Serial.print("    RPM:             "); Serial.println(sys.rpm, 2);
    Serial.print("    Ultima borda:    "); Serial.print(_lastEdgeUs); Serial.println(" us");
    Serial.print("    Ultimo mov.:     ");
    Serial.print(millis() - sys.lastMoveMs); Serial.println(" ms atras");
    Serial.print("    PCNT:            "); Serial.println(_pcntConfigured ? "OK" : "FALHOU");

    if (_faultAStuckLow)  Serial.println("    Fault: A presa em LOW");
    if (_faultAStuckHigh) Serial.println("    Fault: A presa em HIGH");
    if (_faultBStuckLow)  Serial.println("    Fault: B presa em LOW");
    if (_faultBStuckHigh) Serial.println("    Fault: B presa em HIGH");
}
