// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "UObject/Object.h"
#include "Tickable.h"
#include "Objective/MissionTypes.h"
#include "Objective.generated.h"

class UMissionComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FObjectiveStateChangedSignature, UObjective*, Objective, EObjectiveState, State);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FObjectiveProgressChangedSignature, UObjective*, Objective, float, Progress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FObjectiveEnterStateSignature, UObjective*, Objective);

UCLASS(Blueprintable, BlueprintType, ClassGroup = (Custom))
class NAUSEA_API UObjective : public UObject, public FTickableGameObject
{
	GENERATED_UCLASS_BODY()
		
//~ Begin UObject Interface
public:
	virtual void PostInitProperties() override;
	virtual void BeginDestroy() override;
	virtual UWorld* GetWorld() const override final { return (WorldPrivate ? WorldPrivate : GetWorld_Uncached()); }
protected:
	virtual bool IsSupportedForNetworking() const { return true; }
	virtual int32 GetFunctionCallspace(UFunction* Function, FFrame* Stack) override;
	virtual bool CallRemoteFunction(UFunction* Function, void* Parameters, FOutParmRec* OutParms, FFrame* Stack) override;
//~ End UObject Interface

//~ Begin FTickableGameObject Interface
protected:
	virtual void Tick(float DeltaTime) override {}
public:
	virtual ETickableTickType GetTickableTickType() const { return TickType; }
	virtual bool IsTickable() const { return bTickEnabled && !IsPendingKill(); }
	virtual TStatId GetStatId() const { return TStatId(); }
	virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
//~ End FTickableGameObject Interface

public:
	UFUNCTION(BlueprintPure, Category = Objective, meta = (WorldContext = "WorldContextObject", DeterminesOutputType = "ObjectiveClass"))
	static UObjective* CreateObjective(UObject* WorldContextObject, TSubclassOf<UObjective> ObjectiveClass, const FString& InObjectiveID);

	template<class T>
	static T* CreateObjective(UObject* WorldContextObject, TSubclassOf<UObjective> ObjectiveClass, const FString& InObjectiveID)
	{
		static_assert(TPointerIsConvertibleFromTo<T, const UObjective>::Value, "'T' template parameter to CreateObjective must be derived from UObjective");
		return Cast<T>(CreateObjective(WorldContextObject, ObjectiveClass, InObjectiveID));
	}

	UFUNCTION(BlueprintCallable, Category = Objective)
	ENetRole GetLocalRole() const;
	UFUNCTION(BlueprintCallable, Category = Objective)
	bool IsSimulatedProxy() const;
	UFUNCTION(BlueprintCallable, Category = Objective)
	bool IsAuthority() const;

public:
	virtual void Initialize(UMissionComponent* MissionComponent);

	UFUNCTION(BlueprintCallable, Category = Objective)
	UMissionComponent* GetMissionComponent() const { return OwningMissionComponent; }
	
	UFUNCTION(BlueprintCallable, Category = Objective)
	EObjectiveState GetObjecitveState() const { return ObjectiveState; }
	
	UFUNCTION(BlueprintCallable, Category = Objective)
	bool IsOptionalObjective() const { return bOptionalMission; }

	UFUNCTION(BlueprintCallable, Category = Objective)
	bool ShouldContributeToProgress() const { return bContributesToProgress; }
	UFUNCTION(BlueprintCallable, Category = Objective)
	float GetObjecitveProgress() const { return ObjectiveProgress; }

	UFUNCTION(BlueprintPure, Category = MissionComponent)
	TSoftClassPtr<class UMissionObjectiveUserWidget> GetObjectiveWidget() const { return ObjectiveWidget; }

	UFUNCTION(BlueprintCallable, Category = Objective)
	virtual FText GetObjecitveName() const { return ObjectiveName; }

	//Activation and resetting is automatically handled, only completion and fail should be accessible to BP.
	UFUNCTION()
	virtual void Activate();
	UFUNCTION(BlueprintCallable, Category = Objective, BlueprintAuthorityOnly)
	virtual void Complete();
	UFUNCTION(BlueprintCallable, Category = Objective, BlueprintAuthorityOnly)
	virtual void Fail();
	UFUNCTION()
	virtual void Reset();

	UFUNCTION()
	virtual void MissionCompleted();	

	UFUNCTION(BlueprintImplementableEvent, Category = Objective, meta = (DisplayName = "On Objective Start", ScriptName = "StartObjective"))
	void K2_ActivateObjective();
	UFUNCTION(BlueprintImplementableEvent, Category = Objective, meta = (DisplayName = "On Objective Complete", ScriptName = "CompleteObjective"))
	void K2_CompleteObjective();
	UFUNCTION(BlueprintImplementableEvent, Category = Objective, meta = (DisplayName = "On Objective Fail", ScriptName = "FailObjective"))
	void K2_FailObjective();
	UFUNCTION(BlueprintImplementableEvent, Category = Objective, meta = (DisplayName = "On Objective Reset", ScriptName = "ResetObjective"))
	void K2_ResetObjective();
	
	UFUNCTION(BlueprintCallable, Category = Objective)
	virtual void SetTickEnabled(bool bEnableTick);
	UFUNCTION()
	virtual void SetObjectiveState(EObjectiveState State);
	UFUNCTION(BlueprintCallable, Category = Objective, BlueprintAuthorityOnly)
	virtual void SetObjectiveProgress(float Progress);

	virtual FString DescribeObjectiveToGameplayDebugger() const;

protected:
	UFUNCTION()
	virtual void OnRep_ObjectiveState(EObjectiveState PreviousState);
	UFUNCTION()
	virtual void OnRep_ObjectiveProgress();

private:
	UWorld* GetWorld_Uncached() const;

public:
	UPROPERTY(BlueprintAssignable, Category = Objective)
	FObjectiveStateChangedSignature OnObjectiveStateChanged;
	UPROPERTY(BlueprintAssignable, Category = Objective)
	FObjectiveProgressChangedSignature OnObjectiveProgressChanged;
	
	UPROPERTY(BlueprintAssignable, Category = Objective)
	FObjectiveEnterStateSignature OnObjectiveStarted;
	UPROPERTY(BlueprintAssignable, Category = Objective)
	FObjectiveEnterStateSignature OnObjectiveCompleted;
	UPROPERTY(BlueprintAssignable, Category = Objective)
	FObjectiveEnterStateSignature OnObjectiveFailed;

protected:
	UPROPERTY(EditDefaultsOnly, Category = Objective)
	bool bCanEverTick = false;
	UPROPERTY(EditDefaultsOnly, Category = Objective)
	bool bNeverTickOnDedicatedServer = false;
	UPROPERTY(EditDefaultsOnly, Category = Objective)
	bool bTickWhenActivated = false;
	
	//Is this objective optional? (Failure does not fail the mission.)
	UPROPERTY(EditDefaultsOnly, Category = Objective)
	bool bOptionalMission = false;

	//If the owning mission is completed while this optional objective is still active, if bFailObjectiveOnMissionComplete is set to true will cause this objective to fail.
	//If set to false, will force completion.
	UPROPERTY(EditDefaultsOnly, Category = Objective, meta = (EditCondition = "bOptionalMission"))
	bool bFailObjectiveOnMissionComplete = true;

	//Should this objective contribute to the mission component progress?
	UPROPERTY(EditDefaultsOnly, Category = Objective)
	bool bContributesToProgress = false;

	UPROPERTY(EditDefaultsOnly, Category = UI)
	TSoftClassPtr<class UMissionObjectiveUserWidget> ObjectiveWidget;

	UPROPERTY(EditDefaultsOnly, Category = Objective)
	FText ObjectiveName = FText();

private:
	UWorld* WorldPrivate = nullptr;

	UPROPERTY(Transient)
	UMissionComponent* OwningMissionComponent = nullptr;

	ETickableTickType TickType = ETickableTickType::Never;

	UPROPERTY(Transient)
	bool bTickEnabled = false;

	UPROPERTY(Transient)
	bool bTickManuallyEnabled = false;

	UPROPERTY(ReplicatedUsing = OnRep_ObjectiveProgress)
	float ObjectiveProgress = 0.f;
	UPROPERTY(ReplicatedUsing = OnRep_ObjectiveState)
	EObjectiveState ObjectiveState = EObjectiveState::Inactive;
	UPROPERTY(Replicated)
	FString ObjectiveID = "";
};