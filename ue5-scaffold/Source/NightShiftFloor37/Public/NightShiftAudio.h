// Night Shift — Floor 37 | Sprint AF: tiny code-only audio helpers (no Editor assets besides SoundWaves).
#pragma once

#include "CoreMinimal.h"

class USoundBase;
class USoundAttenuation;
class UAudioComponent;
class UObject;

namespace NightShiftAudio
{
	/** World-space one-shot with the shared runtime attenuation (radius ~4 m full, falls off to ~40 m). */
	void PlayAt(const UObject* WorldContext, USoundBase* Sound, const FVector& Location, float Volume = 1.f, float PitchVariance = 0.06f);
	/** Non-spatial one-shot (UI stings, player hurt, reload). */
	void Play2D(const UObject* WorldContext, USoundBase* Sound, float Volume = 1.f, float PitchVariance = 0.f);
	/** Looping non-spatial ambience; returns the component so the caller can stop it. */
	UAudioComponent* StartLoop2D(const UObject* WorldContext, USoundBase* Sound, float Volume = 1.f);
	/** Shared attenuation object (created once per world, GC-rooted). */
	USoundAttenuation* GetAttenuation(const UObject* WorldContext);
}
