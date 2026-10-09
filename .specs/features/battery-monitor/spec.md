# spec.md — Monitor de Tensão de Bateria (Lolin Light / ESP32)

- **Fonte de verdade do produto:** `docs/memorial.txt` (não editar).
- **Fluxo SDD:** Médio/Complexo — novo MCU + nova interface elétrica (elevação
  justificada pela tabela de dimensionamento da skill `sdd-embarcado`).
- **Rastreamento de requisitos:** `docs/agentic/TRACEABILITY.md`.
- **Estado das tarefas:** `tasks.md`.
- **Decisões técnicas:** `design.md`.

## 1. Objetivo

Medir a tensão de uma bateria de lítio através de um divisor de tensão ligado ao
pino analógico **GPIO 35** do ESP32 e transmitir o valor calculado no monitor
serial em formato fixo, para avaliação técnica de bancada sem intervenção
externa.

## 2. Fora de escopo

Explícito no memorial (`docs/memorial.txt` §1.2) — **não implementar**:

- rotinas de sleep / baixo consumo;
- Wi-Fi, Bluetooth ou qualquer comunicação sem fio;
- armazenamento em memória não volátil (NVS/SPIFFS/EEPROM);
- lógica de gerenciamento/carga da bateria;
- qualquer atuador, display, alarme ou persistência.

## 3. Contexto de execução

Loop principal apenas (`setup`/`loop`). Sem ISR, DMA, RTOS ou concorrência.

## 4. Requisitos funcionais

### FR-001 — Aquisição no pino analógico

> Origem: memorial §3.1.1.

O firmware deve configurar o **GPIO 35** (`AO35` / `GPO 35`) como entrada
analógica de alta impedância, usando a conversão ADC nativa da plataforma, e
amostrar o valor bruto do ADC.

| Atributo | Valor |
|---|---|
| Pino | GPIO 35 (ADC1_CH7), entrada pura — sem pull-up/pull-down interno |
| Resolução | 12 bits (0..4095) |
| Atenuação | 11 dB (necessária: o divisor entrega até ~2,1 V, acima da faixa de 0 dB) |
| Conversão para tensão | rotina calibrada do core (`analogReadMilliVolts`) — ver `ADR-002` |

### FR-002 — Cálculo e recomposição da tensão

> Origem: memorial §3.1.2.

O firmware deve converter o valor obtido do ADC para a tensão equivalente em
volts e aplicar a operação **inversa** da rede atenuadora. Como
R1 = R2 = 47 kΩ, a atenuação é 0,5, logo:

$$V_{bateria} = V_{ADC} \times 2{,}0$$

O fator multiplicativo deve ser **fixo e igual a 2,0**.

### FR-003 — Transmissão serial periódica

> Origem: memorial §3.1.3 e §4.1.

O firmware deve inicializar a UART e enviar periodicamente o valor calculado da
tensão da bateria, formatado como **exatamente**:

```text
Tensão da Bateria: X.XX V
```

`X.XX` = ponto flutuante com duas casas decimais, seguido de **um espaço
simples** e da letra maiúscula `V`.

## 5. Requisitos não funcionais

### NFR-001 — Clareza e modularidade

> Origem: memorial §3.2, item 1.

O código deve ser limpo, legível e determinístico, sem abstrações complexas não
solicitadas nem dependências externas além do core Arduino-ESP32. A lógica pura
(cálculo, média e formatação) fica isolada de `Arduino.h` em
`firmware/lib/battery_logic/`, para ser testável em HOST.

### NFR-002 — Estabilidade da leitura

> Origem: memorial §3.2, item 2.

A amostragem deve minimizar flutuações do valor exibido. Implementação: média
móvel de 8 amostras a 4 Hz (janela de 2 s), sem alocação dinâmica.

- **Medido (HIL, 2026-10-09):** 0 mV pico-a-pico em 12 s com o pino em repouso;
  190 mV pico-a-pico em 60 s com o **pino flutuante** (nenhuma fonte conectada).
- *Métrica de variação admissível com fonte estável: **A CONFIRMAR** na bancada
  (não definida no memorial).*

### NFR-003 — Prontidão para testes

> Origem: memorial §3.2, item 3.

O firmware deve iniciar a rotina principal automaticamente após o power-on
reset, sem intervenção externa. O loop é não bloqueante (`millis()`), sem
`delay()`.

### NFR-004 — Orçamento de recursos (derivado)

Não há orçamento numérico no memorial. Adota-se: a aplicação deve compilar sem
warnings e manter folga confortável de flash/RAM.

- Medido: **flash 277 129 B (21,1 %)** e **RAM 21 728 B (6,6 %)**.
- Sem alocação dinâmica; um único buffer estático de 48 B para a mensagem.

## 6. Critérios de aceite

Origem: memorial §4.2.

### CA-001 — Mapeamento do pino de entrada

**DADO** o firmware gravado no ESP32, **QUANDO** a placa é energizada, **ENTÃO**
o GPIO 35 é configurado como entrada analógica (12 bits, atenuação 11 dB) e a
rotina principal passa a amostrá-lo sem intervenção externa.

### CA-002 — Exatidão do fator de conversão

**DADO** um valor de tensão no pino, **QUANDO** a tensão da bateria é calculada,
**ENTÃO** o resultado é o valor do pino multiplicado por exatamente **2,0**.

- Verificação: 2 100 mV → 4,20 V; 1 500 mV → 3,00 V; 1 250 mV → 2,50 V.

### CA-003 — Formatação da saída serial

**DADO** um valor de tensão, **QUANDO** a mensagem é transmitida, **ENTÃO** a
linha é exatamente `Tensão da Bateria: X.XX V` (duas casas decimais, espaço
simples, `V` maiúsculo), na UART a 115 200 baud.

### CA-004 — Execução estável após o boot

**DADO** 60 s de operação contínua após o boot, **QUANDO** o monitor serial é
observado, **ENTÃO** o firmware emite uma linha por segundo, sem reiniciar e
com valores coerentes com a tensão aplicada.

- **Medido (HIL, 2026-10-09):** 60 linhas em 60 s (1 linha/s, jitter ≤ 1 ms),
  sem reinício, primeira linha ~1,19 s após o reset. A parte de *coerência com
  a tensão aplicada* permanece **PENDENTE** — nenhuma tensão foi aplicada.

## 7. Matriz de rastreabilidade

| Requisito | Critério | Tarefa | Código | Teste / Evidência | Nível | Estado |
|---|---|---|---|---|---|---|
| FR-001 | CA-001 | T-005 | `firmware/src/main.cpp` (`setup`), `firmware/src/config.h` | Build + HIL: boot→saída em ~1,19 s no ESP32 real | HIL | **PASS** (configuração) |
| FR-001 | CA-001 | T-012 | idem | Leitura elétrica real com tensão conhecida | BANCADA | **PENDENTE** |
| FR-002 | CA-002 | T-004 | `firmware/lib/battery_logic/src/battery_logic.cpp` | `test_battery_logic` (HOST) — 12/12 PASSED | HOST | **PASS** |
| FR-002 | CA-002 | T-012 | idem | Fator 2,0 confirmado contra multímetro | BANCADA | **PENDENTE** |
| FR-003 | CA-003 | T-004, T-005 | `battery_logic.cpp`, `main.cpp` (`reportVoltage`) | HOST + HIL: **72/72** linhas reais no formato exato | HOST + HIL | **PASS** |
| NFR-001 | — | T-004, T-005 | `lib/battery_logic/` sem `Arduino.h` | `test -e native` compila a lib isolada | HOST | **PASS** |
| NFR-002 | — | T-004 | `MilliVoltAverager` | Testes de média móvel (HOST) + HIL 0 mV p-p em 12 s | HOST + HIL | PASS (algoritmo) |
| NFR-002 | — | T-012 | idem | Métrica admissível com fonte estável | BANCADA | **PENDENTE** |
| NFR-003 | CA-004 | T-005 | `main.cpp` (`setup`/`loop` sem `delay`) | HIL: boot autônomo, sem `delay()` | HIL | **PASS** |
| NFR-004 | — | T-007 | `platformio.ini` | Build: 0 warnings, flash 21,1 %, RAM 6,6 % | Build | **PASS** |
| — | CA-004 | T-011 | — | HIL 60 s: 60/60 linhas, cadência 1000 ms, sem reinício | HIL | **PASS** |
| — | CA-004 | T-012 | — | Coerência dos valores com a tensão aplicada | BANCADA | **PENDENTE** |

## 8. Premissas

| # | Premissa | Status |
|---|---|---|
| P-1 | Placa = WEMOS LOLIN32 Lite (`lolin32_lite`), ESP32 com ADC1 em GPIO 35 | **MCU CONFIRMADO** no HIL: ESP32-D0WDQ6 rev v1.1, 4 MB, VRef em eFuse. Modelo comercial exato continua **A CONFIRMAR** |
| P-2 | Baud da UART = 115 200 (memorial não fixa) | **A CONFIRMAR** |
| P-3 | Período de envio = 1 000 ms e amostragem = 250 ms (memorial diz "periodicamente") | **A CONFIRMAR** |
| P-4 | Bateria de lítio 1S (máx. ~4,2 V → 2,1 V no pino, dentro da faixa linear do ADC com 11 dB) | **A CONFIRMAR** |
| P-5 | Placa alimentada em 3,3 V estáveis | **A CONFIRMAR** |

## 9. Riscos

| # | Risco | Mitigação | Risco residual |
|---|---|---|---|
| R-1 | Não-linearidade do ADC do ESP32 fora da faixa ~150–2 450 mV (com 11 dB) | Operar em 2,1 V (dentro da faixa); comparar com multímetro na bancada | Médio — requer BANCADA |
| R-2 | `Vref` real diferente do nominal (ADC é notoriamente impreciso) | Usar conversão calibrada do core (`analogReadMilliVolts`), não um `Vref` assumido (ADR-002) | Baixo |
| R-3 | Suporte a `%f` no `snprintf` do newlib do ESP32 | Confirmado no build/link; validar a string real no monitor | Baixo |
| R-4 | Identidade da placa diferente de `lolin32_lite` | MCU confirmado por `esptool flash_id` (ESP32-D0WDQ6); modelo comercial segue A CONFIRMAR | Baixo |
| R-5 | **Nenhuma tensão foi aplicada ao divisor** (`GPIO 35` flutuante) | Registrar como limitação; validar com fonte conhecida antes de declarar CA-002 no hardware | Médio |
| R-6 | Sem detecção de sensor/bateria ausente: pino flutuante é reportado como medida (~0,28 V) | Fora do escopo do memorial; documentado como limitação conhecida | Baixo |

## 10. Desvios e pendências de governança

### SPEC_DEVIATION-001 — Alvo de hardware difere de `AGENTS.md` §5

`AGENTS.md` §5 fixa pinagem e proibições do **projeto térmico ESP8266
(NodeMCU v2)**: DS18B20 em D2/GPIO4, buzzer em D0/GPIO16, PWM em D1/GPIO5, FSM em
`firmware/lib/thermal_logic/`. O memorial especifica **ESP32** (GPIO 35 =
ADC1_CH7, entrada pura) — alvo incompatível.

Resolução aplicada: o memorial (`docs/memorial.txt`) é a fonte de verdade desta
tarefa e prevalece; o alvo vigente é registrado em `ADR-001`. `AGENTS.md` **não
foi alterado** nesta sessão (decisão conservadora, para não destruir o registro
do projeto térmico).

**Pendente para o responsável:** decidir entre (a) atualizar `AGENTS.md` §2–§7
para o alvo ESP32, ou (b) separar os dois produtos em documentos de regras
distintos. Enquanto não decidido, `AGENTS.md` §5 permanece **inconsistente** com
o código deste diretório.
