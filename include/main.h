#pragma once
#include <Arduino.h>

// ============================================================
//  PINOS — encoder OVW2 05 2MHC
// ============================================================
#define ENCODER_A_PIN       32
#define ENCODER_B_PIN       33
// Z desabilitado — IO39 sem pull-up externo flutua e cancela counts
#define ENCODER_Z_PIN    14

#define RELAY_PIN           18

// ============================================================
//  ENCODER
// ============================================================
#define PPR                 500           // OVW2-05 = 500 pulsos/volta
#define COUNTS_PER_REV      (PPR * 4)    // quadratura x4 = 2000 cnt/volta
#define SAMPLE_MS           50

// ============================================================
//  RELAY — nível ativo
// ============================================================
#define RELAY_ACTIVE        HIGH
#define RELAY_IDLE          LOW

// ============================================================
//  PARÂMETROS PADRÃO
// ============================================================
#define DEFAULT_THRESHOLD_RPM   5.0f
#define DEFAULT_STOP_RPM        1.0f
#define DEFAULT_PULSE_ON_MS     100UL
#define DEFAULT_PULSE_INTERVAL  500UL
#define DEFAULT_STOP_TIMEOUT    300UL

// ============================================================
//  INTERVALOS
// ============================================================
#define SPEED_CHECK_MS      50UL
#define SERIAL_PRINT_MS     500UL

// ============================================================
//  STRUCTS
// ============================================================
struct Config {
    float    thresholdRPM    = DEFAULT_THRESHOLD_RPM;
    float    stopRPM         = DEFAULT_STOP_RPM;
    uint32_t pulseOnMs       = DEFAULT_PULSE_ON_MS;
    uint32_t pulseIntervalMs = DEFAULT_PULSE_INTERVAL;
    uint32_t stopTimeoutMs   = DEFAULT_STOP_TIMEOUT;
};

struct SystemState {
    float         rpm         = 0.0f;
    bool          active      = false;
    bool          relayOn     = false;
    unsigned long lastMoveMs  = 0;
};

extern Config      cfg;
extern SystemState sys;