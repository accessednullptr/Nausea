// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Objective/ObjectiveWave.h"
#include "Internationalization/StringTableRegistry.h"
#include "GameFramework/GameModeBase.h"
#include "NauseaNetDefines.h"
#include "System/SpawnCharacterSystem.h"
#include "Objective/WaveConfig/WaveSpawnLocationInterface.h"
#include "Character/CoreCharacter.h"
#include "Gameplay/StatusComponent.h"

UObjectiveWave::UObjectiveWave(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ObjectiveName = LOCTABLE("/Game/Localization/ObjectiveStringTable.ObjectiveStringTable", "Objective_Wave_Single");
}

void UObjectiveWave::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_WITH_PARAMS_FAST(UObjectiveWave, TotalWaveSizeCount, PushReplicationParams::Default);
	DOREPLIFETIME_WITH_PARAMS_FAST(UObjectiveWave, WaveStatus, PushReplicationParams::Default);
	DOREPLIFETIME_WITH_PARAMS_FAST(UObjectiveWave, IntermissionTime, PushReplicationParams::Default);

	DOREPLIFETIME_WITH_PARAMS_FAST(UObjectiveWave, WaveConfigClass, PushReplicationParams::InitialOnly);

	DOREPLIFETIME_WITH_PARAMS_FAST(UObjectiveWave, TotalWaveCount, PushReplicationParams::InitialOnly);
	DOREPLIFETIME_WITH_PARAMS_FAST(UObjectiveWave, CurrentWave, PushReplicationParams::Default);
}

void UObjectiveWave::Initialize(UMissionComponent* MissionComponent)
{
	if (GetMissionComponent())
	{
		return;
	}

	Super::Initialize(MissionComponent);

	//Non-auth has no idea if any wave config info has been overrided and so shouldn't ever run code relating to it.
	if (!IsAuthority())
	{
		return;
	}

	if (WaveConfigInstance)
	{
		WaveConfigInstance->Initialize();
	}
}

void UObjectiveWave::SetObjectiveState(EObjectiveState State)
{
	if (State == GetObjecitveState())
	{
		return;
	}

	Super::SetObjectiveState(State);

	if (!IsAuthority())
	{
		return;
	}

	switch (State)
	{
	case EObjectiveState::Active:
		break;
	default:
		USpawnCharacterSystem::CancelRequestsForObject(this, this);
	}
	
	if (!WaveConfigInstance)
	{
		return;
	}

	switch (State)
	{
	case EObjectiveState::Active:
		BindToWaveConfig();
		WaveConfigInstance->SetActive(true);
		break;
	case EObjectiveState::Completed:
		SetWaveStatus(EWaveStatus::Completed);
	default:
		UnbindToWaveConfig();
		WaveConfigInstance->SetActive(false);
	}
}

FText UObjectiveWave::GetObjecitveName() const
{
	if (WaveConfigClass && WaveConfigClass.GetDefaultObject())
	{
		const FText& OverrideName = WaveConfigClass.GetDefaultObject()->GetObjectiveNameOverride();
		if (!OverrideName.IsEmpty())
		{
			return OverrideName;
		}
	}

	return ObjectiveName;
}

FString UObjectiveWave::DescribeObjectiveToGameplayDebugger() const
{
	if (WaveConfigInstance)
	{
		FString WaveConfigDescription = WaveConfigInstance->DescribeWaveConfigToGameplayDebugger();

		if (!WaveConfigDescription.IsEmpty())
		{
			return FString::Printf(TEXT("%s %s\n        {yellow}%s"), *GetName(), *GetWaveStatusString(), *WaveConfigDescription);
		}
	}

	return FString::Printf(TEXT("%s %s"), *GetName(), *GetWaveStatusString());
}

UObjectiveWave* UObjectiveWave::CreateWaveObjective(UObject* WorldContextObject, TSubclassOf<UObjectiveWave> ObjectiveClass, const FString& InObjectiveID, TSubclassOf<UObjectiveWaveConfig> WaveConfigOverrideClass, TArray<TScriptInterface<IWaveSpawnLocationInterface>> SpawnLocationList)
{
	if (!ObjectiveClass)
	{
		return nullptr;
	}

	UObjectiveWave* WaveObjective = CreateObjective<UObjectiveWave>(WorldContextObject, ObjectiveClass, InObjectiveID);

	//If specified, override the new objective's WaveConfigClass with our override.
	if (WaveConfigOverrideClass)
	{
		WaveObjective->WaveConfigClass = WaveConfigOverrideClass;
		MARK_PROPERTY_DIRTY_FROM_NAME(UObjectiveWave, WaveConfigClass, WaveObjective);
	}

	if (!WaveObjective)
	{
		return nullptr;
	}

	//It is possible that WaveConfigClass is nullptr and the objective itself handles spawners.
	if (WaveObjective->WaveConfigClass)
	{
		WaveObjective->WaveConfigInstance = NewObject<UObjectiveWaveConfig>(WaveObjective, WaveObjective->WaveConfigClass);
	}

	//Real function call that is needed to reestablish interface pointer.
	for (TScriptInterface<IWaveSpawnLocationInterface>& SpawnLocation : SpawnLocationList)
	{
		SpawnLocation.SetInterface(Cast<IWaveSpawnLocationInterface>(SpawnLocation.GetObject()));
	}

	WaveObjective->ObjectiveSpawnLocationList = SpawnLocationList;

	return WaveObjective;
}

int32 UObjectiveWave::RequestSpawn(TArray<TSubclassOf<ACoreCharacter>>& SpawnRequest, UObjectiveWaveConfig* WaveConfig)
{
	if (GetObjecitveState() != EObjectiveState::Active)
	{
		return 0;
	}

	if (SpawnRequest.Num() == 0)
	{
		return 0;
	}

	int32 SpawnCount = 0;

	FActorSpawnParameters ActorSpawnParams;
	ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
	ActorSpawnParams.bDeferConstruction = true;

	int32 Index = SpawnRequest.Num() - 1;

	TWeakObjectPtr<UObjectiveWave> WeakThis = this;
	TWeakObjectPtr<UObjectiveWaveConfig> WeakWaveConfig = WaveConfig;
	TWeakObjectPtr<UObject> SpawnLocationObject;
	FTransform SpawnTransform;
	int32 SpawnRequestID = 0;

	//Try to use wave config if we can so that the wave can choose to cancel the spawn if it wants to (instead of trying to selectively cancel all spawns from this objective wave).
	UObject* DelegateOwner = WaveConfig ? (UObject*)WaveConfig : this;

	while (Index >= 0)
	{
		TSubclassOf<ACoreCharacter>& PlayerClass = SpawnRequest[Index];

		if (!PlayerClass)
		{
			Index--;
			continue;
		}

		ACoreCharacter* CharacterCDO = PlayerClass.GetDefaultObject();

		if (!CharacterCDO)
		{
			Index--;
			continue;
		}

		const bool bResult = USpawnCharacterSystem::RequestSpawn(this, PlayerClass, SpawnTransform, ActorSpawnParams,
			FCharacterSpawnRequestDelegate::CreateWeakLambda(DelegateOwner, [WeakThis, WeakWaveConfig, PlayerClass, SpawnTransform, SpawnRequestID] (const FSpawnRequest& Request, ACoreCharacter* Character)
			{
				if (!Character || !WeakThis.IsValid())
				{
					WeakThis->HandleFailedSpawn(PlayerClass, SpawnTransform, WeakWaveConfig.Get());
					return;
				}

				WeakThis->ProcessSpawn(Character, PlayerClass, SpawnTransform, WeakWaveConfig.Get());
			}));

		if (bResult)
		{
			SpawnCount++;
		}
		Index--;
	}

	return SpawnCount;
}

void UObjectiveWave::HandleFailedSpawn(TSubclassOf<ACoreCharacter> CharacterClass, const FTransform& SpawnTransform, UObjectiveWaveConfig* WaveConfig)
{
	if (WaveConfig)
	{
		WaveConfig->FailedSpawnCharacter(CharacterClass);
	}
}

void UObjectiveWave::ProcessSpawn(ACoreCharacter* Character, TSubclassOf<ACoreCharacter> CharacterClass, const FTransform& SpawnTransform, UObjectiveWaveConfig* WaveConfig)
{
	if (!Character || Character->IsPendingKillPending())
	{
		HandleFailedSpawn(CharacterClass, SpawnTransform, WaveConfig);
		return;
	}

	Character->AutoPossessPlayer = EAutoReceiveInput::Disabled;
	Character->AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	int32 SpawnRequestID = INDEX_NONE;
	FTransform UpdatedTransform = FTransform::Identity;
	TScriptInterface<IWaveSpawnLocationInterface> SpawnLocationInterface = GetSpawnTransform(CharacterClass, SpawnRequestID, UpdatedTransform);
	if (UpdatedTransform.Equals(FTransform::Identity))
	{
		Character->Destroy();
		HandleFailedSpawn(CharacterClass, SpawnTransform, WaveConfig);
		return;
	}

	if (SpawnLocationInterface)
	{
		SpawnLocationInterface->ProcessSpawn(Character, SpawnRequestID);
	}

	Character->FinishSpawning(UpdatedTransform, false);

	if (Character->IsPendingKillPending())
	{
		HandleFailedSpawn(CharacterClass, SpawnTransform, WaveConfig);
		return;
	}

	if (AGameModeBase* GameMode = GetWorld()->GetAuthGameMode())
	{
		GameMode->SetPlayerDefaults(Character);
	}

	if (WaveConfig)
	{
		WaveConfig->ProcessSpawnedCharacter(Character);
	}
}

TScriptInterface<IWaveSpawnLocationInterface> UObjectiveWave::GetSpawnTransform(TSubclassOf<ACoreCharacter> SpawnRequest, int32& RequestID, FTransform& SpawnTransform) const
{
	//If no scored list is available, randomly select one.
	if (ScoredObjectiveSpawnLocationList.Num() == 0)
	{
		return GetRandomSpawnTransform(SpawnRequest, SpawnTransform);
	}

	if (!ScoredObjectiveSpawnLocationList[0])
	{
		ScoredObjectiveSpawnLocationList.RemoveAt(0, 1, false);
		RequestID = 0;
		return GetSpawnTransform(SpawnRequest, RequestID, SpawnTransform);
	}

	if (ScoredObjectiveSpawnLocationList[0])
	{
		RequestID = 0;
		SpawnTransform = ScoredObjectiveSpawnLocationList[0]->GetSpawnLocation(SpawnRequest, RequestID);

		if (SpawnTransform.Equals(FTransform::Identity))
		{
			ScoredObjectiveSpawnLocationList.RemoveAt(0, 1, false);
			RequestID = 0;
			return GetSpawnTransform(SpawnRequest, RequestID, SpawnTransform);
		}

		return ScoredObjectiveSpawnLocationList[0];
	}

	return nullptr;
}

TScriptInterface<IWaveSpawnLocationInterface> UObjectiveWave::GetRandomSpawnTransform(TSubclassOf<ACoreCharacter> SpawnRequest, FTransform& SpawnTransform) const
{
	for (const TScriptInterface<IWaveSpawnLocationInterface>& WaveSpawnLocationInterface : ObjectiveSpawnLocationList)
	{
		if (!WaveSpawnLocationInterface)
		{
			continue;
		}

		int32 RandomRequestID = INDEX_NONE;
		SpawnTransform = WaveSpawnLocationInterface->GetSpawnLocation(SpawnRequest, RandomRequestID);

		if (!SpawnTransform.Equals(FTransform::Identity))
		{
			return WaveSpawnLocationInterface;
		}
	}

	return nullptr;
}

void UObjectiveWave::SetWaveStatus(EWaveStatus NewWaveStatus)
{
	if (!IsAuthority() || WaveStatus == NewWaveStatus)
	{
		return;
	}

	WaveStatus = NewWaveStatus;
	OnRep_WaveStatus();
	MARK_PROPERTY_DIRTY_FROM_NAME(UObjectiveWave, WaveStatus, this);
}

void UObjectiveWave::SetIntermissionTime(const FVector2D& NewIntermissionTime)
{
	if (!IsAuthority() || IntermissionTime == NewIntermissionTime)
	{
		return;
	}

	IntermissionTime = NewIntermissionTime;
	OnRep_IntermissionTime();
	MARK_PROPERTY_DIRTY_FROM_NAME(UObjectiveWave, IntermissionTime, this);
}

void UObjectiveWave::SetTotalWaveSizeCount(int32 NewTotalWaveSizeCount)
{
	if (!IsAuthority() || TotalWaveSizeCount == NewTotalWaveSizeCount)
	{
		return;
	}

	TotalWaveSizeCount = NewTotalWaveSizeCount;
	OnRep_TotalWaveSizeCount();
	MARK_PROPERTY_DIRTY_FROM_NAME(UObjectiveWave, TotalWaveSizeCount, this);
}

FText UObjectiveWave::GetFormattedWaveObjecitveName() const
{
	return FText::FormatNamed(GetObjecitveName(),
		TEXT("CurrentWave"), GetCurrentWave(),
		TEXT("TotalWaveCount"), GetTotalWaveCount());
}

void UObjectiveWave::SetCurrentWave(int32 NewCurrentWave)
{
	CurrentWave = NewCurrentWave;
	OnRep_CurrentWave();
	MARK_PROPERTY_DIRTY_FROM_NAME(UObjectiveWave, CurrentWave, this);
}

void UObjectiveWave::SetTotalWaveCount(int32 NewTotalWaveCount)
{
	TotalWaveCount = NewTotalWaveCount;
	OnRep_TotalWaveCount();
	MARK_PROPERTY_DIRTY_FROM_NAME(UObjectiveWave, TotalWaveCount, this);
}

FString UObjectiveWave::GetWaveStatusString() const
{
	switch (WaveStatus)
	{
	case EWaveStatus::Inactive:
		return "{grey}INACTIVE";
	case EWaveStatus::InProgress:
		return "{white}IN PROGRESS";
	case EWaveStatus::Intermission:
		return "{orange}INTERMISSION";
	case EWaveStatus::Completed:
		return "{green}COMPLETED";
	}

	return "{purple}INVALID";
}

void UObjectiveWave::BindToWaveConfig()
{
	if (!WaveConfigInstance)
	{
		return;
	}

	if (!WaveConfigInstance->OnWaveProgressUpdate.IsAlreadyBound(this, &UObjectiveWave::WaveConfigProgressUpdate))
	{
		WaveConfigInstance->OnWaveProgressUpdate.AddDynamic(this, &UObjectiveWave::WaveConfigProgressUpdate);
		WaveConfigInstance->OnWaveStatusUpdate.AddDynamic(this, &UObjectiveWave::WaveConfigStatusUpdate);
		WaveConfigInstance->OnWaveCompleted.AddDynamic(this, &UObjectiveWave::WaveConfigCompleted);

		WaveConfigInstance->OnWaveConfigCurrentWaveChanged.AddDynamic(this, &UObjectiveWave::WaveConfigCurrentWaveUpdate);
		WaveConfigInstance->OnWaveConfigTotalWaveCountChanged.AddDynamic(this, &UObjectiveWave::WaveConfigTotalWaveCountUpdate);
	}
}

void UObjectiveWave::UnbindToWaveConfig()
{
	if (!WaveConfigInstance)
	{
		return;
	}

	WaveConfigInstance->OnWaveProgressUpdate.RemoveDynamic(this, &UObjectiveWave::WaveConfigProgressUpdate);
	WaveConfigInstance->OnWaveStatusUpdate.RemoveDynamic(this, &UObjectiveWave::WaveConfigStatusUpdate);
	WaveConfigInstance->OnWaveCompleted.RemoveDynamic(this, &UObjectiveWave::WaveConfigCompleted);

	WaveConfigInstance->OnWaveConfigCurrentWaveChanged.RemoveDynamic(this, &UObjectiveWave::WaveConfigCurrentWaveUpdate);
	WaveConfigInstance->OnWaveConfigTotalWaveCountChanged.RemoveDynamic(this, &UObjectiveWave::WaveConfigTotalWaveCountUpdate);
}

void UObjectiveWave::WaveConfigProgressUpdate(UObjectiveWaveConfig* Wave, float Progress)
{
	SetObjectiveProgress(Progress);
}

void UObjectiveWave::WaveConfigStatusUpdate(UObjectiveWaveConfig* Wave, EWaveStatus Status)
{
	SetWaveStatus(Status);

	switch (Status)
	{
	case EWaveStatus::InProgress:
		TotalWaveSizeCount = WaveConfigInstance->GetTotalSpawnCount();
		break;
	default:
		TotalWaveSizeCount = -1;
	}

	OnRep_TotalWaveSizeCount();
	MARK_PROPERTY_DIRTY_FROM_NAME(UObjectiveWave, TotalWaveSizeCount, this);
}

void UObjectiveWave::WaveConfigCompleted(UObjectiveWaveConfig* Wave)
{
	if (Wave != WaveConfigInstance)
	{
		return;
	}

	SetObjectiveState(EObjectiveState::Completed);
}

void UObjectiveWave::WaveConfigCurrentWaveUpdate(UObjectiveWaveConfig* Wave, int32 NewCurrentWave)
{
	CurrentWave = NewCurrentWave;
}

void UObjectiveWave::WaveConfigTotalWaveCountUpdate(UObjectiveWaveConfig* Wave, int32 NewTotalWaveCount)
{
	SetTotalWaveCount(NewTotalWaveCount);
}

void UObjectiveWave::OnRep_TotalWaveSizeCount()
{
	OnObjectiveWaveSizeChanged.Broadcast(this, GetTotalWaveSizeCount());
}

void UObjectiveWave::OnRep_WaveStatus()
{
	OnObjectiveWaveStatusChanged.Broadcast(this, GetWaveStatus());
}

void UObjectiveWave::OnRep_IntermissionTime()
{
	OnObjectiveWaveIntermissionTimeChanged.Broadcast(this, IntermissionTime.X, IntermissionTime.Y);
}

void UObjectiveWave::OnRep_TotalWaveCount()
{
	OnObjectiveTotalWaveCountChanged.Broadcast(this, GetTotalWaveCount());
}

void UObjectiveWave::OnRep_CurrentWave()
{
	OnObjectiveCurrentWaveCountChanged.Broadcast(this, GetCurrentWave());
}

UObjectiveWaveConfig::UObjectiveWaveConfig(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UObjectiveWaveConfig::PostInitProperties()
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		return;
	}

	OwningObjective = GetTypedOuter<UObjectiveWave>();
}

void UObjectiveWaveConfig::Initialize()
{
	ensure(GetOwningObjective());
}

void UObjectiveWaveConfig::SetActive(bool bNewIsActive)
{
	if (!GetOwningObjective()->IsAuthority() || bIsActive == bNewIsActive)
	{
		return;
	}

	bIsActive = bNewIsActive;

	if (bIsActive)
	{
		StartSpawning();
	}
	else
	{
		StopSpawning();
	}
}

FString UObjectiveWaveConfig::DescribeWaveConfigToGameplayDebugger() const
{
	return GetName();
}

void UObjectiveWaveConfig::StartSpawning()
{
	WaveStartTime = GetWorld()->GetTimeSeconds();
	SetWaveStatus(EWaveStatus::InProgress);
}

void UObjectiveWaveConfig::StopSpawning()
{
	SetWaveStatus(EWaveStatus::Completed);

	CurrentlySpawnedCharacters.RemoveSwap(nullptr);

	//Unbind from all spawned characters before killing/destroying/not doing anything to them.
	for (TWeakObjectPtr<ACoreCharacter> Character : CurrentlySpawnedCharacters)
	{
		if (!Character.IsValid())
		{
			continue;
		}

		UnbindToSpawnedCharacter(Character.Get());
	}

	switch (EndWaveCharacterHandlingMethod)
	{
	case EEndWaveCharacterHandlingMethod::Kill:
		{
			int32 Index = CurrentlySpawnedCharacters.Num() - 1;
			while (Index >= 0)
			{
				CurrentlySpawnedCharacters[Index]->Kill(0.f, FDamageEvent(UWaveEndKillDamageType::StaticClass()), nullptr, nullptr);
				Index--;
			}
		}
		break;
	case EEndWaveCharacterHandlingMethod::Destroy:
		{
			int32 Index = CurrentlySpawnedCharacters.Num() - 1;
			while (Index >= 0)
			{
				CurrentlySpawnedCharacters[Index]->Destroy();
				Index--;
			}
		}
		break;
	case EEndWaveCharacterHandlingMethod::Ignore:
		break;
	}

	USpawnCharacterSystem::CancelRequestsForObject(this, this);
	CurrentlySpawnedCharacters.Empty();
}

void UObjectiveWaveConfig::SpawningCompleted()
{
	if (IsActive())
	{
		SetActive(false);
	}
}

void UObjectiveWaveConfig::SetWaveStatus(EWaveStatus NewWaveStatus)
{
	if (WaveStatus == NewWaveStatus)
	{
		return;
	}

	WaveStatus = NewWaveStatus;
	OnWaveStatusUpdate.Broadcast(this, WaveStatus);

	switch (WaveStatus)
	{
	case EWaveStatus::Completed:
		OnWaveCompleted.Broadcast(this);
		break;
	}
}

void UObjectiveWaveConfig::ProcessSpawnedCharacter(ACoreCharacter* SpawnedCharacter)
{
	if (!SpawnedCharacter)
	{
		return;
	}

	CurrentlySpawnedCharacters.Add(SpawnedCharacter);
	BindToSpawnedCharacter(SpawnedCharacter);
}

void UObjectiveWaveConfig::FailedSpawnCharacter(TSubclassOf<ACoreCharacter> CharacterClass)
{

}

void UObjectiveWaveConfig::BindToSpawnedCharacter(ACoreCharacter* SpawnedCharacter)
{
	SpawnedCharacter->OnEndPlay.AddDynamic(this, &UObjectiveWaveConfig::SpawnedCharacterDestroyed);
	SpawnedCharacter->GetStatusComponent()->OnDied.AddDynamic(this, &UObjectiveWaveConfig::SpawnedCharacterDied);
}

void UObjectiveWaveConfig::UnbindToSpawnedCharacter(ACoreCharacter* SpawnedCharacter)
{
	SpawnedCharacter->OnEndPlay.RemoveDynamic(this, &UObjectiveWaveConfig::SpawnedCharacterDestroyed);
	SpawnedCharacter->GetStatusComponent()->OnDied.RemoveDynamic(this, &UObjectiveWaveConfig::SpawnedCharacterDied);
}

void UObjectiveWaveConfig::SpawnedCharacterDestroyed(AActor* Actor, EEndPlayReason::Type EndPlayReason)
{
	ACoreCharacter* Character = Cast<ACoreCharacter>(Actor);

	if (!Character || !CurrentlySpawnedCharacters.Contains(Character))
	{
		return;
	}

	SpawnedCharacterDied(Character->GetStatusComponent(), 0.f, FDamageEvent(), nullptr, nullptr);
}

void UObjectiveWaveConfig::SpawnedCharacterDied(UStatusComponent* Component, float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!Component)
	{
		return;
	}

	UnbindToSpawnedCharacter(Cast<ACoreCharacter>(Component->GetOwner()));
	CurrentlySpawnedCharacters.RemoveSwap(Cast<ACoreCharacter>(Component->GetOwner()));
	CurrentlySpawnedCharacters.RemoveSwap(nullptr);
}

UWaveEndKillDamageType::UWaveEndKillDamageType(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCausedByWorld = true;
}