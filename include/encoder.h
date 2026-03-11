// #pragma once

// #include "main.h"

// // ============================================================
// //  encoder.h — interface do módulo AS5600
// // ============================================================

// // Inicializa I2C e verifica presença do sensor
// // Retorna true se AS5600 respondeu
// bool encoderInit();

// // Lê ângulo bruto (0–4095). Retorna -1 em falha I2C
// int  encoderReadRawAngle();

// // Lê magnitude do campo magnético (0–4095)
// int  encoderReadMagnitude();

// // Lê byte de status do AS5600
// uint8_t encoderReadStatus();

// // Calcula e retorna RPM atual. Atualiza sys.rpm, sys.lastAngle,
// // sys.lastAngleUs e sys.lastMoveMs internamente.
// float encoderUpdateRPM();

// // Imprime status detalhado do sensor na Serial
// void encoderPrintStatus();

// bool  encoderInit();
// float encoderUpdateRPM();
// void  encoderPrintStatus();

#pragma once

#include "main.h"

// ============================================================
//  encoder.h — interface do módulo OVW2 25 2MHC
//  Encoder óptico incremental — quadratura A/B via interrupção
// ============================================================

// Configura pinos e interrupções. Sempre retorna true.
bool encoderInit();

// Calcula RPM com base nos pulsos acumulados desde a última
// chamada. Atualiza sys.rpm e sys.lastMoveMs.
// Chamar a cada SPEED_CHECK_MS (50ms).
float encoderUpdateRPM();

// Zera contagem de pulsos e posição acumulada
void encoderReset();

// Retorna contagem bruta de pulsos acumulados (com sinal)
int32_t encoderGetCount();

// Imprime status na Serial
void encoderPrintStatus();
int32_t encoderGetSampleCount();
