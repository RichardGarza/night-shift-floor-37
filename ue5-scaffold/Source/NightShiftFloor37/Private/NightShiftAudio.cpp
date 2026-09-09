#include "NightShiftAudio.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"

namespace
{
	TWeakObjectPtr<USoundAttenuation> GAttenuation;
}

USoundAttenuation* NightShiftAudio::GetAttenuation(const UObject* WorldContext)
{
	if (GAttenuation.IsValid())
	{
		return GAttenuation.Get();
	}
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}
	USoundAttenuation* Att = NewObject<USoundAttenuation>(World, TEXT("NightShiftRuntimeAttenuation"));
	Att->AddToRoot();
	Att->Attenuation.bAttenuate = true;
	Att->Attenuation.bSpatialize = true;
	Att->Attenuation.AttenuationShape = EAttenuationShape::Sphere;
	Att->Attenuation.AttenuationShapeExtents = FVector(400.f, 0.f, 0.f); // full volume inside 4 m
	Att->Attenuation.FalloffDistance = 3600.f;                            // silent by ~40 m (the arena is 50)
	Att->Attenuation.DistanceAlgorithm = EAttenuationDistanceModel::NaturalSound;
	Att->Attenuation.dBAttenuationAtMax = -48.f;
	GAttenuation = Att;
	return Att;
}

void NightShiftAudio::PlayAt(const UObject* WorldContext, USoundBase* Sound, const FVector& Location, float Volume, float PitchVariance)
{
	if (!Sound || !WorldContext || !WorldContext->GetWorld())
	{
		return;
	}
	const float Pitch = 1.f + FMath::FRandRange(-PitchVariance, PitchVariance);
	UGameplayStatics::PlaySoundAtLocation(WorldContext, Sound, Location, FRotator::ZeroRotator, Volume, Pitch, 0.f, GetAttenuation(WorldContext));
}

void NightShiftAudio::Play2D(const UObject* WorldContext, USoundBase* Sound, float Volume, float PitchVariance)
{
	if (!Sound || !WorldContext || !WorldContext->GetWorld())
	{
		return;
	}
	const float Pitch = 1.f + FMath::FRandRange(-PitchVariance, PitchVariance);
	UGameplayStatics::PlaySound2D(WorldContext, Sound, Volume, Pitch);
}

UAudioComponent* NightShiftAudio::StartLoop2D(const UObject* WorldContext, USoundBase* Sound, float Volume)
{
	if (!Sound || !WorldContext || !WorldContext->GetWorld())
	{
		return nullptr;
	}
	UAudioComponent* AC = UGameplayStatics::SpawnSound2D(WorldContext, Sound, Volume, 1.f, 0.f, nullptr, /*bPersistAcrossLevelTransition*/ false, /*bAutoDestroy*/ false);
	if (AC)
	{
		AC->bIsUISound = true; // keeps humming while paused / in menus
	}
	return AC;
}
