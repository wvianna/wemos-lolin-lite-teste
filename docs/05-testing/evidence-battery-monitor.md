# Evidência de validação — Monitor de Tensão de Bateria

- **Data:** 2026-10-09
- **Recurso:** `.specs/features/battery-monitor/`
- **Requisitos cobertos:** FR-001, FR-002, FR-003, NFR-001..NFR-004, CA-001..CA-004

## 1. Ambiente

| Item | Valor |
|---|---|
| Host | Linux (Vaio), Python 3.12 |
| PlatformIO Core | 6.1.19 (instalação pipx; invocada via `PYTHONPATH` — ver README) |
| Platform | `espressif32@6.9.0` |
| Framework | `framework-arduinoespressif32` 3.20017.241212+sha.dcc1105b (Arduino-ESP32 2.0.17) |
| Toolchain | `toolchain-xtensa-esp32` |
| Gravação | esptool v4.11.0 |
| Alvo (HIL) | ESP32-D0WDQ6 rev v1.1 · MAC `a0:dd:6c:af:6c:50` · flash 4 MB · cristal 40 MHz · *VRef calibration in efuse* |
| Porta | `/dev/ttyUSB0` (ponte USB-serial `1a86:USB Serial`, 115200 baud) |

Comando base usado em todos os gates:

```bash
export PYTHONPATH="$HOME/.local/share/pipx/venvs/platformio/lib/python3.12/site-packages"
```

## 2. Gate HOST — testes da lógica pura (NFR-001, CA-002, CA-003)

```bash
python3 -m platformio test -d firmware -e native
```

```text
------------- native:test_battery_logic [PASSED] Took 0.90 seconds -------------
Environment    Test                Status    Duration
-------------  ------------------  --------  ------------
native         test_battery_logic  PASSED    00:00:00.901
================= 12 test cases: 12 succeeded in 00:00:00.900 =================
```

Cobertura: fator exato 2,0; `2100 mV → 4,20 V`, `1500 mV → 3,00 V`,
`1250 mV → 2,50 V`; saturação de entrada negativa; média móvel (parcial, janela
cheia, descarte da mais antiga, reset); string exata do contrato; duas casas
decimais sempre; recusa de buffer pequeno/nulo.

## 3. Gate de build do alvo (NFR-004)

```bash
python3 -m platformio run -d firmware -e lolin32_lite        # após -t clean
```

```text
RAM:   [=         ]   6.6% (used 21728 bytes from 327680 bytes)
Flash: [==        ]  21.1% (used 277129 bytes from 1310720 bytes)
========================= [SUCCESS] Took 5.94 seconds =========================
```

Build limpo do zero: **0 erros e 0 ocorrências de `warning`** no log
(`-Wall -Wextra`).

## 4. HIL 1 — identificação do chip (P-1, CA-001)

```bash
python3 ~/.platformio/packages/tool-esptoolpy/esptool.py --port /dev/ttyUSB0 flash_id
```

```text
Detecting chip type... ESP32
Chip is ESP32-D0WDQ6 (revision v1.1)
Features: WiFi, BT, Dual Core, 240MHz, VRef calibration in efuse, Coding Scheme None
Crystal is 40MHz
MAC: a0:dd:6c:af:6c:50
Detected flash size: 4MB
```

`P-1` confirmado no que diz respeito ao **MCU**: é um ESP32 (D0WDQ6, 4 MB), com
calibração de VRef em eFuse — o que valida a premissa do `ADR-002`
(`analogReadMilliVolts` usa essa calibração). A **marca/modelo exato da placa**
(LOLIN32 Lite) não é distinguível por software e continua `A CONFIRMAR`.

## 5. HIL 2 — gravação (CA-001)

```bash
python3 -m platformio run -d firmware -e lolin32_lite -t upload --upload-port /dev/ttyUSB0
```

```text
Wrote 277488 bytes (154689 compressed) at 0x00010000 in 2.8 seconds (effective 781.3 kbit/s)...
Hash of data verified.
Hard resetting via RTS pin...
========================= [SUCCESS] Took 6.75 seconds =========================
```

## 6. HIL 3 — teste de 12 s (CA-003, NFR-002)

```bash
python3 firmware/scripts/hil_serial_check.py --port /dev/ttyUSB0 --seconds 12
```

```text
tempo observado : 12 s
linhas recebidas: 12
linhas conformes: 12
fora do contrato: 0
tensão: min 0.28 V | max 0.28 V | média 0.28 V
variação pico-a-pico: 0 mV
intervalo entre linhas: média 1000 ms | máx 1001 ms | mín 999 ms
primeiras linhas:
   t=  1.19s  Tensão da Bateria: 0.28 V
RESULTADO: PASS
```

## 7. HIL 4 — teste de 60 s (CA-004)

```bash
python3 firmware/scripts/hil_serial_check.py --port /dev/ttyUSB0 --seconds 60
```

```text
tempo observado : 60 s
linhas recebidas: 60
linhas conformes: 60
fora do contrato: 0
tensão: min 0.28 V | max 0.47 V | média 0.32 V
variação pico-a-pico: 190 mV
intervalo entre linhas: média 1000 ms | máx 1000 ms | mín 999 ms
RESULTADO: PASS
```

Interpretação:

- **Cadência:** exatamente 1 linha/s por 60 s, jitter ≤ 1 ms.
- **Contrato serial:** 72 linhas capturadas nos dois testes, **72/72** no formato
  exato `Tensão da Bateria: X.XX V` (0 fora do contrato).
- **Boot autônomo:** em ambos os testes a primeira linha surgiu em `t ≈ 1,19 s`
  após o reset, sem qualquer intervenção — `setup()` roda no POR (NFR-003).
- **Sem reinício:** contagem de linhas igual ao tempo observado em segundos;
  nenhuma reemissão de "primeira linha" no decorrer do teste.

## 8. Condição de bancada no momento do teste HIL

| Item | Valor |
|---|---|
| Ligado ao GPIO 35 | **nada** (pino flutuante) — confirmado pelo responsável |
| Referência de multímetro | **não medida** |
| Leitura observada | 0,28 V média, 190 mV pico-a-pico (60 s) |

A leitura baixa e ruidosa é o **comportamento esperado de um pino ADC de alta
impedância flutuante**, não um defeito do firmware. Ela não valida nem invalida o
fator de escala: sem tensão aplicada não existe grandeza de referência.

## 9. Resultado por critério de aceite

| Critério | Descrição | Nível de evidência | Resultado |
|---|---|---|---|
| CA-001 (configuração) | GPIO 35 configurado como ADC, amostragem após o boot | HIL (build + execução) | **PASS** — boot→saída em ~1,19 s |
| CA-001 (elétrico) | Leitura correta de uma tensão real no pino | BANCADA | **PENDENTE** — nada conectado |
| CA-002 | Fator multiplicativo exato de 2,0 | HOST | **PASS** — 12 testes |
| CA-002 (hardware) | Fator 2,0 confirmado com tensão conhecida | BANCADA | **PENDENTE** — sem referência |
| CA-003 | Formato exato da mensagem serial | HOST + HIL | **PASS** — 72/72 linhas reais no contrato |
| CA-004 | 60 s estável, 1 linha/s, sem reinício | HIL | **PASS** |
| CA-004 (coerência) | Valores coerentes com a tensão aplicada | BANCADA | **PENDENTE** — nada aplicado |
| NFR-001 | Lógica pura isolada, testável em HOST | HOST | **PASS** |
| NFR-002 (algoritmo) | Média móvel sem alocação dinâmica | HOST + HIL | **PASS** — 0 mV p-p em 12 s |
| NFR-002 (métrica) | Limite admissível de variação no alvo | BANCADA | **PENDENTE** — exige fonte estável |
| NFR-003 | Início automático após POR, sem `delay()` | HIL | **PASS** |
| NFR-004 | Flash/RAM e build sem warnings | Build | **PASS** — 21,1 % / 6,6 % / 0 warnings |

## 10. Limitações e riscos residuais

1. **Validação elétrica não realizada** — nenhuma tensão foi aplicada ao divisor.
   O fator 2,0 está provado apenas em HOST, **não** no hardware. Risco residual
   médio (R-1 da `spec.md`).
2. **Sem detecção de sensor ausente** — com o pino flutuante o firmware reporta
   ~0,28 V como se fosse medida. O memorial não pede detecção de sensor ausente,
   logo isso é uma **limitação conhecida do escopo**, não um defeito.
3. **Identidade da placa** — confirmado "ESP32-D0WDQ6"; o modelo comercial da
   placa permanece `A CONFIRMAR` (não afeta GPIO 35/ADC1).
4. **Baud e cadência** — 115200 baud, 250 ms de amostragem e 1000 ms de relatório
   são escolhas registradas (P-2/P-3), não valores do memorial.
