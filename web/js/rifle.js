import * as THREE from 'three';
import { CONFIG } from './config.js';

const _muzzleDir = new THREE.Vector3();

export class Rifle {
  constructor(player, combat) {
    this.player = player;
    this.combat = combat;
    this.mag = CONFIG.rifle.magSize;
    this.reserve = CONFIG.rifle.reserve;
    this.reloading = false;
    this.reloadT = 0;
    this.fireCooldown = 0;
    this.fireInterval = 60 / CONFIG.rifle.rpm;
  }

  reset() {
    this.mag = CONFIG.rifle.magSize;
    this.reserve = CONFIG.rifle.reserve;
    this.reloading = false;
    this.reloadT = 0;
    this.fireCooldown = 0;
  }

  startReload() {
    if (this.reloading) return;
    if (this.mag >= CONFIG.rifle.magSize) return;
    if (this.reserve <= 0) return;
    this.reloading = true;
    this.reloadT = CONFIG.rifle.reloadTime;
  }

  update(dt, input, getColliders, onKill) {
    if (this.reloading) {
      this.reloadT -= dt;
      if (this.reloadT <= 0) {
        const need = CONFIG.rifle.magSize - this.mag;
        const take = Math.min(need, this.reserve);
        this.mag += take;
        this.reserve -= take;
        this.reloading = false;
        this.reloadT = 0;
      }
    }

    if (this.fireCooldown > 0) this.fireCooldown -= dt;

    if (input.reload) {
      this.startReload();
      input.reload = false;
    }

    if (input.shoot && !this.reloading && this.player.alive) {
      this._tryFire(getColliders, onKill);
    }
  }

  _tryFire(getColliders, onKill) {
    if (this.fireCooldown > 0) return;
    if (this.mag <= 0) {
      this.startReload();
      return;
    }
    this.mag -= 1;
    this.fireCooldown = this.fireInterval;
    this.player.applyRecoil();

    // Aim ray: from the camera (reticle-aligned, OTS) along the recoil-kicked look direction.
    const { origin, dir } = this.player.getAimRay();
    origin.copy(this.player.camera.position);

    const colliders = getColliders();
    let result = this.combat.hitscan(origin, dir, colliders);

    // Re-trace from the muzzle to the camera hit: if geometry sits between the gun and that
    // point, the bullet stops there instead of landing around a corner the character cannot see past.
    const muzzle = this.player.muzzleWorld;
    _muzzleDir.copy(result.point).sub(muzzle);
    const muzzleDist = _muzzleDir.length();
    if (muzzleDist > 0.1) {
      _muzzleDir.multiplyScalar(1 / muzzleDist);
      const check = this.combat.hitscan(muzzle, _muzzleDir, colliders, muzzleDist - 0.05);
      if (check.hit) result = check;
    }

    this.combat.spawnMuzzleFlash(muzzle);
    this.combat.spawnTracer(muzzle, result.point);

    if (result.alien && result.alien.alive) {
      const killed = result.alien.takeHit(result.isHead);
      this.combat.showHitMarker();
      if (killed && onKill) onKill(result.alien);
    }
  }
}
