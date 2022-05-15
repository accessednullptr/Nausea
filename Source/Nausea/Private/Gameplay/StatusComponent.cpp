// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Gameplay/StatusComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "Perception/AIPerceptionSystem.h"
#include "Perception/AISense_Damage.h"
#include "NauseaHelpers.h"
#include "System/NetHelper.h"
#include "Gameplay/StatusInterface.h"
#include "Gameplay/StatusEffect/StatusEffectBase.h"
#include "Gameplay/CoreDamageType.h"
#include "System/CoreGameState.h"
#include "Character/CoreCharacter.h"
#include "Character/CoreCharacterMovementComponent.h"
#include "Player/CorePlayerState.h"
#include "Player/PlayerClassComponent.h"
#include "Player/PlayerStatistics/PlayerStatisticsComponent.h"
#include "Weapon/Weapon.h"
#include "Weapon/FireMode.h"
#include "Gameplay/DamageLogModifier/DamageLogModifierObject.h"

DECLARE_STATS_GROUP(TEXT("StatusComponent"), STATGROUP_StatusComponent, STATCAT_Advanced);

DECLARE_CYCLE_STAT(TEXT("Take Damage"), STAT_StatusComponentTakeDamage, STATGROUP_StatusComponent);
DECLARE_CYCLE_STAT(TEXT("Replicate Subobjects"), STAT_StatusComponentReplicateSubobjects, STATGROUP_StatusComponent);
DECLARE_CYCLE_STAT(TEXT("Add Status Effect"), STAT_StatusComponentAddStatusEffect, STATGROUP_StatusComponent);

FStatStruct InvalidStat = FStatStruct(-1.f, -1.f);
FPartStatStruct InvalidPartStat = FPartStatStruct(-1.f, -1.f);
uint64 FHitEvent::IDCounter = 0;

void FPartStatContainer::PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize)
{
	if (!OwningStatusComponent)
	{
		return;
	}

	for (const int32& Index : AddedIndices)
	{
		OwningStatusComponent->OnReceivedPartHealthUpdate(InstanceList[Index]);
	}
}

void FPartStatContainer::PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize)
{
	if (!OwningStatusComponent)
	{
		return;
	}

	for (const int32& Index : ChangedIndices)
	{
		OwningStatusComponent->OnReceivedPartHealthUpdate(InstanceList[Index]);
	}
}

void FHitEventContainer::PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize)
{
	if (!OwningStatusComponent)
	{
		return;
	}

	for (const int32& Index : AddedIndices)
	{
		OwningStatusComponent->PlayHitEffect(InstanceList[Index]);
	}
}

UStatusComponent::UStatusComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	SetIsReplicatedByDefault(true);

	bWantsInitializeComponent = true;

	StatusConfig = UStatusComponentConfigObject::StaticClass();
	StatusComponentTeam = ETeam::NoTeam;

	ArmorDamageLogModifierClass = UArmorDamageLogModifier::StaticClass();

	PartWeaknessDamageLogModifierClass = UPartWeaknessDamageLogModifier::StaticClass();
	PartResistanceDamageLogModifierClass = UPartResistanceDamageLogModifier::StaticClass();

	PartDestroyedFlatDamageLogModifierClass = UPartDestroyedFlatDamageLogModifier::StaticClass();
	PartDestroyedPercentDamageLogModifierClass = UPartDestroyedPercentDamageLogModifier::StaticClass();

	WeaknessDamageLogModifierClass = UWeaknessDamageLogModifier::StaticClass();
	ResistanceDamageLogModifierClass = UResistanceDamageLogModifier::StaticClass();
}

void UStatusComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DISABLE_REPLICATED_PROPERTY_FAST(UActorComponent, bIsActive);

	DOREPLIFETIME_WITH_PARAMS_FAST(UStatusComponent, Health, PushReplicationParams::Default);
	DOREPLIFETIME_WITH_PARAMS_FAST(UStatusComponent, HealthMovementSpeedModifier, PushReplicationParams::InitialOnly);
	DOREPLIFETIME_WITH_PARAMS_FAST(UStatusComponent, PartHealthList, PushReplicationParams::Default);

	DOREPLIFETIME_WITH_PARAMS_FAST(UStatusComponent, Armour, PushReplicationParams::Default);
	DOREPLIFETIME_WITH_PARAMS_FAST(UStatusComponent, ArmourAbsorption, PushReplicationParams::InitialOnly);
	DOREPLIFETIME_WITH_PARAMS_FAST(UStatusComponent, ArmourDecay, PushReplicationParams::InitialOnly);

	DOREPLIFETIME_WITH_PARAMS_FAST(UStatusComponent, HitEventList, PushReplicationParams::Default);
	DOREPLIFETIME_WITH_PARAMS_FAST(UStatusComponent, PartDestroyedEventList, PushReplicationParams::Default);
	DOREPLIFETIME_WITH_PARAMS_FAST(UStatusComponent, DeathEvent, PushReplicationParams::Default);

	DOREPLIFETIME_WITH_PARAMS_FAST(UStatusComponent, TeamId, PushReplicationParams::InitialOnly);
}

void UStatusComponent::InitializeComponent()
{
	Super::InitializeComponent();
	PartHealthList.SetOwningStatusComponent(this);
	HitEventList.SetOwningStatusComponent(this);

	if (bAutomaticallyInitialize || GetOwnerRole() != ROLE_Authority)
	{
		InitializeStatusComponent();

		if (GetOwnerRole() == ROLE_Authority && IsDead())
		{
			Died(0.f, FDamageEvent(), nullptr, nullptr);
		}
	}

	if (GetOwnerRole() == ROLE_Authority)
	{
		SetGenericTeamId(UPlayerOwnershipInterfaceTypes::GetGenericTeamIdFromTeamEnum(StatusComponentTeam));
	}
}

void UStatusComponent::BeginPlay()
{
	Super::BeginPlay();
}

bool UStatusComponent::ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	SCOPE_CYCLE_COUNTER(STAT_StatusComponentReplicateSubobjects);

	const bool bWroteSomething = ReplicateSubobjectList(Channel, Bunch, RepFlags);
	return Super::ReplicateSubobjects(Channel, Bunch, RepFlags) || bWroteSomething;
}

void UStatusComponent::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	TeamId = NewTeamID;
	OnRep_TeamId();
	MARK_PROPERTY_DIRTY_FROM_NAME(UStatusComponent, TeamId, this);
}

void UStatusComponent::SetPlayerDefaults()
{
	InitializeStatusComponent();
}

void UStatusComponent::InitializeStatusComponent()
{
	if (GetOwnerInterface())
	{
		return;
	}

	StatusInterface = GetOwner();
	TInlineComponentArray<UStatusComponent*> StatusComponents(GetOwner());
	ensure(StatusComponents.Num() <= 1);
	ensure(TSCRIPTINTERFACE_IS_VALID(StatusInterface));

	if (AActor* Actor = GetOwner())
	{
		Actor->OnTakeDamage.AddUObject(this, &UStatusComponent::TakeDamage);
	}

	ensure(StatusConfig);

	if (!StatusConfig)
	{
		StatusConfig = UStatusComponentConfigObject::StaticClass();
	}

	const UStatusComponentConfigObject* StatusConfigObject = StatusConfig->GetDefaultObject<UStatusComponentConfigObject>();
	StatusConfigObject->ConfigureStatusComponent(this);

	OnRep_Health();
	OnRep_Armour();

	for (const FPartStatStruct& PartStat : *PartHealthList)
	{
		OnReceivedPartHealthUpdate(PartStat);
	}
	
	RequestMovementSpeedUpdate();
}

float UStatusComponent::GetMovementSpeedModifier() const
{
	if (!bUpdateMovementSpeedModifier)
	{
		return CachedMovementSpeedModifier;
	}

	float MovementSpeedModifier = 1.f;

	//We divide by 50 because this debuff should only apply on low health.
	MovementSpeedModifier *= FMath::Lerp(HealthMovementSpeedModifier.Y, HealthMovementSpeedModifier.X, FMath::Clamp(Health / 50.f, 0.f, 1.f));

	//Apply status effects (or whoever else is binding to this).
	OnProcessMovementSpeed.Broadcast(this, MovementSpeedModifier);

	CachedMovementSpeedModifier = MovementSpeedModifier;
	bUpdateMovementSpeedModifier = false;

	return CachedMovementSpeedModifier;
}

float UStatusComponent::GetRotationRateModifier() const
{
	if (!bUpdateRotationRateModifier)
	{
		return CachedRotationRateModifier;
	}

	float RotationRateModifier = 1.f;
	OnProcessRotationRate.Broadcast(this, RotationRateModifier);

	CachedRotationRateModifier = RotationRateModifier;
	bUpdateRotationRateModifier = false;

	return CachedRotationRateModifier;
}

float UStatusComponent::HealDamage(float HealAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (IsDead())
	{
		return 0.f;
	}

	SetHealth(Health + HealAmount);

	return HealAmount;
}

void UStatusComponent::Kill(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Died(Damage, DamageEvent, EventInstigator, DamageCauser);
}

void UStatusComponent::Died(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		SetHealth(0.f);
		UpdateDeathEvent(Damage, DamageEvent, EventInstigator, DamageCauser);

		if (bMakeNoiseOnDeath)
		{
			float NoiseMultiplier = 1.f;
			if (const UCoreDamageType* CoreDamageType = Cast<UCoreDamageType>(DamageEvent.DamageTypeClass.GetDefaultObject()))
			{
				NoiseMultiplier *= CoreDamageType->GetDeathNoiseMultiplier();
			}

			DeathNoise.MakeNoise(GetOwner(), FVector::ZeroVector, NoiseMultiplier);
		}

		GetOwner()->ForceNetUpdate();
	}

	OnDied.Broadcast(this, Damage, DamageEvent, EventInstigator, DamageCauser);

	TSCRIPTINTERFACE_CALL_FUNC(StatusInterface, Died, K2_Died, Damage, DamageEvent, EventInstigator, DamageCauser);

	ClearReplicatedSubobjectList();
}

FName UStatusComponent::GetHitBodyPartName(const FName& BoneName) const
{
	int32 Index = GetPartHealthIndexForBone(BoneName);

	if (Index != INDEX_NONE)
	{
		return PartHealthList[Index].PartName;
	}

	return NAME_None;
}

FPartStatStruct& UStatusComponent::GetPartHealthForBone(const FName& BoneName)
{
	int32 Index = GetPartHealthIndexForBone(BoneName);

	if (Index != INDEX_NONE)
	{
		return PartHealthList[Index];
	}

	return InvalidPartStat;
}

int32 UStatusComponent::GetPartHealthIndexForBone(const FName& BoneName) const
{
	if (BoneName != NAME_None && BonePartIndexMap.Contains(BoneName))
	{
		return BonePartIndexMap[BoneName];
	}

	return INDEX_NONE;
}

void UStatusComponent::OnReceivedPartHealthUpdate(const FPartStatStruct& PartStat)
{
	const float PartHealth = PartStat.GetValue();
	const float PreviousPartHealth = PreviousPartHealthMap.Contains(PartStat.PartName) ? PreviousPartHealthMap[PartStat.PartName] : PartStat.GetMaxValue();

	if (PartStat == PreviousPartHealth && PreviousPartHealthMap.Contains(PartStat.PartName))
	{
		return;
	}

	OnPartHealthChanged.Broadcast(this, PartStat.PartName, PartHealth, PreviousPartHealth);
	PreviousPartHealthMap.FindOrAdd(PartStat.PartName) = PartStat.GetValue();
}

void UStatusComponent::OnReceivedHitEvent(const FHitEvent& HitEvent)
{
	OnHitEvent.Broadcast(this, HitEvent);
}

void UStatusComponent::RequestActionBlock(int32& BlockID, bool bInterruptCurrentAction)
{
	while (++BlockingActionCounter == INDEX_NONE || BlockingActionSet.Contains(BlockingActionCounter))
	{
		BlockingActionCounter++;
	}

	//We're about to override the passed in BlockID, so revoke it first.
	if (BlockingActionSet.Contains(BlockID))
	{
		BlockingActionSet.Remove(BlockID);
	}

	BlockingActionSet.Add(BlockingActionCounter);
	BlockID = BlockingActionCounter;

	if (UpdateActionBlock() && IsBlockingAction() && bInterruptCurrentAction)
	{
		InterruptCharacterActions();
	}
}

void UStatusComponent::RevokeActionBlock(int32& BlockID)
{
	BlockingActionSet.Remove(BlockID);
	BlockID = INDEX_NONE;
	
	UpdateActionBlock();
}

bool UStatusComponent::UpdateActionBlock()
{
	if (bIsBlockingAction == BlockingActionSet.Num() > 0)
	{
		return false;
	}

	bIsBlockingAction = BlockingActionSet.Num() > 0;
	OnBlockingActionUpdate.Broadcast(this, bIsBlockingAction);
	return true;
}

void UStatusComponent::InterruptCharacterActions()
{
	OnActionInterrupt.Broadcast(this);
}

void UStatusComponent::OnStatusEffectAdded(UStatusEffectBase* StatusEffect)
{
	RegisterReplicatedSubobject(StatusEffect);
	StatusEffectList.AddUnique(StatusEffect);
}

void UStatusComponent::OnStatusEffectRemoved(UStatusEffectBase* StatusEffect)
{
	UnregisterReplicatedSubobject(StatusEffect);
	StatusEffectList.Remove(StatusEffect);
	StatusEffectList.Remove(nullptr);
}

float UStatusComponent::SetHealth(float InHealth)
{
	if (Health == InHealth)
	{
		return Health;
	}

	const float CurrentHealth = Health.SetValue(InHealth);
	OnRep_Health();
	MARK_PROPERTY_DIRTY_FROM_NAME(UStatusComponent, Health, this);
	return Health;
}

float UStatusComponent::SetMaxHealth(float InMaxHealth)
{
	const float PreviousHealthValue = Health;
	
	Health.SetMaxValue(InMaxHealth);

	if (Health != PreviousHealthValue)
	{
		OnRep_Health();
		MARK_PROPERTY_DIRTY_FROM_NAME(UStatusComponent, Health, this);
	}

	return Health;
}

float UStatusComponent::SetArmour(float InArmour)
{
	const float PreviousArmourValue = Armour;

	Armour.SetMaxValue(InArmour);

	if (Armour != PreviousArmourValue)
	{
		OnRep_Armour();
		MARK_PROPERTY_DIRTY_FROM_NAME(UStatusComponent, Armour, this);
	}

	return Armour;
}

float UStatusComponent::SetMaxArmour(float InMaxArmour)
{
	const float PreviousMaxArmourValue = Armour.GetMaxValue();

	Armour.SetMaxValue(InMaxArmour);

	if (Armour.GetMaxValue() != PreviousMaxArmourValue)
	{
		OnRep_Armour();
		MARK_PROPERTY_DIRTY_FROM_NAME(UStatusComponent, Armour, this);
	}

	return Armour;
}

inline void ProcessDamageEvent(FDamageEvent const& DamageEvent,
	FPointDamageEvent*& PointDamageEvent, FRadialDamageEvent*& RadialDamageEvent,
	TSubclassOf<UWeapon>& OutWeaponClass, TSubclassOf<UFireMode>& OutFireModeClass)
{
	if (DamageEvent.IsOfType(FCoreDamageEvent::ClassID))
	{
		FCoreDamageEvent* CoreDamageEvent = (FCoreDamageEvent*)&DamageEvent;
		OutWeaponClass = CoreDamageEvent->WeaponClass;
		OutFireModeClass = CoreDamageEvent->FireModeClass;
	}
	else if (DamageEvent.IsOfType(FCorePointDamageEvent::ClassID))
	{
		FCorePointDamageEvent* CorePointDamageEvent = (FCorePointDamageEvent*)&DamageEvent;
		PointDamageEvent = CorePointDamageEvent;
		OutWeaponClass = CorePointDamageEvent->WeaponClass;
		OutFireModeClass = CorePointDamageEvent->FireModeClass;
	}
	else if (DamageEvent.IsOfType(FCoreRadialDamageEvent::ClassID))
	{
		FCoreRadialDamageEvent* CoreRadialDamageEvent = (FCoreRadialDamageEvent*)&DamageEvent;
		RadialDamageEvent = CoreRadialDamageEvent;
		OutWeaponClass = CoreRadialDamageEvent->WeaponClass;
		OutFireModeClass = CoreRadialDamageEvent->FireModeClass;
	}
	else if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		PointDamageEvent = (FPointDamageEvent*)&DamageEvent;
	}
	else if (DamageEvent.IsOfType(FRadialDamageEvent::ClassID))
	{
		RadialDamageEvent = (FRadialDamageEvent*)&DamageEvent;
	}
}

inline float GetHitAndElementTypeDamageMultiplier(FDamageEvent const& DamageEvent, const TMap<EDamageHitDescriptor, float>& HitTypeMultiplierMap, const TMap<EDamageElementalDescriptor, float>& ElementalTypeMultiplierMap)
{
	const UCoreDamageType* CoreDamageTypeCDO = Cast<UCoreDamageType>(DamageEvent.DamageTypeClass.GetDefaultObject());
	if (!CoreDamageTypeCDO)
	{
		return 1.f;
	}

	float Multiplier = 1.f;
	if (HitTypeMultiplierMap.Num() > 0)
	{
		const TArray<EDamageHitDescriptor>& HitTypeList = CoreDamageTypeCDO->GetDamageHitDescriptorList();
		for (EDamageHitDescriptor Type : HitTypeList)
		{
			if (HitTypeMultiplierMap.Contains(Type))
			{
				Multiplier *= HitTypeMultiplierMap[Type];
			}
		}
	}

	if (ElementalTypeMultiplierMap.Num() > 0)
	{
		const TArray<EDamageElementalDescriptor>& ElementalTypeList = CoreDamageTypeCDO->GetDamageElementalDescriptorList();
		for (EDamageElementalDescriptor Type : ElementalTypeList)
		{
			if (ElementalTypeMultiplierMap.Contains(Type))
			{
				Multiplier *= ElementalTypeMultiplierMap[Type];
			}
		}
	}

	return Multiplier;
}

void UStatusComponent::TakeDamage(AActor* Actor, float& DamageAmount, FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser)
{
	SCOPE_CYCLE_COUNTER(STAT_StatusComponentTakeDamage);

	if (GetOwnerRole() != ROLE_Authority)
	{
		return;
	}

	if (IsDead())
	{
		return;
	}

	FPointDamageEvent* PointDamageEvent = nullptr;
	FRadialDamageEvent* RadialDamageEvent = nullptr;
	TSubclassOf<UWeapon> WeaponClass = nullptr;
	TSubclassOf<UFireMode> FireModeClass = nullptr;
	ProcessDamageEvent(DamageEvent, PointDamageEvent, RadialDamageEvent, WeaponClass, FireModeClass);

	UObject* Instigator = EventInstigator ? (EventInstigator->PlayerState ? Cast<UObject>(EventInstigator->PlayerState) : EventInstigator->GetPawn()) : nullptr;
	IPlayerOwnershipInterface* EventInstigatorPlayerOwnership = Cast<IPlayerOwnershipInterface>(EventInstigator);
	ACorePlayerState* EventInstigatorPlayerState = EventInstigatorPlayerOwnership ? EventInstigatorPlayerOwnership->GetOwningPlayerState() : nullptr;

	if (bPerformDamageLog)
	{
		FGenericTeamId EventInstigatorTeam = EventInstigatorPlayerOwnership ? EventInstigatorPlayerOwnership->GetOwningTeamId() : FGenericTeamId::NoTeam;
		PushDamageLog(FDamageLogEvent(GetWorld()->GetTimeSeconds(), DamageAmount, Instigator, EventInstigatorTeam,
			WeaponClass, FireModeClass, TSubclassOf<UCoreDamageType>(DamageEvent.DamageTypeClass)));
	}

	if (ACoreGameState* CoreGameState = GetWorld() ? GetWorld()->GetGameState<ACoreGameState>() : nullptr)
	{
		CoreGameState->HandleDamageApplied(Actor, DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	}

	if (UStatusComponent* StatusComponent = UStatusInterfaceStatics::GetStatusComponent(DamageCauser))
	{
		StatusComponent->OnProcessDamageDealt.Broadcast(this, DamageAmount, DamageEvent, EventInstigatorPlayerState);
	}

	OnProcessDamageTaken.Broadcast(this, DamageAmount, DamageEvent, EventInstigatorPlayerState);

	const float HitAndElementTypeMultiplier = GetHitAndElementTypeDamageMultiplier(DamageEvent, HitTypeDamageMultiplier, ElementalTypeDamageMultiplier);
	if (HitAndElementTypeMultiplier != 1.f)
	{
		if (bPerformDamageLog)
		{
			if (HitAndElementTypeMultiplier > 1.f)
			{
				PushDamageLogModifier(FDamageLogEventModifier(WeaknessDamageLogModifierClass, nullptr, HitAndElementTypeMultiplier));
			}
			else
			{
				PushDamageLogModifier(FDamageLogEventModifier(ResistanceDamageLogModifierClass, nullptr, HitAndElementTypeMultiplier));
			}
		}

		DamageAmount *= HitAndElementTypeMultiplier;
	}

	//Intermediary data container.
	struct FAIDamagePerceptionInfo
	{
		FAIDamagePerceptionInfo() {}
		FAIDamagePerceptionInfo(const FVector& InInstigatorLocation, const FVector& InHitLocation)
			: InstigationLocation(InInstigatorLocation), HitLocation(InHitLocation) {}
		
		FVector InstigationLocation = FAISystem::InvalidLocation;
		FVector HitLocation = FAISystem::InvalidLocation;
	};
	FAIDamagePerceptionInfo PerceptionInfo;

	if (PointDamageEvent)
	{
		HandlePointDamage(Actor, DamageAmount, *PointDamageEvent, EventInstigator, DamageCauser);
		PerceptionInfo = FAIDamagePerceptionInfo(PointDamageEvent->HitInfo.TraceStart, PointDamageEvent->HitInfo.ImpactPoint);
	}
	else if (RadialDamageEvent)
	{
		HandleRadialDamage(Actor, DamageAmount, *RadialDamageEvent, EventInstigator, DamageCauser);
		PerceptionInfo = FAIDamagePerceptionInfo(RadialDamageEvent->Origin, RadialDamageEvent->Origin);
	}
	else
	{
		FVector InstigatorLocation = FAISystem::InvalidLocation;
		if (EventInstigator && EventInstigator->GetPawn())
		{
			InstigatorLocation = EventInstigator->GetPawn()->GetActorLocation();
		}

		PerceptionInfo = FAIDamagePerceptionInfo(InstigatorLocation, GetOwner()->GetActorLocation());
	}

	HandleArmourDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	HandleDamageTypeStatus(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	
	if (UAIPerceptionSystem* PerceptionSystem = UAIPerceptionSystem::GetCurrent(this))
	{
		//Threat generating damage must be using a core damage type.
		TSubclassOf<UCoreDamageType> CoreDamageTypeClass = TSubclassOf<UCoreDamageType>(DamageEvent.DamageTypeClass);

		if (const UCoreDamageType* CoreDamageTypeCDO = CoreDamageTypeClass.GetDefaultObject())
		{
			float GeneratedThreat = CoreDamageTypeCDO->GetThreatAmount(DamageAmount);
			OnProcessThreatApplied.Broadcast(this, GeneratedThreat, DamageEvent, EventInstigatorPlayerState);

			if (GeneratedThreat > 0.f)
			{
				FAIDamageEvent Event(GetOwner(), EventInstigator ? EventInstigator->GetPawn() : nullptr, GeneratedThreat, PerceptionInfo.InstigationLocation, PerceptionInfo.HitLocation);
				PerceptionSystem->OnEvent(Event);
			}
		}
	}

	static auto GetDamageDirection([](const FAIDamagePerceptionInfo& Info)->FVector
	{
		if (Info.HitLocation == FAISystem::InvalidLocation || Info.InstigationLocation == FAISystem::InvalidLocation)
		{
			return FAISystem::InvalidDirection;
		}

		return (Info.HitLocation - Info.InstigationLocation).GetSafeNormal();
	});

	const FVector DamageDirection = GetDamageDirection(PerceptionInfo);
	GenerateHitEvent(FHitEvent(TSubclassOf<UCoreDamageType>(DamageEvent.DamageTypeClass), DamageAmount, DamageDirection, PerceptionInfo.HitLocation, GetWorld()->GetTimeSeconds(), FRandomStream(FMath::Rand())));

	SetHealth(Health - DamageAmount);

	if (DamageAmount > 0.f)
	{
		UPlayerStatisticHelpers::IncrementPlayerStatistic(EventInstigator, EPlayerStatisticType::DamageDealt, DamageAmount);
		UPlayerStatisticHelpers::IncrementPlayerStatistic(this, EPlayerStatisticType::DamageReceived, DamageAmount);
	}

	if (bPerformDamageLog)
	{
		if (IsDead()) { MarkDeathEvent(); }
		OnDamageLogPopped.Broadcast(this, PopDamageLog(DamageAmount));
	}

	if (bMakeNoiseOnDamage)
	{
		DamageNoise.MakeNoise(GetOwner());
	}

	OnDamageReceived.Broadcast(this, DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (IsDead())
	{
		Died(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	}
}

void UStatusComponent::HandlePointDamage(AActor* Actor, float& DamageAmount, FPointDamageEvent const& PointDamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	K2_HandlePointDamage(Actor, DamageAmount, DamageAmount, PointDamageEvent, EventInstigator, DamageCauser);

	FPartStatStruct& HitPart = GetPartHealthForBone(PointDamageEvent.HitInfo.BoneName);

	if (!HitPart.IsValid())
	{
		return;
	}

	//If this part has been marked as destroyed, skip damage modification and application.
	if (HitPart.IsDestroyed())
	{
		return;
	}
	
	float Multiplier = GetHitAndElementTypeDamageMultiplier(PointDamageEvent, HitTypeDamageMultiplier, ElementalTypeDamageMultiplier) * HitPart.DamageMultiplier;
	if (Multiplier != 1.f)
	{
		if (bPerformDamageLog)
		{
			if (Multiplier > 1.f)
			{
				PushDamageLogModifier(FDamageLogEventModifier(PartWeaknessDamageLogModifierClass, nullptr, HitPart.DamageMultiplier));
			}
			else
			{
				PushDamageLogModifier(FDamageLogEventModifier(PartResistanceDamageLogModifierClass, nullptr, HitPart.DamageMultiplier));
			}
		}

		DamageAmount *= Multiplier;
	}

	//If HitPart's max health is equal to or less than 0, it does not track health.
	if (HitPart.GetMaxValue() <= 0.f)
	{
		return;
	}

	HitPart -= DamageAmount;
	MARK_PROPERTY_DIRTY_FROM_NAME(UStatusComponent, PartHealthList, this);

	if (HitPart <= 0.f)
	{
		HandleHitPartDestroyed(HitPart, DamageAmount, PointDamageEvent, EventInstigator, DamageCauser);
	}
}

void UStatusComponent::HandleRadialDamage(AActor* Actor, float& DamageAmount, FRadialDamageEvent const& RadialDamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	K2_HandleRadialDamage(Actor, DamageAmount, DamageAmount, RadialDamageEvent, EventInstigator, DamageCauser);
}

void UStatusComponent::HandleHitPartDestroyed(const FPartStatStruct& DestroyedPart, float& DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (DestroyedPart.bApplyFlatDamageOnPartKilled)
	{
		const float FlatDamageModifier = ((DamageAmount + DestroyedPart.FlatDamageOnPartKilled) / DamageAmount);
		PushDamageLogModifier(FDamageLogEventModifier(PartDestroyedFlatDamageLogModifierClass, this, FlatDamageModifier));
		DamageAmount += DestroyedPart.FlatDamageOnPartKilled;
	}

	if (DestroyedPart.bApplyPercentDamageOnPartKilled)
	{
		const float ActualDamage = DestroyedPart.PercentDamageOnPartKilled * Health.GetMaxValue();
		const float PercentDamageModifier = ((DamageAmount + ActualDamage) / DamageAmount);
		PushDamageLogModifier(FDamageLogEventModifier(PartDestroyedPercentDamageLogModifierClass, this, PercentDamageModifier));
		DamageAmount += ActualDamage;
	}
}

void UStatusComponent::HandleArmourDamage(float& DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (Armour <= 0.f || DamageAmount <= 0.f)
	{
		return;
	}

	const UCoreDamageType* CoreDamageType = Cast<UCoreDamageType>(DamageEvent.DamageTypeClass.GetDefaultObject());
	CoreDamageType = CoreDamageType ? CoreDamageType : UCoreDamageType::StaticClass()->GetDefaultObject<UCoreDamageType>();

	float Absorb = CoreDamageType->ShouldIgnoreArmor() ? 0.f : FMath::Lerp(ArmourAbsorption.Y, ArmourAbsorption.X, Armour.GetPercentValue());
	float Decay = (!CoreDamageType->ShouldDecayArmor()) ? 0.f : FMath::Lerp(ArmourDecay.Y, ArmourDecay.X, Armour.GetPercentValue());
	Decay *= CoreDamageType->GetArmorDecayMultiplier();

	if (IPlayerOwnershipInterface* PlayerOwnershipInterface = Cast<IPlayerOwnershipInterface>(GetOwner()))
	{
		if (UPlayerClassComponent* PlayerClassComponent = PlayerOwnershipInterface->GetPlayerClassComponent())
		{
			PlayerClassComponent->OnProcessArmourDamage.Broadcast(Absorb, Decay, DamageEvent, EventInstigator, GetOwner());
		}
	}

	const float AbsorbedDamage = Absorb * DamageAmount;
	Armour -= AbsorbedDamage * Decay;
	DamageAmount = FMath::Max(0.f, DamageAmount - AbsorbedDamage);
	PushDamageLogModifier(FDamageLogEventModifier(ArmorDamageLogModifierClass, this, 1.f - Absorb));
}

inline FVector GetInstigationDirection(struct FDamageEvent const& DamageEvent)
{
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		FPointDamageEvent* PointDamageEvent = (FPointDamageEvent*)&DamageEvent;
		return PointDamageEvent->ShotDirection;
	}
	else if (DamageEvent.IsOfType(FRadialDamageEvent::ClassID))
	{
		FRadialDamageEvent* RadialDamageEvent = (FRadialDamageEvent*)&DamageEvent;
		FHitResult HitResult;
		FVector OutImpulseDirection;

		RadialDamageEvent->GetBestHitInfo(nullptr, nullptr, HitResult, OutImpulseDirection);
		return OutImpulseDirection;
	}

	return FAISystem::InvalidDirection;
}

void UStatusComponent::HandleDamageTypeStatus(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (!DamageEvent.DamageTypeClass)
	{
		return;
	}

	const UCoreDamageType* CoreDamageType = Cast<UCoreDamageType>(DamageEvent.DamageTypeClass.GetDefaultObject());
	
	if (!CoreDamageType)
	{
		return;
	}

	float EffectPowerMultiplier = (CoreDamageType->ShouldScaleStatusEffectPowerByFrameDeltaTime() ? GetWorld()->GetDeltaSeconds() : 1.f);
	EffectPowerMultiplier *= (CoreDamageType->ShouldScaleStatusEffectPowerByDamage() ? Damage : 1.f);

	if (ACoreGameState* CoreGameState = GetWorld() ? GetWorld()->GetGameState<ACoreGameState>() : nullptr)
	{
		CoreGameState->HandleStatusApplied(GetOwner(), EffectPowerMultiplier, DamageEvent, EventInstigator, DamageCauser);
	}

	if (EffectPowerMultiplier <= 0.f)
	{
		return;
	}

	const TMap<EStatusType, float>& DamageTypeGenericStatusEffectMap = CoreDamageType->GetGenericStatusEffectMap();

	for (const TPair<EStatusType, float>& GenericStatusEffect : DamageTypeGenericStatusEffectMap)
	{
		if (!GenericStatusEffectMap.Contains(GenericStatusEffect.Key))
		{
			continue;
		}
		
		float EffectPower = GenericStatusEffect.Value * EffectPowerMultiplier;
		if (GenericStatusEffectMultiplierMap.Contains(GenericStatusEffect.Key))
		{
			EffectPower *= GenericStatusEffectMultiplierMap[GenericStatusEffect.Key];
		}

		AddStatusEffect(GenericStatusEffectMap[GenericStatusEffect.Key], DamageEvent, EventInstigator, EffectPower);
	}

	const TMap<TSoftClassPtr<UStatusEffectBase>, float>& DamageTypeStatusEffectMap = CoreDamageType->GetStatusEffectMap();

	for (const TPair<TSoftClassPtr<UStatusEffectBase>, float>& StatusEntry : DamageTypeStatusEffectMap)
	{
		TSubclassOf<UStatusEffectBase> StatusEffectClass = StatusEntry.Key.LoadSynchronous();

		if (!StatusEffectClass)
		{
			continue;
		}

		AddStatusEffect(StatusEntry.Key.Get(), DamageEvent, EventInstigator, StatusEntry.Value * EffectPowerMultiplier);
	}
}

UStatusEffectBase* UStatusComponent::AddStatusEffect(TSubclassOf<UStatusEffectBase> StatusEffectClass, struct FDamageEvent const& DamageEvent, AController* EventInstigator, float Power)
{
	SCOPE_CYCLE_COUNTER(STAT_StatusComponentAddStatusEffect);

	if (GetOwnerRole() != ROLE_Authority)
	{
		return nullptr;
	}

	if (!StatusEffectClass)
	{
		return nullptr;
	}

	ACorePlayerState* InstigatorPlayerState = EventInstigator ? EventInstigator->GetPlayerState<ACorePlayerState>() : nullptr;

	for (UStatusEffectBase* StatusEffect : StatusEffectList)
	{
		if (!StatusEffect)
		{
			continue;
		}

		if (!StatusEffect->IsPendingKillOrUnreachable() && StatusEffect->GetClass() == StatusEffectClass)
		{
			OnProcessStatusPowerTaken.Broadcast(GetOwner(), Power, DamageEvent, StatusEffect->GetStatusType());
			if (StatusEffect->CanRefreshStatus(InstigatorPlayerState, Power))
			{
				StatusEffect->AddEffectPower(InstigatorPlayerState, Power, GetInstigationDirection(DamageEvent));
			}
			return StatusEffect;
		}
	}

	const UStatusEffectBase* StatusCDO = StatusEffectClass->GetDefaultObject<UStatusEffectBase>();
	OnProcessStatusPowerTaken.Broadcast(GetOwner(), Power, DamageEvent, StatusCDO->GetStatusType());
	if (!StatusCDO->CanActivateStatus(InstigatorPlayerState, Power))
	{
		return nullptr;
	}

	UStatusEffectBase* StatusEffect = NewObject<UStatusEffectBase>(this, StatusEffectClass);

	if (StatusEffect)
	{
		StatusEffect->Initialize(this, InstigatorPlayerState, Power, GetInstigationDirection(DamageEvent));
		OnStatusEffectAdded(StatusEffect);
		StatusEffect->OnActivated(EStatusBeginType::Initial);
	}

	return StatusEffect;
}

void UStatusComponent::UpdateDeathEvent(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	DeathEvent.Damage = Damage;
	DeathEvent.DamageType = DamageEvent.DamageTypeClass;

	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		FPointDamageEvent& PointDamageEvent = (FPointDamageEvent&)DamageEvent;
		DeathEvent.HitLocation = PointDamageEvent.HitInfo.ImpactPoint;
		DeathEvent.HitMomentum = PointDamageEvent.ShotDirection;
	}
	else if (DamageEvent.IsOfType(FRadialDamageEvent::ClassID))
	{
		FRadialDamageEvent& RadialDamageEvent = (FRadialDamageEvent&)DamageEvent;
		DeathEvent.HitLocation = RadialDamageEvent.Origin;

		FHitResult HitResult;
		RadialDamageEvent.GetBestHitInfo(GetOwner(), nullptr, HitResult, DeathEvent.HitMomentum);
		DeathEvent.HitMomentum *= RadialDamageEvent.Params.GetDamageScale(HitResult.Distance);
	}

	OnRep_DeathEvent();
	MARK_PROPERTY_DIRTY_FROM_NAME(UStatusComponent, DeathEvent, this);
}

inline void UStatusComponent::PushDamageLog(FDamageLogEvent&& Entry)
{
	DamageLogStack.Push(MoveTemp(Entry));
}

inline FDamageLogEvent UStatusComponent::PopDamageLog(float DamageDealt)
{
	FDamageLogEvent PoppedDamageLog = DamageLogStack.Pop();
	PoppedDamageLog.FinalizeDamageLog(DamageDealt);
	return PoppedDamageLog;
}

void UStatusComponent::GenerateHitEvent(FHitEvent&& InHitEvent)
{
	FHitEvent& HitEvent = HitEventList->Add_GetRef(MoveTemp(InHitEvent));
	HitEventList.MarkItemDirty(HitEvent);

	FTimerHandle DummyHandle;
	GetWorld()->GetTimerManager().SetTimer(DummyHandle, FTimerDelegate::CreateUObject(this, &UStatusComponent::CleanupHitEvent, HitEvent.ID), 2.f, false);

	PlayHitEffect(HitEvent);
}

void UStatusComponent::CleanupHitEvent(uint64 ID)
{
	for (int32 Index = HitEventList->Num() - 1; Index >= 0; Index--)
	{
		if (HitEventList[Index].ID != ID)
		{
			continue;
		}

		HitEventList->RemoveAt(Index, 1, false);
		HitEventList.MarkArrayDirty();
	}
}

void UStatusComponent::PlayHitEffect(const FHitEvent& HitEvent)
{
	OnHitEventReceived.Broadcast(this, HitEvent);
}

inline void UStatusComponent::MarkDeathEvent()
{
	if (bPerformDamageLog)
	{
		DamageLogStack.MarkDeathEvent();
	}
}

inline void UStatusComponent::PushDamageLogModifier(FDamageLogEventModifier&& Modifier)
{
	if (bPerformDamageLog && Modifier.Modifier != 1.f)
	{
		DamageLogStack.PushEntryModifier(MoveTemp(Modifier));
	}
}

void UStatusComponent::OnRep_Health()
{
	if (Health.IsIdenticalTo(PreviousHealth))
	{
		return;
	}

	//First time initialization.
	if (PreviousHealth == 0.f)
	{
		PreviousHealth = Health;
		OnHealthChanged.Broadcast(this, Health, PreviousHealth);
		OnMaxHealthChanged.Broadcast(this, Health.GetMaxValue(), Health.GetMaxValue());
		return;
	}

	if (Health.GetValue() != PreviousHealth.GetValue())
	{
		OnHealthChanged.Broadcast(this, Health, PreviousHealth);
	}

	if (Health.GetMaxValue() != PreviousHealth.GetMaxValue())
	{
		OnMaxHealthChanged.Broadcast(this, Health.GetMaxValue(), PreviousHealth.GetMaxValue());
	}

	PreviousHealth = Health;

	if (GetOwnerRole() > ROLE_SimulatedProxy)
	{
		RequestMovementSpeedUpdate();
	}
}

void UStatusComponent::OnRep_Armour()
{
	if (Armour.IsIdenticalTo(PreviousArmour))
	{
		return;
	}

	//First time initialization.
	if (PreviousArmour == 0.f)
	{
		PreviousArmour = Armour;
		OnArmourChanged.Broadcast(this, Armour, PreviousArmour);
		OnMaxArmourChanged.Broadcast(this, Armour.GetMaxValue(), PreviousArmour.GetMaxValue());
		return;
	}

	if (Armour.GetValue() != PreviousArmour.GetValue())
	{
		OnArmourChanged.Broadcast(this, Armour, PreviousArmour);
	}

	if (Armour.GetMaxValue() != PreviousArmour.GetMaxValue())
	{
		OnMaxArmourChanged.Broadcast(this, Armour.GetMaxValue(), PreviousArmour.GetMaxValue());
	}

	PreviousArmour = Armour;
}

void UStatusComponent::OnRep_ReplicatedStatusListUpdateCounter()
{
	
}

void UStatusComponent::OnRep_TeamId()
{

}

void UStatusComponent::OnRep_DeathEvent()
{
	//If we've received a DeathEvent OnRep but still have not initialized, do so before processing it.
	if (!TSCRIPTINTERFACE_IS_VALID(StatusInterface))
	{
		InitializeStatusComponent();
	}

	if (GetOwnerRole() != ROLE_Authority)
	{
		Died(DeathEvent.Damage, FDamageEvent(DeathEvent.DamageType), nullptr, nullptr);
	}

	OnDeathEvent.Broadcast(this, DeathEvent.DamageType, DeathEvent.Damage, DeathEvent.HitLocation, DeathEvent.HitMomentum);
}

void UStatusComponent::OnRep_PartDestroyedEventList()
{

}

UStatusComponentConfigObject::UStatusComponentConfigObject(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UStatusComponentConfigObject::ConfigureStatusComponent(UStatusComponent* StatusComponent) const
{
	ACoreGameState* CoreGameState = StatusComponent->GetWorld() ? StatusComponent->GetWorld()->GetGameState<ACoreGameState>() : nullptr;

	const float Difficulty = CoreGameState ? CoreGameState->GetGameDifficultyForScaling() : 0.f;
	const float AlivePlayerCount = CoreGameState ? CoreGameState->GetNumberOfAlivePlayersForScaling() : 0.f;

	const float HealthScale = (HealthDifficultyScalingCurve ? HealthDifficultyScalingCurve->GetFloatValue(Difficulty) : 1.f) * (HealthPlayerCountScalingCurve ? HealthPlayerCountScalingCurve->GetFloatValue(AlivePlayerCount) : 1.f);
	const float ArmourScale = (ArmourDifficultyScalingCurve ? ArmourDifficultyScalingCurve->GetFloatValue(Difficulty) : 1.f) * (ArmourPlayerCountScalingCurve ? ArmourPlayerCountScalingCurve->GetFloatValue(AlivePlayerCount) : 1.f);
	const float StatusScale = (StatusDifficultyScalingCurve ? StatusDifficultyScalingCurve->GetFloatValue(Difficulty) : 1.f) * (StatusPlayerCountScalingCurve ? StatusPlayerCountScalingCurve->GetFloatValue(AlivePlayerCount) : 1.f);

	StatusComponent->Health = Health;
	StatusComponent->Health.Initialize();
	StatusComponent->Health *= HealthScale;
	StatusComponent->HealthMovementSpeedModifier = HealthMovementSpeedModifier;
	MARK_PROPERTY_DIRTY_FROM_NAME(UStatusComponent, Health, StatusComponent);
	MARK_PROPERTY_DIRTY_FROM_NAME(UStatusComponent, HealthMovementSpeedModifier, StatusComponent);

	StatusComponent->Armour = Armour;
	StatusComponent->Armour.Initialize();
	StatusComponent->Armour *= ArmourScale;
	StatusComponent->ArmourAbsorption = ArmourAbsorption;
	StatusComponent->ArmourDecay = ArmourDecay;
	MARK_PROPERTY_DIRTY_FROM_NAME(UStatusComponent, Armour, StatusComponent);
	MARK_PROPERTY_DIRTY_FROM_NAME(UStatusComponent, ArmourAbsorption, StatusComponent);
	MARK_PROPERTY_DIRTY_FROM_NAME(UStatusComponent, ArmourDecay, StatusComponent);

	StatusComponent->HitTypeDamageMultiplier = HitTypeDamageMultiplier;
	StatusComponent->ElementalTypeDamageMultiplier = ElementalTypeDamageMultiplier;

	//Push part list to status component and initialize it here.
	*StatusComponent->PartHealthList = PartHealthList;
	MARK_PROPERTY_DIRTY_FROM_NAME(UStatusComponent, PartHealthList, StatusComponent);

	TArray<FPartStatStruct>& StatusComponentPartHealthList = *StatusComponent->PartHealthList;
	TMap<FName, int32>& StatusComponentBonePartIndexMap = StatusComponent->BonePartIndexMap;

	for (int32 Index = StatusComponentPartHealthList.Num() - 1; Index >= 0; Index--)
	{
		StatusComponentPartHealthList[Index].StatStruct.Initialize();

		const TArray<FName>& BoneList = StatusComponentPartHealthList[Index].BoneList;
		for (const FName& BoneName : BoneList)
		{
			int32& BoneIndex = StatusComponentBonePartIndexMap.Add(BoneName);
			BoneIndex = Index;
		}

		//Add PartName to the map as well.
		int32& BoneIndex = StatusComponentBonePartIndexMap.Add(StatusComponentPartHealthList[Index].PartName);
		BoneIndex = Index;
	}

	if (bHealthScalingEffectsPartHealth)
	{
		for (FPartStatStruct& PartHealth : StatusComponentPartHealthList)
		{
			if (PartHealth.bAlwaysIgnoresHealthScaling)
			{
				continue;
			}

			PartHealth.StatStruct *= HealthScale;
		}
	}

	//Mark everything dirty for first time replication.
	for (FPartStatStruct& PartStatStruct : StatusComponentPartHealthList)
	{
		StatusComponent->PartHealthList.MarkItemDirty(PartStatStruct);
	}
	StatusComponent->PartHealthList.MarkArrayDirty();
	
	TMap<EStatusType, TSubclassOf<UStatusEffectBase>>& StatusComponentGenericStatusEffectMap = StatusComponent->GenericStatusEffectMap;
	StatusComponentGenericStatusEffectMap = GenericStatusEffectMap;

	TMap<EStatusType, float>& StatusComponentGenericStatusEffectMultiplierMap = StatusComponent->GenericStatusEffectMultiplierMap;
	StatusComponentGenericStatusEffectMultiplierMap = GenericStatusEffectMultiplierMap;

	for (TPair<EStatusType, float>& EffectMultiplierEntry : StatusComponentGenericStatusEffectMultiplierMap)
	{
		EffectMultiplierEntry.Value *= StatusScale;
	}
}