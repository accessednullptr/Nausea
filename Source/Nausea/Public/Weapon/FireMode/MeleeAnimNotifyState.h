// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "MeleeAnimNotifyState.generated.h"

USTRUCT(BlueprintType)
struct FMeleeHitbox
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditDefaultsOnly)
	float something;
	UPROPERTY(EditDefaultsOnly)
	FVector someotherthing;
};

/**
 * 
 */
UCLASS()
class NAUSEA_API UMeleeAnimNotifyState : public UAnimNotifyState
{
	GENERATED_UCLASS_BODY()
	
//~ Begin UAnimNotifyState Interface
public:
	virtual void NotifyBegin(USkeletalMeshComponent * MeshComp, UAnimSequenceBase * Animation, float TotalDuration) override;
	virtual void NotifyTick(USkeletalMeshComponent * MeshComp, UAnimSequenceBase * Animation, float FrameDeltaTime) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
//~ End UAnimNotifyState Interface


protected:
	UFUNCTION()
	void SendMeleeHitBox(USkeletalMeshComponent* MeshComp);

};
