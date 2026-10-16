#!/usr/bin/env python3
"""Headless geometry acceptance of the enhanced view (ENHANCED.md §3.10 item 3, §5): SR_ENH_COMPARE=2.

    python tools/enh_compare.py [--exe EXE] [--min PCT] [--mirror-min PCT] [--dump] [--runs drag,cruise] [--scale X]

Runs the demo drag race (road mode 1) with the drive home (town, mode 0), and the cruise to Burger's (town), with the
compare clock and the compare tool at the default resolution (1280x800, 2x2) and --motion-delay 0. For every race
frame the tool renders the frame's snapshot (player position quantized as the original's row depths), takes the centre
sample of each logical pixel, lays the original's passthrough pixels (opponent, starter) over it and matches the colour
edges against the original's page: an edge is matched when the enhanced picture has the same edge within +-1 px (or,
next to a textured background, the boundaries of both colours within +-1 px). Both directions (original edges found in
the enhanced picture and the other way round), horizontal and vertical edges, rows from the top of the ground to the
split. Since S2.5 the road-side objects are the enhanced renderer's (shapes, hydrants, sign pictures, banners): every
call the original makes in front_objects, and since S2.6 in the mirror (its banner and mirror_objects), must be matched
by an enhanced call of the same kind, type, row depth and side with its anchor within +-1 px (and the other way
round). Sizes: the enhanced view draws every object from its largest LOD only, scaled by true perspective (largest LOD
l, made for row depth 9 - l, i.e. z = 8 (9 - l) + 0.5; size x zt / z), not the original's LOD steps; the compare
view still draws the original's LOD (for the edge match), and for every shape (hydrants and banner text included) the
height of the spans the enhanced view would emit at the original's row depth must equal the largest LOD's height x
zt / z within 0.5 px.
Mirror (S2.6): the enhanced mirror (backdrop pictures, road, walls, objects, frame lines; the opponent and the
starter are the original's pixels) is edge-matched the same way inside its rectangle (rows 18..44, x 240..319), with
the original's mirror depths (the k >= 5 step) and banks read forward as the original; the share must be at least
--mirror-min (default 95 %): the original draws each mirror row's spans with the x of the row's top, so its nearest
rows (bottom of the mirror, the town's sidewalks and walls) step by up to 15 px per row where the continuous view has
straight edges, beyond the +-1 px tolerance (cruise 95.6-98.8 %, drag 99.1-99.7 %).
Prints the matched shares per race and mode; exit code 0 when every edge share is at least --min (default 96 %) and
every object call and size check matches.
Draw distance (S2.9): the checks above run at --draw-distance-scale 1 (the original's 14 segments). Each run is then
repeated at --scale (default 1.25, 0 = skip): there the compare tool renders the front segments seg + 1 .. seg + 14
alone and the whole view, and every sample of the near segments must be unchanged in the whole view (the extension is
drawn first, far to near, and never in front of them), and the extension must show (samples > 0); the edge and object
shares of that run are printed for information only.
--dump writes cmpNNNNN.bmp (original / enhanced / difference) every 25 frames to work/ec/<run>/cmp.
"""
import argparse, os, re, shutil, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ENHANCED = os.path.join(ROOT, "build", "sr2-enhanced.exe")
MODES = {0: "town", 1: "drag"}
RUNS = {"drag": "drag", "cruise": "cruise"}       # run name: SR_DEBUG_RACE


def run(a, name, base, scale=1.0):
    tag = name if scale == 1.0 else "%s_x%g" % (name, scale)
    game, log, cmp = os.path.join(base, "game"), os.path.join(base, tag, "compare.log"), os.path.join(base, tag, "cmp")
    os.makedirs(cmp)
    env = dict(os.environ, SDL_VIDEO_DRIVER="dummy", SDL_AUDIO_DRIVER="dummy", SR_SEED="1", SR_KEYS="1:03",
               SR_DEBUG_RACE=RUNS[name], SR_ENH_COMPARE="2", SR_ENH_COMPARE_LOG=log)
    if a.dump:
        env["SR_ENH_COMPARE_DIR"] = cmp
    subprocess.run([a.exe, "--game-dir", game, "--run-ms", "60000", "--motion-delay", "0",
                    "--draw-distance-scale", str(scale), "demo", "v"], env=env,
                   stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=600)
    mode, ok, races, gate = None, True, 0, scale == 1.0
    name = tag
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
            good = min(shares) >= a.min or not gate
            ok &= good
            print("%-6s %-5s %-4s %4d frames  h %.2f%% / %.2f%%  v %.2f%% / %.2f%%  (original in enhanced / enhanced in "
                  "original)  worst frame %s %.2f%%" % (name, MODES.get(mode, mode), "ok" if good else "FAIL", n[0],
                                                       *shares, m.group(10), float(m.group(11))))
            continue
        m = re.match(r"M race frames (\d+)\s+h (\d+)/(\d+) (\d+)/(\d+)\s+v (\d+)/(\d+) (\d+)/(\d+)", line)
        if m:
            n = [int(x) for x in m.groups()]
            shares = [100.0 * n[i] / max(n[i + 1], 1) for i in (1, 3, 5, 7)]
            good = min(shares) >= a.mirror_min or not gate
            ok &= good
            print("%-6s %-5s %-4s mirror:  h %.2f%% / %.2f%%  v %.2f%% / %.2f%%" % (name, MODES.get(mode, mode),
                                                                             "ok" if good else "FAIL", *shares))
            continue
        m = re.match(r"O race frames \d+\s+calls (\d+)/(\d+) (\d+)/(\d+)\s+size (\d+)/(\d+) max err ([\d.]+) px\s+(.*)", line)
        if m:
            n = [int(x) for x in m.groups()[:6]]
            good = n[0] == n[1] and n[2] == n[3] and n[4] == n[5] or not gate
            ok &= good
            print("%-6s %-5s %-4s objects: original calls matched %d/%d, enhanced %d/%d, sizes %d/%d (max %s px) %s"
                  % (name, MODES.get(mode, mode), "ok" if good else "FAIL", *n, m.group(7), m.group(8)))
        m = re.match(r"D race frames (\d+)\s+near changed (\d+)/(\d+)\s+extension (\d+)", line)
        if m:
            n = [int(x) for x in m.groups()]
            good = n[1] == 0 and n[2] > 0 and n[3] > 0
            ok &= good
            print("%-6s %-5s %-4s draw distance x%g: near segments changed %d of %d samples, extension %d samples"
                  % (name, MODES.get(mode, mode), "ok" if good else "FAIL", scale, n[1], n[2], n[3]))
    if not races:
        print("FAIL: no compared race in", log)
    return ok and races > 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--exe", default=ENHANCED)
    ap.add_argument("--min", type=float, default=96.0)
    ap.add_argument("--mirror-min", type=float, default=95.0)
    ap.add_argument("--dump", action="store_true")
    ap.add_argument("--runs", default="drag,cruise")
    ap.add_argument("--scale", type=float, default=1.25)
    a = ap.parse_args()
    os.environ["PATH"] = r"C:\msys64\mingw64\bin" + os.pathsep + os.environ.get("PATH", "")
    base = os.path.join(ROOT, "work", "ec")
    shutil.rmtree(base, ignore_errors=True)
    shutil.copytree(os.path.join(ROOT, "Game"), os.path.join(base, "game"), ignore=shutil.ignore_patterns("*.exo"))
    ok = True
    for name in a.runs.split(","):
        ok &= run(a, name, base)
        if a.scale and a.scale != 1.0:
            ok &= run(a, name, base, a.scale)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
