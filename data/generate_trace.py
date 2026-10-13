#!/usr/bin/env python3
"""Deterministic generator of data/input_trace.csv (Why_Am_I_Lagging, test task 02).

Python standard library only. Same SEED -> byte-identical file.

    python3 -I generate_trace.py              # (re)writes input_trace.csv next to this script
    python3 -I generate_trace.py --check      # verifies the file on disk is identical
    python3 -I generate_trace.py --seed 7 --ticks 3000 --out /tmp/my_trace.csv   # extra traces for your own tests

Model:
  * 4 players, server tick = 60 Hz (TICK_US = 16667 us), ticks 0 .. N-1;
  * the client of player p samples its input for tick t and sends it at send_us = t * TICK_US
    (clients are assumed to be tick-aligned with the server; this is a simplification);
  * one-way delay client -> server is drawn per packet from the player's delay profile;
    arrival_us = send_us + delay_us; packets of one player MAY be reordered (UDP-like); no losses
    (inputs are assumed to be sent redundantly, so every input eventually arrives);
  * arrival_tick = the first server tick T with T * TICK_US >= arrival_us, i.e. the input can be
    used when the server starts processing tick T.
"""
import argparse, hashlib, io, math, os, random, sys

SEED = 20261009
TICK_US = 16667
N_TICKS = 10000
UP, DOWN, LEFT, RIGHT, DASH = 1, 2, 4, 8, 16

# delay profiles: base_ms, jitter_sd_ms, spike_p (per packet), spike_ms, burst_p (start prob per tick), burst_ms, burst_len
PROFILES = [
    dict(name="lan",       base=8.0,  sd=1.5,  spike_p=0.000, spike=0,   burst_p=0.0,    burst=0,   burst_len=(0, 0)),
    dict(name="wan_good",  base=24.0, sd=4.0,  spike_p=0.002, spike=40,  burst_p=0.0,    burst=0,   burst_len=(0, 0)),
    dict(name="wan_jitter",base=38.0, sd=11.0, spike_p=0.010, spike=70,  burst_p=0.0,    burst=0,   burst_len=(0, 0)),
    dict(name="lagging",   base=85.0, sd=20.0, spike_p=0.010, spike=90,  burst_p=0.0012, burst=150, burst_len=(30, 120)),
]
# input behaviour: median hold length (ticks) of a movement intent; dash probability per tick
BEHAVIOUR = [
    dict(hold=45, sigma=0.6, dash_p=0.004),
    dict(hold=30, sigma=0.7, dash_p=0.008),
    dict(hold=60, sigma=0.5, dash_p=0.002),
    dict(hold=25, sigma=0.8, dash_p=0.010),
]
INTENTS = [0, UP, DOWN, LEFT, RIGHT, UP | LEFT, UP | RIGHT, DOWN | LEFT, DOWN | RIGHT]
INTENT_W = [0.16, 0.16, 0.10, 0.12, 0.12, 0.09, 0.09, 0.08, 0.08]


def gen_inputs(rng, beh, n):
    out, cur, left, dash_left = [], 0, 0, 0
    for _ in range(n):
        if left <= 0:
            cur = rng.choices(INTENTS, weights=INTENT_W)[0]
            left = max(2, int(round(beh["hold"] * math.exp(rng.gauss(0, beh["sigma"])))))
        left -= 1
        b = cur
        if dash_left > 0:
            dash_left -= 1
            b |= DASH
        elif cur and rng.random() < beh["dash_p"]:
            dash_left = rng.randint(2, 5) - 1
            b |= DASH
        out.append(b)
    return out


def gen_delays(rng, prof, n):
    out, burst_left = [], 0
    for _ in range(n):
        d = max(1.0, rng.gauss(prof["base"], prof["sd"]))
        if prof["spike_p"] and rng.random() < prof["spike_p"]:
            d += rng.uniform(0.5, 1.5) * prof["spike"]
        if burst_left > 0:
            burst_left -= 1
            d += prof["burst"] * rng.uniform(0.8, 1.2)
        elif prof["burst_p"] and rng.random() < prof["burst_p"]:
            burst_left = rng.randint(*prof["burst_len"])
        out.append(int(round(d * 1000)))
    return out


def generate(seed, n_ticks):
    buf = io.StringIO()
    buf.write("tick,player,buttons,send_us,delay_us,arrival_us,arrival_tick\n")
    cols = []
    for p in range(4):
        inputs = gen_inputs(random.Random("%d/in/%d" % (seed, p)), BEHAVIOUR[p], n_ticks)
        delays = gen_delays(random.Random("%d/net/%d" % (seed, p)), PROFILES[p], n_ticks)
        cols.append((inputs, delays))
    for t in range(n_ticks):
        for p in range(4):
            b, d = cols[p][0][t], cols[p][1][t]
            send = t * TICK_US
            arr = send + d
            arr_tick = -(-arr // TICK_US)
            buf.write("%d,%d,%d,%d,%d,%d,%d\n" % (t, p, b, send, d, arr, arr_tick))
    return buf.getvalue().encode("ascii")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--seed", type=int, default=SEED)
    ap.add_argument("--ticks", type=int, default=N_TICKS)
    ap.add_argument("--out", default=os.path.join(os.path.dirname(os.path.abspath(__file__)), "input_trace.csv"))
    ap.add_argument("--check", action="store_true")
    a = ap.parse_args()
    data = generate(a.seed, a.ticks)
    digest = hashlib.sha256(data).hexdigest()
    if a.check:
        with open(a.out, "rb") as f:
            same = hashlib.sha256(f.read()).hexdigest() == digest
        print("%s %s" % (a.out, "OK" if same else "MISMATCH"))
        return 0 if same else 1
    with open(a.out, "wb") as f:
        f.write(data)
    print("wrote %s: %d bytes, sha256=%s" % (a.out, len(data), digest))
    return 0


if __name__ == "__main__":
    sys.exit(main())
