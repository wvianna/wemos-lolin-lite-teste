# tasks.md — Monitor de Tensão de Bateria

Estados: `[ ]` pendente · `[-]` em andamento · `[x]` concluída · `[!]` bloqueada.

| ID | Tarefa | Requisitos | Estado | Gate / Evidência |
|---|---|---|---|---|
| T-001 | Triagem e roteamento da solicitação | — | `[x]` | Fluxo classificado como Médio/Complexo (novo MCU + interface elétrica) |
| T-002 | Registrar constituição e alvo vigente | NFR-001 | `[x]` | `.specs/project/constitution.md`; `ADR-001` |
| T-003 | Criar projeto PlatformIO (envs alvo + HOST) | NFR-004 | `[x]` | `firmware/platformio.ini`; board `lolin32_lite` |
| T-004 | Implementar lógica pura (fator 2,0, média, formato) | FR-002, FR-003, NFR-001, NFR-002 | `[x]` | `firmware/lib/battery_logic/` |
| T-005 | Implementar camada HAL (ADC GPIO35, UART, loop) | FR-001, FR-003, NFR-003 | `[x]` | `firmware/src/main.cpp`, `firmware/src/config.h` |
| T-006 | Escrever testes Unity da lógica pura | FR-002, FR-003, NFR-002 | `[x]` | `firmware/test/test_battery_logic/test_main.cpp` |
| T-007 | Gate: compilar para o alvo, sem warnings | CA-001, NFR-004 | `[x]` | `pio run -e lolin32_lite` → SUCCESS, 0 warnings, flash 21,1 %, RAM 6,6 % |
| T-008 | Gate: executar testes HOST | CA-002, CA-003 | `[x]` | `pio test -e native` → 12/12 PASSED |
| T-009 | Registrar evidências de teste | todos | `[x]` | `docs/05-testing/evidence-battery-monitor.md` |
| T-010 | Documentar porta de entrada + estado + handoff | — | `[x]` | `README.md`, `STATUS.md`, `HANDOFF.md` |
| T-011 | **HIL**: gravar no alvo e validar o contrato serial (12 s e 60 s) | CA-003, CA-004, NFR-003 | `[x]` | `hil_serial_check.py` → PASS; 72/72 linhas conformes; cadência 1000 ms; boot→saída ~1,19 s |
| T-012 | **Bancada elétrica**: validar fator 2,0 com tensão conhecida e métrica de NFR-002 | CA-001 (elétrico), CA-002 (HW), NFR-002 (métrica), CA-004 (coerência) | `[!]` | **Bloqueada: nenhuma tensão aplicada ao divisor (GPIO 35 flutuante).** `PENDENTE` |
| T-013 | Confirmar identidade da placa e parâmetros não especificados (P-1..P-5) | — | `[-]` | P-1 (MCU) confirmado no HIL: ESP32-D0WDQ6 4 MB. Modelo comercial e P-2..P-5 seguem `A CONFIRMAR` |
| T-014 | Resolver `SPEC_DEVIATION-001` (`AGENTS.md` §5 × alvo ESP32) | — | `[!]` | **Bloqueada: decisão do responsável** (atualizar `AGENTS.md` ou separar regras) |

## Desvios

| ID | Desvio | Justificativa | Estado |
|---|---|---|---|
| SPEC_DEVIATION-001 | Firmware é ESP32; `AGENTS.md` §5 descreve ESP8266 | `docs/memorial.txt` é a fonte de verdade da tarefa (ver `spec.md` §10, `ADR-001`). Confirmado no HIL: o chip conectado é ESP32-D0WDQ6 | Aberto — aguarda decisão do responsável (T-014) |

## O que NÃO foi feito (e por quê)

- **Validação elétrica:** o ESP32 está conectado e o firmware roda, mas **nada
  está ligado ao GPIO 35** (confirmado pelo responsável). Portanto o fator 2,0
  não foi validado no hardware, não há métrica de estabilidade com fonte real e
  a coerência de valores de `CA-004` não se aplica. Esses itens permanecem
  `PENDENTE` em vez de serem declarados `PASS` sem evidência.
- **Detecção de sensor ausente:** com o pino flutuante o firmware reporta
  ~0,28 V como se fosse medida. Isso é limitação do escopo (o memorial não pede
  essa detecção), registrada como R-6.
- Nenhum commit: commits não são automáticos (`AGENTS.md` §8). O gate de
  change-control (`git status` / `git diff`) foi reexecutado após a inicialização
  do repositório e confirmou o escopo das alterações.

## Evidência HIL executada (2026-10-09)

| Passo | Comando | Resultado |
|---|---|---|
| Identificação | `esptool.py --port /dev/ttyUSB0 flash_id` | ESP32-D0WDQ6 rev v1.1 · 4 MB · VRef em eFuse |
| Gravação | `platformio run -e lolin32_lite -t upload` | SUCCESS · hash verificado |
| Contrato serial | `hil_serial_check.py --seconds 12` | PASS · 12/12 conformes · 0 mV p-p |
| Estabilidade | `hil_serial_check.py --seconds 60` | PASS · 60/60 conformes · 1000 ms ± 1 ms |
