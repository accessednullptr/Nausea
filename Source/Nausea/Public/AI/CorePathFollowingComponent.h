// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "CorePathFollowingComponent.generated.h"

class UCoreCharacterMovementComponent;
class UCoreAIPerceptionComponent;

/**
 * 
 */
UCLASS()
class NAUSEA_API UCorePathFollowingComponent : public UCrowdFollowingComponent
{
	GENERATED_UCLASS_BODY()

//~ Begin UActorComponent Interface
public:
	virtual void BeginPlay() override;
//~ End UActorComponent Interface

//~ Begin IPathFollowingAgentInterface Interface
public:
	virtual void OnMoveBlockedBy(const FHitResult& BlockingImpact) override;
//~ End IPathFollowingAgentInterface Interface

//~ Begin UPathFollowingComponent Interface
public:
	virtual void FollowPathSegment(float DeltaTime) override;
	virtual void SetMovementComponent(UNavMovementComponent* MoveComp) override;
	virtual FVector GetMoveFocus(bool bAllowStrafe) const override;
	virtual void OnPathFinished(const FPathFollowingResult& Result) override;
//~ End UPathFollowingComponent Interface

public:
	bool RequestIgnoredByCrowdManager(TObjectKey<UObject> Requester);
	bool RevokeIgnoredByCrowdManager(TObjectKey<UObject> Requester);

	bool RequestPerformPanicMovement(TObjectKey<UObject> Requester);
	bool RevokePerformPanicMovement(TObjectKey<UObject> Requester);

protected:
	UFUNCTION()
	void UpdateIgnoredByCrowdManager();
	UFUNCTION()
	void UpdatePerformPanicMovement();

protected:
	ECrowdSimulationState DefaultCrowdSimulationState = ECrowdSimulationState::Disabled;

	UPROPERTY(EditDefaultsOnly, Category = PathFollowing)
	float JumpCooldown = 0.25f;
	UPROPERTY(Transient)
	FTimerHandle JumpCooldownHandle;

	UPROPERTY(Transient)
	UCoreCharacterMovementComponent* CoreCharacterMovementComponent = nullptr;
	UPROPERTY(Transient)
	UCoreAIPerceptionComponent* CoreAIPerceptionComponent = nullptr;

	TSet<TObjectKey<UObject>> IgnoredByCrowdManagerRequestList;
	UPROPERTY(Transient)
	bool bIgnoredByCrowdManager = false;

	TSet<TObjectKey<UObject>> PerformPanicMovementRequestList;
	UPROPERTY(Transient)
	bool bPerformPanicMovement = false;
};
