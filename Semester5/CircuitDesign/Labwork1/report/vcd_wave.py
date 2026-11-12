"""Render a window of a VCD file as a black-on-white SVG timing diagram.

Usage: python3 vcd_wave.py VCD OUT.svg T0 T1 SIGNAL[=LABEL] ...
T0 and T1 are in ns, SIGNAL is the full hierarchical name (tb_boe.d).
Only the standard library is used.
"""
import sys
from xml.sax.saxutils import escape


def parse_vcd(path):
    ids, widths, changes = {}, {}, {}
    scope, t, scale = [], 0, 1.0
    tokens = open(path).read().split()
    i = 0
    while i < len(tokens):
        tok = tokens[i]
        if tok == "$timescale":
            j = tokens.index("$end", i)
            unit = "".join(tokens[i + 1:j])
            num = "".join(c for c in unit if c.isdigit())
            mult = {"s": 1e9, "ms": 1e6, "us": 1e3, "ns": 1, "ps": 1e-3, "fs": 1e-6}
            scale = int(num) * mult[unit[len(num):]]
            i = j
        elif tok == "$scope":
            scope.append(tokens[i + 2])
            i += 3
        elif tok == "$upscope":
            scope.pop()
        elif tok == "$var":
            width, code, name = int(tokens[i + 2]), tokens[i + 3], tokens[i + 4]
            full = ".".join(scope + [name])
            ids.setdefault(code, []).append(full)
            widths[full] = width
            changes.setdefault(code, [])
            i = tokens.index("$end", i)
        elif tok.startswith("#"):
            t = int(tok[1:]) * scale
        elif tok.startswith("$"):
            if tok in ("$dumpvars", "$end", "$dumpall", "$dumpon", "$dumpoff"):
                pass
            else:
                i = tokens.index("$end", i)
        elif tok[0] in "bBrR":
            changes[tokens[i + 1]].append((t, tok[1:]))
            i += 1
        elif tok[0] in "01xXzZ":
            changes[tok[1:]].append((t, tok[0]))
        i += 1
    signals = {}
    for code, names in ids.items():
        for name in names:
            signals[name] = (widths[name], changes[code])
    return signals


def value_at(trace, t):
    v = "x"
    for tc, val in trace:
        if tc > t:
            break
        v = val
    return v


def fmt(val, width):
    if any(c in "xXzZ" for c in val):
        return "x"
    return format(int(val, 2), "0%dX" % ((width + 3) // 4))


def render(signals, t0, t1, rows, out):
    label_w, row_h, amp, plot_w = 125, 36, 22, 620
    width = label_w + plot_w + 40
    height = row_h * len(rows) + 40
    x = lambda t: label_w + (t - t0) / (t1 - t0) * plot_w
    svg = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" '
           f'viewBox="0 0 {width} {height}" font-family="DejaVu Sans Mono, monospace" font-size="16">',
           f'<rect width="{width}" height="{height}" fill="white"/>']
    step = 10 if t1 - t0 <= 120 else 20
    for tg in range(int(t0 // step * step), int(t1) + 1, step):
        if tg < t0:
            continue
        svg.append(f'<line x1="{x(tg):.1f}" y1="8" x2="{x(tg):.1f}" y2="{height - 26}" '
                   'stroke="#bbb" stroke-width="0.6" stroke-dasharray="3,3"/>')
        svg.append(f'<text x="{x(tg):.1f}" y="{height - 10}" text-anchor="middle" fill="black">{tg:g}</text>')
    svg.append(f'<text x="{width - 6}" y="{height - 10}" text-anchor="end" fill="black">нс</text>')
    for r, (name, label) in enumerate(rows):
        width_bits, trace = signals[name]
        base = 14 + r * row_h + amp
        top = base - amp
        svg.append(f'<text x="6" y="{base - amp / 2 + 6}" fill="black">{escape(label)}</text>')
        points = [(t0, value_at(trace, t0))] + [(tc, v) for tc, v in trace if t0 < tc < t1]
        for k, (ts, v) in enumerate(points):
            te = points[k + 1][0] if k + 1 < len(points) else t1
            xs, xe = x(ts), x(te)
            if width_bits == 1:
                y = top if v == "1" else base
                color = "black" if v in "01" else "#c00"
                svg.append(f'<line x1="{xs:.1f}" y1="{y}" x2="{xe:.1f}" y2="{y}" stroke="{color}" stroke-width="1.6"/>')
                if k + 1 < len(points):
                    svg.append(f'<line x1="{xe:.1f}" y1="{top}" x2="{xe:.1f}" y2="{base}" stroke="black" stroke-width="1.6"/>')
            else:
                e = min(3, (xe - xs) / 2)
                mid = (top + base) / 2
                svg.append(f'<polygon points="{xs:.1f},{mid} {xs + e:.1f},{top} {xe - e:.1f},{top} '
                           f'{xe:.1f},{mid} {xe - e:.1f},{base} {xs + e:.1f},{base}" '
                           'fill="none" stroke="black" stroke-width="1.3"/>')
                if xe - xs > 22:
                    svg.append(f'<text x="{(xs + xe) / 2:.1f}" y="{mid + 5.5}" text-anchor="middle" '
                               f'fill="black">{fmt(v, width_bits)}</text>')
    svg.append("</svg>")
    with open(out, "w") as f:
        f.write("\n".join(svg) + "\n")


def main():
    vcd, out, t0, t1, *specs = sys.argv[1:]
    signals = parse_vcd(vcd)
    rows = []
    for spec in specs:
        name, _, label = spec.partition("=")
        rows.append((name, label or name.split(".")[-1]))
    render(signals, float(t0), float(t1), rows, out)


if __name__ == "__main__":
    main()
