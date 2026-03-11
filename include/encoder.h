#pragma once
#include "main.h"

bool    encoderInit();
bool    encoderIsMoving();   // true se detectou movimento na última janela
void    encoderUpdate();     // chama a cada SAMPLE_MS
void    encoderReset();
int32_t encoderGetTotal();
void    encoderPrintStatus();