// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Player/NauseaCharacter.h"
#include "FreerunCharacter.generated.h"

/**
 * 
 */
UCLASS()
class NAUSEA_API AFreerunCharacter : public ANauseaCharacter
{
	GENERATED_UCLASS_BODY()

//~ Begin AActor Interface
public:
	virtual void PostInitializeComponents() override;
//~ End AActor Interface

//~ Begin APawn Interface
public:
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
//~ End APawn Interface

//~ Begin ACharacter Interface
public:
	virtual void Jump() override;
	virtual bool CanJumpInternal_Implementation() const override;
//~ End ACharacter Interface

public:
	UFUNCTION(BlueprintPure, Category = FreerunCharacter)
	UFreerunMovementComponent* GetFreerunMovementComponent() const { return FreerunMovementComponent; }

	virtual bool CanWallJump(FOverlapResult* JumpOffOverlap = nullptr) const;

protected:
	UFUNCTION()
	virtual void StartSprint();
	UFUNCTION()
	virtual void StopSprint();
	UFUNCTION()
	virtual void PressLongJump();

protected:
	UPROPERTY(EditDefaultsOnly, Category = Freerun)
	float WallJumpExtentSize = 52.f;
	UPROPERTY(EditDefaultsOnly, Category = Freerun)
	FVector WallJumpRelativeOffset = FVector(0, 0, -32);

private:
	UPROPERTY(Transient)
	UFreerunMovementComponent* FreerunMovementComponent = nullptr;
};
