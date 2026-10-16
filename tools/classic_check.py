#!/usr/bin/env python3
"""--classic identity check: the same headless runs on the faithful release and on sr2-enhanced --classic.

    python tools/classic_check.py [--faithful EXE] [--enhanced EXE] [path ...]

Each path runs on both executables (SDL dummy drivers, own copy of Game/ under work/cc/, SR_SEED=1, the demo autopilot),
with snapshots every SR_SNAPSHOT_MS. The run timing is wall-clock, so the two runs need not snapshot the same moments; a
path is "ok" when every distinct frame of the classic run is also a frame of the faithful run (and the other way round,
reported as coverage). Static screens (title, menus) and race frames of the deterministic demo autopilot match exactly
when the picture is the same. With --scaled N the Enhanced defaults run too (output 320*N x 200*N); their frames,
sampled back to 320x200, must be faithful frames as long as the picture is the scaled EGA one (S2.1).
With --clock the deterministic host clock (SR_ENH_COMPARE=1, ENHANCED.md §5) is checked instead: two --classic runs
and one Enhanced run (--res-scale 4) of each path must give the same frames in the same order (the shorter run is a
prefix: --run-ms stays wall-clock while the compare clock runs faster than real time).
Exit code 0 only if every path is ok.
"""
import argparse, hashlib, os, shutil, struct, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FAITHFUL = os.path.join(ROOT, "..", "StreetRod2", "release", "sr2port-v0.1.0-win64", "sr2port.exe")
ENHANCED = os.path.join(ROOT, "build", "sr2-enhanced.exe")

# name: (args, env, SR_KEYS, run seconds)
PATHS = {
    "title": (["v"], {}, "1:03,25:1e,26:1c", 34),
    "race_drag": (["demo", "v"], {"SR_DEBUG_RACE": "drag"}, "1:03", 45),
}


def bmp_320(data, n):
    """Pixels of a bottom-up 24/32-bit BMP, every n-th pixel of every n-th row, as bytes (top row first)."""
    off, = struct.unpack_from("<I", data, 10)
    w, h, _, bpp = struct.unpack_from("<iiHH", data, 18)
    bpr, step = (w * bpp // 8 + 3) & ~3, bpp // 8
    rows = []
    n = max(1, abs(h) // 200)                   # the text mode (640x200) is never scaled
    for y in range(0, abs(h), n):
        r = off + ((abs(h) - 1 - y) if h > 0 else y) * bpr
        rows.append(b"".join(data[r + x * step:r + x * step + 3] for x in range(0, w, n)))
    return b"".join(rows)


def run(exe, extra, name, tag, n=1, env_extra=None, ordered=False):
    args, env_add, keys, secs = PATHS[name]
    base = os.path.join(ROOT, "work", "cc", name, tag)
    shutil.rmtree(base, ignore_errors=True)
    snap, game = os.path.join(base, "snap"), os.path.join(base, "game")
    os.makedirs(snap)
    shutil.copytree(os.path.join(ROOT, "Game"), game, ignore=shutil.ignore_patterns("*.exo"))
    env = dict(os.environ, SDL_VIDEO_DRIVER="dummy", SDL_AUDIO_DRIVER="dummy", SR_SEED="1",
               SR_SNAPSHOT_DIR=snap, SR_SNAPSHOT_MS="250", SR_KEYS=keys)
    env.update(env_add)
    env.update(env_extra or {})
    subprocess.run([exe, "--game-dir", game, "--run-ms", str(secs * 1000)] + extra + args, env=env,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=secs + 60)
    frames, seq = {}, []
    for f in sorted(os.listdir(snap)):
        if f.endswith(".bmp"):
            with open(os.path.join(snap, f), "rb") as fh:
                h = hashlib.sha1(bmp_320(fh.read(), n)).hexdigest()
            frames.setdefault(h, f)
            seq.append(h)
    return seq if ordered else frames


def check_clock(a, name):
    """SR_ENH_COMPARE=1: classic, classic again and the scaled Enhanced run give one frame sequence."""
    env = {"SR_ENH_COMPARE": "1"}
    runs = [run(a.enhanced, ["--classic"], name, "clock_c1", 1, env, True),
            run(a.enhanced, ["--classic"], name, "clock_c2", 1, env, True),
            run(a.enhanced, ["--res-scale", "4"], name, "clock_e4", 4, env, True)]
    n = min(len(r) for r in runs)
    same = [r[:n] == runs[0][:n] for r in runs]
    good = n > 0 and all(same)
    print("%-10s %-4s compare clock: %d snapshots in common (runs %s), classic repeat %s, enhanced %s"
          % (name, "ok" if good else "FAIL", n, "/".join(str(len(r)) for r in runs),
             "same" if same[1] else "DIFFERS", "same" if same[2] else "DIFFERS"))
    return good


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("paths", nargs="*")
    ap.add_argument("--faithful", default=FAITHFUL)
    ap.add_argument("--enhanced", default=ENHANCED)
    ap.add_argument("--scaled", type=int, default=0, help="also check the Enhanced defaults at --res-scale N")
    ap.add_argument("--clock", action="store_true", help="check the deterministic compare clock instead")
    a = ap.parse_args()
    os.environ["PATH"] = r"C:\msys64\mingw64\bin" + os.pathsep + os.environ.get("PATH", "")
    if a.clock:
        return 0 if all([check_clock(a, name) for name in a.paths or list(PATHS)]) else 1
    ok = True
    for name in a.paths or list(PATHS):
        f = run(a.faithful, [], name, "faithful")
        c = run(a.enhanced, ["--classic"], name, "classic")
        missing = [c[h] for h in c if h not in f]
        good = bool(c) and not missing
        ok &= good
        print("%-10s %-4s classic %d distinct frames, %d not in the faithful run%s; faithful %d distinct, %d shared"
              % (name, "ok" if good else "FAIL", len(c), len(missing),
                 (" (" + ", ".join(missing[:5]) + ")") if missing else "", len(f), len(set(f) & set(c))))
        if a.scaled:
            e = run(a.enhanced, ["--res-scale", str(a.scaled)], name, "scaled", a.scaled)
            missing = [e[h] for h in e if h not in f]
            ok &= bool(e) and not missing
            print("%-10s %-4s scaled x%d %d distinct frames, %d not in the faithful run%s"
                  % (name, "ok" if e and not missing else "FAIL", a.scaled, len(e), len(missing),
                     (" (" + ", ".join(missing[:5]) + ")") if missing else ""))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
