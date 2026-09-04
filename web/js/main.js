import { Game } from './game.js';

const canvas = document.getElementById('game-canvas');
if (!canvas) {
  console.error('Missing #game-canvas');
} else {
  // eslint-disable-next-line no-new
  new Game(canvas);
}
