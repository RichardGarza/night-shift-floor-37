import * as THREE from 'three';
import { CONFIG } from './config.js';
import { Player } from './player.js';
import { Rifle } from './rifle.js';
import { CombatSystem } from './combat.js';
import { buildArena } from './arena.js';
import { AlienManager } from './alien.js';
import { HUD } from './hud.js';

export class Game {
  constructor(canvas) {
    this.canvas = canvas;
    this.clock = new THREE.Clock();
    this.running = false;
    this.kills = 0;
    this.elapsed = 0;
    this.locked = false;
    /** After UI click / unpause / pointer-lock re-acquire: ignore fire until LMB up */
    this.suppressFireUntilUp = false;
    /** Safety frames so suppress cannot stick if pointer-lock swallows mouseup */
    this._suppressFrames = 0;

    this.renderer = new THREE.WebGLRenderer({
      canvas,
      antialias: true,
      powerPreference: 'high-performance',
    });
    this.renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 2));
    this.renderer.setSize(window.innerWidth, window.innerHeight);
    this.renderer.outputColorSpace = THREE.SRGBColorSpace;
    this.renderer.toneMapping = THREE.ACESFilmicToneMapping;
    this.renderer.toneMappingExposure = CONFIG.arena.toneMappingExposure ?? 1.7;
    this.renderer.shadowMap.enabled = false;

    this.scene = new THREE.Scene();

    const arena = buildArena(this.scene);
    this.solids = arena.solids;
    this.spawnPoints = arena.spawnPoints;
    this.half = arena.half;

    this.combat = new CombatSystem(this.scene);
    this.player = new Player(this.scene);
    this.rifle = new Rifle(this.player, this.combat);
    this.aliens = new AlienManager(this.scene, this.spawnPoints);
    this.hud = new HUD();

    // Hot-path: reused shot collider list + prebuilt world entries (no per-shot alloc)
    this._shotCols = [];
    this._worldShotCols = [];
    for (let i = 0; i < this.solids.length; i++) {
      const s = this.solids[i];
      if (!s.mesh || !s.blockXZ) continue;
      if (s.max.y - s.min.y < 0.8) continue;
      this._worldShotCols.push({ mesh: s.mesh, kind: 'world' });
    }

    this.input = {
      forward: false,
      back: false,
      left: false,
      right: false,
      sprint: false,
      jump: false,
      shoot: false,
      reload: false,
      dYaw: 0,
      dPitch: 0,
    };

    this._onResize = () => this._resize();
    this._onKeyDown = (e) => this._key(e, true);
    this._onKeyUp = (e) => this._key(e, false);
    this._onMouseMove = (e) => this._mouseMove(e);
    this._onMouseDown = (e) => this._mouseDown(e);
    this._onMouseUp = (e) => this._mouseUp(e);
    this._onPointerLockChange = () => this._lockChange();

    window.addEventListener('resize', this._onResize);
    window.addEventListener('keydown', this._onKeyDown);
    window.addEventListener('keyup', this._onKeyUp);
    document.addEventListener('mousemove', this._onMouseMove);
    document.addEventListener('mousedown', this._onMouseDown);
    document.addEventListener('mouseup', this._onMouseUp);
    document.addEventListener('pointerlockchange', this._onPointerLockChange);

    // mousedown + stopPropagation: UI primary-click never reaches combat mousedown
    this.hud.overlay.addEventListener('mousedown', (e) => {
      if (e.button !== 0) return;
      e.preventDefault();
      e.stopPropagation();
      this._armFireSuppress();
      this.requestPlay();
    });

    this._resize();
    this.softReset();
    this.hud.setMode('start');
    this._loop = this._loop.bind(this);
    requestAnimationFrame(this._loop);
  }

  _armFireSuppress() {
    this.suppressFireUntilUp = true;
    this._suppressFrames = 8;
    this.input.shoot = false;
  }

  requestPlay() {
    // Overlay / HUD primary-click must not also fire the rifle
    this._armFireSuppress();
    if (this.hud.mode === 'paused') {
      this.canvas.requestPointerLock?.();
      this.running = true;
      this.hud.setMode('playing');
      return;
    }
    this.canvas.requestPointerLock?.();
    this.softReset();
    this.running = true;
    this.hud.setMode('playing');
  }

  /** Pause sim + unlock pointer. Idempotent; no-op if not playing. */
  pause() {
    if (this.hud.mode !== 'playing') return;
    this.running = false;
    document.exitPointerLock?.();
    this.input.forward = false;
    this.input.back = false;
    this.input.left = false;
    this.input.right = false;
    this.input.sprint = false;
    this.input.jump = false;
    this.input.shoot = false;
    this.input.reload = false;
    this.hud.setMode('paused');
  }

  softReset() {
    this.kills = 0;
    this.elapsed = 0;
    this.player.reset(new THREE.Vector3(0, 0, 10));
    this.rifle.reset();
    this.aliens.softReset(this.player.position);
    this.combat.vignette = 0;
    // Clear held fire on reset
    this.input.shoot = false;
  }

  _resize() {
    const w = window.innerWidth;
    const h = window.innerHeight;
    this.renderer.setSize(w, h);
    this.player.camera.aspect = w / Math.max(1, h);
    this.player.camera.updateProjectionMatrix();
  }

  _key(e, down) {
    const k = e.code;
    if (k === 'KeyW') this.input.forward = down;
    else if (k === 'KeyS') this.input.back = down;
    else if (k === 'KeyA') this.input.left = down;
    else if (k === 'KeyD') this.input.right = down;
    else if (k === 'ShiftLeft' || k === 'ShiftRight') this.input.sprint = down;
    else if (k === 'Space') {
      if (down) this.input.jump = true;
      e.preventDefault();
    } else if (k === 'KeyR' && down) this.input.reload = true;
    else if (k === 'KeyQ' && down) {
      this.player.shoulder *= -1;
    } else if (k === 'Escape' && down) {
      if (this.hud.mode === 'playing') this.pause();
      else document.exitPointerLock?.();
    }
  }

  _mouseMove(e) {
    if (!this.locked) return;
    this.input.dYaw -= e.movementX * CONFIG.camera.mouseSens;
    this.input.dPitch -= e.movementY * CONFIG.camera.mouseSens;
  }

  _mouseDown(e) {
    if (e.button !== 0) return;
    // WaitingToStart / Won / Lost / paused: LMB is UI only — never combat fire
    if (this.hud.mode !== 'playing' || !this.running) return;
    if (this.suppressFireUntilUp) return;
    if (!this.locked) {
      // Re-acquire lock without treating this press as a shot
      this._armFireSuppress();
      this.canvas.requestPointerLock?.();
      return;
    }
    this.input.shoot = true;
  }

  _mouseUp(e) {
    if (e.button !== 0) return;
    this.input.shoot = false;
    this.suppressFireUntilUp = false;
    this._suppressFrames = 0;
  }

  _lockChange() {
    const wasLocked = this.locked;
    this.locked = document.pointerLockElement === this.canvas;
    if (this.locked && !wasLocked) {
      // Pointer-lock re-acquire must not trigger a shot
      this._armFireSuppress();
    }
    // Esc / OS unlock while playing should pause (running already false on dead/win)
    if (!this.locked && this.running && this.hud.mode === 'playing') {
      this.pause();
    }
  }

  _loop() {
    requestAnimationFrame(this._loop);
    let dt = this.clock.getDelta();
    if (dt > CONFIG.maxDt) dt = CONFIG.maxDt;

    // Consume UI/lock suppress: keep shoot cleared; drop flag after a few frames if mouseup was lost
    if (this.suppressFireUntilUp) {
      this.input.shoot = false;
      if (this._suppressFrames > 0) this._suppressFrames -= 1;
      else this.suppressFireUntilUp = false;
    }

    if (this.running && this.hud.mode === 'playing') {
      this.elapsed += dt;

      // Soft lock before movement camera update
      this.player.applySoftLock(this.aliens.aliens);

      this.player.update(dt, this.input, this.solids, this.half);

      this.rifle.update(
        dt,
        this.input,
        () => {
          const cols = this._shotCols;
          cols.length = 0;
          const aliens = this.aliens.getColliders();
          for (let i = 0; i < aliens.length; i++) cols.push(aliens[i]);
          // Block bullets with tall cover / walls (prebuilt; skip thin platforms)
          for (let i = 0; i < this._worldShotCols.length; i++) {
            cols.push(this._worldShotCols[i]);
          }
          return cols;
        },
        () => {
          this.kills += 1;
          if (this.kills >= CONFIG.match.winKills) {
            this.running = false;
            document.exitPointerLock?.();
            this.hud.setMode('win', { time: this.elapsed });
          }
        }
      );

      this.aliens.update(dt, this.player, this.solids, this.combat, this.half);

      if (!this.player.alive) {
        this.running = false;
        document.exitPointerLock?.();
        this.hud.setMode('dead');
      }
    } else {
      // Still update camera gently when paused
      this.player._updateCamera(dt);
    }

    const feedback = this.combat.update(dt);
    this.hud.update(this.player, this.rifle, this.kills, this.elapsed, feedback);

    this.renderer.render(this.scene, this.player.camera);
  }
}
