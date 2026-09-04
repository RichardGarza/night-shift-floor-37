import { CONFIG } from './config.js';

export class HUD {
  constructor() {
    this.hpBar = document.getElementById('hp-bar');
    this.hpNum = document.getElementById('hp-num');
    this.mag = document.getElementById('mag');
    this.reserve = document.getElementById('reserve');
    this.kills = document.getElementById('kills');
    this.timer = document.getElementById('timer');
    this.crosshair = document.getElementById('crosshair');
    this.vignette = document.getElementById('vignette');
    this.overlay = document.getElementById('overlay');
    this.overlayHint = document.getElementById('overlay-hint');
    this.overlayTitle = this.overlay.querySelector('h1');
    this._mode = 'start'; // start | playing | dead | win
  }

  setMode(mode, extra = {}) {
    this._mode = mode;
    if (mode === 'playing') {
      this.overlay.classList.add('hidden');
      return;
    }
    this.overlay.classList.remove('hidden');
    if (mode === 'start') {
      this.overlayTitle.textContent = 'Night Shift — Floor 37';
      this.overlayHint.textContent = 'Click to play';
    } else if (mode === 'dead') {
      this.overlayTitle.textContent = 'You died';
      this.overlayHint.textContent = 'Click to restart';
    } else if (mode === 'win') {
      this.overlayTitle.textContent = 'Floor cleared';
      const t = extra.time || 0;
      this.overlayHint.textContent = `25 kills in ${formatTime(t)} — click to play again`;
    }
  }

  get mode() {
    return this._mode;
  }

  update(player, rifle, kills, elapsed, feedback) {
    const hpPct = Math.max(0, player.hp / CONFIG.player.hpMax) * 100;
    this.hpBar.style.width = hpPct + '%';
    this.hpNum.textContent = Math.ceil(player.hp);
    this.mag.textContent = rifle.mag;
    this.reserve.textContent = rifle.reserve;
    this.kills.textContent = kills;
    this.timer.textContent = formatTime(elapsed);

    if (feedback.hitMarker) this.crosshair.classList.add('hit');
    else this.crosshair.classList.remove('hit');

    this.vignette.style.opacity = String(feedback.vignette || 0);
  }
}

function formatTime(sec) {
  const s = Math.floor(sec);
  const m = Math.floor(s / 60);
  const r = s % 60;
  return m + ':' + String(r).padStart(2, '0');
}
