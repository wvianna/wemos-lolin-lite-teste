// Monitor de tensão de bateria — WEMOS LOLIN32 Lite (ESP32).
//
// Camada HAL/BSP (depende do SDK): configuração do ADC, da UART e agendamento
// não bloqueante com millis(). Toda a matemática vive em lib/battery_logic
// (lógica pura, testada em HOST).
//
// Contexto de execução: apenas o loop principal (setup/loop). Não há ISR, DMA
// nem persistência — ver design.md.
#include <Arduino.h>

#include "battery_logic.h"
#include "config.h"

namespace {

battery::MilliVoltAverager g_averager;
char g_message[kMessageBufferSize];
uint32_t g_lastSampleMs = 0;
uint32_t g_lastReportMs = 0;

// Lê o ADC e alimenta a média móvel (NFR-002).
void sampleAdc(uint32_t nowMs) {
  // analogReadMilliVolts aplica a calibração do core (eFuse), evitando assumir
  // um Vref nominal inexato — ver ADR-002 em design.md.
  g_averager.add(analogReadMilliVolts(kAdcPin));
  g_lastSampleMs = nowMs;
}

// Calcula e transmite a tensão da bateria no formato exigido (FR-003).
void reportVoltage(uint32_t nowMs) {
  const float volts = battery::batteryVolts(g_averager.averageMilliVolts());
  if (battery::formatBatteryMessage(volts, g_message, sizeof(g_message)) > 0) {
    Serial.println(g_message);
  }
  g_lastReportMs = nowMs;
}

}  // namespace

void setup() {
  Serial.begin(kSerialBaud);

  // GPIO 35 é entrada pura (ADC1_CH7): sem pull-up/pull-down interno.
  pinMode(kAdcPin, INPUT);
  analogReadResolution(kAdcResolutionBits);
  analogSetPinAttenuation(kAdcPin, kAdcAttenuation);

  g_averager.reset();

  const uint32_t now = millis();
  g_lastSampleMs = now;
  g_lastReportMs = now;
  sampleAdc(now);
}

void loop() {
  const uint32_t now = millis();

  if (static_cast<uint32_t>(now - g_lastSampleMs) >= kSampleIntervalMs) {
    sampleAdc(now);
  }
  if (static_cast<uint32_t>(now - g_lastReportMs) >= kReportIntervalMs) {
    reportVoltage(now);
  }
}
