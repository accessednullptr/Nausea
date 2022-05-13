// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Objective/MissionTypes.h"
#include "MissionComponent.generated.h"

class UObjective;
class UMissionContainerUserWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMissionStatusChangedSignature, UMissionComponent*, Mission, EMissionStatus, Status);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMissionProgressChangedSignature, UMissionComponent*, Mission, float, Progress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMissionEnterStateSignature, UMissionComponent*, Mission);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMissionObjectivesChangedSignature, UMissionComponent*, Mission, const TArray<UObjective*>&, ObjectiveList);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FMissionObjectiveStateChangedSignature, UMissionComponent*, Mission, UObjective*, Objective, EObjectiveState, State);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FMissionObjectiveProgressChangedSignature, UMissionComponent*, Mission, UObjective*, Objective, float, Progress);

UCLASS(Blueprintable, BlueprintType, ClassGroup=(Custom), HideCategories = (ComponentTick, Collision, Tags, Variable, Activation, ComponentReplication, Cooking))
class NAUSEA_API UMissionComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

//~ Begin UActorComponent Interface
protected:
	virtual void BeginPlay() override;
public:
	virtual bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
///UNHOOKING DEFAULT ACTIVATE/DEACTIVATE BEHAVIOUR.
private:
	virtual void Activate(bool bReset) override {}
	virtual void Deactivate() override {}
	virtual void OnRep_IsActive() override {}
//~ End UActorComponent Interface
		
//Generic mission creation functions.
public:
	UFUNCTION(BlueprintPure, Category = MissionComponent, meta = (WorldContext = "WorldContextObject", DeterminesOutputType = "MissionComponentClass"))
	static UMissionComponent* CreateMission(UObject* WorldContextObject, TSubclassOf<UMissionComponent> MissionComponentClassOverride, TSubclassOf<UMissionInfo> MissionClassInfoOverride);
	UFUNCTION(BlueprintPure, Category = MissionComponent, meta = (WorldContext = "WorldContextObject", DeterminesOutputType = "MissionComponent"))
	static UMissionComponent* InitializeMission(UMissionComponent* MissionComponent, const TArray<UObjective*>& Objectives, UMissionComponent* NextMission);

public:
	UFUNCTION(BlueprintPure, Category = MissionComponent)
	bool IsMissionInProgress() const { return MissionStatus == EMissionStatus::Active; }
	UFUNCTION(BlueprintPure, Category = MissionComponent)
	virtual float GetMissionProgress() const;

	UFUNCTION(BlueprintPure, Category = MissionComponent)
	const FText& GetMissionTitle() const;
	UFUNCTION(BlueprintPure, Category = MissionComponent)
	TSoftClassPtr<UMissionContainerUserWidget> GetMissionContainerWidget() const;

	UFUNCTION(BlueprintPure, Category = MissionComponent)
	const TArray<UObjective*>& GetObjectiveList() const { return ObjectiveList; }

	UFUNCTION(BlueprintPure, Category = MissionComponent)
	UMissionComponent* GetFollowingMission() const { return FollowingMission; }


	UFUNCTION()
	virtual void SetMissionStatus(EMissionStatus NewStatus);

	UFUNCTION()
	virtual void StartMission();
	UFUNCTION()
	virtual void CompletedMission();
	UFUNCTION()
	virtual void FailedMission();
	UFUNCTION()
	virtual void ResetMission();

	virtual FString DescribeMissionToGameplayDebugger(int32 OrderIndex) const;
	virtual TArray<FString> DescribeObjectivesToGameplayDebugger(int32 OrderIndex) const;
	FString GetMissionStatusString() const;
	static FString GetObjectiveStatusString(const UObjective* Objective);

public:
	UPROPERTY(BlueprintAssignable, Category = Mission)
	FMissionStatusChangedSignature OnMissionStatusChanged;
	UPROPERTY(BlueprintAssignable, Category = Mission)
	FMissionProgressChangedSignature OnMissionProgressChanged;
	UPROPERTY(BlueprintAssignable, Category = Mission)
	FMissionObjectivesChangedSignature OnMissionObjectivesChanged;
	

	//State specific versions of OnObjectiveStateChanged.
	UPROPERTY(BlueprintAssignable, Category = Mission)
	FMissionEnterStateSignature OnMissionStarted;
	UPROPERTY(BlueprintAssignable, Category = Mission)
	FMissionEnterStateSignature OnMissionCompleted;
	UPROPERTY(BlueprintAssignable, Category = Mission)
	FMissionEnterStateSignature OnMissionFailed;
	UPROPERTY(BlueprintAssignable, Category = Mission)
	FMissionEnterStateSignature OnMissionReset;

	UPROPERTY(BlueprintAssignable, Category = Mission)
	FMissionObjectiveStateChangedSignature OnMissionObjectiveStateChanged;
	UPROPERTY(BlueprintAssignable, Category = Mission)
	FMissionObjectiveProgressChangedSignature OnMissionObjectiveProgressChanged;

protected:
	UFUNCTION()
	void SetObjectiveList(const TArray<UObjective*>& InObjectiveList);
	
	UFUNCTION()
	void OnRep_MissionStatus();

	UFUNCTION()
	void OnRep_ObjectiveList();

	virtual void BindToObjective(UObjective* Objective);
	virtual void UnbindToObjective(UObjective* Objective);

	UFUNCTION()
	virtual void ObjectiveStateChanged(UObjective* Objective, EObjectiveState State);
	UFUNCTION(BlueprintImplementableEvent, Category = Mission, meta = (DisplayName="On Objective State Changed",ScriptName="ObjectiveStateChanged"))
	void K2_ObjectiveStateChanged(UObjective* Objective, EObjectiveState State);

	UFUNCTION()
	virtual void ObjectiveProgressChanged(UObjective* Objective, float Progress);
	UFUNCTION(BlueprintImplementableEvent, Category = Mission, meta = (DisplayName="On Objective Progress Changed",ScriptName="ObjectiveProgressChanged"))
	void K2_ObjectiveProgressChanged(UObjective* Objective, float Progress);

	UFUNCTION(Reliable, NetMulticast)
	void Multicast_Reliable_MissionStarted();
	UFUNCTION(Reliable, NetMulticast)
	void Multicast_Reliable_MissionCompleted();
	UFUNCTION(Reliable, NetMulticast)
	void Multicast_Reliable_MissionFailed();

protected:
	UPROPERTY(ReplicatedUsing = OnRep_ObjectiveList)
	TArray<UObjective*> ObjectiveList = TArray<UObjective*>();
	UPROPERTY()
	TArray<UObjective*> PreviousObjectiveList = TArray<UObjective*>();

	UPROPERTY(ReplicatedUsing = OnRep_MissionStatus)
	EMissionStatus MissionStatus = EMissionStatus::Inactive;

	UPROPERTY(Replicated)
	TSubclassOf<UMissionInfo> MissionInfoClass = nullptr;

	UPROPERTY()
	TSoftClassPtr<UMissionContainerUserWidget> DefaultMissionContainerWidget = nullptr;
	UPROPERTY()
	FText DefaultMissionTitle = FText::FromString("MISSION");

	UPROPERTY()
	UMissionComponent* FollowingMission = nullptr;
};

UCLASS(BlueprintType, Blueprintable, AutoExpandCategories = (Default))
class NAUSEA_API UMissionInfo : public UObject
{
	GENERATED_BODY()

public:
	const FText& GetMissionTitle() const { return MissionTitle; }
	TSoftClassPtr<UMissionContainerUserWidget> GetMissionContainerWidget() const{ return MissionContainerWidget; }

protected:
	UPROPERTY(EditDefaultsOnly, Category = UI)
	FText MissionTitle = FText();

	UPROPERTY(EditDefaultsOnly, Category = UI)
	TSoftClassPtr<UMissionContainerUserWidget> MissionContainerWidget = nullptr;
};