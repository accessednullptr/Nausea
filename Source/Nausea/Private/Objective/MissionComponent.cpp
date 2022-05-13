// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Objective/MissionComponent.h"
#include "Engine/ActorChannel.h"
#include "Engine/LevelScriptActor.h"
#include "System/NetHelper.h"
#include "System/NauseaGameState.h"
#include "Objective/Objective.h"

UMissionComponent::UMissionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
		
	SetIsReplicatedByDefault(true);
}

void UMissionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_WITH_PARAMS_FAST(UMissionComponent, MissionStatus, PushReplicationParams::Default);
	DOREPLIFETIME_WITH_PARAMS_FAST(UMissionComponent, ObjectiveList, PushReplicationParams::Default);

	DOREPLIFETIME_WITH_PARAMS_FAST(UMissionComponent, MissionInfoClass, PushReplicationParams::InitialOnly);
}

void UMissionComponent::BeginPlay()
{
	Super::BeginPlay();
}

bool UMissionComponent::ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	if (RepFlags && !RepFlags->bNetInitial && !IsMissionInProgress())
	{
		return Super::ReplicateSubobjects(Channel, Bunch, RepFlags);
	}

	bool bWroteSomething = false;

	for (UObjective* Objective : ObjectiveList)
	{
		if (!Objective)
		{
			continue;
		}
		
		bWroteSomething |= Channel->ReplicateSubobject(Objective, *Bunch, *RepFlags);
	}

	return Super::ReplicateSubobjects(Channel, Bunch, RepFlags) || bWroteSomething;
}

UMissionComponent* UMissionComponent::CreateMission(UObject* WorldContextObject, TSubclassOf<UMissionComponent> MissionComponentClassOverride, TSubclassOf<UMissionInfo> MissionClassInfoOverride)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	if (!World)
	{
		return nullptr;
	}

	ANauseaGameState* NauseaGameState = Cast<ANauseaGameState>(World->GetGameState());

	if (!NauseaGameState)
	{
		return nullptr;
	}

	UMissionComponent* Mission = NewObject<UMissionComponent>(NauseaGameState, MissionComponentClassOverride ? MissionComponentClassOverride.Get() : UMissionComponent::StaticClass());
	Mission->MissionInfoClass = MissionClassInfoOverride;
	MARK_PROPERTY_DIRTY_FROM_NAME(UMissionComponent, MissionInfoClass, Mission);
	Mission->RegisterComponent();
	return Mission;
}

UMissionComponent* UMissionComponent::InitializeMission(UMissionComponent* MissionComponent, const TArray<UObjective*>& Objectives, UMissionComponent* NextMission)
{
	if (!MissionComponent)
	{
		return nullptr;
	}

	MissionComponent->SetObjectiveList(Objectives);
	MissionComponent->FollowingMission = NextMission;
	return MissionComponent;
}

float UMissionComponent::GetMissionProgress() const
{
	float ProgressSum = 0.f;
	int32 ObjectiveCount = 0;
	for (UObjective* Objective : ObjectiveList)
	{
		if (!Objective || !Objective->ShouldContributeToProgress())
		{
			continue;
		}

		ProgressSum += Objective->GetObjecitveProgress();
		ObjectiveCount++;
	}

	return ObjectiveCount != 0 ? (ProgressSum / float(ObjectiveCount)) : 0.f;
}

const FText& UMissionComponent::GetMissionTitle() const
{
	if (const UMissionInfo* MissionInfo = MissionInfoClass ? MissionInfoClass.GetDefaultObject() : nullptr)
	{
		return MissionInfo->GetMissionTitle();
	}

	return DefaultMissionTitle;
}

TSoftClassPtr<UMissionContainerUserWidget> UMissionComponent::GetMissionContainerWidget() const
{
	if (const UMissionInfo* MissionInfo = MissionInfoClass ? MissionInfoClass.GetDefaultObject() : nullptr)
	{
		return MissionInfo->GetMissionContainerWidget();
	}

	return DefaultMissionContainerWidget;
}

void UMissionComponent::SetMissionStatus(EMissionStatus NewStatus)
{
	if (NewStatus == MissionStatus)
	{
		return;
	}

	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	MissionStatus = NewStatus;

	switch (MissionStatus)
	{
	case EMissionStatus::Active:
		StartMission();
		break;
	case EMissionStatus::Completed:
		CompletedMission();
		break;
	case EMissionStatus::Failed:
		FailedMission();
		break;
	case EMissionStatus::Inactive:
		ResetMission();
		break;
	}

	OnRep_MissionStatus();
	MARK_PROPERTY_DIRTY_FROM_NAME(UMissionComponent, MissionStatus, this);
}

void UMissionComponent::StartMission()
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	for (UObjective* Objective : ObjectiveList)
	{
		Objective->Activate();
	}
}

void UMissionComponent::CompletedMission()
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	//Notify all incomplete objectives that the mission is complete.
	for (UObjective* Objective : ObjectiveList)
	{
		Objective->MissionCompleted();
	}	
}

void UMissionComponent::FailedMission()
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	for (UObjective* Objective : ObjectiveList)
	{
		if (Objective->GetObjecitveState() == EObjectiveState::Active)
		{
			Objective->Fail();
		}
	}
}

void UMissionComponent::ResetMission()
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	for (UObjective* Objective : ObjectiveList)
	{
		Objective->Reset();
	}
}

FString UMissionComponent::DescribeMissionToGameplayDebugger(int32 OrderIndex) const
{
	return FString::Printf(TEXT("{white}%i | {yellow}%s: %s"), OrderIndex, *GetMissionTitle().ToString(), *GetMissionStatusString());
}

TArray<FString> UMissionComponent::DescribeObjectivesToGameplayDebugger(int32 OrderIndex) const
{
	TArray<FString> Description;
	Description.Reserve(ObjectiveList.Num());

	int32 Index = 0;
	for (UObjective* Objective : ObjectiveList)
	{
		Index++;
		Description.Add(FString::Printf(TEXT("    %i | %s%s{yellow}%s"),
			Index,
			*GetObjectiveStatusString(Objective),
			*(Objective->IsOptionalObjective() ? FString(" {grey}(OPTIONAL) ") : FString(" ")),
			* Objective->DescribeObjectiveToGameplayDebugger()
		));
	}

	return Description;
}

FString UMissionComponent::GetMissionStatusString() const
{
	switch (MissionStatus)
	{
	case EMissionStatus::Inactive:
		return "{grey}INACTIVE";
	case EMissionStatus::Active:
		return "{white}ACTIVE";
	case EMissionStatus::Completed:
		return "{green}COMPLETED";
	case EMissionStatus::Failed:
		return "{red}FAILED";
	}

	return "{purple}INVALID";
}

FString UMissionComponent::GetObjectiveStatusString(const UObjective* Objective)
{
	switch (Objective->GetObjecitveState())
	{
	case EObjectiveState::Inactive:
		return "{grey}INACTIVE";
	case EObjectiveState::Active:
		return "{white}ACTIVE";
	case EObjectiveState::Completed:
		return "{green}COMPLETED";
	case EObjectiveState::Failed:
		return "{red}FAILED";
	}

	return "{purple}INVALID";
}

void UMissionComponent::SetObjectiveList(const TArray<UObjective*>& InObjectiveList)
{
	ObjectiveList = InObjectiveList;
	OnRep_ObjectiveList();
	MARK_PROPERTY_DIRTY_FROM_NAME(UMissionComponent, ObjectiveList, this);
}

void UMissionComponent::OnRep_MissionStatus()
{
	switch (MissionStatus)
	{
	case EMissionStatus::Active:
		OnMissionStarted.Broadcast(this);
		break;
	case EMissionStatus::Completed:
		OnMissionCompleted.Broadcast(this);
		break;
	case EMissionStatus::Failed:
		OnMissionFailed.Broadcast(this);
		break;
	case EMissionStatus::Inactive:
		OnMissionReset.Broadcast(this);
		break;
	}

	OnMissionStatusChanged.Broadcast(this, MissionStatus);
}

void UMissionComponent::OnRep_ObjectiveList()
{
	for (UObjective* Objective : PreviousObjectiveList)
	{
		if (!Objective)
		{
			continue;
		}

		UnbindToObjective(Objective);
	}

	for (UObjective* Objective : ObjectiveList)
	{
		if (!Objective)
		{
			continue;
		}

		BindToObjective(Objective);
	}

	ObjectiveList.Remove(nullptr);
	PreviousObjectiveList = ObjectiveList;
	OnMissionObjectivesChanged.Broadcast(this, ObjectiveList);
}

void UMissionComponent::BindToObjective(UObjective* Objective)
{
	//Before anything else, notify the object this mission component now owns it.	
	//Objective->Rename(nullptr, this); Maybe don't. Replicating UObjects after renaming may not work as desired...
	Objective->Initialize(this);

	//Assume we're already bound if this delegate is.
	if (Objective->OnObjectiveStateChanged.IsAlreadyBound(this, &UMissionComponent::ObjectiveStateChanged))
	{
		return;
	}

	Objective->OnObjectiveStateChanged.AddDynamic(this, &UMissionComponent::ObjectiveStateChanged);
	Objective->OnObjectiveProgressChanged.AddDynamic(this, &UMissionComponent::ObjectiveProgressChanged);
}

void UMissionComponent::UnbindToObjective(UObjective* Objective)
{
	//Assume we're already unbound if this delegate is.
	if (!Objective || Objective->IsPendingKillOrUnreachable() || !Objective->OnObjectiveStateChanged.IsAlreadyBound(this, &UMissionComponent::ObjectiveStateChanged))
	{
		return;
	}

	Objective->OnObjectiveStateChanged.RemoveDynamic(this, &UMissionComponent::ObjectiveStateChanged);
	Objective->OnObjectiveProgressChanged.RemoveDynamic(this, &UMissionComponent::ObjectiveProgressChanged);
}

void UMissionComponent::ObjectiveStateChanged(UObjective* Objective, EObjectiveState State)
{
	K2_ObjectiveStateChanged(Objective, State);
	OnMissionObjectiveStateChanged.Broadcast(this, Objective, State);

	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	//If not an optional mission, check for mission completion/failure.
	if (!Objective->IsOptionalObjective())
	{
		switch (State)
		{
		case EObjectiveState::Failed:
			SetMissionStatus(EMissionStatus::Failed);
			break;
		case EObjectiveState::Completed:
			bool bAllObjectivesComplete = true;
			for (UObjective* MissionObjective : ObjectiveList)
			{
				if (!MissionObjective->IsOptionalObjective() && MissionObjective->GetObjecitveState() != EObjectiveState::Completed)
				{
					bAllObjectivesComplete = false;
					break;
				}
			}

			if (bAllObjectivesComplete)
			{
				SetMissionStatus(EMissionStatus::Completed);
			}
			break;
		}
	}
}

void UMissionComponent::ObjectiveProgressChanged(UObjective* Objective, float Progress)
{
	K2_ObjectiveProgressChanged(Objective, Progress);

	OnMissionProgressChanged.Broadcast(this, GetMissionProgress());
	OnMissionObjectiveProgressChanged.Broadcast(this, Objective, Progress);
}

void UMissionComponent::Multicast_Reliable_MissionStarted_Implementation()
{
	
}

void UMissionComponent::Multicast_Reliable_MissionCompleted_Implementation()
{
	
}

void UMissionComponent::Multicast_Reliable_MissionFailed_Implementation()
{

}