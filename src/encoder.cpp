#include "encoder.h"

// ============================================================
// encoder.cpp - analog movement detection for encoder channels
//
// Detection sources:
// 1) logical AB transition (classical)
// 2) low/high seen inside a burst window
// 3) avg in mid-range (very high speed ADC averaging)
// 4) dynamic edges inside the analog burst (adaptive Schmitt)
// ============================================================

#define LOW_MV                500U
#define HIGH_MV              1000U
#define START_TRANSITIONS       2U
#define BURST_SAMPLES          32U
#define MIN_SWING_MV          300U
#define MID_LOW_MV            400U
#define MID_HIGH_MV          2800U

#define MIN_DYNAMIC_SWING_MV   60U
#define MIN_DYNAMIC_EDGES       1U
#define MIN_RATE_SWING_MV      90U
#define WIDE_SWING_MV         130U
#define START_DECAY_DIV         3U

static bool _ok = false;
static bool _moving = false;

static uint16_t _aMv = 0;
static uint16_t _bMv = 0;
static uint16_t _aMinMv = 0;
static uint16_t _aMaxMv = 0;
static uint16_t _bMinMv = 0;
static uint16_t _bMaxMv = 0;
static uint8_t  _aDynEdges = 0;
static uint8_t  _bDynEdges = 0;

static int8_t  _aState = -1;   // 0=LOW, 1=HIGH, -1=undefined
static int8_t  _bState = -1;
static uint8_t _lastAB = 0xFF;

// Sliding start confidence in [0, START_TRANSITIONS * 2]
static int8_t  _startCounter = 0;
static uint8_t _missCycles = 0;

static uint32_t _transitionTotal = 0;

// Last movement trigger for debug
static const char* _lastTrigger = "nenhum";

struct ChannelWindow {
    uint16_t avgMv;
    uint16_t minMv;
    uint16_t maxMv;
    bool sawLow;
    bool sawHigh;
    uint8_t dynamicEdges;
};

static uint16_t readMv(uint8_t pin) {
#if defined(ARDUINO_ARCH_ESP32)
    uint32_t sum = 0;
    for (uint8_t i = 0; i < 8; i++) {
        sum += (uint32_t)analogReadMilliVolts(pin);
    }
    return (uint16_t)(sum / 8U);
#else
    int raw = analogRead(pin);
    if (raw < 0) raw = 0;
    if (raw > 1023) raw = 1023;
    return (uint16_t)((raw * 3300L) / 1023L);
#endif
}

static ChannelWindow sampleWindow(uint8_t pin) {
    uint16_t samples[BURST_SAMPLES];
    uint32_t sum = 0;
    uint16_t mn = 65535U;
    uint16_t mx = 0U;
    bool low = false;
    bool high = false;

    for (uint8_t i = 0; i < BURST_SAMPLES; i++) {
        uint16_t mv = readMv(pin);
        samples[i] = mv;
        sum += mv;
        if (mv < mn) mn = mv;
        if (mv > mx) mx = mv;
        if (mv <= LOW_MV) low = true;
        if (mv >= HIGH_MV) high = true;
    }

    uint8_t dynamicEdges = 0;
    uint16_t swing = (uint16_t)(mx - mn);
    if (swing >= MIN_DYNAMIC_SWING_MV) {
        uint16_t mid = (uint16_t)((mx + mn) / 2U);
        uint16_t hyst = (uint16_t)(swing / 6U);
        if (hyst < 15U) hyst = 15U;

        int8_t state = -1;
        for (uint8_t i = 0; i < BURST_SAMPLES; i++) {
            int8_t next = state;
            if (samples[i] > (uint16_t)(mid + hyst)) {
                next = 1;
            } else if (samples[i] < (uint16_t)(mid - hyst)) {
                next = 0;
            }

            if (state >= 0 && next != state) {
                dynamicEdges++;
            }
            state = next;
        }
    }

    ChannelWindow out;
    out.avgMv = (uint16_t)(sum / BURST_SAMPLES);
    out.minMv = mn;
    out.maxMv = mx;
    out.sawLow = low;
    out.sawHigh = high;
    out.dynamicEdges = dynamicEdges;
    return out;
}

static uint8_t updateState(uint16_t mv, int8_t &state) {
    if (mv <= LOW_MV) {
        state = 0;
    } else if (mv >= HIGH_MV) {
        state = 1;
    }

    if (state < 0) {
        return 0;
    }
    return (uint8_t)state;
}

static bool detectMovement(const ChannelWindow &aWin,
                           const ChannelWindow &bWin,
                           uint8_t ab,
                           const char **trigger) {
    bool logicTransition = (_lastAB != 0xFFU && ab != _lastAB);

    bool burstTransition = (aWin.sawLow && aWin.sawHigh) ||
                           (bWin.sawLow && bWin.sawHigh);

    uint16_t swingA = (uint16_t)(aWin.maxMv - aWin.minMv);
    uint16_t swingB = (uint16_t)(bWin.maxMv - bWin.minMv);
    bool enoughSwing = (swingA >= MIN_SWING_MV) || (swingB >= MIN_SWING_MV);

    bool aMidRange = (aWin.avgMv >= MID_LOW_MV && aWin.avgMv <= MID_HIGH_MV);
    bool bMidRange = (bWin.avgMv >= MID_LOW_MV && bWin.avgMv <= MID_HIGH_MV);
    bool midRangeDetect = (aMidRange || bMidRange);

    uint8_t dynEdges = (uint8_t)(aWin.dynamicEdges + bWin.dynamicEdges);
    bool enoughRateSwing = (swingA >= MIN_RATE_SWING_MV) || (swingB >= MIN_RATE_SWING_MV);
    bool dynamicRate = (dynEdges >= MIN_DYNAMIC_EDGES) && enoughRateSwing;
    bool wideSwingDetect = (swingA >= WIDE_SWING_MV) || (swingB >= WIDE_SWING_MV);

    bool classical = (logicTransition || burstTransition) && enoughSwing;
    bool highSpeed = midRangeDetect;
    bool rateDetect = dynamicRate;

    if (classical) {
        *trigger = "classical";
        return true;
    }
    if (rateDetect) {
        *trigger = "dynamic-edges";
        return true;
    }
    if (wideSwingDetect) {
        *trigger = "wide-swing";
        return true;
    }
    if (highSpeed) {
        *trigger = "high-speed-avg";
        return true;
    }

    *trigger = nullptr;
    return false;
}

bool encoderInit() {
    pinMode(ENCODER_A_PIN, INPUT_PULLUP);
    pinMode(ENCODER_B_PIN, INPUT_PULLUP);

#if defined(ARDUINO_ARCH_ESP32)
    analogSetPinAttenuation(ENCODER_A_PIN, ADC_11db);
    analogSetPinAttenuation(ENCODER_B_PIN, ADC_11db);
#endif

    _ok = true;
    _moving = false;
    _startCounter = 0;
    _missCycles = 0;
    _transitionTotal = 0;
    _aState = -1;
    _bState = -1;
    _lastAB = 0xFF;
    _lastTrigger = "nenhum";

    _aMv = readMv(ENCODER_A_PIN);
    _bMv = readMv(ENCODER_B_PIN);

    uint8_t a = updateState(_aMv, _aState);
    uint8_t b = updateState(_bMv, _bState);
    if (_aState >= 0 && _bState >= 0) {
        _lastAB = (uint8_t)((a << 1) | b);
    }

    Serial.println("Encoder iniciado (v2 - multi-criterio)");
    Serial.print("  A=GPIO"); Serial.print(ENCODER_A_PIN);
    Serial.print("  B=GPIO"); Serial.println(ENCODER_B_PIN);
    Serial.print("  LOW<="); Serial.print(LOW_MV);
    Serial.print("mV  HIGH>="); Serial.print(HIGH_MV);
    Serial.print("mV  MID=["); Serial.print(MID_LOW_MV);
    Serial.print(".."); Serial.print(MID_HIGH_MV);
    Serial.println("]mV");
    return true;
}

void encoderUpdate() {
    if (!_ok) {
        return;
    }

    ChannelWindow aWin = sampleWindow(ENCODER_A_PIN);
    ChannelWindow bWin = sampleWindow(ENCODER_B_PIN);

    _aMv = aWin.avgMv;
    _bMv = bWin.avgMv;
    _aMinMv = aWin.minMv;
    _aMaxMv = aWin.maxMv;
    _bMinMv = bWin.minMv;
    _bMaxMv = bWin.maxMv;
    _aDynEdges = aWin.dynamicEdges;
    _bDynEdges = bWin.dynamicEdges;

    uint8_t a = updateState(_aMv, _aState);
    uint8_t b = updateState(_bMv, _bState);
    bool validAB = (_aState >= 0 && _bState >= 0);

    if (validAB) {
        uint8_t ab = (uint8_t)((a << 1) | b);

        const char *trigger = nullptr;
        bool detected = detectMovement(aWin, bWin, ab, &trigger);

        if (detected && trigger != nullptr) {
            bool isEdgeBased = (strcmp(trigger, "high-speed-avg") != 0);
            if (isEdgeBased) {
                _transitionTotal++;
            }

            sys.lastMoveMs = millis();
            _lastTrigger = trigger;
            _missCycles = 0;

            if (_startCounter < (int8_t)(START_TRANSITIONS * 2U)) {
                _startCounter++;
            }
            if (!_moving && _startCounter >= (int8_t)START_TRANSITIONS) {
                _moving = true;
            }
        } else {
            if (_missCycles < 255U) {
                _missCycles++;
            }
            // Decaimento mais lento para nao perder partida por 1-2 janelas sem detecao.
            if (_missCycles >= START_DECAY_DIV) {
                _missCycles = 0;
                if (_startCounter > 0) {
                    _startCounter--;
                }
            }
            _lastTrigger = "nenhum";
        }

        _lastAB = ab;
    }

    if (_moving) {
        uint32_t elapsed = (sys.lastMoveMs > 0U)
                         ? (millis() - sys.lastMoveMs)
                         : cfg.stopTimeoutMs;
        if (elapsed >= cfg.stopTimeoutMs) {
            _moving = false;
            _startCounter = 0;
            _missCycles = 0;
        }
    }

    sys.moving = _moving;
    sys.totalEdges = (int32_t)_transitionTotal;
}

bool encoderIsMoving() { return _moving; }
int32_t encoderGetTotal() { return (int32_t)_transitionTotal; }

void encoderReset() {
    _moving = false;
    _startCounter = 0;
    _missCycles = 0;
    _transitionTotal = 0;
    _aState = -1;
    _bState = -1;
    _lastAB = 0xFF;
    _lastTrigger = "nenhum";

    _aMv = readMv(ENCODER_A_PIN);
    _bMv = readMv(ENCODER_B_PIN);

    sys.moving = false;
    sys.lastMoveMs = 0;
    sys.totalEdges = 0;

    Serial.println("Encoder zerado");
}

void encoderPrintStatus() {
    uint16_t swingA = (uint16_t)(_aMaxMv - _aMinMv);
    uint16_t swingB = (uint16_t)(_bMaxMv - _bMinMv);

    Serial.println("  Encoder analogico (v2):");
    Serial.print("    Estado:         "); Serial.println(_moving ? "MOVENDO" : "PARADO");
    Serial.print("    Trigger:        "); Serial.println(_lastTrigger);
    Serial.print("    A avg:          "); Serial.print(_aMv); Serial.println(" mV");
    Serial.print("    A [min..max]:   "); Serial.print(_aMinMv); Serial.print(".."); Serial.print(_aMaxMv);
    Serial.print(" mV  swing="); Serial.print(swingA); Serial.println(" mV");
    Serial.print("    B avg:          "); Serial.print(_bMv); Serial.println(" mV");
    Serial.print("    B [min..max]:   "); Serial.print(_bMinMv); Serial.print(".."); Serial.print(_bMaxMv);
    Serial.print(" mV  swing="); Serial.print(swingB); Serial.println(" mV");
    Serial.print("    A_dyn_edges:    "); Serial.println(_aDynEdges);
    Serial.print("    B_dyn_edges:    "); Serial.println(_bDynEdges);
    Serial.print("    A_logico:       "); Serial.println(_aState < 0 ? -1 : _aState);
    Serial.print("    B_logico:       "); Serial.println(_bState < 0 ? -1 : _bState);
    Serial.print("    StartCounter:   "); Serial.print(_startCounter);
    Serial.print(" / "); Serial.println(START_TRANSITIONS);
    Serial.print("    Transicoes:     "); Serial.println(_transitionTotal);
    Serial.print("    Ultimo mov.:    ");
    if (sys.lastMoveMs > 0) {
        Serial.print(millis() - sys.lastMoveMs);
        Serial.println(" ms atras");
    } else {
        Serial.println("nunca");
    }
}
