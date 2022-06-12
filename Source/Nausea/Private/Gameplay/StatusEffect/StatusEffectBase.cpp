// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Gameplay/StatusEffect/StatusEffectBase.h"
#include "Engine/NetDriver.h"
#include "NauseaGlobalDefines.h"
#include "NauseaNetDefines.h"
#include "Gameplay/StatusComponent.h"
#include "Character/CoreCharacter.h"
#include "Character/CoreCharacterAnimInstanceTypes.h"
#include "AI/CoreAIController.h"
#include "AI/CorePathFollowingComponent.h"
#include "GameFramework/GameState.h"

#if WITH_EDITOR
#include "Editor.h"
extern UNREALED_API UEditorEngine* GEditor;
#endif

UStatusEffectBase::UStatusEffectBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetSkipReplicationLogic(ESkipReplicationLogic::SkipOwnerInitial);
}

void UStatusEffectBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	UBlueprintGeneratedClass* BPClass = Cast<UBlueprintGeneratedClass>(GetClass());
	if (BPClass != nullptr)
	{
		BPClass->GetLifetimeBlueprintReplicationList(OutLifetimeProps);
	}

	DOREPLIFETIME_WITH_PARAMS_FAST(UStatusEffectBase, RefreshCounter, PushReplicationParams::Default);
	DOREPLIFETIME_WITH_PARAMS_FAST(UStatusEffectBase, StatusEffectInsitgator, PushReplicationParams::Default);
	DOREPLIFETIME_WITH_PARAMS_FAST(UStatusEffectBase, StatusEffectInstigationDirection, PushReplicationParams::Default);
}

void UStatusEffectBase::PostInitProperties()
{
	Super::PostInitProperties();
}

void UStatusEffectBase::PreNetReceive()
{
	Super::PreNetReceive();

	if (!IsInitialized())
	{
		UStatusComponent* OuterStatusComponent = UStatusInterfaceStatics::GetStatusComponent(TScriptInterface<IStatusInterface>(GetTypedOuter<AActor>()));
		Initialize(OuterStatusComponent, nullptr, -1.f, FAISystem::InvalidDirection);
	}
}

void UStatusEffectBase::PostNetReceive()
{
	Super::PostNetReceive();

	if (!bDoneRemotePostNetReceiveActivation)
	{
		bDoneRemotePostNetReceiveActivation = true;

		ensure(OwningStatusComponent);
		OwningStatusComponent->OnStatusEffectAdded(this);
		OnActivated(EStatusBeginType::Initial);
	}
}

void UStatusEffectBase::PreDestroyFromReplication()
{
	Super::PreDestroyFromReplication();

	if (IsInitialized())
	{
		OnDestroyed();
	}
}

void UStatusEffectBase::BeginDestroy()
{
	WorldPrivate = nullptr;
	OwningStatusComponent = nullptr;
	Super::BeginDestroy();
}

int32 UStatusEffectBase::GetFunctionCallspace(UFunction* Function, FFrame* Stack)
{
	check(GetOuter());
	return GetOuter()->GetFunctionCallspace(Function, Stack);
}

bool UStatusEffectBase::CallRemoteFunction(UFunction* Function, void* Parameters, FOutParmRec* OutParams, FFrame* Stack)
{
	bool bProcessed = false;

	if (AActor* MyOwner = GetOwningStatusComponent()->GetOwner())
	{
		FWorldContext* const Context = GEngine->GetWorldContextFromWorld(GetWorld());
		if (Context != nullptr)
		{
			for (FNamedNetDriver& Driver : Context->ActiveNetDrivers)
			{
				if (Driver.NetDriver != nullptr && Driver.NetDriver->ShouldReplicateFunction(MyOwner, Function))
				{
					Driver.NetDriver->ProcessRemoteFunction(MyOwner, Function, Parameters, OutParams, Stack, this);
					bProcessed = true;
				}
			}
		}
	}
	return bProcessed;
}

void UStatusEffectBase::Initialize(UStatusComponent* StatusComponent, ACorePlayerState* Instigator, float Power, const FVector& InstigationDirection)
{
	if (IsInitialized())
	{
		return;
	}

	ensure(StatusComponent);
	OwningStatusComponent = StatusComponent;
	WorldPrivate = GetOwningStatusComponent()->GetWorld();

	if (Instigator)
	{
		UpdateInstigator(Instigator);
	}

	if (InstigationDirection != FAISystem::InvalidDirection)
	{
		StatusEffectInstigationDirection = InstigationDirection;
	}

	OwningStatusComponent->OnDied.AddDynamic(this, &UStatusEffectBase::OnOwnerDied);
}

void UStatusEffectBase::OnDestroyed()
{
	OnDeactivated(EStatusEndType::Expired);
}

bool UStatusEffectBase::CanActivateStatus(ACorePlayerState* Instigator, float Power) const
{
	return true;
}

bool UStatusEffectBase::CanRefreshStatus(ACorePlayerState* Instigator, float Power) const
{
	return true;
}

void UStatusEffectBase::AddEffectPower(ACorePlayerState* Instigator, float Power, const FVector& InstigationDirection)
{

}

void UStatusEffectBase::OnActivated(EStatusBeginType BeginType)
{
	K2_OnActivated(BeginType);

	OnEffectBegin.Broadcast(this, BeginType);
	GetOwningStatusComponent()->OnStatusEffectBegin.Broadcast(GetOwningStatusComponent(), this, BeginType);
}

void UStatusEffectBase::OnDeactivated(EStatusEndType EndType)
{
	if (!IsInitialized() || IsPendingKill())
	{
		return;
	}

	K2_OnDeactivated(EndType);

	OnEffectEnd.Broadcast(this, EndType);
	GetOwningStatusComponent()->OnStatusEffectEnd.Broadcast(GetOwningStatusComponent(), this, EndType);

	OwningStatusComponent->OnDied.RemoveDynamic(this, &UStatusEffectBase::OnOwnerDied);
	ClearStatModifiers();
	UnblockCharacterActions();
	ResetIgnoredByCrowdManager();
	ResetPerformPanicMovement();

	if (GetWorld())
	{
		GetWorld()->GetLatentActionManager().RemoveActionsForObject(this);
		GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
	}

	OwningStatusComponent->OnStatusEffectRemoved(this);

	WorldPrivate = nullptr;
	OwningStatusComponent = nullptr;
	MarkPendingKill();
}

void UStatusEffectBase::UpdateInstigator(ACorePlayerState* Instigator)
{
	StatusEffectInsitgator = Instigator;
	MARK_PROPERTY_DIRTY_FROM_NAME(UStatusEffectBase, StatusEffectInsitgator, this);
	OnRep_StatusEffectInsitgator();
}

void UStatusEffectBase::UpdateInstigationDirection(const FVector& Direction)
{
	StatusEffectInstigationDirection = Direction;
	MARK_PROPERTY_DIRTY_FROM_NAME(UStatusEffectBase, StatusEffectInstigationDirection, this);
	OnRep_StatusEffectInstigationDirection();
}

bool UStatusEffectBase::IsAuthority() const
{
	if (UStatusComponent* StatusComponent = GetOwningStatusComponent())
	{
		return StatusComponent->GetOwner()->GetLocalRole() == ROLE_Authority;
	}

	return false;
}

bool UStatusEffectBase::IsAutonomousProxy() const
{
	if (UStatusComponent* StatusComponent = GetOwningStatusComponent())
	{
		return StatusComponent->GetOwner()->GetLocalRole() == ROLE_AutonomousProxy;
	}

	return false;
}

bool UStatusEffectBase::IsSimulatedProxy() const
{
	if (UStatusComponent* StatusComponent = GetOwningStatusComponent())
	{
		return StatusComponent->GetOwner()->GetLocalRole() == ROLE_SimulatedProxy;
	}

	return true;
}

void UStatusEffectBase::SetStatModifier(EStatusEffectStatModifier Stat, float InModifier)
{
	UStatusComponent* StatusComponent = GetOwningStatusComponent();

	if (!StatusComponent)
	{
		return;
	}

	if (StatusModificationMap.Contains(Stat))
	{
		if (InModifier == 1.f)
		{
			UnbindStatModifier(Stat, StatusModificationMap[Stat]);
			StatusModificationMap.Remove(Stat);
		}
		else
		{
			StatusModificationMap[Stat].Value = InModifier;
			UpdateStatModifier(Stat);
		}
		return;
	}

	FStatusEffectDelegateEntry& StatusEffectDelegateEntry = StatusModificationMap.Add(Stat);
	StatusEffectDelegateEntry.Value = InModifier;
	BindStatModifier(Stat, StatusEffectDelegateEntry);
}

float UStatusEffectBase::GetStatModifier(EStatusEffectStatModifier Stat) const
{
	if (StatusModificationMap.Contains(Stat))
	{
		return StatusModificationMap[Stat].Value;
	}

	return 1.f;
}

void UStatusEffectBase::ClearStatModifiers()
{
	TArray<EStatusEffectStatModifier> KeyArray;
	StatusModificationMap.GenerateKeyArray(KeyArray);
	for (EStatusEffectStatModifier Key : KeyArray)
	{
		UnbindStatModifier(Key, StatusModificationMap[Key]);
	}

	StatusModificationMap.Empty();
}

void UStatusEffectBase::RequestMovementSpeedUpdate()
{
	if (UStatusComponent* StatusComponent = GetOwningStatusComponent())
	{
		StatusComponent->RequestMovementSpeedUpdate();
	}
}

void UStatusEffectBase::RequestRotationRateUpdate()
{
	if (UStatusComponent* StatusComponent = GetOwningStatusComponent())
	{
		StatusComponent->RequestRotationRateUpdate();
	}
}

void UStatusEffectBase::BlockCharacterActions(bool bInterruptCurrentAction)
{
	if (UStatusComponent* StatusComponent = GetOwningStatusComponent())
	{
		StatusComponent->RequestActionBlock(BlockActionID, bInterruptCurrentAction);
	}
}

void UStatusEffectBase::UnblockCharacterActions()
{
	if (BlockActionID == INDEX_NONE)
	{
		return;
	}

	if (UStatusComponent* StatusComponent = GetOwningStatusComponent())
	{
		StatusComponent->RevokeActionBlock(BlockActionID);
	}
}

void UStatusEffectBase::SetIgnoredByCrowdManager(bool bInIgnoredByCrowdManager)
{
	if (bIgnoredByCrowdManager == bInIgnoredByCrowdManager)
	{
		return;
	}

	UStatusComponent* StatusComponent = GetOwningStatusComponent();

	if (!StatusComponent)
	{
		return;
	}

	ACoreCharacter* Character = StatusComponent->GetOwner<ACoreCharacter>();

	if (!Character)
	{
		return;
	}

	ACoreAIController* CoreAIController = Cast<ACoreAIController>(Character->GetController());

	if (!CoreAIController)
	{
		return;
	}

	UCorePathFollowingComponent* PathFollowingComponent = Cast<UCorePathFollowingComponent>(CoreAIController->GetPathFollowingComponent());

	if (!PathFollowingComponent)
	{
		return;
	}

	bIgnoredByCrowdManager = bInIgnoredByCrowdManager;
	if (bIgnoredByCrowdManager)
	{
		PathFollowingComponent->RequestIgnoredByCrowdManager(this);
	}
	else
	{
		PathFollowingComponent->RevokeIgnoredByCrowdManager(this);
	}
}

void UStatusEffectBase::ResetIgnoredByCrowdManager()
{
	SetIgnoredByCrowdManager(false);
}

void UStatusEffectBase::SetPerformPanicMovement(bool bInPerformPanicMovement)
{
	if (bPerformingPanicMovement == bInPerformPanicMovement)
	{
		return;
	}
}

void UStatusEffectBase::ResetPerformPanicMovement()
{
	SetPerformPanicMovement(false);
}

#define BIND_STAT_MODIFIER(Delegate, ...)\
StatusEffectModifierEntry.StatusEffectDelegate = StatusComponent->Delegate.AddWeakLambda(this, [WeakStatusEffect, Stat](__VA_ARGS__){\
if (WeakStatusEffect.IsValid()) { Value *= WeakStatusEffect->GetStatModifier(Stat); } });\

#define BIND_STAT_MODIFIER_BOOL(Delegate, Operator, ...)\
StatusEffectModifierEntry.StatusEffectDelegate = StatusComponent->Delegate.AddWeakLambda(this, [WeakStatusEffect, Stat](__VA_ARGS__){\
if (WeakStatusEffect.IsValid()) { Value Operator (WeakStatusEffect->GetStatModifier(Stat) == 1.f); } });\

void UStatusEffectBase::BindStatModifier(EStatusEffectStatModifier Stat, FStatusEffectDelegateEntry& StatusEffectModifierEntry)
{
	UStatusComponent* StatusComponent = GetOwningStatusComponent();

	if (!StatusComponent)
	{
		return;
	}

	TWeakObjectPtr<UStatusEffectBase> WeakStatusEffect = this;

	switch (Stat)
	{
	case EStatusEffectStatModifier::MovementSpeed:
		BIND_STAT_MODIFIER(OnProcessMovementSpeed, const UStatusComponent*, float& Value);
		break;
	case EStatusEffectStatModifier::RotationRate:
		BIND_STAT_MODIFIER(OnProcessRotationRate, const UStatusComponent*, float& Value);
		break;
	case EStatusEffectStatModifier::DamageTaken:
		BIND_STAT_MODIFIER(OnProcessDamageTaken, UStatusComponent*, float& Value, const struct FDamageEvent&, ACorePlayerState*);
		break;
	case EStatusEffectStatModifier::DamageDealt:
		BIND_STAT_MODIFIER(OnProcessDamageDealt, UStatusComponent*, float& Value, const struct FDamageEvent&, ACorePlayerState*);
		break;
	case EStatusEffectStatModifier::StatusPowerTaken:
		BIND_STAT_MODIFIER(OnProcessStatusPowerTaken, AActor*, float& Value, const struct FDamageEvent&, EStatusType);
		break;
	case EStatusEffectStatModifier::StatusPowerDealt:
		BIND_STAT_MODIFIER(OnProcessStatusPowerDealt, AActor*, float& Value, const struct FDamageEvent&, EStatusType);
		break;
	case EStatusEffectStatModifier::ActionDisabled:
		BIND_STAT_MODIFIER_BOOL(OnProcessActionDisabled, |=, const UStatusComponent*, bool& Value);
		break;
	}

	UpdateStatModifier(Stat);
}

void UStatusEffectBase::UpdateStatModifier(EStatusEffectStatModifier Stat)
{
	switch (Stat)
	{
	case EStatusEffectStatModifier::MovementSpeed:
		RequestMovementSpeedUpdate();
		break;
	case EStatusEffectStatModifier::RotationRate:
		RequestRotationRateUpdate();
		break;
	}
}

#define UNBIND_STAT_MODIFIER(Delegate, ...)\
StatusComponent->Delegate.Remove(StatusEffectModifierEntry.StatusEffectDelegate);\

void UStatusEffectBase::UnbindStatModifier(EStatusEffectStatModifier Stat, FStatusEffectDelegateEntry& StatusEffectModifierEntry)
{
	UStatusComponent* StatusComponent = GetOwningStatusComponent();

	if (!StatusComponent)
	{
		return;
	}

	switch (Stat)
	{
	case EStatusEffectStatModifier::MovementSpeed:
		UNBIND_STAT_MODIFIER(OnProcessMovementSpeed);
		break;
	case EStatusEffectStatModifier::RotationRate:
		UNBIND_STAT_MODIFIER(OnProcessRotationRate);
		break;
	case EStatusEffectStatModifier::DamageTaken:
		UNBIND_STAT_MODIFIER(OnProcessDamageTaken);
		break;
	case EStatusEffectStatModifier::DamageDealt:
		UNBIND_STAT_MODIFIER(OnProcessDamageDealt);
		break;
	case EStatusEffectStatModifier::StatusPowerTaken:
		UNBIND_STAT_MODIFIER(OnProcessStatusPowerTaken);
		break;
	case EStatusEffectStatModifier::StatusPowerDealt:
		UNBIND_STAT_MODIFIER(OnProcessStatusPowerDealt);
		break;
	case EStatusEffectStatModifier::ActionDisabled:
		UNBIND_STAT_MODIFIER(OnProcessActionDisabled);
		break;
	}

	UpdateStatModifier(Stat);
}

void UStatusEffectBase::OnOwnerDied(UStatusComponent* StatusComponent, float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	K2_OnOwnerDied(Damage, DamageEvent, EventInstigator, DamageCauser);
	OnDeactivated(EStatusEndType::Interrupted);
}

void UStatusEffectBase::OnRep_RefreshCounter()
{
	OnActivated(EStatusBeginType::Refresh);
}

void UStatusEffectBase::OnRep_StatusEffectInsitgator()
{

}

void UStatusEffectBase::OnRep_StatusEffectInstigationDirection()
{
	UE_LOG(LogTemp, Warning, TEXT("direction %s"), *StatusEffectInstigationDirection.ToString());
}

UWorld* UStatusEffectBase::GetWorld_Uncached() const
{
	UWorld* ObjectWorld = nullptr;

	const UObject* Owner = GetOwningStatusComponent();

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

UStatusEffectBasic::UStatusEffectBasic(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UStatusEffectBasic::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_WITH_PARAMS_FAST(UStatusEffectBasic, StatusTime, PushReplicationParams::Default);
	DOREPLIFETIME_WITH_PARAMS_FAST(UStatusEffectBasic, CurrentPower, PushReplicationParams::Default);
	DOREPLIFETIME_WITH_PARAMS_FAST(UStatusEffectBasic, bCriticalPointReached, PushReplicationParams::Default);
}

void UStatusEffectBasic::PostInitProperties()
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		TickType = ETickableTickType::Never;
		bTickEnabled = false;
		bK2TickImplemented = IS_K2_FUNCTION_IMPLEMENTED(this, K2_Tick);
		bK2ProcessDamageImplemented = IS_K2_FUNCTION_IMPLEMENTED(this, K2_ProcessDamage);
		bK2ReceivedDamageImplemented = IS_K2_FUNCTION_IMPLEMENTED(this, K2_ReceivedDamage);
		return;
	}

#if WITH_EDITOR
	if (GEditor)
	{
		bK2TickImplemented = IS_K2_FUNCTION_IMPLEMENTED(this, K2_Tick);
		bK2ProcessDamageImplemented = IS_K2_FUNCTION_IMPLEMENTED(this, K2_ProcessDamage);
		bK2ReceivedDamageImplemented = IS_K2_FUNCTION_IMPLEMENTED(this, K2_ReceivedDamage);
	}
#endif
}

void UStatusEffectBasic::BeginDestroy()
{
	TickType = ETickableTickType::Never;
	bTickEnabled = false;

	Super::BeginDestroy();
}

void UStatusEffectBasic::Initialize(UStatusComponent* StatusComponent, ACorePlayerState* Instigator, float Power, const FVector& InstigationDirection)
{
	if (IsInitialized())
	{
		return;
	}

	Super::Initialize(StatusComponent, Instigator, Power, InstigationDirection);

	if (Power != -1.f)
	{
		CurrentPower = Power;
		MARK_PROPERTY_DIRTY_FROM_NAME(UStatusEffectBasic, CurrentPower, this);
	}

	if (bK2TickImplemented)
	{
		bTickEnabled = true;
		TickType = ETickableTickType::Always;
	}
	else if (PowerDecayRate > 0.f)
	{
		bTickEnabled = true;
		TickType = ETickableTickType::Conditional;
	}

	if (ShouldBindToProcessDamage())
	{
		ProcessDamageHandle = StatusComponent->OnProcessDamageTaken.AddUObject(this, &UStatusEffectBasic::ProcessDamage);
	}

	if (ShouldBindToReceivedDamage())
	{
		StatusComponent->OnDamageReceived.AddDynamic(this, &UStatusEffectBasic::ReceivedDamage);
	}
}

void UStatusEffectBasic::OnDestroyed()
{
	bool bWasInterrupted = false;

	if (AGameState* GameState = GetWorld()->GetGameState<AGameState>())
	{
		bWasInterrupted = FMath::IsNearlyEqual(StatusTime.Y, GameState->GetServerWorldTimeSeconds(), 0.25f);
	}

	GetWorld()->GetTimerManager().ClearTimer(StatusTimer);

	OnDeactivated(bWasInterrupted ? EStatusEndType::Interrupted : EStatusEndType::Expired);
}

void UStatusEffectBasic::OnActivated(EStatusBeginType BeginType)
{
	if (IsAuthority())
	{
		const float Duration = GetDurationAtCurrentPower();

		if (Duration != -1.f)
		{
			GetWorld()->GetTimerManager().ClearTimer(StatusTimer);
			GetWorld()->GetTimerManager().SetTimer(StatusTimer,
				FTimerDelegate::CreateUObject(this, &UStatusEffectBasic::OnDeactivated, EStatusEndType::Expired),
				Duration, false);

			StatusTime.X = GetWorld()->GetGameState<AGameState>()->GetServerWorldTimeSeconds();
			StatusTime.Y = StatusTime.X + Duration;
		}
		else
		{
			StatusTime.X = -1.f;
			StatusTime.Y = -1.f;
		}
		
		OnRep_StatusTime();
		MARK_PROPERTY_DIRTY_FROM_NAME(UStatusEffectBasic, StatusTime, this);
	}

	if (PowerDecayDelay > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(PowerDecayTimer, PowerDecayDelay, false);
	}

	Super::OnActivated(BeginType);
}

void UStatusEffectBasic::OnDeactivated(EStatusEndType EndType)
{
	if (!IsInitialized() || IsPendingKill())
	{
		return;
	}

	TickType = ETickableTickType::Never;
	bTickEnabled = false;

	if (ShouldBindToProcessDamage() && ProcessDamageHandle.IsValid())
	{
		GetOwningStatusComponent()->OnProcessDamageTaken.Remove(ProcessDamageHandle);
		ProcessDamageHandle.Reset();
	}

	if (ShouldBindToReceivedDamage())
	{
		GetOwningStatusComponent()->OnDamageReceived.RemoveDynamic(this, &UStatusEffectBasic::ReceivedDamage);
	}

	Super::OnDeactivated(EndType);
}

bool UStatusEffectBasic::CanActivateStatus(ACorePlayerState* Instigator, float Power) const
{
	switch (StatusEffectType)
	{
	case EBasicStatusEffectType::Cumulative:
		if (HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject)) { return true; }
	case EBasicStatusEffectType::Instant:
		return Power >= GetPowerRequirement();
	}

	ensure(false);
	return false;
}

bool UStatusEffectBasic::CanRefreshStatus(ACorePlayerState* Instigator, float Power) const
{
	switch (StatusEffectType)
	{
	case EBasicStatusEffectType::Cumulative:
		return true;
	case EBasicStatusEffectType::Instant:
		return CanActivateStatus(Instigator, Power);
	}

	ensure(false);
	return false;
}

void UStatusEffectBasic::AddEffectPower(ACorePlayerState* Instigator, float Power, const FVector& InstigationDirection)
{
	const float CachedCurrentPower = CurrentPower;

	switch (StatusEffectType)
	{
	case EBasicStatusEffectType::Instant:
		CurrentPower = FMath::Max(CurrentPower, FMath::Min(Power, EffectPowerRange.Y));
		break;
	case EBasicStatusEffectType::Cumulative:
		CurrentPower += Power;
		CurrentPower = FMath::Min(CurrentPower, EffectPowerRange.Y);
		break;
	}

	switch (InstigationUpdateDirectionRule)
	{
	case EInstigationDirectionUpdateRule::Always:
		UpdateInstigationDirection(InstigationDirection);
		break;
	}

	if (InstigatorUpdateRule == EBasicStatusEffectInstigatorRule::LargestAmount)
	{
		switch (StatusEffectType)
		{
		case EBasicStatusEffectType::Instant:
			if (CurrentPower > CachedCurrentPower)
			{
				UpdateInstigator(Instigator);
			}
			break;
		case EBasicStatusEffectType::Cumulative:
			UpdateInsitgatorCumulativePower(Instigator, CurrentPower - CachedCurrentPower); //Only apply the delta, not the flat amount.
			break;
		}
	}

	OnRep_CurrentPower();

	if (CanRefreshStatus(Instigator, Power))
	{
		RefreshCounter++;
		MARK_PROPERTY_DIRTY_FROM_NAME(UStatusEffectBase, RefreshCounter, this);

		if (InstigatorUpdateRule == EBasicStatusEffectInstigatorRule::Refresh && StatusEffectType != EBasicStatusEffectType::Cumulative)
		{
			UpdateInstigator(Instigator);
		}

		switch (InstigationUpdateDirectionRule)
		{
		case EInstigationDirectionUpdateRule::Refresh:
			UpdateInstigationDirection(InstigationDirection);
			break;
		}

		OnRep_RefreshCounter();
	}

	if (IsCriticalPointReached())
	{
		SetCriticalPointReached(true);
	}

	if (CachedCurrentPower != CurrentPower)
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(UStatusEffectBasic, CurrentPower, this);
	}
}

void UStatusEffectBasic::GetStatusTime(float& StartTime, float& EndTime) const
{
	StartTime = StatusTime.X;
	EndTime = StatusTime.Y;
}

float UStatusEffectBasic::GetStatusTimeRemaining() const
{
	if (!GetWorld() || !GetWorld()->GetGameState())
	{
		return -1.f;
	}

	return FMath::Max(0.f, StatusTime.Y - GetWorld()->GetGameState()->GetServerWorldTimeSeconds());
}

void UStatusEffectBasic::Tick(float DeltaTime)
{
	if (!IsInitialized())
	{
		MarkPendingKill();
		return;
	}

	const float CachedCurrentPower = CurrentPower;

	if (!GetWorld()->GetTimerManager().IsTimerActive(PowerDecayTimer))
	{
		CurrentPower -= PowerDecayRate * DeltaTime;
		CurrentPower = FMath::Max(CurrentPower, 0.f);
		OnRep_CurrentPower();
	}

	if (bK2TickImplemented)
	{
		K2_Tick(DeltaTime);
	}

	if (CachedCurrentPower != CurrentPower)
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(UStatusEffectBasic, CurrentPower, this);
	}
}

float UStatusEffectBasic::GetDurationAtCurrentPower() const
{
	const float Power = GetPowerPercent();
	const float CurrentDuration = GetWorld()->GetTimerManager().GetTimerRemaining(StatusTimer);
	return FMath::Max(CurrentDuration, FMath::Lerp(EffectDuration.X, EffectDuration.Y, Power));
}

float UStatusEffectBasic::GetPowerPercent() const
{
	return FMath::Clamp((CurrentPower - EffectPowerRange.X) / (EffectPowerRange.Y - EffectPowerRange.X), 0.f, 1.f);
}

bool UStatusEffectBasic::IsCriticalPointReached() const
{
	if (!bNotifyWhenCriticalPointReached)
	{
		return false;
	}

	//Critical point is lower bound power range.
	return CurrentPower >= EffectPowerRange.X;
}

void UStatusEffectBasic::OnRep_StatusTime()
{
	OnStatusTimeUpdate.Broadcast(this, StatusTime.X, StatusTime.Y);
}

void UStatusEffectBasic::OnRep_CurrentPower()
{
	OnPowerUpdate.Broadcast(this, CurrentPower);
	K2_OnPowerChanged(CurrentPower);
}

void UStatusEffectBasic::ProcessDamage(UStatusComponent* Component, float& Damage, const struct FDamageEvent& DamageEvent, ACorePlayerState* Instigator)
{
	if (bK2ProcessDamageImplemented)
	{
		Damage = K2_ProcessDamage(Component, Damage, DamageEvent, Instigator);
	}
}

void UStatusEffectBasic::ReceivedDamage(UStatusComponent* Component, float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bK2ReceivedDamageImplemented)
	{
		K2_ReceivedDamage(Component, Damage, DamageEvent, EventInstigator, DamageCauser);
	}
}

void UStatusEffectBasic::SetCriticalPointReached(bool bReached)
{
	if (bCriticalPointReached == bReached)
	{
		return;
	}

	bCriticalPointReached = bReached;
	OnRep_CriticalPointReached();

	MARK_PROPERTY_DIRTY_FROM_NAME(UStatusEffectBasic, bCriticalPointReached, this);
}

void UStatusEffectBasic::OnRep_CriticalPointReached()
{
	OnCriticalPointReached(bCriticalPointReached);
}

void UStatusEffectBasic::UpdateInsitgatorCumulativePower(ACorePlayerState* Instigator, float Power)
{
	if (CumulativePowerMap.Contains(Instigator))
	{
		CumulativePowerMap[Instigator] += Power;
	}
	else
	{
		CumulativePowerMap.Add(Instigator) = Power;
	}

	//If this is already the top instigator, we do not need check.
	if (Instigator == StatusEffectInsitgator)
	{
		return;
	}

	ACorePlayerState* TopPowerInstigator = nullptr;
	float TopPower = 0.f;
	const float CurrentInstigatorPower = CumulativePowerMap.Contains(StatusEffectInsitgator) ? CumulativePowerMap[StatusEffectInsitgator] : 0.f;

	for(const TPair<ACorePlayerState*, float>& Entry : CumulativePowerMap)
	{
		if (Entry.Value > TopPower)
		{
			TopPowerInstigator = Entry.Key;
			TopPower = Entry.Value;
		}
	}

	if (TopPowerInstigator != StatusEffectInsitgator && TopPower > CurrentInstigatorPower)
	{
		UpdateInstigator(TopPowerInstigator);
	}
}

UStatusEffectStack::UStatusEffectStack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UStatusEffectStack::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_WITH_PARAMS_FAST(UStatusEffectStack, CurrentStackCount, PushReplicationParams::Default);
	DOREPLIFETIME_WITH_PARAMS_FAST(UStatusEffectStack, StatusTime, PushReplicationParams::Default);
}

void UStatusEffectStack::Initialize(UStatusComponent* StatusComponent, ACorePlayerState* Instigator, float Power, const FVector& InstigationDirection)
{
	if (IsInitialized())
	{
		return;
	}

	if (Power > 0.f)
	{
		CurrentStackCount = FMath::CeilToInt(Power);
	}
	else
	{
		CurrentStackCount = 1;
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(UStatusEffectStack, CurrentStackCount, this);
	OnRep_CurrentStackCount();

	Super::Initialize(StatusComponent, Instigator, Power, InstigationDirection);
}

void UStatusEffectStack::OnDestroyed()
{
	bool bWasInterrupted = false;

	if (AGameState* GameState = GetWorld()->GetGameState<AGameState>())
	{
		bWasInterrupted = FMath::IsNearlyEqual(StatusTime.Y, GameState->GetServerWorldTimeSeconds(), 0.25f);
	}

	GetWorld()->GetTimerManager().ClearTimer(StatusTimer);

	OnDeactivated(bWasInterrupted ? EStatusEndType::Interrupted : EStatusEndType::Expired);
}

void UStatusEffectStack::OnActivated(EStatusBeginType BeginType)
{
	if (IsAuthority())
	{
		if (StackDuration != -1.f)
		{
			GetWorld()->GetTimerManager().ClearTimer(StatusTimer);
			GetWorld()->GetTimerManager().SetTimer(StatusTimer,
				FTimerDelegate::CreateUObject(this, &UStatusEffectStack::OnStackExpired),
				StackDuration, false);

			StatusTime.X = GetWorld()->GetGameState<AGameState>()->GetServerWorldTimeSeconds();
			StatusTime.Y = StatusTime.X + StackDuration;
		}
		else
		{
			StatusTime.X = -1.f;
			StatusTime.Y = -1.f;
		}
		
		OnRep_StatusTime();
		MARK_PROPERTY_DIRTY_FROM_NAME(UStatusEffectStack, StatusTime, this);
	}

	Super::OnActivated(BeginType);
}

void UStatusEffectStack::OnDeactivated(EStatusEndType EndType)
{
	if (!IsInitialized() || IsPendingKill())
	{
		return;
	}

	Super::OnDeactivated(EndType);
}

bool UStatusEffectStack::CanActivateStatus(ACorePlayerState* Instigator, float Power) const
{
	return true;
}

bool UStatusEffectStack::CanRefreshStatus(ACorePlayerState* Instigator, float Power) const
{
	return true;
}

void UStatusEffectStack::AddEffectPower(ACorePlayerState* Instigator, float Power, const FVector& InstigationDirection)
{
	const uint8 CachedStackCount = CurrentStackCount;
	CurrentStackCount = FMath::FloorToInt(float(CurrentStackCount) + Power);

	switch (InstigatorUpdateRule)
	{
	case EStackStatusEffectInstigatorRule::Any:
		UpdateInstigator(Instigator);
		break;
	case EStackStatusEffectInstigatorRule::StackIncrease:
		if (CachedStackCount < CurrentStackCount)
		{
			UpdateInstigator(Instigator);
		}
		break;
	}

	switch (InstigationUpdateDirectionRule)
	{
	case EInstigationDirectionUpdateRule::Always:
		UpdateInstigationDirection(InstigationDirection);
		break;
	}

	OnRep_CurrentStackCount();

	if (CanRefreshStatus(Instigator, Power))
	{
		switch (InstigationUpdateDirectionRule)
		{
		case EInstigationDirectionUpdateRule::Refresh:
			UpdateInstigationDirection(InstigationDirection);
			break;
		}

		OnActivated(EStatusBeginType::Refresh);
	}
}

void UStatusEffectStack::OnRep_StatusTime()
{
	OnStatusTimeUpdate.Broadcast(this, StatusTime.X, StatusTime.Y);
}

void UStatusEffectStack::OnRep_CurrentStackCount()
{
	OnStackCountUpdate.Broadcast(this, CurrentStackCount);
	K2_OnPowerChanged(StackDuration);
}

void UStatusEffectStack::OnStackExpired()
{
	CurrentStackCount--;

	if (CurrentStackCount > 0)
	{
		OnActivated(EStatusBeginType::Refresh);
	}
	else
	{
		OnDeactivated(EStatusEndType::Expired);
		return;
	}

	OnRep_CurrentStackCount();
	MARK_PROPERTY_DIRTY_FROM_NAME(UStatusEffectStack, CurrentStackCount, this);
}