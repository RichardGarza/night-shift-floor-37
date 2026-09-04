import * as THREE from 'three';
import { CONFIG } from './config.js';

const _to = new THREE.Vector3();
const _side = new THREE.Vector3();
const _push = new THREE.Vector3();
const _aim = new THREE.Vector3();
const _origin = new THREE.Vector3();
const _dir = new THREE.Vector3();

let _bodyGeo = null;
let _headGeo = null;
let _bodyMat = null;
let _headMat = null;
let _flashMat = null;

function ensureShared() {
  if (_bodyGeo) return;
  const h = CONFIG.alien.height;
  const r = CONFIG.alien.radius;
  _bodyGeo = new THREE.CapsuleGeometry(r, h - r * 2, 3, 8);
  _headGeo = new THREE.SphereGeometry(r * 0.85, 8, 6);
  _bodyMat = new THREE.MeshStandardMaterial({
    color: CONFIG.alien.color,
    roughness: 0.45,
    metalness: 0.1,
    emissive: CONFIG.alien.color,
    emissiveIntensity: 0.35,
  });
  _headMat = new THREE.MeshStandardMaterial({
    color: CONFIG.alien.headColor,
    roughness: 0.4,
    metalness: 0.15,
    emissive: CONFIG.alien.headColor,
    emissiveIntensity: 0.5,
  });
  _flashMat = new THREE.MeshBasicMaterial({ color: 0xffffff });
}

function rampHeightAt(b, x, z) {
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

export class Alien {
  constructor(scene) {
    ensureShared();
    this.scene = scene;
    this.root = new THREE.Group();
    this.body = new THREE.Mesh(_bodyGeo, _bodyMat.clone());
    this.body.position.y = CONFIG.alien.height * 0.5;
    this.body.castShadow = true;
    this.body.userData.alienRef = this;
    this.root.add(this.body);

    this.headMesh = new THREE.Mesh(_headGeo, _headMat.clone());
    this.headMesh.position.y = CONFIG.alien.height * 0.88;
    this.headMesh.userData.alienRef = this;
    this.root.add(this.headMesh);

    this.root.visible = false;
    scene.add(this.root);

    this.alive = false;
    this.hpBody = 0;
    this.hpHead = 0;
    this.velocity = new THREE.Vector3();
    this.strafeDir = 1;
    this.burstLeft = 0;
    this.burstCd = 0;
    this.shotCd = 0;
    this.flashT = 0;
    this.respawnAt = 0;
    this._baseBodyMat = this.body.material;
    this._baseHeadMat = this.headMesh.material;
    this._stuckT = 0;
    this._detourT = 0;
    this._detourDir = 1;
    this._lastX = 0;
    this._lastZ = 0;
  }

  get position() {
    return this.root.position;
  }

  get aimPoint() {
    return _aim.set(
      this.root.position.x,
      this.root.position.y + CONFIG.alien.height * 0.75,
      this.root.position.z
    );
  }

  get colliderMesh() {
    return this.body;
  }

  spawnAt(pos) {
    this.root.position.copy(pos);
    this.root.position.y = 0;
    this.root.visible = true;
    this.alive = true;
    // HP pools = hits×dmg so CONFIG.rifle bodyDmg/headDmg stay live; DESIGN TTK still 3 body OR 2 head
    this.hpBody = CONFIG.alien.bodyHitsToKill * CONFIG.rifle.bodyDmg;
    this.hpHead = CONFIG.alien.headHitsToKill * CONFIG.rifle.headDmg;
    this.velocity.set(0, 0, 0);
    this.strafeDir = Math.random() < 0.5 ? -1 : 1;
    this.burstLeft = 0;
    this.burstCd = 0.5 + Math.random();
    this.shotCd = 0;
    this.flashT = 0;
    this._stuckT = 0;
    this._detourT = 0;
    this._lastX = this.root.position.x;
    this._lastZ = this.root.position.z;
    this.body.material = this._baseBodyMat;
    this.headMesh.material = this._baseHeadMat;
  }

  kill() {
    this.alive = false;
    this.root.visible = false;
    this.respawnAt = performance.now() / 1000 + CONFIG.alien.respawnDelay;
  }

  /** @returns true if killed */
  takeHit(isHead) {
    if (!this.alive) return false;
    if (isHead) this.hpHead -= CONFIG.rifle.headDmg;
    else this.hpBody -= CONFIG.rifle.bodyDmg;
    this.flashT = CONFIG.feedback.flashMs / 1000;
    this.body.material = _flashMat;
    this.headMesh.material = _flashMat;
    // Kill if either pool is depleted (DESIGN: 3 body OR 2 head)
    if (this.hpBody <= 0 || this.hpHead <= 0) {
      this.kill();
      return true;
    }
    return false;
  }

  update(dt, player, solids, aliens, combat, half) {
    if (!this.alive) return;

    if (this.flashT > 0) {
      this.flashT -= dt;
      if (this.flashT <= 0) {
        this.body.material = this._baseBodyMat;
        this.headMesh.material = this._baseHeadMat;
      }
    }

    const pos = this.root.position;
    _to.copy(player.position).sub(pos);
    _to.y = 0;
    const dist = _to.length();
    if (dist > 0.01) _to.multiplyScalar(1 / dist);

    const hasLos = dist <= CONFIG.alien.engageRange && this._hasLos(player, solids);
    const chasing = !(hasLos && dist <= CONFIG.alien.engageRange);

    if (!chasing) {
      // Stop, strafe, burst
      _side.set(-_to.z, 0, _to.x).multiplyScalar(this.strafeDir);
      this.velocity.x = _side.x * CONFIG.alien.speed * 0.7;
      this.velocity.z = _side.z * CONFIG.alien.speed * 0.7;
      if (Math.random() < dt * 0.4) this.strafeDir *= -1;

      this.burstCd -= dt;
      if (this.burstLeft > 0) {
        this.shotCd -= dt;
        if (this.shotCd <= 0) {
          this._fireAt(player, combat);
          this.burstLeft -= 1;
          this.shotCd = 60 / CONFIG.alien.burstRpm;
        }
      } else if (this.burstCd <= 0) {
        this.burstLeft = CONFIG.alien.burstShots;
        this.burstCd = CONFIG.alien.burstInterval;
        this.shotCd = 0;
      }
      this._stuckT = 0;
      this._detourT = 0;
    } else {
      // Chase with stuck steering / left-right detours
      this.velocity.x = _to.x * CONFIG.alien.speed;
      this.velocity.z = _to.z * CONFIG.alien.speed;
      this.burstLeft = 0;

      const moved = Math.hypot(pos.x - this._lastX, pos.z - this._lastZ);
      const movedSpeed = dt > 1e-6 ? moved / dt : CONFIG.alien.speed;
      if (dist > 1.2 && movedSpeed < CONFIG.alien.stuckSpeed) {
        this._stuckT += dt;
      } else {
        this._stuckT = Math.max(0, this._stuckT - dt * 1.5);
      }

      if (this._stuckT >= CONFIG.alien.stuckTime && this._detourT <= 0) {
        this._detourDir = this._pickDetour(pos, solids, _to);
        this._detourT = CONFIG.alien.detourDuration;
        this._stuckT = 0;
      }

      if (this._detourT > 0) {
        this._detourT -= dt;
        _side.set(-_to.z, 0, _to.x).multiplyScalar(this._detourDir);
        const blend = CONFIG.alien.detourBlend;
        this.velocity.x =
          (_to.x * (1 - blend) + _side.x * blend) * CONFIG.alien.speed;
        this.velocity.z =
          (_to.z * (1 - blend) + _side.z * blend) * CONFIG.alien.speed;
      } else {
        // Mild wall-slide when already overlapping / grazing an obstacle
        this._nudgeSlide(pos, solids);
      }
    }

    // Cheap sphere push-apart
    for (let i = 0; i < aliens.length; i++) {
      const o = aliens[i];
      if (o === this || !o.alive) continue;
      _push.copy(pos).sub(o.position);
      _push.y = 0;
      const d = _push.length();
      const minD = CONFIG.alien.pushRadius * 2;
      if (d > 0.001 && d < minD) {
        _push.multiplyScalar(((minD - d) / d) * CONFIG.alien.pushStrength * dt);
        pos.x += _push.x;
        pos.z += _push.z;
      }
    }

    this._lastX = pos.x;
    this._lastZ = pos.z;

    // Move + separated axis collision with step-up
    pos.x += this.velocity.x * dt;
    this._resolve(pos, solids, 'x');
    pos.z += this.velocity.z * dt;
    this._resolve(pos, solids, 'z');

    // Stay on walkable / floor / ramps
    let gy = 0;
    const r = CONFIG.alien.radius;
    for (let i = 0; i < solids.length; i++) {
      const b = solids[i];
      if (b.ramp) {
        const h = rampHeightAt(b, pos.x, pos.z);
        if (h != null && h > gy && h < pos.y + 1.2) gy = h;
        continue;
      }
      if (!b.walkable) continue;
      if (pos.x + r > b.min.x && pos.x - r < b.max.x && pos.z + r > b.min.z && pos.z - r < b.max.z) {
        if (b.max.y > gy && b.max.y < pos.y + 1.2) gy = b.max.y;
      }
    }
    pos.y = gy;

    const lim = half - r - 0.3;
    pos.x = Math.max(-lim, Math.min(lim, pos.x));
    pos.z = Math.max(-lim, Math.min(lim, pos.z));

    this.root.rotation.y = Math.atan2(_to.x, _to.z);
  }

  _pickDetour(pos, solids, toward) {
    // Probe left vs right clearance; prefer the emptier side
    const probe = 1.1;
    _side.set(-toward.z, 0, toward.x);
    let leftHits = 0;
    let rightHits = 0;
    for (let s = 0.4; s <= probe; s += 0.35) {
      const lx = pos.x + _side.x * s;
      const lz = pos.z + _side.z * s;
      const rx = pos.x - _side.x * s;
      const rz = pos.z - _side.z * s;
      if (this._pointBlocked(lx, pos.y, lz, solids)) leftHits++;
      if (this._pointBlocked(rx, pos.y, rz, solids)) rightHits++;
    }
    if (leftHits === rightHits) return Math.random() < 0.5 ? -1 : 1;
    return leftHits < rightHits ? 1 : -1;
  }

  _pointBlocked(x, y, z, solids) {
    const r = CONFIG.alien.radius * 0.85;
    const h = CONFIG.alien.height;
    const py = y + h * 0.5;
    for (let i = 0; i < solids.length; i++) {
      const b = solids[i];
      if (!b.blockXZ || b.ramp) continue;
      if (py + h * 0.35 < b.min.y || py - h * 0.35 > b.max.y) continue;
      if (b.walkable && b.max.y - b.min.y < 0.5 && Math.abs(y - b.max.y) < 0.45) continue;
      if (
        x > b.min.x - r &&
        x < b.max.x + r &&
        z > b.min.z - r &&
        z < b.max.z + r
      ) {
        return true;
      }
    }
    return false;
  }

  _nudgeSlide(pos, solids) {
    // If forward sample is blocked, bias velocity along free tangent
    const look = 0.55;
    const fx = pos.x + _to.x * look;
    const fz = pos.z + _to.z * look;
    if (!this._pointBlocked(fx, pos.y, fz, solids)) return;
    _side.set(-_to.z, 0, _to.x);
    const leftClear = !this._pointBlocked(pos.x + _side.x * look, pos.y, pos.z + _side.z * look, solids);
    const rightClear = !this._pointBlocked(pos.x - _side.x * look, pos.y, pos.z - _side.z * look, solids);
    let s = 0;
    if (leftClear && !rightClear) s = 1;
    else if (rightClear && !leftClear) s = -1;
    else if (leftClear && rightClear) s = this._detourDir || 1;
    else return;
    this.velocity.x = (_to.x * 0.25 + _side.x * s * 0.9) * CONFIG.alien.speed;
    this.velocity.z = (_to.z * 0.25 + _side.z * s * 0.9) * CONFIG.alien.speed;
  }

  _hasLos(player, solids) {
    _origin.set(this.position.x, this.position.y + 1.2, this.position.z);
    _dir.set(
      player.position.x - _origin.x,
      player.position.y + 1.2 - _origin.y,
      player.position.z - _origin.z
    );
    const len = _dir.length();
    if (len < 0.1) return true;
    _dir.multiplyScalar(1 / len);
    for (let t = 1; t < len; t += 1.2) {
      const x = _origin.x + _dir.x * t;
      const y = _origin.y + _dir.y * t;
      const z = _origin.z + _dir.z * t;
      for (let i = 0; i < solids.length; i++) {
        const b = solids[i];
        if (!b.blockXZ || b.ramp) continue;
        if (b.max.y - b.min.y < 1.4) continue;
        if (x > b.min.x && x < b.max.x && z > b.min.z && z < b.max.z && y > b.min.y && y < b.max.y) {
          return false;
        }
      }
    }
    return true;
  }

  _fireAt(player, combat) {
    if (!player.alive) return;
    const hit = Math.random() < CONFIG.alien.accuracy;
    _origin.set(this.position.x, this.position.y + 1.3, this.position.z);
    if (hit) {
      _aim.set(
        player.position.x,
        player.position.y + CONFIG.player.eyeOffset,
        player.position.z
      );
      player.takeDamage(CONFIG.alien.damage);
      combat.addDamageVignette(0.55);
      combat.spawnTracer(_origin, _aim);
    } else {
      _aim.set(
        player.position.x + (Math.random() - 0.5) * 3,
        player.position.y + 1.2 + (Math.random() - 0.5) * 1.5,
        player.position.z + (Math.random() - 0.5) * 3
      );
      combat.spawnTracer(_origin, _aim);
    }
  }

  _resolve(pos, solids, axis) {
    const r = CONFIG.alien.radius;
    const h = CONFIG.alien.height;
    const stepUp = CONFIG.alien.stepUp;
    for (let i = 0; i < solids.length; i++) {
      const b = solids[i];
      if (!b.blockXZ || b.ramp) continue;
      const py = pos.y + h * 0.5;
      if (py + h * 0.4 < b.min.y || py - h * 0.4 > b.max.y) continue;
      if (b.walkable && b.max.y - b.min.y < 0.5 && Math.abs(pos.y - b.max.y) < 0.4) continue;
      const minX = b.min.x - r;
      const maxX = b.max.x + r;
      const minZ = b.min.z - r;
      const maxZ = b.max.z + r;
      if (pos.x > minX && pos.x < maxX && pos.z > minZ && pos.z < maxZ) {
        // Small step-up onto low ledges / fine stairs
        const top = b.max.y;
        if (top > pos.y + 0.02 && top <= pos.y + stepUp) {
          pos.y = top;
          continue;
        }
        if (axis === 'x') {
          const cx = (b.min.x + b.max.x) * 0.5;
          pos.x = pos.x < cx ? minX : maxX;
        } else {
          const cz = (b.min.z + b.max.z) * 0.5;
          pos.z = pos.z < cz ? minZ : maxZ;
        }
      }
    }
  }
}

export class AlienManager {
  constructor(scene, spawnPoints) {
    this.spawnPoints = spawnPoints;
    this.aliens = [];
    for (let i = 0; i < CONFIG.alien.count; i++) {
      this.aliens.push(new Alien(scene));
    }
  }

  farthestSpawn(playerPos) {
    let best = this.spawnPoints[0];
    let bestD = -1;
    for (let i = 0; i < this.spawnPoints.length; i++) {
      const p = this.spawnPoints[i];
      const d = p.distanceToSquared(playerPos);
      if (d > bestD) {
        bestD = d;
        best = p;
      }
    }
    return best.clone();
  }

  softReset(playerPos) {
    const now = performance.now() / 1000;
    for (let i = 0; i < this.aliens.length; i++) {
      const a = this.aliens[i];
      a.kill();
      a.respawnAt = now + i * 0.15;
    }
    for (let i = 0; i < this.aliens.length; i++) {
      this.aliens[i].spawnAt(this.farthestSpawn(playerPos));
      this.aliens[i].position.x += (Math.random() - 0.5) * 2;
      this.aliens[i].position.z += (Math.random() - 0.5) * 2;
    }
  }

  update(dt, player, solids, combat, half) {
    const now = performance.now() / 1000;
    for (let i = 0; i < this.aliens.length; i++) {
      const a = this.aliens[i];
      if (!a.alive) {
        if (a.respawnAt > 0 && now >= a.respawnAt) {
          a.spawnAt(this.farthestSpawn(player.position));
          a.position.x += (Math.random() - 0.5) * 1.5;
          a.position.z += (Math.random() - 0.5) * 1.5;
        }
        continue;
      }
      a.update(dt, player, solids, this.aliens, combat, half);
    }
  }

  getColliders() {
    const out = [];
    for (let i = 0; i < this.aliens.length; i++) {
      const a = this.aliens[i];
      if (!a.alive) continue;
      out.push({ mesh: a.body, kind: 'alien', alien: a });
      out.push({ mesh: a.headMesh, kind: 'alien', alien: a });
    }
    return out;
  }
}
