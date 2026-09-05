import * as THREE from 'three';
import { CONFIG } from './config.js';

/**
 * Builds the ~50x50 office floor + atrium.
 * Returns { group, solids, spawnPoints, half }
 * solids: { min, max, walkable, blockXZ, obb?, ceiling? } AABB (+ optional yaw OBB)
 */
export function buildArena(scene) {
  const half = CONFIG.mapSize * 0.5;
  const group = new THREE.Group();
  const solids = [];
  const shared = {
    floor: new THREE.MeshStandardMaterial({
      color: CONFIG.arena.floorColor,
      roughness: 0.85,
      metalness: 0.0,
      emissive: 0x3a4a38,
      emissiveIntensity: 0.45,
    }),
    wet: new THREE.MeshStandardMaterial({
      color: 0x7a8a78,
      roughness: 0.55,
      metalness: 0.08,
      emissive: 0x2a3a28,
      emissiveIntensity: 0.3,
    }),
    wall: new THREE.MeshStandardMaterial({
      color: CONFIG.arena.wallColor,
      roughness: 0.9,
      metalness: 0.0,
      emissive: 0x2a322c,
      emissiveIntensity: 0.28,
    }),
    cubicle: new THREE.MeshStandardMaterial({
      color: 0x9aaa96,
      roughness: 0.82,
      metalness: 0.0,
      emissive: 0x2a3828,
      emissiveIntensity: 0.35,
    }),
    metal: new THREE.MeshStandardMaterial({
      color: 0x8a929a,
      roughness: 0.6,
      metalness: 0.25,
      emissive: 0x202830,
      emissiveIntensity: 0.22,
    }),
    resin: new THREE.MeshStandardMaterial({
      color: 0x4aaa60,
      roughness: 0.5,
      metalness: 0.05,
      emissive: 0x2a8040,
      emissiveIntensity: 0.9,
    }),
    amber: new THREE.MeshStandardMaterial({
      color: CONFIG.arena.accentAmber,
      roughness: 0.45,
      metalness: 0.1,
      emissive: CONFIG.arena.accentAmber,
      emissiveIntensity: 1.1,
    }),
    greenGlow: new THREE.MeshStandardMaterial({
      color: CONFIG.arena.accentGreen,
      roughness: 0.55,
      metalness: 0.05,
      emissive: CONFIG.arena.accentGreen,
      emissiveIntensity: 1.15,
    }),
    concrete: new THREE.MeshStandardMaterial({
      color: 0x8a9288,
      roughness: 0.95,
      metalness: 0.0,
      emissive: 0x2a3028,
      emissiveIntensity: 0.3,
    }),
  };

  const boxGeo = new THREE.BoxGeometry(1, 1, 1);
  const cylGeo = new THREE.CylinderGeometry(0.5, 0.5, 1, 10);

  function addBox(x, y, z, sx, sy, sz, mat, opts = {}) {
    const mesh = new THREE.Mesh(boxGeo, mat);
    mesh.position.set(x, y, z);
    mesh.scale.set(sx, sy, sz);
    mesh.castShadow = !!opts.cast;
    mesh.receiveShadow = true;
    group.add(mesh);
    const hx = sx * 0.5, hy = sy * 0.5, hz = sz * 0.5;
    const solid = {
      min: new THREE.Vector3(x - hx, y - hy, z - hz),
      max: new THREE.Vector3(x + hx, y + hy, z + hz),
      walkable: opts.walkable !== false && opts.platform === true,
      blockXZ: opts.blockXZ !== false && !opts.noCollide,
      mesh,
    };
    if (opts.platform) solid.walkable = true;
    if (!opts.noCollide) solids.push(solid);
    return mesh;
  }

  function addCyl(x, y, z, r, h, mat, opts = {}) {
    const mesh = new THREE.Mesh(cylGeo, mat);
    mesh.position.set(x, y, z);
    mesh.scale.set(r * 2, h, r * 2);
    mesh.castShadow = true;
    group.add(mesh);
    if (!opts.noCollide) {
      solids.push({
        min: new THREE.Vector3(x - r, y - h * 0.5, z - r),
        max: new THREE.Vector3(x + r, y + h * 0.5, z + r),
        walkable: false,
        blockXZ: true,
        mesh,
      });
    }
    return mesh;
  }

  // Main floor (visual)
  const floor = new THREE.Mesh(
    new THREE.PlaneGeometry(CONFIG.mapSize, CONFIG.mapSize),
    shared.wet
  );
  floor.rotation.x = -Math.PI / 2;
  floor.receiveShadow = true;
  group.add(floor);

  // Perimeter low walls / planters
  const wallH = 1.2;
  const t = 0.6;
  addBox(0, wallH * 0.5, -half + t * 0.5, CONFIG.mapSize, wallH, t, shared.wall, { cast: true });
  addBox(0, wallH * 0.5, half - t * 0.5, CONFIG.mapSize, wallH, t, shared.wall, { cast: true });
  addBox(-half + t * 0.5, wallH * 0.5, 0, t, wallH, CONFIG.mapSize - t * 2, shared.wall, { cast: true });
  addBox(half - t * 0.5, wallH * 0.5, 0, t, wallH, CONFIG.mapSize - t * 2, shared.wall, { cast: true });

  // Glass-ish exterior strips (decorative)
  for (let i = -2; i <= 2; i++) {
    if (i === 0) continue;
    addBox(i * 8, 3.5, -half + 0.2, 6, 5, 0.15, shared.metal, { noCollide: true });
    addBox(i * 8, 3.5, half - 0.2, 6, 5, 0.15, shared.metal, { noCollide: true });
  }

  // === ATRIUM TOWER (~14m / 3 stories) ===
  const atriumLevels = [0, 4.5, 9, 13.5];
  const platSize = 8;

  // Central column hints
  addBox(0, 7, 0, 1.2, 14, 1.2, shared.concrete, { cast: true, blockXZ: true });

  // Platforms at each level (open atrium floors)
  for (let li = 1; li < atriumLevels.length; li++) {
    const y = atriumLevels[li];
    // Four L-shaped / open platforms around center
    const pads = [
      [0, 5], [0, -5], [5, 0], [-5, 0],
    ];
    for (const [px, pz] of pads) {
      addBox(px, y - 0.15, pz, platSize * 0.7, 0.3, platSize * 0.55, shared.concrete, {
        cast: true,
        platform: true,
        blockXZ: true,
      });
    }
    // Connecting ring corners
    addBox(4, y - 0.15, 4, 3.5, 0.3, 3.5, shared.concrete, { platform: true, blockXZ: true, cast: true });
    addBox(-4, y - 0.15, 4, 3.5, 0.3, 3.5, shared.concrete, { platform: true, blockXZ: true, cast: true });
    addBox(4, y - 0.15, -4, 3.5, 0.3, 3.5, shared.concrete, { platform: true, blockXZ: true, cast: true });
    addBox(-4, y - 0.15, -4, 3.5, 0.3, 3.5, shared.concrete, { platform: true, blockXZ: true, cast: true });
  }

  // Continuous ramps between levels (no rails). Visual slanted box +
  // interpolated walkable solid (no XZ wall snags while climbing).
  function addRamp(x0, z0, x1, z1, y0, y1, width) {
    const dx = x1 - x0;
    const dz = z1 - z0;
    const dy = y1 - y0;
    const len = Math.hypot(dx, dz);
    if (len < 0.05) return;

    const dirX = dx / len;
    const dirZ = dz / len;
    const thick = CONFIG.arena.rampThickness;
    const midX = (x0 + x1) * 0.5;
    const midZ = (z0 + z1) * 0.5;
    const midY = (y0 + y1) * 0.5;
    const yaw = Math.atan2(dx, dz);

    // Flat bridge / landing: oriented walkable platform (top flush at y0)
    if (Math.abs(dy) < 0.05) {
      const mesh = new THREE.Mesh(boxGeo, shared.concrete);
      mesh.position.set(midX, y0 - thick * 0.5, midZ);
      mesh.scale.set(width, thick, len);
      mesh.rotation.y = yaw;
      mesh.castShadow = true;
      mesh.receiveShadow = true;
      group.add(mesh);
      const hx = len * 0.5;
      const hz = width * 0.5;
      const cos = Math.abs(Math.cos(yaw));
      const sin = Math.abs(Math.sin(yaw));
      const extX = hx * sin + hz * cos;
      const extZ = hx * cos + hz * sin;
      solids.push({
        min: new THREE.Vector3(midX - extX, y0 - thick, midZ - extZ),
        max: new THREE.Vector3(midX + extX, y0, midZ + extZ),
        walkable: true,
        blockXZ: true,
        mesh,
      });
      return;
    }

    // Sloped visual
    const pitch = Math.atan2(dy, len);
    const mesh = new THREE.Mesh(boxGeo, shared.concrete);
    mesh.position.set(midX, midY + thick * 0.35, midZ);
    mesh.scale.set(width, thick, len / Math.cos(pitch));
    mesh.rotation.order = 'YXZ';
    mesh.rotation.y = yaw;
    mesh.rotation.x = -pitch;
    mesh.castShadow = true;
    mesh.receiveShadow = true;
    group.add(mesh);

    // Conservative AABB for broad-phase (height from ramp lerp; no XZ block)
    const pad = 0.15;
    const minY = Math.min(y0, y1) - 0.05;
    const maxY = Math.max(y0, y1) + thick + 0.05;
    const hw = width * 0.5 + pad;
    const corners = [
      [x0 - dirZ * hw, z0 + dirX * hw],
      [x0 + dirZ * hw, z0 - dirX * hw],
      [x1 - dirZ * hw, z1 + dirX * hw],
      [x1 + dirZ * hw, z1 - dirX * hw],
    ];
    let minX = Infinity, maxX = -Infinity, minZ = Infinity, maxZ = -Infinity;
    for (const [cx, cz] of corners) {
      if (cx < minX) minX = cx;
      if (cx > maxX) maxX = cx;
      if (cz < minZ) minZ = cz;
      if (cz > maxZ) maxZ = cz;
    }

    solids.push({
      min: new THREE.Vector3(minX, minY, minZ),
      max: new THREE.Vector3(maxX, maxY, maxZ),
      walkable: true,
      blockXZ: false,
      ramp: true,
      x0, z0, x1, z1, y0, y1,
      dirX, dirZ, len, width,
      mesh,
    });
  }

  // Ramps winding up the atrium (continuous slopes + flat landings)
  addRamp(6, 2, 6, 8, 0, 4.5, 2.2);
  addRamp(6, 8, -2, 8, 4.5, 4.5, 2.2); // landing bridge
  addRamp(-6, 6, -6, 0, 4.5, 9, 2.2);
  addRamp(-6, -2, -6, -8, 9, 9, 2.2);
  addRamp(-4, -8, 4, -8, 9, 13.5, 2.2);
  addRamp(6, -6, 6, 0, 13.5, 13.5, 2.0);

  // Extra access ramps from floor to L1
  addRamp(-8, -6, -5, -5, 0, 4.5, 2.0);
  addRamp(8, 4, 5, 5, 0, 4.5, 2.0);

  // === CUBICLE COVER ===
  const cubH = 1.15;
  const cubPositions = [
    // NW maze
    [-16, -14], [-14, -14], [-12, -14], [-16, -12], [-12, -12], [-16, -10], [-14, -10],
    [-18, -16], [-10, -16], [-10, -12],
    // NE
    [12, -14], [14, -14], [16, -14], [12, -12], [16, -12], [14, -10], [16, -10],
    // SW
    [-16, 12], [-14, 12], [-12, 14], [-16, 14], [-14, 16], [-10, 14],
    // SE
    [12, 12], [14, 14], [16, 12], [16, 16], [12, 16], [10, 12],
  ];
  for (const [cx, cz] of cubPositions) {
    addBox(cx, cubH * 0.5, cz, 1.8, cubH, 0.12, shared.cubicle, { cast: true });
    addBox(cx + 0.7, cubH * 0.5, cz + 0.9, 0.12, cubH, 1.8, shared.cubicle, { cast: true });
  }

  // === SERVER RACKS (6) ===
  // One rack is yaw-rotated; collision uses OBB matching the mesh (not pre-rotation AABB).
  const racks = [
    [18, 0, 0], [18, 0, 3], [18.5, 0, -3.5],
    [-18, 0, 2], [-18, 0, -1], [-17.5, 0, -4],
  ];
  racks.forEach(([x, , z], i) => {
    const h = i < 2 ? 3.6 : 2.2; // two stacked taller
    const ang = i === 2 ? 0.4 : 0;
    const sx = 1.0;
    const sz = 0.7;
    let mesh;
    if (ang !== 0) {
      mesh = new THREE.Mesh(boxGeo, shared.metal);
      mesh.position.set(x, h * 0.5, z);
      mesh.scale.set(sx, h, sz);
      mesh.rotation.y = ang;
      mesh.castShadow = true;
      mesh.receiveShadow = true;
      group.add(mesh);
      const cos = Math.cos(ang);
      const sin = Math.sin(ang);
      const hx = sx * 0.5;
      const hz = sz * 0.5;
      const extX = Math.abs(hx * cos) + Math.abs(hz * sin);
      const extZ = Math.abs(hx * sin) + Math.abs(hz * cos);
      solids.push({
        min: new THREE.Vector3(x - extX, 0, z - extZ),
        max: new THREE.Vector3(x + extX, h, z + extZ),
        walkable: false,
        blockXZ: true,
        mesh,
        obb: { cx: x, cz: z, hx, hz, cos, sin, yaw: ang },
      });
    } else {
      mesh = addBox(x, h * 0.5, z, sx, h, sz, shared.metal, { cast: true });
    }
  });
  // Shared rack cluster lights (2 instead of 6) — east green / west amber
  {
    const east = new THREE.PointLight(0x88ffaa, 4.0, 28, 2);
    east.position.set(18.2, 2.8, 0);
    group.add(east);
    const west = new THREE.PointLight(0xffcc55, 4.5, 32, 2);
    west.position.set(-18, 2.6, -1);
    group.add(west);
  }

  // === CABLE TRAYS at 1.5m ===
  for (let i = -20; i <= 20; i += 4) {
    addBox(i, 1.5, -18, 3.5, 0.08, 0.5, shared.metal, { noCollide: true });
    addBox(-18, 1.5, i, 0.5, 0.08, 3.5, shared.metal, { noCollide: true });
  }
  addBox(10, 1.5, 0, 12, 0.08, 0.45, shared.metal, { noCollide: true });

  // === CONFERENCE PAD ===
  addBox(14, 0.35, 8, 8, 0.7, 6, shared.concrete, { platform: true, blockXZ: true, cast: true });
  addBox(14, 1.6, 10.5, 6, 1.8, 0.15, shared.metal, { cast: true }); // broken glass wall visual
  // Table
  addBox(14, 1.15, 8, 4, 0.1, 1.6, shared.metal, { cast: true, blockXZ: true });

  // === BERMS / PLANTERS ===
  const berms = [
    [-8, 10, 4, 1.0, 2], [8, -12, 5, 0.9, 2.5], [-10, -8, 3.5, 0.8, 2],
    [10, 14, 4, 0.7, 1.8], [-14, 6, 3, 0.85, 2.2],
  ];
  for (const [bx, bz, sx, sy, sz] of berms) {
    addBox(bx, sy * 0.5, bz, sx, sy, sz, shared.concrete, { cast: true, platform: true, blockXZ: true });
  }

  // === RESIN BARRELS (4 clusters) ===
  const clusters = [
    [-6, 16], [6, -16], [16, -8], [-16, 8],
  ];
  for (const [cx, cz] of clusters) {
    for (let i = 0; i < 4; i++) {
      const ox = (i % 2) * 1.1 - 0.4;
      const oz = Math.floor(i / 2) * 1.1 - 0.4;
      addCyl(cx + ox, 0.55, cz + oz, 0.4, 1.1, shared.resin);
    }
  }

  // Mood lights — heavy amber practicals for floor readability
  const moodScale = CONFIG.arena.moodIntensityScale ?? 3.2;
  const moodLights = [
    [0, 14, 0, 0x88e088, 3.0, 70],
    [-16, 5, -12, 0xffb020, 3.2, 50],
    [16, 5, 12, 0x66cc66, 2.6, 48],
    [0, 3.5, 0, 0xffc033, 3.0, 45],
    [20, 3.5, -16, 0xffaa22, 2.4, 40],
    [-20, 3.5, 16, 0xffb040, 2.4, 40],
    [0, 6, -20, 0xe8a010, 2.2, 42],
    [0, 6, 20, 0x55bb66, 2.0, 42],
  ];
  for (const [x, y, z, col, int, dist] of moodLights) {
    const pl = new THREE.PointLight(col, int * moodScale, dist, 2);
    pl.position.set(x, y, z);
    group.add(pl);
  }

  // Neon / amber trim strips — local midtone anchors
  for (let a = 0; a < 4; a++) {
    const ang = (a / 4) * Math.PI * 2;
    const tx = Math.cos(ang) * 22;
    const tz = Math.sin(ang) * 22;
    addBox(tx, 0.08, tz, 10, 0.08, 0.25, shared.greenGlow, { noCollide: true });
    addBox(tx * 0.55, 0.08, tz * 0.55, 6, 0.08, 0.22, shared.amber, { noCollide: true });
  }
  // Cross floor amber runners
  addBox(0, 0.06, 0, 40, 0.05, 0.35, shared.amber, { noCollide: true });
  addBox(0, 0.06, 0, 0.35, 0.05, 40, shared.amber, { noCollide: true });

  // Strong fill — must read on a normal monitor
  const hemi = new THREE.HemisphereLight(
    0xb0d0b0,
    0x4a4030,
    CONFIG.arena.hemiIntensity ?? 2.2,
  );
  group.add(hemi);
  const amb = new THREE.AmbientLight(0x889888, CONFIG.arena.ambientIntensity ?? 2.4);
  group.add(amb);
  const dir = new THREE.DirectionalLight(0xe8f0d8, CONFIG.arena.dirIntensity ?? 1.8);
  dir.position.set(-15, 40, 15);
  dir.castShadow = false;
  group.add(dir);

  scene.add(group);
  scene.fog = new THREE.Fog(CONFIG.arena.fogColor, CONFIG.arena.fogNear, CONFIG.arena.fogFar);
  scene.background = new THREE.Color(CONFIG.arena.fogColor);

  // Invisible ceiling — solid box + vertical clamp (tower ~14m + headroom)
  {
    const ceilY = CONFIG.arena.ceilingHeight;
    const thick = 0.5;
    const mesh = new THREE.Mesh(boxGeo, shared.wall);
    mesh.position.set(0, ceilY + thick * 0.5, 0);
    mesh.scale.set(CONFIG.mapSize, thick, CONFIG.mapSize);
    mesh.visible = false;
    group.add(mesh);
    solids.push({
      min: new THREE.Vector3(-half, ceilY, -half),
      max: new THREE.Vector3(half, ceilY + thick, half),
      walkable: false,
      blockXZ: false,
      ceiling: true,
      mesh,
    });
  }

  // 8 edge spawn points
  const spawnPoints = [
    new THREE.Vector3(-22, 0, -22), // stairwell-ish
    new THREE.Vector3(22, 0, -22),
    new THREE.Vector3(-22, 0, 22),
    new THREE.Vector3(22, 0, 22),
    new THREE.Vector3(0, 0, -23),   // loading dock
    new THREE.Vector3(0, 0, 23),
    new THREE.Vector3(-23, 0, 0),   // elevator / corridor
    new THREE.Vector3(23, 0, 0),
  ];

  return { group, solids, spawnPoints, half, shared };
}
