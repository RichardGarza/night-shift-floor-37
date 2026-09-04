/** Night Shift — Floor 37 — all tunables in one place */
export const CONFIG = Object.freeze({
  // World
  mapSize: 50,
  gravity: 15,
  maxDt: 0.05,

  // Player
  player: {
    height: 1.8,
    radius: 0.35,
    walkSpeed: 6,
    sprintSpeed: 9,
    jumpSpeed: 5,
    hpMax: 100,
    regenRate: 10,
    regenDelay: 5,
    fallDamageHeight: 6,
    fallDamagePerMeter: 12,
    eyeOffset: 1.55,
    stepUp: 0.55,
  },

  // Camera
  camera: {
    distance: 3.2,
    height: 1.65,
    shoulderOffset: 0.55,
    fov: 70,
    mouseSens: 0.0022,
    pitchMin: -1.2,
    pitchMax: 1.35,
    softLockConeDeg: 8,
    softLockRange: 40,
    softLockStrength: 0.08,
  },

  // Rifle
  rifle: {
    magSize: 30,
    reserve: 90,
    rpm: 600,
    // DESIGN kill remains 3 body / 2 head (hit-counts). Alien HP pools = hits×dmg so both stay live.
    bodyDmg: 25,
    headDmg: 50,
    reloadTime: 1.5,
    recoilPitch: 0.018,
    recoilYaw: 0.01,
    recoilRecover: 8,
    range: 80,
    muzzleOffset: { x: 0.25, y: 1.35, z: -0.6 },
  },

  // Combat feedback
  feedback: {
    flashMs: 80,
    tracerMs: 60,
    tracerPool: 24,
    muzzlePool: 8,
    muzzleLightIntensity: 4,
    muzzleLightDuration: 0.05,
    hitMarkerMs: 120,
    vignetteDecay: 2.5,
  },

  // Aliens
  alien: {
    count: 6,
    height: 1.7,
    radius: 0.4,
    speed: 4,
    engageRange: 12,
    burstShots: 3,
    burstInterval: 1.5,
    burstRpm: 480,
    accuracy: 0.3,
    damage: 10,
    bodyHitsToKill: 3,
    headHitsToKill: 2,
    respawnDelay: 3,
    pushRadius: 0.85,
    pushStrength: 6,
    color: 0x33ff88,
    headColor: 0xaaffcc,
    // Steering / anti-snag
    stuckSpeed: 0.4,
    stuckTime: 0.22,
    detourDuration: 0.55,
    detourBlend: 0.85,
    stepUp: 0.55,
  },

  // Match
  match: {
    winKills: 25,
  },

  // Arena visual
  arena: {
    floorColor: 0x1a1e1a,
    wallColor: 0x2a2e2c,
    accentGreen: 0x3a6b3a,
    accentAmber: 0xb8860b,
    fogColor: 0x0a120c,
    fogNear: 18,
    fogFar: 55,
    // Continuous ramp thickness / collision pad
    rampThickness: 0.28,
  },
});
