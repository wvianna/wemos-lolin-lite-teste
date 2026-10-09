# STATUS — Monitor de Tensão de Bateria (ESP32 / LOLIN32 Lite)

- **Data:** 2026-10-09
- **Recurso:** `.specs/features/battery-monitor/`
- **Última alteração relevante:** implementação completa + gravação e teste HIL
  no ESP32 físico (`/dev/ttyUSB0`) e README reescrito com a skill `create-readme`
  (licença MIT).

## Concluído

- Projeto PlatformIO criado: envs `lolin32_lite` (alvo, default) e `native` (HOST).
- Lógica pura em `firmware/lib/battery_logic/` (fator 2,0, média móvel de 8
  amostras, formatação da mensagem) **sem** `Arduino.h`.
- Camada HAL em `firmware/src/` (ADC GPIO 35 a 12 bits / 11 dB, UART 115200,
  loop não bloqueante com `millis()`).
- 12 testes Unity — **12/12 PASSED** no env `native`.
- Build limpo do alvo: **0 erros, 0 warnings**, flash 277 129 B (21,1 %),
  RAM 21 728 B (6,6 %).
- **HIL no hardware real:** chip identificado (ESP32-D0WDQ6 rev v1.1, 4 MB),
  firmware gravado e verificado.
- Contrato serial validado no alvo: **72/72 linhas** no formato exato
  `Tensão da Bateria: X.XX V`, cadência de 1 linha/s (jitter ≤ 1 ms) por 60 s
  contínuos, primeira linha em ~1,19 s após o reset, sem reinício.
- Verificador HIL reprodutível: `firmware/scripts/hil_serial_check.py`.
- Artefatos SDD: `constitution.md`, `spec.md` (FR/NFR/CA + rastreabilidade),
  `design.md` (ADR-001..004), `tasks.md`, evidências em `docs/05-testing/`.
- `README.md` reescrito conforme a skill `create-readme` (header com badges,
  admonitions GFM, tom conciso, sem seção de licença).
- **Licença do projeto definida como MIT** (decisão do responsável, 2026-10-09):
  criado `LICENSE`; `AGENTS.md` §9 sincronizado de Apache 2.0 para MIT.
- A licença é declarada no `README.md` por **badge + rodapé** (não por uma seção
  dedicada), conforme a skill `create-readme`, que reserva licença/contribuição
  para arquivos próprios. Isso é a "decisão registrada em contrário" prevista em
  `.github/skills/sdd-embarcado/SKILL.md` §8.

## Em andamento

- Nada bloqueando a implementação. O trabalho restante é de **validação física**.

## Pendências

| # | Pendência | Bloqueio | Responsável |
|---|---|---|---|
| 1 | Validar o fator 2,0 no hardware (CA-002) com tensão conhecida | Sem tensão aplicada ao divisor; GPIO 35 flutuante | Bancada / responsável |
| 2 | Métrica admissível de estabilidade (NFR-002) no alvo | Exige fonte estável | Bancada |
| 3 | CA-004 com "valores coerentes" | Sem tensão aplicada | Bancada |
| 4 | Confirmar modelo exato da placa (P-1) | Não distinguível por software | Responsável |
| 5 | Confirmar baud e cadências (P-2/P-3) | Não fixados no memorial | Responsável |
| 6 | Resolver `SPEC_DEVIATION-001` (`AGENTS.md` §5 × alvo ESP32) | Decisão de governança | Responsável |

## Problemas e erros conhecidos

- **Leitura ~0,28 V com 190 mV de ruído:** comportamento esperado de pino ADC
  flutuante, não é defeito. Com o divisor conectado a uma bateria, a leitura
  deve subir para ~2× a tensão da bateria no valor reportado.
- **`pio` do `PATH` não executa** neste host (venv pipx com *symlink* quebrado
  para `/usr/bin/python3`). Contorno documentado no `README.md`
  (`PYTHONPATH` + `python3 -m platformio`).
- **Alterações desta sessão não commitadas.** O repositório git existe (commit
  `460f004`); o working tree tem 4 arquivos modificados (`AGENTS.md`, `README.md`,
  `STATUS.md`, `HANDOFF.md`) e `LICENSE` não rastreado. O gate de change-control
  (`git status` / `git diff --stat`) foi reexecutado e confirma o escopo: do
  `AGENTS.md` só o §9 mudou. Nenhum commit foi feito pelo agente (`AGENTS.md` §8).

## Testes realizados

| Nível | Comando | Resultado |
|---|---|---|
| HOST | `platformio test -e native` | 12/12 PASSED |
| Build | `platformio run -e lolin32_lite` (após `clean`) | SUCCESS, 0 warnings |
| HIL | `hil_serial_check.py --seconds 12` | PASS — 12/12 linhas conformes |
| HIL | `hil_serial_check.py --seconds 60` | PASS — 60/60 linhas conformes |
| Bancada elétrica | — | **não realizado** (sem tensão aplicada) |

## Próximo passo recomendado

Conectar a bateria ao divisor (topo de R1) e repetir o procedimento da seção
"HIL — fator 2,0 com tensão conhecida" do `README.md`, registrando a referência
do multímetro. Isso fecha CA-002 (hardware), NFR-002 (métrica) e a parte de
coerência de CA-004.
