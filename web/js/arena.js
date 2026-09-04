import * as THREE from 'three';
import { CONFIG } from './config.js';

/**
 * Builds the ~50x50 office floor + atrium.
 * Returns { group, solids, spawnPoints, half }
 * solids: { min, max, walkable, blockXZ } AABB in world space
 */
export function buildArena(scene) {
  const half = CONFIG.mapSize * 0.5;
  const group = new THREE.Group();
  const solids = [];
  const shared = {
    floor: new THREE.MeshStandardMaterial({
      color: CONFIG.arena.floorColor,
      roughness: 0.35,
      metalness: 0.25,
    }),
    wet: new THREE.MeshStandardMaterial({
      color: 0x151a16,
      roughness: 0.2,
      metalness: 0.45,
    }),
    wall: new THREE.MeshStandardMaterial({
      color: CONFIG.arena.wallColor,
      roughness: 0.85,
      metalness: 0.05,
    }),
    cubicle: new THREE.MeshStandardMaterial({
      color: 0x3a4540,
      roughness: 0.75,
      metalness: 0.1,
    }),
    metal: new THREE.MeshStandardMaterial({
      color: 0x3a4048,
      roughness: 0.4,
      metalness: 0.7,
    }),
    resin: new THREE.MeshStandardMaterial({
      color: 0x2a6040,
      roughness: 0.55,
      metalness: 0.15,
      emissive: 0x0a2814,
      emissiveIntensity: 0.35,
    }),
    amber: new THREE.MeshStandardMaterial({
      color: CONFIG.arena.accentAmber,
      roughness: 0.5,
      metalness: 0.2,
      emissive: CONFIG.arena.accentAmber,
      emissiveIntensity: 0.4,
    }),
    greenGlow: new THREE.MeshStandardMaterial({
      color: CONFIG.arena.accentGreen,
      roughness: 0.6,
      metalness: 0.1,
      emissive: CONFIG.arena.accentGreen,
      emissiveIntensity: 0.55,
    }),
    concrete: new THREE.MeshStandardMaterial({
      color: 0x2c302c,
      roughness: 0.9,
      metalness: 0.05,
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
  const racks = [
    [18, 0, 0], [18, 0, 3], [18.5, 0, -3.5],
    [-18, 0, 2], [-18, 0, -1], [-17.5, 0, -4],
  ];
  racks.forEach(([x, , z], i) => {
    const h = i < 2 ? 3.6 : 2.2; // two stacked taller
    const ang = i === 2 ? 0.4 : 0;
    const mesh = addBox(x, h * 0.5, z, 1.0, h, 0.7, shared.metal, { cast: true });
    mesh.rotation.y = ang;
    // Blink lights
    const light = new THREE.PointLight(i % 2 ? 0x44ff66 : 0xffaa33, 0.6, 5, 2);
    light.position.set(x, h * 0.7, z);
    group.add(light);
  });

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

  // Mood lights — sick green / amber
  const moodLights = [
    [0, 12, 0, 0x44aa55, 1.2, 28],
    [-12, 4, -12, 0xb8860b, 0.9, 16],
    [12, 4, 12, 0x3a8a3a, 0.8, 14],
    [-12, 5, 12, 0xaa7733, 0.7, 12],
    [12, 5, -12, 0x55aa66, 0.7, 12],
    [0, 16, 0, 0x66cc77, 0.5, 20],
    [14, 3, 8, 0xcc9944, 0.6, 10],
  ];
  for (const [x, y, z, col, int, dist] of moodLights) {
    const pl = new THREE.PointLight(col, int, dist, 2);
    pl.position.set(x, y, z);
    group.add(pl);
  }

  // Neon trim strips
  for (let a = 0; a < 4; a++) {
    const ang = (a / 4) * Math.PI * 2;
    const tx = Math.cos(ang) * 22;
    const tz = Math.sin(ang) * 22;
    addBox(tx, 0.08, tz, 8, 0.06, 0.2, shared.greenGlow, { noCollide: true });
  }

  // Ambient + hemi for readability
  const hemi = new THREE.HemisphereLight(0x4a6a50, 0x1a1510, 0.45);
  group.add(hemi);
  const amb = new THREE.AmbientLight(0x1a2218, 0.35);
  group.add(amb);
  const dir = new THREE.DirectionalLight(0x889977, 0.35);
  dir.position.set(-20, 30, 10);
  dir.castShadow = false;
  group.add(dir);

  scene.add(group);
  scene.fog = new THREE.Fog(CONFIG.arena.fogColor, CONFIG.arena.fogNear, CONFIG.arena.fogFar);
  scene.background = new THREE.Color(CONFIG.arena.fogColor);

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
