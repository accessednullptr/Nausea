// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "TimerManager.h"
#include "Character/CoreCharacterMovementComponent.h"
#include "FreerunMovementComponent.generated.h"

class AFreerunCharacter;

/**
 * 
 */
UCLASS()
class NAUSEA_API UFreerunMovementComponent : public UCoreCharacterMovementComponent
{
	GENERATED_UCLASS_BODY()

	friend class FSavedMove_FreerunCharacter;

//~ Begin UActorComponent Interface
public:
	virtual void BeginPlay() override;
//~ Begin UActorComponent Interface
	
//~ Begin UMovementComponent Interface
public:
	virtual float GetMaxSpeed() const override;
	virtual float GetMaxAcceleration() const override;
//~ End UMovementComponent Interface
	
//~ Begin UCharacterMovementComponent Interface
public:
	virtual bool DoJump(bool bReplayingMoves) override;
protected:
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
//~ Begin UCharacterMovementComponent Interface

public:
	UFUNCTION(BlueprintPure, Category = FreerunMovement)
	AFreerunCharacter* GetFreerunCharacter() const { return FreerunCharacterOwner; }
	UFUNCTION(BlueprintPure, Category = FreerunMovement)
	virtual float GetMaxSpeedForDirection(const FVector& Direction) const;
	UFUNCTION(BlueprintPure, Category = FreerunMovement)
	virtual float GetMaxPossibleSpeed() const;
	UFUNCTION(BlueprintPure, Category = FreerunMovement)
	virtual float GetSprintVelocityMultiplier(const FVector& Direction, bool bCalculatingMaxPossible = false) const;

	UFUNCTION(BlueprintPure, Category = FreerunMovement)
	bool IsJumpCooldownActive() const { return !IsClientReplayingMoves() && GetWorld()->GetTimerManager().IsTimerActive(JumpTimer); }

	UFUNCTION()
	void SetSprint(bool bInSprint);

	UFUNCTION()
	void DoLongJump();

	UFUNCTION(BlueprintPure, Category = FreerunMovement)
	virtual bool CanLongJump() const;


protected:
	UFUNCTION()
	void StartJumpCooldown();

protected:
	UPROPERTY(Category="Character Movement: Jumping / Falling", EditAnywhere, BlueprintReadOnly, meta=(DisplayName="Jump Cooldown Time", ClampMin="0", UIMin="0"))
	float JumpCooldownTime = 0.25f;
	UPROPERTY(Category="Character Movement: Jumping / Falling", EditAnywhere, BlueprintReadOnly, meta=(DisplayName="Wall Jump Velocity Multiplier", ClampMin="0", UIMin="0"))
	float WallJumpImpulseStrength = 0.9f;
	UPROPERTY(Category="Character Movement: Jumping / Falling", EditAnywhere, BlueprintReadOnly, meta=(DisplayName="Consecutive Wall Jump Decay Multiplier", ClampMin="0", UIMin="0"))
	float WallJumpStrengthDecay = 0.75f;
	UPROPERTY(Category="Character Movement: Jumping / Falling", EditAnywhere, BlueprintReadOnly, meta=(DisplayName="Wall Jump Pitch Angle", ClampMin="0", UIMin="0"))
	float WallJumpPitchAngle = 55.f;

	UPROPERTY()
	FTimerHandle JumpTimer;
	UPROPERTY()
	FVector WallJumpDirection = FVector(MAX_FLT);

	UPROPERTY(Category = "Character Movement: Walking", EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0", UIMin = "0"))
	float SprintVelocityMultiplier = 1.5f;

	UPROPERTY()
	bool bWantsToSprint = false;

	UPROPERTY(Category="Character Movement: Jumping / Falling", EditAnywhere, BlueprintReadOnly, meta=(DisplayName="Long Jump Velocity Strength", ClampMin="0", UIMin="0"))
	float LongJumpVelocityStrength = 1.5f;
	UPROPERTY(Category="Character Movement: Jumping / Falling", EditAnywhere, BlueprintReadOnly, meta=(DisplayName="Long Jump Speed Penalty Duration", ClampMin="0", UIMin="0"))
	float LongJumpSpeedPenaltyDuration = 4.f;
	UPROPERTY(Category="Character Movement: Jumping / Falling", EditAnywhere, BlueprintReadOnly, meta=(DisplayName="Long Jump Pitch Angle", ClampMin="0", UIMin="0"))
	float LongJumpPitchAngle = 35.f;
	
	UPROPERTY()
	bool bWantsLongJump = false;
	UPROPERTY()
	FTimerHandle LongJumpPenaltyTimer;

private:
	UPROPERTY(Transient, DuplicateTransient)
	AFreerunCharacter* FreerunCharacterOwner = nullptr;
};

class FSavedMove_FreerunCharacter : public FSavedMove_CoreCharacter
{
	typedef FSavedMove_CoreCharacter Super;
	
//~ Begin FSavedMove_Character Interface
public:
	virtual void Clear() override;
	virtual uint8 GetCompressedFlags() const override;
	virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const override;
	virtual void SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData) override;
	virtual void PrepMoveFor(ACharacter* Character) override;
	virtual bool IsImportantMove(const FSavedMovePtr& LastAckedMovePtr) const override;
	virtual void PostUpdate(ACharacter* C, EPostUpdateMode PostUpdateMode) override;
//~ End FSavedMove_Character Interface

public:
	uint8 bSavedWantsToSprint : 1;
	uint8 bSavedWantsToLongJump : 1;
	FVector_NetQuantizeNormal SavedWallJumpDirection = FVector(MAX_FLT);
};

class FNetworkPredictionData_Client_FreerunCharacter : public FNetworkPredictionData_Client_CoreCharacter
{
	typedef FNetworkPredictionData_Client_CoreCharacter Super;

public:
	FNetworkPredictionData_Client_FreerunCharacter(const UCharacterMovementComponent& ClientMovement);

	virtual FSavedMovePtr AllocateNewMove() override;
};