// Lógica pura do monitor de tensão de bateria.
//
// Esta biblioteca NÃO inclui Arduino.h nem headers do ESP-IDF: é o que garante
// o teste em HOST (env `native`). Ver AGENTS.md §6 e NFR-001.
#ifndef BATTERY_LOGIC_H
#define BATTERY_LOGIC_H

#include <cstddef>
#include <cstdint>

namespace battery {

// Fator de reconstrução de escala do divisor de tensão.
// R1 = R2 = 47 kΩ → atenuação de 0,5 → V_bateria = V_pino × 2,0 (FR-002).
constexpr float kDividerScaleFactor = 2.0f;

// Janela da média móvel usada para estabilizar a leitura (NFR-002).
constexpr std::size_t kAveragingWindow = 8;

// Converte a tensão medida no pino do ADC, em milivolts, na tensão da bateria
// em volts, aplicando o fator de escala do divisor (FR-002).
// Entradas nulas ou negativas são saturadas em 0,0 V (comportamento defensivo).
float batteryVolts(std::int32_t pinMilliVolts);

// Média móvel circular de amostras do ADC. Buffer estático, sem alocação
// dinâmica. Enquanto a janela não estiver cheia, a média considera apenas as
// amostras já coletadas.
class MilliVoltAverager {
 public:
  void reset();
  void add(std::int32_t pinMilliVolts);
  std::int32_t averageMilliVolts() const;
  std::size_t count() const;

 private:
  std::int32_t samples_[kAveragingWindow] = {};
  std::size_t next_ = 0;
  std::size_t count_ = 0;
};

// Formata a mensagem do terminal serial no formato exigido pela especificação:
//   "Tensão da Bateria: X.XX V"
// Retorna o número de caracteres gravados (sem o '\0') ou -1 quando o buffer é
// nulo/insuficiente (nesse caso ele fica truncado e terminado em '\0').
int formatBatteryMessage(float volts, char* out, std::size_t outSize);

}  // namespace battery

#endif  // BATTERY_LOGIC_H
