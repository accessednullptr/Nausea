// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/FireMode/WeaponFireMode.h"
#include "MeleeFireMode.generated.h"

class UWeaponAnimInstance;

/**
 * 
 */
UCLASS()
class NAUSEA_API UMeleeFireMode : public UWeaponFireMode
{
	GENERATED_UCLASS_BODY()
	
//~ Begin UFireMode Interface 
public:
	virtual bool Fire(float WorldTimeOverride = -1.f) override;
protected:
	virtual void PerformFire() override;
	virtual void FireComplete() override;
//~ End UFireMode Interface 

public:
	void ProcessHitbox(UWeaponAnimInstance* AnimationInstance, const struct FMeleeNotifyData& Hitbox);

protected:
	//If true, melee attack pierces through individual targets (except for world hits).
	UPROPERTY(EditDefaultsOnly, Category = Melee)
	bool bAllowMultipleHits = false;
	//If bAllowMultipleHits is true, this value determines how many times this swing can pierce through targets.
	UPROPERTY(EditDefaultsOnly, Category = Melee, meta = (EditCondition = bAllowMultipleHits))
	uint8 MaxMultiHitCount = 0;
	//If true, world hits are treated as a pierceable hit target.
	UPROPERTY(EditDefaultsOnly, Category = Melee, meta = (EditCondition = bAllowMultipleHits))
	bool bWorldHitPierces = false;
	//If true, world hits are treated as a pierceable hit target.
	UPROPERTY(EditDefaultsOnly, Category = Melee, meta = (EditCondition = "!bAllowMultipleHits"))
	bool bTargetHitBlocksSwing = false;
	//If true, will play HitReturn montage section of current montage once the last piercing hit of the swing is used or a world hit occurs.
	UPROPERTY(EditDefaultsOnly, Category = Melee)
	bool bPlayHitReturnOnBlockingHit = false;

	UPROPERTY(EditDefaultsOnly, Category = AI, meta = (PinHiddenByDefault, InlineEditConditionToggle))
	bool bMakeNoiseOnSwing = true;
	UPROPERTY(EditDefaultsOnly, Category = AI, meta = (EditCondition = "bMakeNoiseOnSwing", DisplayName = "Make Noise On Swing"))
	FCoreNoiseParams SwingNoise = FCoreNoiseParams(CoreNoiseTag::MeleeSwing, 0.1f, 0.f);
	UPROPERTY(EditDefaultsOnly, Category = AI, meta = (PinHiddenByDefault, InlineEditConditionToggle))
	bool bMakeNoiseOnSwingHit = true;
	UPROPERTY(EditDefaultsOnly, Category = AI, meta = (EditCondition = "bMakeNoiseOnSwingHit", DisplayName = "Make Noise On Swing Hit"))
	FCoreNoiseParams SwingHitNoise = FCoreNoiseParams(CoreNoiseTag::MeleeHit, 1.f, 0.f);

	UPROPERTY(Transient)
	TArray<AActor*> HitTargetList;
};
