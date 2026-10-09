// Parâmetros de hardware e de cadência do monitor de tensão de bateria.
// Fonte: docs/memorial.txt (seções 2 e 3) e .specs/features/battery-monitor/.
#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Pino de entrada analógica. Aliases documentais do memorial: AO35 / GPO 35.
// GPIO 35 = ADC1_CH7, entrada pura (input-only): não possui pull-up/pull-down.
constexpr uint8_t kAdcPin = 35;

// UART/monitor serial. Valor não fixado na especificação (A CONFIRMAR).
constexpr uint32_t kSerialBaud = 115200;

// Resolução do ADC do ESP32: 12 bits (0..4095).
constexpr uint8_t kAdcResolutionBits = 12;

// Atenuação de 11 dB: faixa útil do pino ~0..3,1 V.
// Necessário porque o divisor entrega até ~2,1 V (4,2 V / 2) — acima da faixa
// da atenuação padrão (0 dB, ~1,1 V), que saturaria a leitura.
constexpr adc_attenuation_t kAdcAttenuation = ADC_11db;

// Cadência (não fixada na especificação; "periodicamente" — A CONFIRMAR).
constexpr uint32_t kSampleIntervalMs = 250;   // 4 amostras por segundo
constexpr uint32_t kReportIntervalMs = 1000;  // 1 mensagem por segundo

// Buffer estático da mensagem serial (proibido acumular String no loop).
constexpr size_t kMessageBufferSize = 48;

#endif  // CONFIG_H
