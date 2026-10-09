# Constituição do Projeto — Monitor de Tensão de Bateria

> Derivada de `.github/skills/sdd-embarcado/references/constitution.md`.
> Princípios estáveis do projeto. Requisitos específicos de um recurso vivem em
> `.specs/features/<recurso>/spec.md`.
>
> **Alvo vigente deste firmware é ESP32 (WEMOS LOLIN32 Lite), conforme
> `ADR-001`. As restrições de hardware descritas em `AGENTS.md` §5 referem-se ao
> projeto térmico ESP8266 e NÃO se aplicam a este firmware — ver
> `SPEC_DEVIATION-001` em `spec.md`.**

## Identidade do alvo

| Item | Valor |
|---|---|
| Produto/sistema | Monitor de tensão de bateria de lítio com saída em monitor serial |
| MCU e variante | Espressif ESP32 (Xtensa LX6, ADC1). GPIO 35 = ADC1_CH7, entrada pura |
| Placa/revisão | WEMOS LOLIN32 Lite — **A CONFIRMAR** (o memorial diz "Lolin Light") |
| Toolchain/SDK | PlatformIO Core 6.1.19 · platform `espressif32@6.9.0` · framework Arduino (`framework-arduinoespressif32` 3.20017.241212+sha.dcc1105b = Arduino-ESP32 2.0.17) · `toolchain-xtensa-esp32` |
| Clock e alimentação | 240 MHz; placa alimentada em 3,3 V |
| Ambientes de validação | `HOST` **disponível**; `SIMULADOR` não se aplica; `BANCADA`/`HIL` **indisponíveis nesta sessão** |

## Princípios obrigatórios

### 1. Segurança e estado seguro

- Este produto não possui atuador; o "estado seguro" é não interferir — o firmware apenas mede e reporta.
- A proteção contra sobretensão no pino é **elétrica** (divisor R1/R2 de 47 kΩ, atenuação 0,5). Nenhuma alteração de firmware substitui ou contorna essa proteção.
- Alterações que afetem limites elétricos exigem decisão registrada (ADR).

### 2. Determinismo e concorrência

- Não há ISR, DMA, task de RTOS nem persistência. Todo o código roda no loop principal (`setup`/`loop`).
- `delay()` bloqueante é proibido no loop; o agendamento usa `millis()`.
- Alocação dinâmica é **proibida**: buffers estáticos de tamanho definido em tempo de compilação.
- Cada requisito de tempo é declarado em `spec.md` com período e tolerância.

### 3. Recursos limitados

- Limites do alvo: 1 310 720 B de flash e 327 680 B de RAM para aplicação (ESP32, 4 MB).
- Orçamento atual medido: flash 277 129 B (21,1 %) e RAM 21 728 B (6,6 %) — ver `NFR-004`.
- Sem persistência (NVS/SPIFFS/EEPROM) fora do escopo deste firmware.

### 4. Interfaces de hardware e comunicação

- Pino, unidade, escala, faixa e atenuação do ADC são documentados em `design.md` **antes** da implementação.
- A UART é o único canal de saída; o formato da mensagem é um contrato verificável (`CA-003`).
- Alterações de pinagem, atenuação ou formato da mensagem exigem revisão de compatibilidade.

### 5. Qualidade e rastreabilidade

- Todo requisito funcional recebe `FR-###`; todo não funcional `NFR-###`; todo critério de aceite `CA-###`; toda tarefa `T-###`; toda decisão arquitetural `ADR-###`.
- Cada requisito tem teste ou evidência; o que não é verificável aqui fica `PENDENTE` com risco residual.
- O build deve ser reproduzível: versões de plataforma, framework e toolchain ficam fixadas em `platformio.ini` e registradas na evidência.
- Warnings de compilação são tratados como defeito (`-Wall -Wextra`, tolerância zero).

### 6. Diagnóstico e recuperação

- O diagnóstico é a própria saída serial: uma linha por segundo no formato contratado.
- O firmware não imprime banner nem mensagens extras, para não poluir a análise automatizada do monitor serial.
- Não há watchdog próprio, brownout handler nem recuperação de falha especificada; reset do ESP32 reinicia o ciclo de amostragem — comportamento aceito para o escopo.

### 7. Processo de mudança

- Mudança de comportamento exige atualização prévia ou acompanhante de `spec.md`; a especificação não pode ficar obsoleta em relação ao código.
- Decisões que alterem risco, arquitetura, timing, memória ou compatibilidade são registradas em `design.md` (ADR) e `STATUS.md`.
- Não se adicionam abstrações, dependências ou camadas sem benefício verificável no alvo.
- Commits não são feitos automaticamente.

## Gates padrão

- [x] Requisitos e critérios têm IDs e são observáveis.
- [x] Alvo, versão de toolchain e dependências foram confirmados.
- [x] Caminhos de erro (buffer insuficiente, entrada negativa) foram considerados.
- [x] RAM, flash e timing foram avaliados; concorrência e energia não se aplicam (sem ISR/persistência).
- [x] Testes executados no nível declarado: `HOST` (PASS) e build de alvo (PASS).
- [x] Resultado e limitações registrados em `docs/05-testing/evidence-battery-monitor.md`.
