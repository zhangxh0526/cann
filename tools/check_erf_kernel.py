import math
import pathlib
import re
import struct
import sys


ROOT = pathlib.Path(__file__).resolve().parents[1]
KERNEL = ROOT / "op_kernel" / "erf.cpp"

EXPECTED_NAMES = [
    "ERF_CLAMP",
    "ERF_C0",
    "ERF_C1",
    "ERF_C2",
    "ERF_C3",
    "ERF_C4",
    "ERF_C5",
    "ERF_C6",
    "ERF_C7",
    "ERF_C8",
]


def load_constants():
    text = KERNEL.read_text(encoding="utf-8")
    constants = {}
    for name in EXPECTED_NAMES:
        match = re.search(rf"\b{name}\s*=\s*([-+0-9.eEfF]+)", text)
        if not match:
            raise AssertionError(f"missing constant {name} in {KERNEL}")
        constants[name] = float(match.group(1).rstrip("fF"))
    return constants


def f32(value):
    return struct.unpack("f", struct.pack("f", float(value)))[0]


def erf_approx(x, c):
    clamped = f32(min(max(f32(x), f32(-c["ERF_CLAMP"])), f32(c["ERF_CLAMP"])))
    x2 = f32(clamped * clamped)
    poly = f32(c["ERF_C8"])
    for idx in range(7, -1, -1):
        poly = f32(f32(poly * x2) + c[f"ERF_C{idx}"])
    return f32(clamped * poly)


def check_accuracy():
    constants = load_constants()
    samples = []
    samples.extend([i / 100.0 for i in range(-1000, 1001)])
    samples.extend([i / 1000.0 for i in range(-100, 101)])
    samples.extend([-3.92, -3.0, -2.0, -1.0, -0.1, 0.0, 0.1, 1.0, 2.0, 3.0, 3.92])

    max_abs = 0.0
    max_rel = 0.0
    worst = 0.0
    for x in samples:
        got = erf_approx(x, constants)
        want = math.erf(x)
        abs_err = abs(got - want)
        rel_err = abs_err / max(abs(want), 1e-12)
        if abs_err > max_abs:
            max_abs = abs_err
            worst = x
        max_rel = max(max_rel, rel_err)

    if max_abs >= 1e-4 or max_rel >= 1e-4:
        raise AssertionError(f"accuracy failed: max_abs={max_abs:.8g}, max_rel={max_rel:.8g}, worst_x={worst}")


if __name__ == "__main__":
    try:
        check_accuracy()
    except Exception as exc:
        print(exc, file=sys.stderr)
        sys.exit(1)
    print("erf kernel constants pass sampled accuracy checks")
