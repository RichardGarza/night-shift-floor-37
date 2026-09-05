import * as THREE from 'three';
import { CONFIG } from './config.js';
import { overlapsSolidXZ, resolveSolidAxis, rampHeightAt } from './collision.js';

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
    emissiveIntensity: 1.25,
  });
  _headMat = new THREE.MeshStandardMaterial({
    color: CONFIG.alien.headColor,
    roughness: 0.4,
    metalness: 0.15,
    emissive: CONFIG.alien.headColor,
    emissiveIntensity: 1.25,
  });
  _flashMat = new THREE.MeshBasicMaterial({ color: 0xffffff });
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
    // Cosmetic only — hit volume is top 25% of body capsule (see combat.js)
    this.headMesh.raycast = () => {};
    this.root.add(this.headMesh);

    this.root.visible = false;
    scene.add(this.root);

    this.alive = false;
    this.bodyHitsLeft = 0;
    this.headHitsLeft = 0;
    this.velocity = new THREE.Vector3();
    this.strafeDir = 1;
    this.burstLeft = 0;
    this.burstCd = 0;
    this.shotCd = 0;
    this.flashT = 0;
    this.respawnTimer = 0;
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
    // Independent hit counters: DESIGN kill = 3 body OR 2 headshots (mixed do not combine)
    this.bodyHitsLeft = CONFIG.alien.bodyHitsToKill;
    this.headHitsLeft = CONFIG.alien.headHitsToKill;
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
    this.respawnTimer = CONFIG.alien.respawnDelay;
  }

  /** @returns true if killed */
  takeHit(isHead) {
    if (!this.alive) return false;
    if (isHead) this.headHitsLeft -= 1;
    else this.bodyHitsLeft -= 1;
    this.flashT = CONFIG.feedback.flashMs / 1000;
    this.body.material = _flashMat;
    this.headMesh.material = _flashMat;
    // Kill if either counter reaches 0 (DESIGN: 3 body OR 2 head; independent pools)
    if (this.bodyHitsLeft <= 0 || this.headHitsLeft <= 0) {
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
          // Per-shot LoS: if broken mid-burst, abort and let chase take over next frame
          if (!this._hasLos(player, solids)) {
            this.burstLeft = 0;
          } else {
            this._fireAt(player, combat, solids);
            this.burstLeft -= 1;
            this.shotCd = 60 / CONFIG.alien.burstRpm;
          }
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
    const maxFeetY = CONFIG.arena.ceilingHeight - CONFIG.alien.height;
    if (pos.y > maxFeetY) pos.y = maxFeetY;

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
      if (!b.blockXZ || b.ramp || b.ceiling) continue;
      if (py + h * 0.35 < b.min.y || py - h * 0.35 > b.max.y) continue;
      if (b.walkable && b.max.y - b.min.y < 0.5 && Math.abs(y - b.max.y) < 0.45) continue;
      if (overlapsSolidXZ(b, x, z, r)) return true;
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
        if (!b.blockXZ || b.ramp || b.ceiling) continue;
        // Ignore only knee-high solids; cubicle panels (1.15 m) must block alien fire
        if (b.max.y - b.min.y < 1.0) continue;
        if (y > b.min.y && y < b.max.y && overlapsSolidXZ(b, x, z, 0)) {
          return false;
        }
      }
    }
    return true;
  }

  _fireAt(player, combat, solids) {
    if (!player.alive) return;
    // Engage fire only with line of sight (belt-and-suspenders with burst loop)
    if (solids && !this._hasLos(player, solids)) return;
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
      if (!b.blockXZ || b.ramp || b.ceiling) continue;
      const py = pos.y + h * 0.5;
      if (py + h * 0.4 < b.min.y || py - h * 0.4 > b.max.y) continue;
      if (b.walkable && b.max.y - b.min.y < 0.5 && Math.abs(pos.y - b.max.y) < 0.4) continue;
      if (!overlapsSolidXZ(b, pos.x, pos.z, r)) continue;
      // Small step-up onto low ledges / fine stairs
      const top = b.max.y;
      if (top > pos.y + 0.02 && top <= pos.y + stepUp) {
        pos.y = top;
        continue;
      }
      resolveSolidAxis(pos, r, b, axis);
    }
  }
}

export class AlienManager {
  constructor(scene, spawnPoints) {
    this.spawnPoints = spawnPoints;
    this.aliens = [];
    /** Reused each getColliders() call — avoid per-shot array alloc */
    this._colliderBuf = [];
    /** Reused spawn-index scratch list */
    this._usedBuf = [];
    for (let i = 0; i < CONFIG.alien.count; i++) {
      this.aliens.push(new Alien(scene));
    }
  }

  /** Index of the spawn point farthest from playerPos, skipping indices in `used` (any point if all used). */
  _pickSpawnIndex(playerPos, used) {
    let best = -1;
    let bestD = -1;
    for (let pass = 0; pass < 2 && best < 0; pass++) {
      for (let i = 0; i < this.spawnPoints.length; i++) {
        if (pass === 0 && used.indexOf(i) !== -1) continue;
        const d = this.spawnPoints[i].distanceToSquared(playerPos);
        if (d > bestD) {
          bestD = d;
          best = i;
        }
      }
    }
    return best;
  }

  /** Spawn indices with a living alien within 3 m, so single respawns do not stack on a corner. */
  _occupiedSpawns(out) {
    out.length = 0;
    for (let i = 0; i < this.spawnPoints.length; i++) {
      const p = this.spawnPoints[i];
      for (let j = 0; j < this.aliens.length; j++) {
        const a = this.aliens[j];
        if (a.alive && a.position.distanceToSquared(p) < 9) {
          out.push(i);
          break;
        }
      }
    }
    return out;
  }

  _spawnAlienAt(a, idx, jitter) {
    a.spawnAt(this.spawnPoints[idx]);
    a.position.x += (Math.random() - 0.5) * jitter;
    a.position.z += (Math.random() - 0.5) * jitter;
  }

  softReset(playerPos) {
    // Spread the batch across distinct spawn points (farthest-first, no repeats).
    const used = this._usedBuf;
    used.length = 0;
    for (let i = 0; i < this.aliens.length; i++) {
      const a = this.aliens[i];
      a.kill();
      const idx = this._pickSpawnIndex(playerPos, used);
      used.push(idx);
      this._spawnAlienAt(a, idx, 2);
    }
  }

  update(dt, player, solids, combat, half) {
    for (let i = 0; i < this.aliens.length; i++) {
      const a = this.aliens[i];
      if (!a.alive) {
        // dt-driven respawn timer: a pause no longer respawns every dead alien on unpause
        if (a.respawnTimer > 0) {
          a.respawnTimer -= dt;
          if (a.respawnTimer <= 0) {
            a.respawnTimer = 0;
            const idx = this._pickSpawnIndex(player.position, this._occupiedSpawns(this._usedBuf));
            this._spawnAlienAt(a, idx, 1.5);
          }
        }
        continue;
      }
      a.update(dt, player, solids, this.aliens, combat, half);
    }
  }

  getColliders() {
    const out = this._colliderBuf;
    out.length = 0;
    for (let i = 0; i < this.aliens.length; i++) {
      const a = this.aliens[i];
      if (!a.alive) continue;
      // Body capsule only; head mesh is visual-only (isHead via Y threshold)
      out.push({ mesh: a.body, kind: 'alien', alien: a });
    }
    return out;
  }
}
