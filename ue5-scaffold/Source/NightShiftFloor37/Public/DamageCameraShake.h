// Night Shift — Floor 37 | Short perlin shake played when the player takes damage
#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraShakeBase.h"
#include "DamageCameraShake.generated.h"

/** ~0.25 s pitch/yaw/roll wobble. Scaled by UGameConfig::CameraShakeScale at play time. */
UCLASS()
class NIGHTSHIFTFLOOR37_API UDamageCameraShake : public UCameraShakeBase
{
	GENERATED_BODY()
public:
	UDamageCameraShake(const FObjectInitializer& ObjectInitializer);
};
