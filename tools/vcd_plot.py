# Побудова графіків з wokwi.vcd (Wokwi Logic Analyzer) + декодер I2C.
# Використання: python3 tools/vcd_plot.py wokwi.vcd docs/screenshots
import sys, os, matplotlib; matplotlib.use('Agg'); import matplotlib.pyplot as plt
vcd, outdir = sys.argv[1], sys.argv[2]
ids = {'!': 'SCL', '"': 'SDA', '#': 'PWM', '$': 'TX'}
ev = {k: [] for k in ids}; t = 0
for l in open(vcd):
    l = l.strip()
    if l.startswith('#'): t = int(l[1:])
    elif len(l) == 2 and l[1] in ids: ev[l[1]].append((t, int(l[0])))

def level_at(sig, tt):
    v = None
    for a, b in ev[sig]:
        if a > tt: break
        v = b
    return v

def decode_i2c(t0, t1):
    """Повертає список (t, label) — S/Sr/P і байти з ACK/NACK."""
    allev = sorted([(a, 'C', v) for a, v in ev['!'] if t0 <= a <= t1] +
                   [(a, 'D', v) for a, v in ev['"'] if t0 <= a <= t1], key=lambda x: (x[0], x[1] == 'D'))
    scl = level_at('!', t0); sda = level_at('"', t0)
    out = []; bits = []; started = False; tb = None
    for a, k, v in allev:
        if k == 'D':
            if scl == 1 and sda == 1 and v == 0:
                out.append((a, 'Sr' if started else 'S')); started = True; bits = []
            elif scl == 1 and sda == 0 and v == 1:
                out.append((a, 'P')); started = False; bits = []
            sda = v
        else:
            if v == 1 and started:
                if not bits: tb = a
                bits.append(sda)
                if len(bits) == 9:
                    byte = int(''.join(map(str, bits[:8])), 2)
                    out.append((tb, '0x%02X %s' % (byte, 'ACK' if bits[8] == 0 else 'NACK'))); bits = []
            scl = v
    return out

def series(sig, t0, t1):
    xs, ys = [], []; last = level_at(sig, t0)
    xs.append(0); ys.append(last)
    for a, v in ev[sig]:
        if a < t0: continue
        if a > t1: break
        xs += [(a - t0) / 1e3] * 2; ys += [last, v]; last = v
    xs.append((t1 - t0) / 1e3); ys.append(last)
    return xs, ys

# --- кадр WHO_AM_I: перший програмний кадр (SCL-імпульси 20 мкс..2 мс) після 1.3 с
scl = ev['!']
t_i2c = next(a for (a, va), (b, vb) in zip(scl, scl[1:]) if va == 1 and 20e3 < b - a < 2e6 and a > 1.3e9)
t0 = t_i2c - 300e3; t1 = t0 + 9e6
dec = decode_i2c(t0, t1)
print('I2C decode:', ' | '.join(l for _, l in dec))
fig, ax = plt.subplots(figsize=(14, 4.2))
for i, (sig, name) in enumerate([('!', 'D0  SCL (PB8)'), ('"', 'D1  SDA (PB9)')]):
    x, y = series(sig, t0, t1); base = 2.6 - i * 1.6
    ax.plot(x, [base + v for v in y], drawstyle='default', lw=1.2, color=['#1f6feb', '#d97706'][i])
    ax.text(-90, base + 0.4, name, ha='right', va='center', fontsize=10)
colors = {'S': '#16a34a', 'Sr': '#16a34a', 'P': '#7c3aed'}
for a, lab in dec:
    x = (a - t0) / 1e3
    c = colors.get(lab, '#0891b2' if 'ACK' in lab and 'NACK' not in lab else '#dc2626')
    ax.annotate(lab, (x, -0.35), fontsize=9, color='white', ha='left', va='center',
                bbox=dict(boxstyle='round,pad=0.25', fc=c, ec='none'))
ax.set_xlim(-10, (t1 - t0) / 1e3); ax.set_ylim(-0.9, 3.9); ax.set_yticks([])
ax.set_xlabel('мкс від початку вікна (t0 = %.3f мс симуляції)' % (t0 / 1e6))
ax.set_title('I²C: читання WHO_AM_I з MPU-6050 — wokwi.vcd, декодовано tools/vcd_plot.py')
ax.grid(axis='x', alpha=.3); plt.tight_layout()
plt.savefig(os.path.join(outdir, 'logic-i2c-whoami.png'), dpi=120); plt.close()

# --- PWM на D12: 3 періоди + виміряна ширина
pw = ev['#']
widths = sorted(set(round((b - a) / 1e3) for (a, va), (b, vb) in zip(pw, pw[1:]) if va == 1))
periods = [round((c - a) / 1e3) for (a, va), _, (c, vc) in zip(pw, pw[1:], pw[2:]) if va == 1 and vc == 1]
p0 = pw[len(pw) // 2][0] - 2e6; p1 = p0 + 60e6
fig, ax = plt.subplots(figsize=(14, 2.8))
x, y = series('#', p0, p1); ax.plot([v / 1e3 for v in x], y, color='#dc2626')
ax.set_yticks([0, 1]); ax.set_xlabel('мс'); ax.grid(alpha=.3)
ax.set_title('D2 = D12 (PA6, TIM3_CH1) PWM на серво pan: період %d мкс, ширини імпульсів у записі: %s мкс'
             % (max(set(periods), key=periods.count), ', '.join(map(str, widths))))
plt.tight_layout(); plt.savefig(os.path.join(outdir, 'logic-pwm.png'), dpi=120); plt.close()
print('PWM widths', widths, 'period', max(set(periods), key=periods.count))
