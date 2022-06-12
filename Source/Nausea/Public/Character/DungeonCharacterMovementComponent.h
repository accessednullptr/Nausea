// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Character/CoreCharacterMovementComponent.h"
#include "DungeonCharacterMovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class NAUSEA_API UDungeonCharacterMovementComponent : public UCoreCharacterMovementComponent
{
	GENERATED_UCLASS_BODY()

//~ Begin UCharacterMovementComponent Interface
public:
	virtual void SetMovementMode(EMovementMode NewMovementMode, uint8 NewCustomMode = 0) override;
	virtual bool FindNavFloor(const FVector& TestLocation, FNavLocation& NavFloorLocation) const override;
	virtual void PerformMovement(float DeltaTime) override;
	virtual bool TryToLeaveNavWalking() override;
//~ End UCharacterMovementComponent Interface

public:
	void RefreshUnstuckTime() { UnstuckTime = FMath::RandRange(1.f, 2.f); }
	void ConsumeUnstuckTime(float DeltaTime) { UnstuckTime -= DeltaTime; }
	bool PerformUnstuck() const { return UnstuckTime > 0.f; }

protected:
	UPROPERTY(Transient)
	float UnstuckTime = 0.f;

	UPROPERTY(Transient)
	FTimerHandle AttemptResetToNavMeshTimerHandle;
};
