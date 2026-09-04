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

    this.renderer = new THREE.WebGLRenderer({
      canvas,
      antialias: true,
      powerPreference: 'high-performance',
    });
    this.renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 2));
    this.renderer.setSize(window.innerWidth, window.innerHeight);
    this.renderer.outputColorSpace = THREE.SRGBColorSpace;
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

    this.hud.overlay.addEventListener('click', () => this.requestPlay());

    this._resize();
    this.softReset();
    this.hud.setMode('start');
    this._loop = this._loop.bind(this);
    requestAnimationFrame(this._loop);
  }

  requestPlay() {
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
    if (e.button === 0) {
      if (!this.locked && this.hud.mode === 'playing') {
        this.canvas.requestPointerLock?.();
      }
      this.input.shoot = true;
    }
  }

  _mouseUp(e) {
    if (e.button === 0) this.input.shoot = false;
  }

  _lockChange() {
    this.locked = document.pointerLockElement === this.canvas;
    // Esc / OS unlock while playing should pause (running already false on dead/win)
    if (!this.locked && this.running && this.hud.mode === 'playing') {
      this.pause();
    }
  }

  _loop() {
    requestAnimationFrame(this._loop);
    let dt = this.clock.getDelta();
    if (dt > CONFIG.maxDt) dt = CONFIG.maxDt;

    if (this.running && this.hud.mode === 'playing') {
      this.elapsed += dt;

      // Soft lock before movement camera update
      this.player.applySoftLock(this.aliens.aliens);

      this.player.update(dt, this.input, this.solids, this.half);

      this.rifle.update(
        dt,
        this.input,
        () => {
          const cols = this.aliens.getColliders();
          // Block bullets with tall cover / walls (skip thin walkable platforms)
          for (let i = 0; i < this.solids.length; i++) {
            const s = this.solids[i];
            if (!s.mesh || !s.blockXZ) continue;
            if (s.max.y - s.min.y < 0.8) continue;
            cols.push({ mesh: s.mesh, kind: 'world' });
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
