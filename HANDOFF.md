# HANDOFF — Monitor de Tensão de Bateria (ESP32 / LOLIN32 Lite)

> Sessão de 2026-10-09. Continuidade necessária: **validação física pendente**
> (CA-002 no hardware, métrica de NFR-002 e coerência de CA-004).

## Contexto

Implementar `docs/memorial.txt` com framework Arduino + PlatformIO, em um
workspace que continha apenas o pacote agêntico SDD (nenhum firmware, nenhum
diretório `.specs/`). O alvo do memorial é **ESP32** (GPIO 35 = ADC1_CH7), o que
difere do alvo descrito em `AGENTS.md` §5 (ESP8266 NodeMCU v2).

## Estado atual

Firmware **implementado, gravado e rodando** no ESP32 físico. Gates de HOST e de
build passam; o contrato serial foi validado no hardware (72/72 linhas
conformes). O que falta é **validação elétrica com tensão conhecida**.

## Alterações realizadas (inventário de arquivos)

Criados:

| Arquivo | Papel |
|---|---|
| `firmware/platformio.ini` | envs `lolin32_lite` (default) e `native`; `espressif32@6.9.0` fixado |
| `firmware/src/config.h` | pino 35, 12 bits, `ADC_11db`, cadências, buffer de 48 B |
| `firmware/src/main.cpp` | HAL: `setup`/`loop` não bloqueante, `analogReadMilliVolts` |
| `firmware/lib/battery_logic/library.json` | manifesto da lib pura |
| `firmware/lib/battery_logic/src/battery_logic.h` | API pura (fator 2,0, média, formato) |
| `firmware/lib/battery_logic/src/battery_logic.cpp` | implementação (sem `Arduino.h`) |
| `firmware/test/test_battery_logic/test_main.cpp` | 12 testes Unity |
| `firmware/scripts/hil_serial_check.py` | verificador HIL do contrato serial |
| `.specs/project/constitution.md` | constituição do projeto (alvo ESP32) |
| `.specs/features/battery-monitor/spec.md` | FR/NFR/CA + rastreabilidade + `SPEC_DEVIATION-001` |
| `.specs/features/battery-monitor/design.md` | ADR-001..004 + interface do ADC |
| `.specs/features/battery-monitor/tasks.md` | T-001..T-013 |
| `docs/05-testing/evidence-battery-monitor.md` | evidências HOST/build/HIL |
| `README.md` | porta de entrada (skill `create-readme`): badges, hardware, comandos, verificação, limitações |
| `LICENSE` | texto integral da MIT License — Copyright (c) 2026 William |
| `STATUS.md` | estado atual |
| `HANDOFF.md` | este documento |

**Alterados parcialmente:** `AGENTS.md` — **somente o §9 (licença)**, de Apache 2.0
para MIT. Os §§2–7 continuam descrevendo o projeto térmico ESP8266 e seguem
pendentes de decisão (`SPEC_DEVIATION-001`).

**Não alterados:** `docs/memorial.txt`, `AVALIACAO.md`, `README-AGENTIC.md`,
`.github/**`, `.agents/**`, `.vscode/**`, `.gitignore`.

## Decisões

| ID | Decisão |
|---|---|
| ADR-001 | Alvo é ESP32 (`lolin32_lite`); `AGENTS.md` §5 (ESP8266) não se aplica a este firmware |
| ADR-002 | Converter via `analogReadMilliVolts` (calibração eFuse) em vez de assumir `Vref` — confirmado: o chip reporta *VRef calibration in efuse* |
| ADR-003 | Média móvel de 8 amostras a 4 Hz para atender NFR-002 (não é desvio: o fator 2,0 e o formato permanecem exatos) |
| ADR-004 | Saída serial contém **apenas** a mensagem contratada (sem banner), para parsing automatizado |
| — | **Licença MIT** (decisão do responsável, 2026-10-09), substituindo o Apache 2.0 do `AGENTS.md` §9; declarada no README por badge + rodapé, conforme a skill `create-readme` |
| — | Baud 115200, amostragem 250 ms, relatório 1000 ms: escolhas registradas (P-2/P-3), não fixadas no memorial |

## Problemas

1. **`SPEC_DEVIATION-001` (aberto):** `AGENTS.md` §5/§6 ainda descreve o projeto
   ESP8266 e a lib `thermal_logic`, contradizendo este firmware. Optou-se por
   **não** alterar `AGENTS.md` (decisão conservadora). Requer decisão do
   responsável: (a) atualizar `AGENTS.md` para o alvo ESP32, ou (b) separar as
   regras dos dois produtos.
2. **Alterações não commitadas:** o repositório git existe (commit `460f004`) e o
   gate `git status`/`git diff` foi reexecutado — escopo confirmado (4 arquivos
   modificados + `LICENSE` novo). O commit fica a critério do responsável
   (`AGENTS.md` §8: sem commit automático).
3. **`pio` do `PATH` quebrado** (pipx venv com symlink inválido). Use
   `PYTHONPATH=... python3 -m platformio`.

## Testes

| Nível | Resultado |
|---|---|
| HOST (`test -e native`) | 12/12 PASSED |
| Build (`run -e lolin32_lite`, após clean) | SUCCESS · 0 warnings · flash 21,1 % · RAM 6,6 % |
| HIL 12 s | PASS · 12/12 linhas conformes · 0 mV p-p |
| HIL 60 s | PASS · 60/60 linhas conformes · cadência 1000 ms · boot→saída ~1,19 s |
| Bancada elétrica | **não realizado** |

## Pendências

- Aplicar tensão conhecida na entrada do divisor e validar o fator 2,0 no
  hardware (CA-002) com referência de multímetro.
- Definir a métrica admissível de estabilidade (NFR-002) e medir no alvo.
- Confirmar modelo exato da placa (P-1) e os parâmetros P-2/P-3.
- Resolver `SPEC_DEVIATION-001`.

## Próximo passo

1. Conectar a bateria (ou fonte) ao topo de R1, com R1/R2 de 47 kΩ e o tap
   central no GPIO 35.
2. Medir com multímetro e registrar.
3. Rodar `python3 firmware/scripts/hil_serial_check.py --port /dev/ttyUSB0 --seconds 60`.
4. Atualizar `docs/05-testing/evidence-battery-monitor.md`, `STATUS.md` e a
   matriz de rastreabilidade de `spec.md` com o resultado.

## Cuidados

- **Não** ligar a bateria diretamente no GPIO 35: o pino só tolera ~3,3 V e o
  divisor é a única proteção. Sem o divisor, a sobretensão danifica o ESP32.
- O GPIO 35 é **entrada pura**: `pinMode(35, INPUT)`; não usar pull-up/pull-down
  nem `OUTPUT`.
- Não alterar a atenuação para menos de 11 dB sem revisar a faixa (2,1 V no pino
  saturaria a leitura).
- Manter a lógica de cálculo em `lib/battery_logic/` livre de `Arduino.h`, senão
  o gate HOST (`test -e native`) quebra.

## Critério de conclusão

A tarefa estará concluída quando: (1) CA-002, a métrica de NFR-002 e a coerência
de CA-004 tiverem evidência de bancada com tensão de referência, (2) as
pendências 4–6 forem decididas pelo responsável, e (3) `STATUS.md`,
`spec.md` (matriz) e as evidências estiverem atualizados e consistentes com o
código.
