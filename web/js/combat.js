import * as THREE from 'three';
import { CONFIG } from './config.js';

const _origin = new THREE.Vector3();
const _dir = new THREE.Vector3();
const _end = new THREE.Vector3();
const _n = new THREE.Vector3();

/** Shared hitscan + pooled tracers / muzzle flashes / hit feedback */
export class CombatSystem {
  constructor(scene) {
    this.scene = scene;
    this.raycaster = new THREE.Raycaster();
    this.tracers = [];
    this.muzzles = [];
    this.hitMarkerUntil = 0;
    this.vignette = 0;
    this._tracerGeo = new THREE.BufferGeometry();
    this._tracerMat = new THREE.LineBasicMaterial({
      color: 0xffeeaa,
      transparent: true,
      opacity: 0.9,
      depthWrite: false,
    });
    this._initPools();
  }

  _initPools() {
    const pos = new Float32Array(6);
    for (let i = 0; i < CONFIG.feedback.tracerPool; i++) {
      const geo = new THREE.BufferGeometry();
      geo.setAttribute('position', new THREE.BufferAttribute(pos.slice(), 3));
      const line = new THREE.Line(geo, this._tracerMat.clone());
      line.visible = false;
      line.frustumCulled = false;
      this.scene.add(line);
      this.tracers.push({ mesh: line, t: 0 });
    }
    for (let i = 0; i < CONFIG.feedback.muzzlePool; i++) {
      const light = new THREE.PointLight(0xffcc66, 0, 6, 2);
      light.visible = false;
      this.scene.add(light);
      this.muzzles.push({ light, t: 0 });
    }
  }

  spawnTracer(from, to) {
    for (let i = 0; i < this.tracers.length; i++) {
      const tr = this.tracers[i];
      if (tr.t > 0) continue;
      const arr = tr.mesh.geometry.attributes.position.array;
      arr[0] = from.x; arr[1] = from.y; arr[2] = from.z;
      arr[3] = to.x; arr[4] = to.y; arr[5] = to.z;
      tr.mesh.geometry.attributes.position.needsUpdate = true;
      tr.mesh.visible = true;
      tr.mesh.material.opacity = 0.95;
      tr.t = CONFIG.feedback.tracerMs / 1000;
      return;
    }
  }

  spawnMuzzleFlash(pos) {
    for (let i = 0; i < this.muzzles.length; i++) {
      const m = this.muzzles[i];
      if (m.t > 0) continue;
      m.light.position.copy(pos);
      m.light.intensity = CONFIG.feedback.muzzleLightIntensity;
      m.light.visible = true;
      m.t = CONFIG.feedback.muzzleLightDuration;
      return;
    }
  }

  showHitMarker() {
    this.hitMarkerUntil = performance.now() + CONFIG.feedback.hitMarkerMs;
  }

  addDamageVignette(amount = 0.7) {
    this.vignette = Math.min(1, this.vignette + amount);
  }

  /**
   * Hitscan from camera through aim. Returns { hit, point, alien, isHead } or null.
   * colliders: array of { mesh, kind: 'alien'|'world', alien? }
   */
  hitscan(origin, direction, colliders, range = CONFIG.rifle.range) {
    this.raycaster.set(origin, direction);
    this.raycaster.far = range;
    const meshes = [];
    for (let i = 0; i < colliders.length; i++) {
      if (colliders[i].mesh.visible) meshes.push(colliders[i].mesh);
    }
    const hits = this.raycaster.intersectObjects(meshes, false);
    if (!hits.length) {
      _end.copy(origin).addScaledVector(direction, range);
      return { hit: false, point: _end.clone(), alien: null, isHead: false };
    }
    const h = hits[0];
    let alien = null;
    let isHead = false;
    for (let i = 0; i < colliders.length; i++) {
      const c = colliders[i];
      if (c.mesh === h.object || (c.mesh.children && c.mesh.children.includes(h.object))) {
        if (c.kind === 'alien') {
          alien = c.alien;
          // Head = top 25% of capsule height
          const localY = h.point.y - alien.position.y;
          isHead = localY >= CONFIG.alien.height * 0.75;
        }
        break;
      }
      // Also check userData
      if (h.object.userData && h.object.userData.alienRef) {
        alien = h.object.userData.alienRef;
        const localY = h.point.y - alien.position.y;
        isHead = localY >= CONFIG.alien.height * 0.75;
        break;
      }
    }
    // Walk parent chain for alien ref
    if (!alien) {
      let o = h.object;
      while (o) {
        if (o.userData && o.userData.alienRef) {
          alien = o.userData.alienRef;
          const localY = h.point.y - alien.position.y;
          isHead = localY >= CONFIG.alien.height * 0.75;
          break;
        }
        o = o.parent;
      }
    }
    return { hit: true, point: h.point.clone(), alien, isHead, distance: h.distance };
  }

  update(dt) {
    const now = performance.now();
    for (let i = 0; i < this.tracers.length; i++) {
      const tr = this.tracers[i];
      if (tr.t <= 0) continue;
      tr.t -= dt;
      if (tr.t <= 0) {
        tr.t = 0;
        tr.mesh.visible = false;
      } else {
        tr.mesh.material.opacity = Math.max(0, tr.t / (CONFIG.feedback.tracerMs / 1000));
      }
    }
    for (let i = 0; i < this.muzzles.length; i++) {
      const m = this.muzzles[i];
      if (m.t <= 0) continue;
      m.t -= dt;
      if (m.t <= 0) {
        m.t = 0;
        m.light.intensity = 0;
        m.light.visible = false;
      } else {
        m.light.intensity = CONFIG.feedback.muzzleLightIntensity * (m.t / CONFIG.feedback.muzzleLightDuration);
      }
    }
    if (this.vignette > 0) {
      this.vignette = Math.max(0, this.vignette - CONFIG.feedback.vignetteDecay * dt);
    }
    return {
      hitMarker: now < this.hitMarkerUntil,
      vignette: this.vignette,
    };
  }
}

export { _origin, _dir, _end, _n };
