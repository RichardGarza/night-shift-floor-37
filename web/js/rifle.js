import { CONFIG } from './config.js';

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

    const { origin, dir } = this.player.getAimRay();
    // Prefer camera-forward for hitscan (OTS)
    const cam = this.player.camera;
    origin.copy(cam.position);
    cam.getWorldDirection(dir);

    const colliders = getColliders();
    const result = this.combat.hitscan(origin, dir, colliders);

    const muzzle = this.player.muzzleWorld;
    this.combat.spawnMuzzleFlash(muzzle);
    this.combat.spawnTracer(muzzle, result.point);

    if (result.alien && result.alien.alive) {
      const killed = result.alien.takeHit(result.isHead);
      this.combat.showHitMarker();
      if (killed && onKill) onKill(result.alien);
    }
  }
}
