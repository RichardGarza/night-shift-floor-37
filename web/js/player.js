import * as THREE from 'three';
import { CONFIG } from './config.js';
import { overlapsSolidXZ, resolveSolidAxis, rampHeightAt } from './collision.js';

const _fwd = new THREE.Vector3();
const _right = new THREE.Vector3();
const _wish = new THREE.Vector3();
const _tmp = new THREE.Vector3();
const _eye = new THREE.Vector3();
const _muzzle = new THREE.Vector3();
const _aimOrigin = new THREE.Vector3();
const _aimDir = new THREE.Vector3();
const _back = new THREE.Vector3();
const _camRight = new THREE.Vector3();
const _aimRay = { origin: _aimOrigin, dir: _aimDir };


export class Player {
  constructor(scene) {
    this.scene = scene;
    this.yaw = 0;
    this.pitch = 0;
    this.shoulder = 1; // 1 = right, -1 = left
    this.velocity = new THREE.Vector3();
    this.onGround = true;
    this.hp = CONFIG.player.hpMax;
    this.timeSinceDamage = Infinity;
    this.alive = true;
    this.airborneFromY = 0;
    this.shake = 0;

    // Capsule-ish mesh for 3rd person readability
    this.root = new THREE.Group();
    this.root.position.set(0, 0, 8);

    const bodyMat = new THREE.MeshStandardMaterial({
      color: 0x3a4a55,
      roughness: 0.7,
      metalness: 0.15,
    });
    const accentMat = new THREE.MeshStandardMaterial({
      color: 0x6a8a70,
      roughness: 0.5,
      metalness: 0.2,
      emissive: 0x112211,
    });

    const torso = new THREE.Mesh(
      new THREE.CapsuleGeometry(CONFIG.player.radius, CONFIG.player.height - CONFIG.player.radius * 2, 4, 8),
      bodyMat
    );
    torso.position.y = CONFIG.player.height * 0.5;
    torso.castShadow = true;
    this.root.add(torso);

    const head = new THREE.Mesh(new THREE.SphereGeometry(0.22, 10, 8), accentMat);
    head.position.y = CONFIG.player.height - 0.15;
    this.root.add(head);

    // Simple rifle prop on right shoulder
    const gun = new THREE.Mesh(
      new THREE.BoxGeometry(0.12, 0.14, 0.7),
      new THREE.MeshStandardMaterial({ color: 0x22282a, roughness: 0.4, metalness: 0.6 })
    );
    gun.position.set(0.28, 1.2, -0.25);
    this.root.add(gun);
    this.gunMesh = gun;

    scene.add(this.root);

    this.camera = new THREE.PerspectiveCamera(
      CONFIG.camera.fov,
      1,
      0.1,
      120
    );
    this._camPos = new THREE.Vector3();
    this._lookAt = new THREE.Vector3();
    this.recoilPitch = 0;
    this.recoilYaw = 0;
  }

  get position() {
    return this.root.position;
  }

  get muzzleWorld() {
    const m = CONFIG.rifle.muzzleOffset;
    _muzzle.set(m.x * this.shoulder, m.y, m.z);
    _muzzle.applyAxisAngle(_up, this.yaw);
    _muzzle.add(this.root.position);
    return _muzzle;
  }

  reset(spawnPos) {
    this.root.position.copy(spawnPos || new THREE.Vector3(0, 0, 8));
    this.velocity.set(0, 0, 0);
    this.yaw = Math.PI;
    this.pitch = 0;
    this.hp = CONFIG.player.hpMax;
    this.timeSinceDamage = Infinity;
    this.alive = true;
    this.onGround = true;
    this.airborneFromY = 0;
    this.recoilPitch = 0;
    this.recoilYaw = 0;
    this.shake = 0;
    this.root.visible = true;
  }

  takeDamage(amount) {
    if (!this.alive) return;
    this.hp = Math.max(0, this.hp - amount);
    this.timeSinceDamage = 0;
    this.shake = Math.min(0.35, this.shake + 0.12);
    if (this.hp <= 0) {
      this.alive = false;
      this.hp = 0;
    }
  }

  applyRecoil() {
    this.recoilPitch += CONFIG.rifle.recoilPitch * (0.7 + Math.random() * 0.6);
    this.recoilYaw += (Math.random() - 0.5) * 2 * CONFIG.rifle.recoilYaw;
  }

  /** Soft-lock: bias pitch/yaw toward nearest alien in cone */
  applySoftLock(aliens, dt, strength = CONFIG.camera.softLockStrength) {
    const cone = (CONFIG.camera.softLockConeDeg * Math.PI) / 180;
    let best = null;
    let bestAng = cone;
    const eye = this._eyePoint();
    _fwd.set(-Math.sin(this.yaw) * Math.cos(this.pitch), Math.sin(this.pitch), -Math.cos(this.yaw) * Math.cos(this.pitch));
    for (let i = 0; i < aliens.length; i++) {
      const a = aliens[i];
      if (!a.alive) continue;
      _tmp.copy(a.aimPoint).sub(eye);
      const dist = _tmp.length();
      if (dist > CONFIG.camera.softLockRange || dist < 0.5) continue;
      _tmp.normalize();
      const ang = Math.acos(Math.min(1, Math.max(-1, _fwd.dot(_tmp))));
      if (ang < bestAng) {
        bestAng = ang;
        best = a;
      }
    }
    if (!best) return;
    _tmp.copy(best.aimPoint).sub(eye).normalize();
    const targetYaw = Math.atan2(-_tmp.x, -_tmp.z);
    const targetPitch = Math.asin(Math.max(-1, Math.min(1, _tmp.y)));
    let dy = targetYaw - this.yaw;
    while (dy > Math.PI) dy -= Math.PI * 2;
    while (dy < -Math.PI) dy += Math.PI * 2;
    // `strength` is the per-frame blend at 60 Hz; rescale by dt so aim assist is framerate-independent
    const k = 1 - Math.pow(1 - strength, dt * 60);
    this.yaw += dy * k;
    this.pitch += (targetPitch - this.pitch) * k;
  }

  _eyePoint() {
    return _eye.set(
      this.root.position.x,
      this.root.position.y + CONFIG.player.eyeOffset,
      this.root.position.z
    );
  }

  getAimRay() {
    _aimOrigin.set(
      this.root.position.x,
      this.root.position.y + CONFIG.player.eyeOffset,
      this.root.position.z
    );
    // Slight shoulder offset for OTS feel on aim origin (camera-based)
    const pitch = this.pitch + this.recoilPitch;
    const yaw = this.yaw + this.recoilYaw;
    _aimDir.set(
      -Math.sin(yaw) * Math.cos(pitch),
      Math.sin(pitch),
      -Math.cos(yaw) * Math.cos(pitch)
    ).normalize();
    return _aimRay;
  }

  update(dt, input, solids, half) {
    if (!this.alive) {
      this._updateCamera(dt);
      return;
    }

    // Mouse look already applied via input.dYaw / dPitch each frame
    this.yaw += input.dYaw;
    this.pitch = Math.max(
      CONFIG.camera.pitchMin,
      Math.min(CONFIG.camera.pitchMax, this.pitch + input.dPitch)
    );
    input.dYaw = 0;
    input.dPitch = 0;

    // Recoil recover
    const rr = CONFIG.rifle.recoilRecover * dt;
    this.recoilPitch *= Math.max(0, 1 - rr);
    this.recoilYaw *= Math.max(0, 1 - rr);

    // Movement relative to yaw
    _fwd.set(-Math.sin(this.yaw), 0, -Math.cos(this.yaw));
    _right.set(Math.cos(this.yaw), 0, -Math.sin(this.yaw));
    _wish.set(0, 0, 0);
    if (input.forward) _wish.add(_fwd);
    if (input.back) _wish.sub(_fwd);
    if (input.right) _wish.add(_right);
    if (input.left) _wish.sub(_right);
    if (_wish.lengthSq() > 0) _wish.normalize();

    const speed = input.sprint && (input.forward || input.back || input.left || input.right)
      ? CONFIG.player.sprintSpeed
      : CONFIG.player.walkSpeed;

    this.velocity.x = _wish.x * speed;
    this.velocity.z = _wish.z * speed;

    // Jump / gravity
    if (this.onGround && input.jump) {
      this.velocity.y = CONFIG.player.jumpSpeed;
      this.onGround = false;
      this.airborneFromY = this.root.position.y;
      input.jump = false;
    }
    if (!this.onGround) {
      this.velocity.y -= CONFIG.gravity * dt;
    }

    // Integrate with simple AABB collision vs solids
    const pos = this.root.position;
    const r = CONFIG.player.radius;
    const h = CONFIG.player.height;

    // X
    pos.x += this.velocity.x * dt;
    this._resolveXZ(pos, r, solids, 'x');
    // Z
    pos.z += this.velocity.z * dt;
    this._resolveXZ(pos, r, solids, 'z');
    // Y
    pos.y += this.velocity.y * dt;

    // Ground / platforms
    const groundY = this._groundHeight(pos, r, solids);
    if (pos.y <= groundY && this.velocity.y <= 0) {
      // Fall damage
      if (!this.onGround) {
        const drop = this.airborneFromY - pos.y;
        if (drop > CONFIG.player.fallDamageHeight) {
          const dmg = (drop - CONFIG.player.fallDamageHeight) * CONFIG.player.fallDamagePerMeter;
          this.takeDamage(dmg);
        }
      }
      pos.y = groundY;
      this.velocity.y = 0;
      this.onGround = true;
      this.airborneFromY = pos.y;
    } else if (pos.y > groundY + 0.05) {
      if (this.onGround) {
        this.onGround = false;
        this.airborneFromY = pos.y;
      }
    }

    // Invisible bounds (XZ walls + ceiling)
    const lim = half - r - 0.2;
    pos.x = Math.max(-lim, Math.min(lim, pos.x));
    pos.z = Math.max(-lim, Math.min(lim, pos.z));
    const maxFeetY = CONFIG.arena.ceilingHeight - h;
    if (pos.y > maxFeetY) {
      pos.y = maxFeetY;
      if (this.velocity.y > 0) this.velocity.y = 0;
    }
    if (pos.y < -2) {
      this.takeDamage(100);
    }

    // Face movement / aim yaw
    this.root.rotation.y = this.yaw;

    // Regen — dt-driven so a pause cannot silently complete the delay
    this.timeSinceDamage += dt;
    if (this.hp < CONFIG.player.hpMax && this.timeSinceDamage >= CONFIG.player.regenDelay) {
      this.hp = Math.min(CONFIG.player.hpMax, this.hp + CONFIG.player.regenRate * dt);
    }

    if (this.shake > 0) this.shake = Math.max(0, this.shake - dt * 2.5);

    this._updateCamera(dt);
  }

  _resolveXZ(pos, r, solids, axis) {
    const stepUp = CONFIG.player.stepUp;
    for (let i = 0; i < solids.length; i++) {
      const b = solids[i];
      if (!b.blockXZ || b.ramp || b.ceiling) continue;
      // Only collide if feet/body overlaps box in Y
      const py = pos.y + CONFIG.player.height * 0.5;
      if (py + CONFIG.player.height * 0.45 < b.min.y || py - CONFIG.player.height * 0.45 > b.max.y) continue;
      if (!overlapsSolidXZ(b, pos.x, pos.z, r)) continue;
      const top = b.max.y;
      if (this.onGround && top > pos.y + 0.02 && top <= pos.y + stepUp) {
        pos.y = top;
        continue;
      }
      resolveSolidAxis(pos, r, b, axis);
    }
  }

  _groundHeight(pos, r, solids) {
    let y = 0;
    const stepUp = CONFIG.player.stepUp;
    for (let i = 0; i < solids.length; i++) {
      const b = solids[i];
      if (b.ramp) {
        const top = rampHeightAt(b, pos.x, pos.z);
        if (top == null) continue;
        if (pos.y >= top - stepUp && pos.y <= top + 0.7 && this.velocity.y <= 0.8) {
          if (top > y) y = top;
        }
        continue;
      }
      if (!b.walkable) continue;
      // Feet within XZ of top surface
      if (
        pos.x + r > b.min.x &&
        pos.x - r < b.max.x &&
        pos.z + r > b.min.z &&
        pos.z - r < b.max.z
      ) {
        // Standing on top if close enough from above
        const top = b.max.y;
        if (pos.y >= top - stepUp && pos.y <= top + 0.7 && this.velocity.y <= 0.8) {
          if (top > y) y = top;
        }
      }
    }
    return y;
  }

  _updateCamera(dt) {
    const shoulder = CONFIG.camera.shoulderOffset * this.shoulder;
    const dist = CONFIG.camera.distance;
    const height = CONFIG.camera.height;
    const pitch = this.pitch + this.recoilPitch;
    const yaw = this.yaw + this.recoilYaw;

    // Camera behind and to the side
    const cosP = Math.cos(pitch);
    const sinP = Math.sin(pitch);
    _back.set(
      Math.sin(yaw) * cosP,
      -sinP,
      Math.cos(yaw) * cosP
    );
    _camRight.set(Math.cos(yaw), 0, -Math.sin(yaw));

    const target = this.root.position;
    this._camPos.set(
      target.x + _back.x * dist + _camRight.x * shoulder,
      target.y + height + _back.y * dist,
      target.z + _back.z * dist + _camRight.z * shoulder
    );

    // Shake
    if (this.shake > 0) {
      this._camPos.x += (Math.random() - 0.5) * this.shake;
      this._camPos.y += (Math.random() - 0.5) * this.shake;
    }

    this.camera.position.copy(this._camPos);
    this._lookAt.set(
      target.x - Math.sin(yaw) * Math.cos(pitch) * 8 + _camRight.x * shoulder * 0.3,
      target.y + CONFIG.player.eyeOffset + Math.sin(pitch) * 8,
      target.z - Math.cos(yaw) * Math.cos(pitch) * 8 + _camRight.z * shoulder * 0.3
    );
    this.camera.lookAt(this._lookAt);
  }
}

const _up = new THREE.Vector3(0, 1, 0);
