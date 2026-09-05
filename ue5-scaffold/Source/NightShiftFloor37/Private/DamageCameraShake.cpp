#include "DamageCameraShake.h"
#include "Shakes/PerlinNoiseCameraShakePattern.h"

UDamageCameraShake::UDamageCameraShake(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bSingleInstance = true;
	UPerlinNoiseCameraShakePattern* Pattern = ObjectInitializer.CreateDefaultSubobject<UPerlinNoiseCameraShakePattern>(this, TEXT("Pattern"));
	Pattern->Duration = 0.25f;
	Pattern->BlendInTime = 0.02f;
	Pattern->BlendOutTime = 0.15f;
	Pattern->RotationAmplitudeMultiplier = 1.f;
	Pattern->RotationFrequencyMultiplier = 1.f;
	Pattern->Pitch.Amplitude = 2.0f;
	Pattern->Pitch.Frequency = 22.f;
	Pattern->Yaw.Amplitude = 1.4f;
	Pattern->Yaw.Frequency = 18.f;
	Pattern->Roll.Amplitude = 0.8f;
	Pattern->Roll.Frequency = 16.f;
	Pattern->LocationAmplitudeMultiplier = 0.f;
	SetRootShakePattern(Pattern);
}
