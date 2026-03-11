#pragma once

#include "main.h"

// ============================================================
//  serial_cmd.h — interface do módulo de comandos serial
// ============================================================

// Verifica Serial.available() e processa comandos pendentes
// Deve ser chamado no loop() a cada iteração
void serialCmdUpdate();

// Imprime o menu de comandos
void serialPrintMenu();

// Imprime status completo do sistema
void serialPrintStatus();
