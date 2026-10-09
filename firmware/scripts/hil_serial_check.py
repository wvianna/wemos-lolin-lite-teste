#!/usr/bin/env python3
"""Verificação HIL do monitor de tensão de bateria (ESP32 / LOLIN32 Lite).

Lê o monitor serial por N segundos e valida o contrato de saída do firmware:

  * toda linha casa com ``Tensão da Bateria: X.XX V``  (CA-003)
  * cadência próxima de 1 linha por segundo            (CA-004)
  * número de linhas compatível com o tempo observado   (CA-004)

Uso (o pyserial vem do ambiente do PlatformIO):

    PYTHONPATH=$HOME/.local/share/pipx/venvs/platformio/lib/python3.12/site-packages \\
      python3 firmware/scripts/hil_serial_check.py --port /dev/ttyUSB0 --seconds 12

Código de saída: 0 = PASS, 1 = FAIL.
"""

import argparse
import re
import statistics
import sys
import time

# Contrato exato de docs/memorial.txt §4.1.
PATTERN = re.compile(r"^Tensão da Bateria: (\d+\.\d{2}) V$")


def main() -> int:
    parser = argparse.ArgumentParser(description="Verificação HIL do monitor de tensão.")
    parser.add_argument("--port", required=True, help="ex.: /dev/ttyUSB0")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--seconds", type=float, default=12.0)
    args = parser.parse_args()

    try:
        import serial  # pyserial
    except ImportError:
        print("ERRO: pyserial ausente. Use PYTHONPATH apontando para o site-packages do PlatformIO.")
        return 1

    lines: list[tuple[float, str]] = []
    start = time.monotonic()

    with serial.Serial(args.port, args.baud, timeout=1) as port:
        # Libera DTR/RTS para o ESP32 rodar normalmente (evita segurar reset/bootloader).
        port.dtr = False
        port.rts = False
        time.sleep(0.2)
        port.reset_input_buffer()

        while time.monotonic() - start < args.seconds:
            raw = port.readline()
            if not raw:
                continue
            text = raw.decode("utf-8", errors="replace").rstrip("\r\n")
            lines.append((time.monotonic() - start, text))

    if not lines:
        print("RESULTADO: FAIL — nenhuma linha recebida")
        return 1

    volts: list[float] = []
    bad: list[tuple[float, str]] = []
    for stamp, text in lines:
        match = PATTERN.match(text)
        if match:
            volts.append(float(match.group(1)))
        else:
            bad.append((stamp, text))

    gaps = [b[0] - a[0] for a, b in zip(lines, lines[1:])]

    print(f"tempo observado : {args.seconds:.0f} s")
    print(f"linhas recebidas: {len(lines)}")
    print(f"linhas conformes: {len(volts)}")
    print(f"fora do contrato: {len(bad)}")
    for stamp, text in bad[:5]:
        print(f"   t={stamp:6.2f}s  {text!r}")

    if volts:
        print(
            f"tensão: min {min(volts):.2f} V | max {max(volts):.2f} V | "
            f"média {statistics.mean(volts):.2f} V"
        )
        print(f"variação pico-a-pico: {(max(volts) - min(volts)) * 1000:.0f} mV")
    if gaps:
        print(
            f"intervalo entre linhas: média {statistics.mean(gaps) * 1000:.0f} ms | "
            f"máx {max(gaps) * 1000:.0f} ms | mín {min(gaps) * 1000:.0f} ms"
        )

    print("primeiras linhas:")
    for stamp, text in lines[:3]:
        print(f"   t={stamp:6.2f}s  {text}")

    ok = not bad and len(volts) >= int(args.seconds * 0.8)
    print("RESULTADO:", "PASS" if ok else "FAIL")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
