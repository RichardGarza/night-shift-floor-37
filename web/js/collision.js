/** Shared XZ solid tests — AABB + optional yaw OBB (angled cover). */

/** True if (x,z) circle of radius r overlaps solid footprint. */
export function overlapsSolidXZ(b, x, z, r = 0) {
  if (b.obb) {
    const o = b.obb;
    const dx = x - o.cx;
    const dz = z - o.cz;
    const lx = dx * o.cos + dz * o.sin;
    const lz = -dx * o.sin + dz * o.cos;
    return Math.abs(lx) < o.hx + r && Math.abs(lz) < o.hz + r;
  }
  return (
    x > b.min.x - r &&
    x < b.max.x + r &&
    z > b.min.z - r &&
    z < b.max.z + r
  );
}

/**
 * Push pos out of solid XZ footprint (min local penetration for OBB).
 * Returns true if a push occurred.
 */
export function pushOutSolidXZ(pos, r, b) {
  if (b.obb) {
    const o = b.obb;
    const dx = pos.x - o.cx;
    const dz = pos.z - o.cz;
    let lx = dx * o.cos + dz * o.sin;
    let lz = -dx * o.sin + dz * o.cos;
    const mx = o.hx + r;
    const mz = o.hz + r;
    if (Math.abs(lx) >= mx || Math.abs(lz) >= mz) return false;
    const penL = mx - Math.abs(lx);
    const penW = mz - Math.abs(lz);
    if (penL < penW) lx = lx > 0 ? mx : -mx;
    else lz = lz > 0 ? mz : -mz;
    pos.x = o.cx + lx * o.cos - lz * o.sin;
    pos.z = o.cz + lx * o.sin + lz * o.cos;
    return true;
  }
  const minX = b.min.x - r;
  const maxX = b.max.x + r;
  const minZ = b.min.z - r;
  const maxZ = b.max.z + r;
  if (pos.x <= minX || pos.x >= maxX || pos.z <= minZ || pos.z >= maxZ) return false;
  const cx = (b.min.x + b.max.x) * 0.5;
  const cz = (b.min.z + b.max.z) * 0.5;
  const penX = Math.min(pos.x - minX, maxX - pos.x);
  const penZ = Math.min(pos.z - minZ, maxZ - pos.z);
  if (penX < penZ) pos.x = pos.x < cx ? minX : maxX;
  else pos.z = pos.z < cz ? minZ : maxZ;
  return true;
}

/** Axis-separated resolve (AABB). OBB uses full min-pen push. */
export function resolveSolidAxis(pos, r, b, axis) {
  if (b.obb) {
    pushOutSolidXZ(pos, r, b);
    return;
  }
  const minX = b.min.x - r;
  const maxX = b.max.x + r;
  const minZ = b.min.z - r;
  const maxZ = b.max.z + r;
  if (pos.x <= minX || pos.x >= maxX || pos.z <= minZ || pos.z >= maxZ) return;
  if (axis === 'x') {
    const cx = (b.min.x + b.max.x) * 0.5;
    pos.x = pos.x < cx ? minX : maxX;
  } else {
    const cz = (b.min.z + b.max.z) * 0.5;
    pos.z = pos.z < cz ? minZ : maxZ;
  }
}

/** Surface height of a ramp solid at (x,z), or null if outside its footprint (shared by player + aliens). */
export function rampHeightAt(b, x, z) {
  const dx = x - b.x0;
  const dz = z - b.z0;
  const along = dx * b.dirX + dz * b.dirZ;
  if (along < -0.05 || along > b.len + 0.05) return null;
  const lat = -dx * b.dirZ + dz * b.dirX;
  const hw = b.width * 0.5;
  if (lat < -hw - 0.05 || lat > hw + 0.05) return null;
  const t = Math.max(0, Math.min(1, along / b.len));
  return b.y0 + (b.y1 - b.y0) * t;
}

/** Solids that should occlude the OTS camera (walls/cover/racks; not floors/ceilings/ramps). */
export function blocksCamera(b) {
  if (!b || b.ceiling || b.ramp || !b.blockXZ) return false;
  // Thin walkable platforms are floors — colliding with them pulls the cam into the ground
  if (b.walkable) {
    const h = b.max.y - b.min.y;
    if (h < 0.45) return false;
  }
  return true;
}

/**
 * Ray vs AABB (slab). Returns entry t in [0, maxT], or -1.
 * Skips hits with t < eps (origin inside / grazing).
 */
export function raycastAABB(ox, oy, oz, dx, dy, dz, min, max, maxT, eps = 1e-4) {
  let tmin = 0;
  let tmax = maxT;
  // X
  if (Math.abs(dx) < 1e-12) {
    if (ox < min.x || ox > max.x) return -1;
  } else {
    let t1 = (min.x - ox) / dx;
    let t2 = (max.x - ox) / dx;
    if (t1 > t2) { const s = t1; t1 = t2; t2 = s; }
    if (t1 > tmin) tmin = t1;
    if (t2 < tmax) tmax = t2;
    if (tmin > tmax) return -1;
  }
  // Y
  if (Math.abs(dy) < 1e-12) {
    if (oy < min.y || oy > max.y) return -1;
  } else {
    let t1 = (min.y - oy) / dy;
    let t2 = (max.y - oy) / dy;
    if (t1 > t2) { const s = t1; t1 = t2; t2 = s; }
    if (t1 > tmin) tmin = t1;
    if (t2 < tmax) tmax = t2;
    if (tmin > tmax) return -1;
  }
  // Z
  if (Math.abs(dz) < 1e-12) {
    if (oz < min.z || oz > max.z) return -1;
  } else {
    let t1 = (min.z - oz) / dz;
    let t2 = (max.z - oz) / dz;
    if (t1 > t2) { const s = t1; t1 = t2; t2 = s; }
    if (t1 > tmin) tmin = t1;
    if (t2 < tmax) tmax = t2;
    if (tmin > tmax) return -1;
  }
  if (tmin < eps) {
    // Origin inside solid — treat as immediate occlusion (pull toward minDistance)
    return 0;
  }
  return tmin <= maxT ? tmin : -1;
}

/** Ray vs solid (AABB or yaw-OBB). Returns entry t or -1. */
export function raycastSolid(ox, oy, oz, dx, dy, dz, maxT, b) {
  if (b.obb) {
    const o = b.obb;
    // World → local (yaw about Y; Y unchanged)
    const pdx = ox - o.cx;
    const pdz = oz - o.cz;
    const lx = pdx * o.cos + pdz * o.sin;
    const lz = -pdx * o.sin + pdz * o.cos;
    const ldx = dx * o.cos + dz * o.sin;
    const ldz = -dx * o.sin + dz * o.cos;
    const min = { x: -o.hx, y: b.min.y, z: -o.hz };
    const max = { x: o.hx, y: b.max.y, z: o.hz };
    return raycastAABB(lx, oy, lz, ldx, dy, ldz, min, max, maxT);
  }
  return raycastAABB(ox, oy, oz, dx, dy, dz, b.min, b.max, maxT);
}

/**
 * Closest camera-blocking hit along ray. Returns distance or -1 if clear.
 * Hot-path friendly: no allocations.
 */
export function raycastCameraSolids(ox, oy, oz, dx, dy, dz, maxDist, solids) {
  let best = -1;
  if (!solids || maxDist <= 0) return best;
  for (let i = 0; i < solids.length; i++) {
    const b = solids[i];
    if (!blocksCamera(b)) continue;
    const t = raycastSolid(ox, oy, oz, dx, dy, dz, maxDist, b);
    if (t < 0) continue;
    if (best < 0 || t < best) best = t;
  }
  return best;
}

