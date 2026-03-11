#pragma once

#include "main.h"

// ============================================================
//  relay.h — interface do módulo de controle do relay
// ============================================================

// Configura o pino e garante relay desligado no boot
void relayInit();

// Deve ser chamado no loop() continuamente (sem delay).
// Decide se o sistema deve estar ativo e dispara os pulsos.
void relayUpdate();

// Força relay ON ou OFF manualmente (para testes)
void relayForceOn();
void relayForceOff();

// Para tudo: sistema inativo, relay off
// reason: string curta para log serial
void relayStopAll(const char* reason);
