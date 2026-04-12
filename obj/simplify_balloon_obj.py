#!/usr/bin/env python3
import math
from pathlib import Path

SRC = Path('/Users/timo/opengl/oldflight/obj/unpacked/Hot_air_balloon_v1_L2.123c69a97f0e-9977-45dd-9570-457189ce2941/11809_Hot_air_balloon_l2.obj')
OUT = Path('/Users/timo/opengl/oldflight/obj/balloon-lowpoly.obj')
GRID = 60.0  # a bit finer so the hull stops looking like a hostile pineapple

verts = []
faces = []
current_mtl = None
current_group = None

with SRC.open('r', errors='ignore') as f:
    for line in f:
        if line.startswith('v '):
            _, x, y, z = line.split()[:4]
            verts.append((float(x), float(y), float(z)))
        elif line.startswith('usemtl '):
            current_mtl = line.split(None, 1)[1].strip()
        elif line.startswith('g '):
            current_group = line.split(None, 1)[1].strip()
        elif line.startswith('f '):
            toks = line.split()[1:]
            idx = []
            for tok in toks:
                v = tok.split('/')[0]
                idx.append(int(v) - 1)
            if len(idx) >= 3:
                for i in range(1, len(idx) - 1):
                    faces.append((idx[0], idx[i], idx[i+1], current_mtl, current_group))

# vertex clustering
cluster_to_new = {}
new_verts = []
old_to_new = {}
for i, (x, y, z) in enumerate(verts):
    key = (round(x / GRID), round(y / GRID), round(z / GRID))
    ni = cluster_to_new.get(key)
    if ni is None:
        ni = len(new_verts)
        cluster_to_new[key] = ni
        new_verts.append((key[0] * GRID, key[1] * GRID, key[2] * GRID))
    old_to_new[i] = ni

new_faces = []
seen = set()
for a, b, c, mtl, grp in faces:
    na, nb, nc = old_to_new[a], old_to_new[b], old_to_new[c]
    if len({na, nb, nc}) < 3:
        continue
    key = tuple(sorted((na, nb, nc))) + (mtl,)
    if key in seen:
        continue
    seen.add(key)
    new_faces.append((na, nb, nc, mtl, grp))

with OUT.open('w') as f:
    f.write('# Low-poly derived balloon OBJ for oldflight experiments\n')
    f.write(f'# source={SRC.name} grid={GRID}\n')
    for x, y, z in new_verts:
        f.write(f'v {x:.4f} {y:.4f} {z:.4f}\n')
    last_grp = last_mtl = None
    for a, b, c, mtl, grp in new_faces:
        if grp != last_grp and grp is not None:
            f.write(f'g {grp}\n')
            last_grp = grp
        if mtl != last_mtl and mtl is not None:
            f.write(f'usemtl {mtl}\n')
            last_mtl = mtl
        f.write(f'f {a+1} {b+1} {c+1}\n')

print(f'original verts={len(verts)} tris={len(faces)}')
print(f'lowpoly verts={len(new_verts)} tris={len(new_faces)}')
print(f'wrote {OUT}')
