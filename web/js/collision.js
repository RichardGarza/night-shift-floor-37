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
