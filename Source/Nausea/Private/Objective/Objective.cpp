// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Objective/Objective.h"
#include "Engine/NetDriver.h"
#include "Engine/ActorChannel.h"
#include "System/NetHelper.h"
#include "System/NauseaGameState.h"
#include "Objective/MissionComponent.h"

UObjective::UObjective(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UObjective::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(GetClass());
	if (BPClass != NULL)
	{
		BPClass->GetLifetimeBlueprintReplicationList(OutLifetimeProps);
	}

	DOREPLIFETIME_WITH_PARAMS_FAST(UObjective, ObjectiveState, PushReplicationParams::Default);
	DOREPLIFETIME_WITH_PARAMS_FAST(UObjective, ObjectiveProgress, PushReplicationParams::Default);
	DOREPLIFETIME_WITH_PARAMS_FAST(UObjective, ObjectiveID, PushReplicationParams::InitialOnly);
}

void UObjective::PostInitProperties()
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		TickType = ETickableTickType::Never;
		bTickEnabled = false;
		return;
	}
}

void UObjective::BeginDestroy()
{
	OwningMissionComponent = nullptr;
	WorldPrivate = nullptr;
	bTickEnabled = false;

	Super::BeginDestroy();
}

int32 UObjective::GetFunctionCallspace(UFunction* Function, FFrame* Stack)
{
	check(GetOuter());
	return GetOuter()->GetFunctionCallspace(Function, Stack);
}

bool UObjective::CallRemoteFunction(UFunction* Function, void* Parameters, FOutParmRec* OutParms, FFrame* Stack)
{
	bool bProcessed = false;

	if (AActor* MyOwner = GetTypedOuter<AActor>())
	{
		FWorldContext* const Context = GEngine->GetWorldContextFromWorld(GetWorld());
		if (Context != nullptr)
		{
			for (FNamedNetDriver& Driver : Context->ActiveNetDrivers)
			{
				if (Driver.NetDriver != nullptr && Driver.NetDriver->ShouldReplicateFunction(MyOwner, Function))
				{
					Driver.NetDriver->ProcessRemoteFunction(MyOwner, Function, Parameters, OutParms, Stack, this);
					bProcessed = true;
				}
			}
		}
	}
	return bProcessed;
}

UObjective* UObjective::CreateObjective(UObject* WorldContextObject, TSubclassOf<UObjective> ObjectiveClass, const FString& InObjectiveID)
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

	UObjective* Objective = NewObject<UObjective>(NauseaGameState, ObjectiveClass ? ObjectiveClass.Get() : UObjective::StaticClass());

	if (Objective)
	{
		Objective->ObjectiveID = InObjectiveID;
		MARK_PROPERTY_DIRTY_FROM_NAME(UObjective, ObjectiveProgress, Objective);
	}

	return Objective;
}

ENetRole UObjective::GetLocalRole() const
{
	if (GetMissionComponent())
	{
		return GetMissionComponent()->GetOwnerRole();
	}

	return ENetRole::ROLE_None;
}

bool UObjective::IsSimulatedProxy() const
{
	return GetLocalRole() != ROLE_Authority;
}

bool UObjective::IsAuthority() const
{
	return GetLocalRole() == ROLE_Authority;
}

void UObjective::Initialize(UMissionComponent* MissionComponent)
{
	ensure(MissionComponent);

	if (OwningMissionComponent)
	{
		return;
	}

	OwningMissionComponent = MissionComponent;
	WorldPrivate = GetMissionComponent()->GetWorld();

	const bool bShouldTick = bCanEverTick && !(bNeverTickOnDedicatedServer && GetMissionComponent()->IsNetMode(NM_DedicatedServer));
	TickType = bShouldTick ? ETickableTickType::Conditional : ETickableTickType::Never;
	bTickEnabled = false;
}

void UObjective::Activate()
{
	if (GetObjecitveState() != EObjectiveState::Inactive)
	{
		return;
	}

	SetObjectiveState(EObjectiveState::Active);
}

void UObjective::Complete()
{
	if (GetObjecitveState() == EObjectiveState::Completed)
	{
		return;
	}

	SetObjectiveState(EObjectiveState::Completed);
}

void UObjective::Fail()
{
	if (GetObjecitveState() == EObjectiveState::Failed)
	{
		return;
	}

	SetObjectiveState(EObjectiveState::Failed);
}

void UObjective::Reset()
{
	if (GetObjecitveState() != EObjectiveState::Inactive)
	{
		SetObjectiveState(EObjectiveState::Inactive);
	}

	SetObjectiveProgress(0.f);
}

void UObjective::MissionCompleted()
{
	//If this objective was active but the mission completed, this must be an optional objective.
	if (GetObjecitveState() != EObjectiveState::Active)
	{
		return;
	}

	ensure(IsOptionalObjective());

	SetObjectiveState(bFailObjectiveOnMissionComplete ? EObjectiveState::Failed : EObjectiveState::Completed);
}

void UObjective::SetTickEnabled(bool bEnableTick)
{
	bTickManuallyEnabled = true;

	if (bEnableTick == bTickEnabled)
	{
		return;
	}

	bTickEnabled = bEnableTick;
}

void UObjective::SetObjectiveState(EObjectiveState State)
{
	if (State == ObjectiveState)
	{
		return;
	}

	EObjectiveState PreviousState = ObjectiveState;
	ObjectiveState = State;
	OnRep_ObjectiveState(PreviousState);
	MARK_PROPERTY_DIRTY_FROM_NAME(UObjective, ObjectiveState, this);
}

void UObjective::SetObjectiveProgress(float Progress)
{
	if (!IsAuthority() || Progress == ObjectiveProgress)
	{
		return;
	}

	ObjectiveProgress = Progress;
	OnRep_ObjectiveProgress();
	MARK_PROPERTY_DIRTY_FROM_NAME(UObjective, ObjectiveProgress, this);
}

FString UObjective::DescribeObjectiveToGameplayDebugger() const
{
	return GetName();
}

void UObjective::OnRep_ObjectiveState(EObjectiveState PreviousState)
{
	if (!bTickManuallyEnabled && bTickWhenActivated && TickType != ETickableTickType::Never)
	{
		bTickEnabled = ObjectiveState == EObjectiveState::Active;
	}

	switch (GetObjecitveState())
	{
	case EObjectiveState::Active:
		K2_ActivateObjective();
		break;
	case EObjectiveState::Completed:
		K2_CompleteObjective();
		break;
	case EObjectiveState::Failed:
		K2_FailObjective();
		break;
	case EObjectiveState::Inactive:
		if (PreviousState != EObjectiveState::Inactive)
		{
			K2_ResetObjective();
		}
		break;
	}

	OnObjectiveStateChanged.Broadcast(this, GetObjecitveState());
}

void UObjective::OnRep_ObjectiveProgress()
{
	OnObjectiveProgressChanged.Broadcast(this, GetObjecitveProgress());
}

UWorld* UObjective::GetWorld_Uncached() const
{
	UWorld* ObjectWorld = nullptr;

	const UObject* Owner = GetMissionComponent();

	if (Owner && !Owner->HasAnyFlags(RF_ClassDefaultObject))
	{
		ObjectWorld = Owner->GetWorld();
	}

	if (ObjectWorld == nullptr)
	{
		if (AActor* Actor = GetTypedOuter<AActor>())
		{
			ObjectWorld = Actor->GetWorld();
		}
		else
		{
			ObjectWorld = Cast<UWorld>(GetOuter());
		}
	}

	return ObjectWorld;
}