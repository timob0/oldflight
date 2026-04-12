#!/usr/bin/env python3
from pathlib import Path
from collections import defaultdict
import math

SRC = Path('/Users/timo/opengl/oldflight/obj/balloon-lowpoly.obj')
OUT = Path('/Users/timo/opengl/oldflight/balloon_mesh.h')
SCALE = 0.018

verts = []
faces = []
cur_mtl = 'default'

for line in SRC.open():
    if line.startswith('v '):
        _, x, y, z = line.split()[:4]
        verts.append((float(x), float(y), float(z)))
    elif line.startswith('usemtl '):
        cur_mtl = line.split(None, 1)[1].strip()
    elif line.startswith('f '):
        idx = [int(tok.split('/')[0]) - 1 for tok in line.split()[1:4]]
        faces.append((idx[0], idx[1], idx[2], cur_mtl))

# second-pass smoothing by averaging quantized source vertices per cluster
GRID = 60.0
cluster_acc = {}
cluster_count = {}
old_to_new = {}
for i, (x, y, z) in enumerate(verts):
    key = (round(x / GRID), round(y / GRID), round(z / GRID))
    cluster_acc[key] = cluster_acc.get(key, [0.0, 0.0, 0.0])
    cluster_acc[key][0] += x
    cluster_acc[key][1] += y
    cluster_acc[key][2] += z
    cluster_count[key] = cluster_count.get(key, 0) + 1
    old_to_new[i] = key

cluster_to_index = {}
new_verts = []
for key, acc in cluster_acc.items():
    n = cluster_count[key]
    cluster_to_index[key] = len(new_verts)
    new_verts.append((acc[0] / n, acc[1] / n, acc[2] / n))

tris_by_mtl = defaultdict(list)
for a, b, c, mtl in faces:
    na = cluster_to_index[old_to_new[a]]
    nb = cluster_to_index[old_to_new[b]]
    nc = cluster_to_index[old_to_new[c]]
    if len({na, nb, nc}) < 3:
        continue
    tris_by_mtl[mtl].append((na, nb, nc))

# de-dupe identical triangles per material
for mtl, tris in list(tris_by_mtl.items()):
    seen = set()
    out = []
    for tri in tris:
        key = tuple(sorted(tri))
        if key in seen:
            continue
        seen.add(key)
        out.append(tri)
    tris_by_mtl[mtl] = out

# center and re-axis for oldflight: OBJ z-up -> game y-up
xs = [v[0] for v in new_verts]
ys = [v[1] for v in new_verts]
zs = [v[2] for v in new_verts]
mid_x = (min(xs) + max(xs)) * 0.5
mid_y = (min(ys) + max(ys)) * 0.5
min_z = min(zs)

converted = []
for x, y, z in new_verts:
    ox = (x - mid_x) * SCALE
    oy = (z - min_z) * SCALE
    oz = -(y - mid_y) * SCALE
    converted.append((ox, oy, oz))

# smooth vertex normals from triangle normals
vert_normals = [[0.0, 0.0, 0.0] for _ in converted]
for tris in tris_by_mtl.values():
    for a, b, c in tris:
        ax, ay, az = converted[a]
        bx, by, bz = converted[b]
        cx, cy, cz = converted[c]
        ux, uy, uz = bx - ax, by - ay, bz - az
        vx, vy, vz = cx - ax, cy - ay, cz - az
        nx = uy * vz - uz * vy
        ny = uz * vx - ux * vz
        nz = ux * vy - uy * vx
        length = math.sqrt(nx * nx + ny * ny + nz * nz)
        if length <= 1e-9:
            continue
        nx /= length
        ny /= length
        nz /= length
        cx = (ax + bx + cx) / 3.0
        cy = (ay + by + cy) / 3.0
        cz = (az + bz + cz) / 3.0
        if (cx * nx + cy * ny + cz * nz) < 0.0:
            nx = -nx
            ny = -ny
            nz = -nz
        for idx in (a, b, c):
            vert_normals[idx][0] += nx
            vert_normals[idx][1] += ny
            vert_normals[idx][2] += nz

for n in vert_normals:
    length = math.sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2])
    if length <= 1e-9:
        n[0], n[1], n[2] = 0.0, 1.0, 0.0
    else:
        n[0] /= length
        n[1] /= length
        n[2] /= length

materials = [
    ('11809_Hot_air_balloon_Balloon', (0.86, 0.15, 0.12)),
    ('11809_Hot_air_balloon_top',     (0.95, 0.82, 0.22)),
    ('11809_Hot_air_balloon_Basket',  (0.55, 0.33, 0.14)),
    ('11809_Hot_air_balloon_Floor',   (0.45, 0.27, 0.11)),
    ('11809_Hot_air_balloon_Support', (0.35, 0.35, 0.35)),
    ('11809_Hot_air_balloon_Burner',  (0.18, 0.18, 0.18)),
]

with OUT.open('w') as f:
    f.write('/* auto-generated from obj/balloon-lowpoly.obj */\n')
    f.write('#pragma once\n\n')
    for mtl, color in materials:
        tris = tris_by_mtl.get(mtl, [])
        name = 'm_' + ''.join(ch if ch.isalnum() else '_' for ch in mtl).lower()
        f.write(f'static const float {name}_color[3] = {{{color[0]:.3f}f, {color[1]:.3f}f, {color[2]:.3f}f}};\n')
        f.write(f'static const float {name}_verts[] = {{\n')
        f.write(f'static const float {name}_normals[] = {{\n')
        # accumulate strings first to keep structure simple
    
with OUT.open('w') as f:
    f.write('/* auto-generated from obj/balloon-lowpoly.obj */\n')
    f.write('#pragma once\n\n')
    for mtl, color in materials:
        tris = tris_by_mtl.get(mtl, [])
        name = 'm_' + ''.join(ch if ch.isalnum() else '_' for ch in mtl).lower()
        f.write(f'static const float {name}_color[3] = {{{color[0]:.3f}f, {color[1]:.3f}f, {color[2]:.3f}f}};\n')
        f.write(f'static const float {name}_verts[] = {{\n')
        for a, b, c in tris:
            for idx in (a, b, c):
                x, y, z = converted[idx]
                f.write(f'  {x:.4f}f, {y:.4f}f, {z:.4f}f,\n')
        f.write('};\n')
        f.write(f'static const float {name}_normals[] = {{\n')
        for a, b, c in tris:
            for idx in (a, b, c):
                nx, ny, nz = vert_normals[idx]
                f.write(f'  {nx:.5f}f, {ny:.5f}f, {nz:.5f}f,\n')
        f.write('};\n')
        f.write(f'static const int {name}_tri_count = {len(tris)};\n\n')

print(f'original verts={len(verts)} tris={len(faces)}')
print(f'lowpoly verts={len(new_verts)} tris={sum(len(v) for v in tris_by_mtl.values())}')
print(f'wrote {OUT}')
