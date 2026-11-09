"""Расчёт точек ВАХ и сопротивлений при Io по SPICE-модели UMN11N из standard.dio LTspice.

Модель: .model UMN11N D(Is=936.79E-12 N=1.7211 Rs=1.0196 Ikf=28.904 Cjo=1.7934E-12
M=.18986 Vj=.86529 Isr=2.0505E-9 Nr=2.4000 Bv=80 Tt=37.000E-9 Iave=0.1 Vpk=80)
Уравнения статической ВАХ — как в LTspice (T = 27 °C); результат сверен со скриншотами DC sweep.
"""
import json, math, pathlib

IS, N, RS, IKF = 936.79e-12, 1.7211, 1.0196, 28.904
ISR, NR, M, VJ = 2.0505e-9, 2.4, 0.18986, 0.86529
VT = 1.380649e-23 * 300.15 / 1.602176634e-19
IO = 0.1  # Iave = Io по даташиту, А
OUT = pathlib.Path(__file__).parent / "results"; OUT.mkdir(exist_ok=True)

def i_junction(vd):
    inor = IS * (math.exp(vd / (N * VT)) - 1)
    kinj = math.sqrt(IKF / (IKF + inor)) if inor > 0 else 1.0
    irec = ISR * (math.exp(vd / (NR * VT)) - 1)
    kgen = ((1 - vd / VJ) ** 2 + 0.005) ** (M / 2)
    return inor * kinj + irec * kgen

def current(v):
    """Ток диода при напряжении v на выводах (с учётом Rs), бисекция по Vd."""
    lo, hi = min(v, 0) - 1, max(v, 0) + 1e-12
    for _ in range(200):
        vd = (lo + hi) / 2
        if vd + i_junction(vd) * RS > v: hi = vd
        else: lo = vd
    return i_junction((lo + hi) / 2)

def voltage_at(i):
    lo, hi = 0.0, 2.0
    for _ in range(200):
        mid = (lo + hi) / 2
        if current(mid) > i: hi = mid
        else: lo = mid
    return (lo + hi) / 2

# --- ВАХ, сопротивления при Io
v_io = voltage_at(IO)
h = 1e-5
r_dyn = 2 * h / (current(v_io + h) - current(v_io - h))
r_st = v_io / IO
v_th_tangent = v_io - IO * r_dyn  # отсечка касательной в рабочей точке
v_1ma = voltage_at(1e-3)
fwd = {v: current(v) for v in (0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 1.1, 1.2)}
rev = {v: current(-v) for v in (0.1, 0.5, 1.0)}

res = dict(
    v_io=v_io, r_st=r_st, r_dyn=r_dyn, v_th=v_th_tangent, v_1ma=v_1ma, i_12=current(1.2),
    fwd={str(k): v for k, v in fwd.items()}, rev={str(k): v for k, v in rev.items()},
)
(OUT / "data.json").write_text(json.dumps(res, indent=2, ensure_ascii=False))
print(json.dumps(res, indent=2))
