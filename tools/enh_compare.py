#!/usr/bin/env python3
"""Headless geometry acceptance of the enhanced view (ENHANCED.md §3.10 item 3, §5): SR_ENH_COMPARE=2.

    python tools/enh_compare.py [--exe EXE] [--min PCT] [--dump] [--runs drag,cruise]

Runs the demo drag race (road mode 1) with the drive home (town, mode 0), and the cruise to Burger's (town), with the
compare clock and the compare tool at the default resolution (1280x800, 2x2) and --motion-delay 0. For every race
frame the tool renders the frame's snapshot (player position quantized as the original's row depths), takes the centre
sample of each logical pixel, lays the original's passthrough pixels (opponent, mirror) over it and matches the colour
edges against the original's page: an edge is matched when the enhanced picture has the same edge within +-1 px (or,
next to a textured background, the boundaries of both colours within +-1 px). Both directions (original edges found in
the enhanced picture and the other way round), horizontal and vertical edges, rows from the top of the ground to the
split. Since S2.5 the road-side objects are the enhanced renderer's (shapes, hydrants, sign pictures, banners): every
call the original makes in front_objects must be matched by an enhanced call of the same kind, type, row depth and
side with its anchor within +-1 px (and the other way round), and the continuous size at the original's row depth
must be within half a LOD step + 0.5 px of the original's LOD.
Prints the matched shares per race and mode; exit code 0 when every edge share is at least --min (default 96 %) and
every object call and size check matches.
--dump writes cmpNNNNN.bmp (original / enhanced / difference) every 25 frames to work/ec/<run>/cmp.
"""
import argparse, os, re, shutil, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ENHANCED = os.path.join(ROOT, "build", "sr2-enhanced.exe")
MODES = {0: "town", 1: "drag"}
RUNS = {"drag": "drag", "cruise": "cruise"}       # run name: SR_DEBUG_RACE


def run(a, name, base):
    game, log, cmp = os.path.join(base, "game"), os.path.join(base, name, "compare.log"), os.path.join(base, name, "cmp")
    os.makedirs(cmp)
    env = dict(os.environ, SDL_VIDEO_DRIVER="dummy", SDL_AUDIO_DRIVER="dummy", SR_SEED="1", SR_KEYS="1:03",
               SR_DEBUG_RACE=RUNS[name], SR_ENH_COMPARE="2", SR_ENH_COMPARE_LOG=log)
    if a.dump:
        env["SR_ENH_COMPARE_DIR"] = cmp
    subprocess.run([a.exe, "--game-dir", game, "--run-ms", "60000", "--motion-delay", "0", "demo", "v"], env=env,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=600)
    mode, ok, races = None, True, 0
    for line in open(log):
        m = re.match(r"K \d+ (\d+) ", line)
        if m:
            mode = int(m.group(1))
        m = re.match(r"K race frames (\d+)\s+h (\d+)/(\d+) (\d+)/(\d+)\s+v (\d+)/(\d+) (\d+)/(\d+)\s+worst frame (\d+) ([\d.]+)%",
                     line)
        if m:
            races += 1
            n = [int(x) for x in m.groups()[:9]]
            shares = [100.0 * n[i] / max(n[i + 1], 1) for i in (1, 3, 5, 7)]
            good = min(shares) >= a.min
            ok &= good
            print("%-6s %-5s %-4s %4d frames  h %.2f%% / %.2f%%  v %.2f%% / %.2f%%  (original in enhanced / enhanced in "
                  "original)  worst frame %s %.2f%%" % (name, MODES.get(mode, mode), "ok" if good else "FAIL", n[0],
                                                       *shares, m.group(10), float(m.group(11))))
            continue
        m = re.match(r"O race frames \d+\s+calls (\d+)/(\d+) (\d+)/(\d+)\s+size (\d+)/(\d+) max err ([\d.]+) px\s+(.*)", line)
        if m:
            n = [int(x) for x in m.groups()[:6]]
            good = n[0] == n[1] and n[2] == n[3] and n[4] == n[5]
            ok &= good
            print("%-6s %-5s %-4s objects: original calls matched %d/%d, enhanced %d/%d, sizes %d/%d (max %s px) %s"
                  % (name, MODES.get(mode, mode), "ok" if good else "FAIL", *n, m.group(7), m.group(8)))
    if not races:
        print("FAIL: no compared race in", log)
    return ok and races > 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--exe", default=ENHANCED)
    ap.add_argument("--min", type=float, default=96.0)
    ap.add_argument("--dump", action="store_true")
    ap.add_argument("--runs", default="drag,cruise")
    a = ap.parse_args()
    os.environ["PATH"] = r"C:\msys64\mingw64\bin" + os.pathsep + os.environ.get("PATH", "")
    base = os.path.join(ROOT, "work", "ec")
    shutil.rmtree(base, ignore_errors=True)
    shutil.copytree(os.path.join(ROOT, "Game"), os.path.join(base, "game"), ignore=shutil.ignore_patterns("*.exo"))
    ok = True
    for name in a.runs.split(","):
        ok &= run(a, name, base)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
