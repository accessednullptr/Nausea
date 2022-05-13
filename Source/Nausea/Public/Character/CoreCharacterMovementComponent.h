// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Character/Pilot/PilotMovementComponent.h"
#include "CoreCharacterMovementComponent.generated.h"

class ACoreCharacter;
class ACorePlayerState;
class UPlayerClassComponent;

USTRUCT()
struct FReplayPayload
{
	GENERATED_USTRUCT_BODY()

	FReplayPayload()
	{
		FReplayPayload(-1.f);
	}

	FReplayPayload(float InMaxSpeed)
	{
		MaxSpeed = InMaxSpeed;
	}

public:
	UPROPERTY()
	float MaxSpeed = -1.f;
	UPROPERTY()
	bool bValid = false;
};

/**
 * 
 */
UCLASS()
class NAUSEA_API UCoreCharacterMovementComponent : public UPilotMovementComponent
{
	GENERATED_UCLASS_BODY()
	
	//Configuration will perform the initialization so it needs access to all the internals of this class.
	friend class FSavedMove_CoreCharacter;

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
	virtual void Crouch(bool bClientSimulation = false) override;
	virtual void UnCrouch(bool bClientSimulation = false) override;
	virtual bool StepUp(const FVector& GravDir, const FVector& Delta, const FHitResult& Hit, struct UCharacterMovementComponent::FStepDownResult* OutStepDownResult = NULL) override;
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;
	virtual void UpdateCharacterStateAfterMovement(float DeltaSeconds) override;
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;
	virtual void PerformAirControlForPathFollowing(FVector Direction, float ZDiff) override;
protected:
	virtual float BoostAirControl(float DeltaTime, float TickAirControl, const FVector& FallAcceleration) override;
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;
//~ Begin UCharacterMovementComponent Interface

public:
	UFUNCTION()
	virtual void SetPlayerDefaults();

	virtual void InitializeMovementComponent();

	void ModifyRotationRate(FRotator& RotationRate) const;

	UFUNCTION(BlueprintPure, Category = FreerunMovement)
	ACoreCharacter* GetCoreCharacter() const { return CoreCharacterOwner; }
	UFUNCTION(BlueprintPure, Category = FreerunMovement)
	bool IsClientReplayingMoves() const;

	bool IsReplayPayloadAvailable() const;

public:
	DECLARE_EVENT_TwoParams(UCoreCharacterMovementComponent, FMovementSpeedUpdateSignature, const ACoreCharacter*, float&)
	FMovementSpeedUpdateSignature OnProcessMovementSpeed;

	DECLARE_EVENT_TwoParams(UCoreCharacterMovementComponent, FRotationRateUpdateSignature, const ACoreCharacter*, float&)
	FRotationRateUpdateSignature OnProcessRotationRate;

protected:
	UFUNCTION()
	void OnPlayerClassChanged(ACorePlayerState* PlayerState, UPlayerClassComponent* PlayerClassComponent);

protected:
	UPROPERTY(Category="Character Movement: Jumping / Falling", EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0"))
	float AIAirControlBoostMultiplier = 2.f;

private:
	UPROPERTY(Transient, DuplicateTransient)
	ACoreCharacter* CoreCharacterOwner = nullptr;

	UPROPERTY(Transient)
	FReplayPayload ReplayPayload = FReplayPayload();

	UPROPERTY(Transient)
	bool bSteppedUp = false;
	UPROPERTY(Transient)
	float LastCharacterCameraZ = -1.f;
};

class FSavedMove_CoreCharacter : public FSavedMove_Character
{
	typedef FSavedMove_Character Super;

public:
	virtual void PrepMoveFor(ACharacter* Character) override;
	virtual void PostUpdate(ACharacter* Character, EPostUpdateMode PostUpdateMode) override;
};

class FNetworkPredictionData_Client_CoreCharacter : public FNetworkPredictionData_Client_Character
{
	typedef FNetworkPredictionData_Client_Character Super;

public:
	FNetworkPredictionData_Client_CoreCharacter(const UCharacterMovementComponent& ClientMovement);

	virtual FSavedMovePtr AllocateNewMove() override;
};