#pragma once
#include <Arduino.h>

// ============================================================
//  PINOS
// ============================================================
#define ENCODER_A_PIN       32
#define ENCODER_B_PIN       33
#define RELAY_PIN           18

// ============================================================
//  DETECÇÃO DE MOVIMENTO
//  Mínimo de bordas em SAMPLE_MS para considerar em movimento
// ============================================================
#define SAMPLE_MS           100UL   // janela de amostragem
#define MIN_EDGES           3       // bordas mínimas para "movendo"
#define STOP_TIMEOUT_MS    1800UL   // ms sem bordas = parado

// ============================================================
//  RELAY
// ============================================================
#define RELAY_ACTIVE        HIGH
#define RELAY_IDLE          LOW

#define DEFAULT_PULSE_ON_MS     100UL
#define DEFAULT_PULSE_INTERVAL  2500UL
#define RELAY_START_DELAY_MS     500UL

// ============================================================
//  SERIAL
// ============================================================
#define SERIAL_PRINT_MS     500UL

// ============================================================
//  STRUCTS
// ============================================================
struct Config {
    uint32_t pulseOnMs       = DEFAULT_PULSE_ON_MS;
    uint32_t pulseIntervalMs = DEFAULT_PULSE_INTERVAL;
    uint32_t stopTimeoutMs   = STOP_TIMEOUT_MS;
    uint8_t  minEdges        = MIN_EDGES;
};

struct SystemState {
    bool          moving      = false;
    bool          relayOn     = false;
    unsigned long lastMoveMs  = 0;
    int32_t       totalEdges  = 0;
};

extern Config      cfg;
extern SystemState sys;
