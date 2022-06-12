// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Player/PlayerOwnershipInterface.h"
#include "CoreAIController.generated.h"

class UActionBrainComponent;
class UEnemySelectionComponent;
class URoutineManagerComponent;
class ACorePlayerState;
class ACoreCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAIPawnUpdatedSignature, ACoreAIController*, AIController, ACoreCharacter*, Pawn);

/**
 * 
 */
UCLASS()
class NAUSEA_API ACoreAIController : public AAIController, public IPlayerOwnershipInterface
{
	GENERATED_UCLASS_BODY()

//~ Begin AActor Interface
protected:
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;
//~ End AActor Interface

//~ Begin AController Interface
public:
	virtual void InitPlayerState() override;
	virtual void SetPawn(APawn* InPawn) override;
//~ End AController Interface

//~ Begin AAIController Interface
public:
	virtual bool RunBehaviorTree(UBehaviorTree* BTAsset) override;
	virtual void UpdateControlRotation(float DeltaTime, bool bUpdatePawn) override;
//~ End AAIController Interface

//~ Begin IPlayerOwnershipInterface Interface
public:
	virtual ACorePlayerState* GetOwningPlayerState() const override;
	virtual UPlayerStatisticsComponent* GetPlayerStatisticsComponent() const override { return nullptr; }
	virtual AController* GetOwningController() const { return const_cast<ACoreAIController*>(this); }
	virtual APawn* GetOwningPawn() const override { return GetPawn(); }
	virtual FGenericTeamId GetOwningTeamId() const override { return CachedTeamIdOverride; }
//~ End IPlayerOwnershipInterface Interface

public:
	UFUNCTION(BlueprintCallable, Category = AIController)
	UActionBrainComponent* GetActionBrainComponent() const { return ActionBrainComponent; }
	UFUNCTION(BlueprintCallable, Category = AIController)
	UEnemySelectionComponent* GetEnemySelectionComponent() const { return EnemySelectionComponent; }
	UFUNCTION(BlueprintCallable, Category = AIController)
	URoutineManagerComponent* GetRoutineManagerComponent() const { return RoutineManagerComponent; }

	UFUNCTION(BlueprintCallable, Category = AIController)
	float GetMaxRotationRate() const;

public:
	UPROPERTY(BlueprintAssignable)
	FAIPawnUpdatedSignature OnPawnUpdated;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "AI")
	void OnReceivePlayerState(ACorePlayerState* CorePlayerState);

private:
	UPROPERTY(VisibleDefaultsOnly, Category = BrainComponent)
	UActionBrainComponent* ActionBrainComponent = nullptr;

	UPROPERTY(VisibleDefaultsOnly, Category = BrainComponent)
	UEnemySelectionComponent* EnemySelectionComponent = nullptr;

	UPROPERTY(VisibleDefaultsOnly, Category = BrainComponent)
	URoutineManagerComponent* RoutineManagerComponent = nullptr;
	
	UPROPERTY(EditAnywhere, Category = Team, meta = (PinHiddenByDefault, InlineEditConditionToggle))
	bool bOverrideOwningTeam = false;
	UPROPERTY(EditDefaultsOnly, Category = Team, meta = (EditCondition = "bOverrideOwningTeam"))
	ETeam TeamOverride = ETeam::NoTeam;
	UPROPERTY(Transient)
	FGenericTeamId CachedTeamIdOverride = FGenericTeamId::NoTeam;
};
