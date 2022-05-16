// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Player/PlayerClassComponent.h"
#include "Internationalization/StringTableRegistry.h"
#include "Engine/ActorChannel.h"
#include "NauseaNetDefines.h"
#include "Player/CorePlayerController.h"
#include "Player/CorePlayerState.h"
#include "Player/PlayerStatistics/PlayerStatisticsComponent.h"
#include "Weapon/InventoryManagerComponent.h"
#include "Player/PlayerClass/PlayerClassSkill.h"
#include "Weapon/FireMode.h"
#include "Player/PlayerClass/PlayerClassExperienceSource.h"

UPlayerClassComponent::UPlayerClassComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	SetIsReplicatedByDefault(true);
}

void UPlayerClassComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_WITH_PARAMS_FAST(UPlayerClassComponent, Level, PushReplicationParams::Default);
	DOREPLIFETIME_WITH_PARAMS_FAST(UPlayerClassComponent, ExperiencePercent, PushReplicationParams::Default);

	DOREPLIFETIME_WITH_PARAMS_FAST(UPlayerClassComponent, Variant, PushReplicationParams::InitialOnly);
}

void UPlayerClassComponent::Serialize(FArchive& Ar)
{
	if (!Ar.IsSaving() || !HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject) || GetNameSafe(GetClass()).StartsWith("REINST"))
	{
		Super::Serialize(Ar);
		return;
	}
	
	CoreSkillList = { Tier1CoreSkill, Tier2CoreSkill, Tier3CoreSkill, Tier4CoreSkill };
	PrimarySkillList = { Tier1PrimarySkill, Tier2PrimarySkill, Tier3PrimarySkill, Tier4PrimarySkill };
	AlternativeSkillList = { Tier1AlternativeSkill, Tier2AlternativeSkill, Tier3AlternativeSkill, Tier4AlternativeSkill };

	auto InitializeSkillData = [this](TArray<UPlayerClassSkill*> SkillList, EPlayerClassVariant Variant)
	{
		for (int32 Index = 0; Index < SkillList.Num(); Index++)
		{
			if (!SkillList[Index])
			{
				continue;
			}

			SkillList[Index]->SetVariantAndIndex(Variant, Index);
		}
	};

	InitializeSkillData(CoreSkillList, EPlayerClassVariant::Core);
	InitializeSkillData(PrimarySkillList, EPlayerClassVariant::Primary);
	InitializeSkillData(AlternativeSkillList, EPlayerClassVariant::Alternative);

	Super::Serialize(Ar);
}

void UPlayerClassComponent::PostNetReceive()
{
	Super::PostNetReceive();
}

void UPlayerClassComponent::BeginPlay()
{
	for (const FExperienceSourceEntry& ExperienceSource : ExperienceSourceList)
	{
		if (!ExperienceSource.ExperienceSource)
		{
			continue;
		}

		ExperienceSource.ExperienceSource->Initialize(this);
	}

	OwningPlayerState = Cast<ACorePlayerState>(GetOwner());
	ensure(OwningPlayerState);

	if (GetOwnerRole() != ROLE_Authority)
	{
		Super::BeginPlay();
		CheckRemoteReady();
		return;
	}

	InitializePlayerClass();

	auto RegisterReplicatingSkills = [this](TArray<UPlayerClassSkill*> PlayerClassList)
	{
		for (UPlayerClassSkill* Skill : PlayerClassList)
		{
			if (!Skill || !Skill->IsReplicated())
			{
				continue;
			}

			RegisterReplicatedSubobject(Skill);
		}
	};

	RegisterReplicatingSkills(CoreSkillList);

	switch (GetVariant())
	{
	case EPlayerClassVariant::Primary:
		RegisterReplicatingSkills(PrimarySkillList);
		break;
	case EPlayerClassVariant::Alternative:
		RegisterReplicatingSkills(AlternativeSkillList);
		break;
	}

	int32 ResultLevel;
	int64 ExperienceProgress, ExperienceRequirement;
	float PercentProgress;
	UPlayerClassComponent::GetPlayerClassLevelInfo(this, GetClass(), GetVariant(), ResultLevel, ExperienceProgress, ExperienceRequirement, PercentProgress);
	SetLevel(ResultLevel);
	SetExperiencePercent(PercentProgress);

	if (GetPlayerStatisticsComponent())
	{
		GetPlayerStatisticsComponent()->OnPlayerExperienceUpdate.AddDynamic(this, &UPlayerClassComponent::OnReceiveExperienceUpdate);
	}

	Super::BeginPlay();
}

void UPlayerClassComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	TArray<UPlayerClassObject*> PlayerClassObjects;
	PlayerClassObjects.Append(CoreSkillList);

	switch (GetVariant())
	{
	case EPlayerClassVariant::Primary:
		PlayerClassObjects.Append(PrimarySkillList);
		break;
	case EPlayerClassVariant::Alternative:
		PlayerClassObjects.Append(AlternativeSkillList);
		break;
	}

	for (UPlayerClassObject* PlayerClassObject : PlayerClassObjects)
	{
		if (!PlayerClassObject)
		{
			continue;
		}

		PlayerClassObject->EndPlay(EndPlayReason);
	}
}

bool UPlayerClassComponent::ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	const bool bWroteSomething = ReplicateSubobjectList(Channel, Bunch, RepFlags);
	return Super::ReplicateSubobjects(Channel, Bunch, RepFlags) || bWroteSomething;
}

ACorePlayerState* UPlayerClassComponent::GetOwningPlayerState() const
{
	if (!OwningPlayerState)
	{
		return GetTypedOuter<ACorePlayerState>();
	}

	return OwningPlayerState;
}

AController* UPlayerClassComponent::GetOwningController() const
{
	return Cast<AController>(GetOwningPlayerState()->GetOwner());
}

APawn* UPlayerClassComponent::GetOwningPawn() const
{
	return GetOwningPlayerState()->GetPawn();
}

const FLinearColor& UPlayerClassComponent::GetPlayerClassColour() const
{
	switch (GetVariant())
	{
	case EPlayerClassVariant::Primary:
		return PlayerClassColourPrimaryVariant;
	case EPlayerClassVariant::Alternative:
		return PlayerClassColourAlternativeVariant;
	}

	return PlayerClassCoreColour;
}

int32 UPlayerClassComponent::SetLevel(int32 InLevel)
{
	InLevel = FMath::Min(InLevel, GetMaxLevel());

	if (Level == InLevel)
	{
		return Level;
	}

	Level = InLevel;
	OnRep_Level();
	MARK_PROPERTY_DIRTY_FROM_NAME(UPlayerClassComponent, Level, this);
	return Level;
}

float UPlayerClassComponent::SetExperiencePercent(float InExperiencePercent)
{
	if (ExperiencePercent == InExperiencePercent)
	{
		return ExperiencePercent;
	}

	ExperiencePercent = FMath::Clamp(InExperiencePercent, 0.f, 1.f);
	OnRep_ExperiencePercent();
	MARK_PROPERTY_DIRTY_FROM_NAME(UPlayerClassComponent, ExperiencePercent, this);
	return ExperiencePercent;
}

void UPlayerClassComponent::SetVariant(EPlayerClassVariant InVariant)
{
	ensure(!bRegistered);
	Variant = InVariant;
	MARK_PROPERTY_DIRTY_FROM_NAME(UPlayerClassComponent, Variant, this);
}

TArray<UPlayerClassSkill*> UPlayerClassComponent::GetActiveSkillList() const
{
	TArray<UPlayerClassSkill*> ActiveSkillList(CoreSkillList);
	switch (Variant)
	{
	case EPlayerClassVariant::Primary:
		ActiveSkillList.Append(PrimarySkillList);
		break;
	case EPlayerClassVariant::Alternative:
		ActiveSkillList.Append(AlternativeSkillList);
		break;
	}

	return ActiveSkillList;
}

const UPlayerClassSkill* UPlayerClassComponent::GetPlayerClassSkillByVariantAndIndex(EPlayerClassVariant SkillVariant, int32 SkillIndex) const
{
	switch (SkillVariant)
	{
	case EPlayerClassVariant::Core:
		return CoreSkillList.IsValidIndex(SkillIndex) ? CoreSkillList[SkillIndex] : nullptr;
	case EPlayerClassVariant::Primary:
		return PrimarySkillList.IsValidIndex(SkillIndex) ? PrimarySkillList[SkillIndex] : nullptr;
	case EPlayerClassVariant::Alternative:
		return AlternativeSkillList.IsValidIndex(SkillIndex) ? AlternativeSkillList[SkillIndex] : nullptr;
	}

	return nullptr;
}

TArray<UPlayerClassExperienceSource*> UPlayerClassComponent::GetExperienceSourceList() const
{
	TArray<UPlayerClassExperienceSource*> PlayerExperienceSourceList;
	for (const FExperienceSourceEntry& ExperienceSource : ExperienceSourceList)
	{
		if (!ExperienceSource.ExperienceSource)
		{
			continue;
		}

		PlayerExperienceSourceList.Add(ExperienceSource.ExperienceSource);
	}

	return PlayerExperienceSourceList;
}

TArray<TSoftClassPtr<UInventory>> UPlayerClassComponent::GetDefaultInventoryList() const
{
	TArray<TSoftClassPtr<UInventory>> DefaultInventoryList = CoreDefaultInventory;

	switch (Variant)
	{
	case EPlayerClassVariant::Primary:
		DefaultInventoryList.Append(PrimaryDefaultInventory);
		break;
	case EPlayerClassVariant::Alternative:
		DefaultInventoryList.Append(AlternateDefaultInventory);
		break;
	}

	return DefaultInventoryList;
}

void UPlayerClassComponent::ProcessInitialInventoryList(TArray<TSubclassOf<UInventory>>& InventoryClassList) const
{
	if (ACorePlayerController* CorePlayerController = GetOwningPlayerState() ? Cast<ACorePlayerController>(GetOwningPlayerState()->GetOwningController()) : nullptr)
	{
		InventoryClassList.Append(CorePlayerController->GetInitialInventorySelection(GetClass(), Variant));
	}
}

void UPlayerClassComponent::OnRep_Level()
{
	CheckRemoteReady();
	OnLevelChanged.Broadcast(this, Level);
}

void UPlayerClassComponent::OnRep_ExperiencePercent()
{
	OnLevelProgressChanged.Broadcast(this, ExperiencePercent);
}

void UPlayerClassComponent::OnRep_Variant()
{
	CheckRemoteReady();
}

void UPlayerClassComponent::InitializePlayerClass()
{
	bInitialized = true;

	TArray<UPlayerClassSkill*> ActiveSkillList = GetActiveSkillList();

	for (UPlayerClassSkill* Skill : ActiveSkillList)
	{
		if (!Skill)
		{
			continue;
		}

		Skill->Initialize(this);
	}
}

void UPlayerClassComponent::OnReceiveExperienceUpdate(UPlayerStatisticsComponent* PlayerStatistics, TSubclassOf<UPlayerClassComponent> TargetPlayerClass, EPlayerClassVariant TargetVariant, uint64 Delta)
{
	if (GetWorld()->GetTimerManager().IsTimerActive(ExperienceUpdateThrottleTimer))
	{
		bPendingExperienceUpdate = true;
		return;
	}

	if (TargetPlayerClass != GetClass() || TargetVariant != GetVariant())
	{
		return;
	}

	TWeakObjectPtr<UPlayerClassComponent> WeakThis = this;
	GetWorld()->GetTimerManager().SetTimer(ExperienceUpdateThrottleTimer, FTimerDelegate::CreateWeakLambda(this, [WeakThis]()
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			int32 ResultLevel;
			int64 ExperienceProgress, ExperienceRequirement;
			float PercentProgress;
			UPlayerClassComponent::GetPlayerClassLevelInfo(WeakThis.Get(), WeakThis->GetClass(), WeakThis->GetVariant(), ResultLevel, ExperienceProgress, ExperienceRequirement, PercentProgress);
			WeakThis->SetLevel(ResultLevel);
			WeakThis->SetExperiencePercent(PercentProgress);

			if (WeakThis->bPendingExperienceUpdate)
			{
				WeakThis->GetWorld()->GetTimerManager().ClearTimer(WeakThis->ExperienceUpdateThrottleTimer);
				WeakThis->OnReceiveExperienceUpdate(WeakThis->GetPlayerStatisticsComponent(), WeakThis->GetClass(), WeakThis->GetVariant(), 0);
			}
		}), 5.f, false);
}

void UPlayerClassComponent::CheckRemoteReady()
{
	//Waiting for init.
	if (!HasBegunPlay() || !OwningPlayerState)
	{
		return;
	}

	//Waiting for properties.
	if (Variant == EPlayerClassVariant::Invalid || Level == INDEX_NONE)
	{
		return;
	}

	if (bInitialized)
	{
		return;
	}

	InitializePlayerClass();
	OwningPlayerState->SetPlayerClassComponent(this);
}

inline const UPlayerClassComponent* GetPlayerClassCDO(TSubclassOf<UPlayerClassComponent> PlayerClass)
{
	if (PlayerClass)
	{
		return PlayerClass->GetDefaultObject<UPlayerClassComponent>();
	}

	return nullptr;
}

TArray<UPlayerClassSkill*> UPlayerClassComponent::GetCoreSkillListOfClass(TSubclassOf<UPlayerClassComponent> PlayerClass)
{
	if(const UPlayerClassComponent* PlayerClassCDO = GetPlayerClassCDO(PlayerClass))
	{
		return PlayerClassCDO->GetCoreSkillList();
	}

	return TArray<UPlayerClassSkill*>();
}

TArray<UPlayerClassSkill*> UPlayerClassComponent::GetVariantSkillListOfClass(TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant PlayerClassVariant)
{
	if (const UPlayerClassComponent* PlayerClassCDO = GetPlayerClassCDO(PlayerClass))
	{
		switch (PlayerClassVariant)
		{
		case EPlayerClassVariant::Primary:
			return PlayerClassCDO->GetPrimaryVariantSkillList();
		case EPlayerClassVariant::Alternative:
			return PlayerClassCDO->GetAlternativeVariantSkillList();
		}
	}

	return TArray<UPlayerClassSkill*>();
}

TArray<UPlayerClassSkill*> UPlayerClassComponent::GetActiveSkillListListOfClass(TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant PlayerClassVariant)
{
	if (const UPlayerClassComponent* PlayerClassCDO = GetPlayerClassCDO(PlayerClass))
	{
		return PlayerClassCDO->GetAlternativeVariantSkillList();
	}

	return TArray<UPlayerClassSkill*>();
}

FText UPlayerClassComponent::GetPlayerClassTitle(TSubclassOf<UPlayerClassComponent> PlayerClass)
{
	if (const UPlayerClassComponent* PlayerClassCDO = GetPlayerClassCDO(PlayerClass))
	{
		return PlayerClassCDO->GetTitle();
	}

	return FText::FromString("");
}

FText UPlayerClassComponent::GetVariantName(EPlayerClassVariant PlayerClassVariant)
{
	static FText VariantTitle = LOCTABLE("/Game/Localization/PlayerClassStringTable.PlayerClassStringTable", "Variant_Type");
	switch (PlayerClassVariant)
	{
	case EPlayerClassVariant::Primary:
		return FText::Format(FText::FromString("{0} {1}"), VariantTitle, FText::FromString("I"));
	case EPlayerClassVariant::Alternative:
		return FText::Format(FText::FromString("{0} {1}"), VariantTitle, FText::FromString("II"));
	case EPlayerClassVariant::Special:
		return FText::Format(FText::FromString("{0} {1}"), VariantTitle, FText::FromString("S"));
	default:
		return FText::FromString("");
	}
}

FText UPlayerClassComponent::GetPlayerClassDescription(TSubclassOf<UPlayerClassComponent> PlayerClass)
{
	if (const UPlayerClassComponent* PlayerClassCDO = GetPlayerClassCDO(PlayerClass))
	{
		return PlayerClassCDO->GetDescription();
	}

	return FText::FromString("");
}

TSoftObjectPtr<UTexture2D> UPlayerClassComponent::GetPlayerClassIcon(TSubclassOf<UPlayerClassComponent> PlayerClass)
{
	if (const UPlayerClassComponent* PlayerClassCDO = GetPlayerClassCDO(PlayerClass))
	{
		return PlayerClassCDO->PlayerClassIcon;
	}

	return nullptr;
}

TSoftObjectPtr<UTexture2D> UPlayerClassComponent::GetPlayerClassIconLarge(TSubclassOf<UPlayerClassComponent> PlayerClass)
{
	if (const UPlayerClassComponent* PlayerClassCDO = GetPlayerClassCDO(PlayerClass))
	{
		return PlayerClassCDO->PlayerClassIconLarge;
	}

	return nullptr;
}

const FLinearColor& UPlayerClassComponent::GetPlayerClassCoreColour(TSubclassOf<UPlayerClassComponent> PlayerClass)
{
	if (const UPlayerClassComponent* PlayerClassCDO = GetPlayerClassCDO(PlayerClass))
	{
		return PlayerClassCDO->PlayerClassCoreColour;
	}

	return FLinearColor::Black;
}

const FLinearColor& UPlayerClassComponent::GetPlayerClassVariantColour(TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant PlayerClassVariant)
{
	if (const UPlayerClassComponent* PlayerClassCDO = GetPlayerClassCDO(PlayerClass))
	{
		switch (PlayerClassVariant)
		{
		case EPlayerClassVariant::Primary:
			return PlayerClassCDO->PlayerClassColourPrimaryVariant;
		case EPlayerClassVariant::Alternative:
			return PlayerClassCDO->PlayerClassColourAlternativeVariant;
		}
	}

	return FLinearColor::Black;
}

UMaterialInterface* UPlayerClassComponent::GetPlayerClassBackplateMaterial(TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant PlayerClassVariant)
{
	if (!PlayerClass)
	{
		return nullptr;
	}

	const UPlayerClassComponent* PlayerClassCDO = PlayerClass.GetDefaultObject();

	if (!PlayerClassCDO)
	{
		return nullptr;
	}

	switch (PlayerClassVariant)
	{
	case EPlayerClassVariant::Primary:
		return PlayerClassCDO->PlayerClassPrimaryBackplate;
	case EPlayerClassVariant::Alternative:
		return PlayerClassCDO->PlayerClassAlternativeBackplate;
	}

	return nullptr;
}

UMaterialInterface* UPlayerClassComponent::GetPlayerClassHexagonBackplateMaterial(TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant PlayerClassVariant)
{
	if (!PlayerClass)
	{
		return nullptr;
	}

	const UPlayerClassComponent* PlayerClassCDO = PlayerClass.GetDefaultObject();

	if (!PlayerClassCDO)
	{
		return nullptr;
	}

	switch (PlayerClassVariant)
	{
	case EPlayerClassVariant::Primary:
		return PlayerClassCDO->PlayerClassPrimaryHexagonBackplate;
	case EPlayerClassVariant::Alternative:
		return PlayerClassCDO->PlayerClassAlternativeHexagonBackplate;
	}

	return nullptr;
}

TArray<TSoftClassPtr<UInventory>> UPlayerClassComponent::GetDefaultInventoryListFromClass(TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant PlayerClassVariant)
{
	TArray<TSoftClassPtr<UInventory>> DefaultInventoryList;
	if (const UPlayerClassComponent* PlayerClassCDO = GetPlayerClassCDO(PlayerClass))
	{
		DefaultInventoryList = PlayerClassCDO->CoreDefaultInventory;

		switch (PlayerClassVariant)
		{
		case EPlayerClassVariant::Primary:
			DefaultInventoryList.Append(PlayerClassCDO->PrimaryDefaultInventory);
			break;
		case EPlayerClassVariant::Alternative:
			DefaultInventoryList.Append(PlayerClassCDO->AlternateDefaultInventory);
			break;
		}
	}

	return DefaultInventoryList;
}

int32 UPlayerClassComponent::GetPlayerClassLevel(TScriptInterface<IPlayerOwnershipInterface> PlayerOwnedInterface, TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant PlayerClassVariant)
{
	const UPlayerClassComponent* PlayerClassCDO = PlayerClass.GetDefaultObject();

	if (!PlayerClassCDO || !PlayerClassCDO->GetExperienceCurve())
	{
		return 0;
	}

	const uint64 PlayerClassExperience = UPlayerStatisticHelpers::GetPlayerClassExperience(PlayerOwnedInterface, PlayerClass, PlayerClassVariant);
	uint64 RequiredExpirence = 0;
	int32 PlayerClassLevel = 0;

	while (PlayerClassLevel <= PlayerClassCDO->GetMaxLevel())
	{
		PlayerClassLevel++;

		if (uint64(PlayerClassCDO->GetExperienceCurve()->GetFloatValue(PlayerClassLevel)) > PlayerClassExperience)
		{
			return PlayerClassLevel - 1;
		}
	}

	return PlayerClassCDO->GetMaxLevel();
}

void UPlayerClassComponent::GetPlayerClassLevelInfo(TScriptInterface<IPlayerOwnershipInterface> PlayerOwnedInterface, TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant PlayerClassVariant, int32& PlayerClassLevel, int64& ExperienceProgress, int64& ExperienceRequirement, float& PercentProgress)
{
	const UPlayerClassComponent* PlayerClassCDO = PlayerClass.GetDefaultObject();

	if (!PlayerClassCDO || !PlayerClassCDO->GetExperienceCurve())
	{
		PlayerClassLevel = 0;
		ExperienceProgress = 0;
		ExperienceRequirement = 0;
		PercentProgress = 0.f;
		return;
	}

	const int64 PlayerClassExperience = int64(FMath::Min(UPlayerStatisticHelpers::GetPlayerClassExperience(PlayerOwnedInterface, PlayerClass, PlayerClassVariant), MAX_uint64 / 2));

	PlayerClassLevel = PlayerClassCDO->GetMaxLevel();

	int32 CheckLevel = 0;
	while (CheckLevel <= PlayerClassCDO->GetMaxLevel())
	{
		CheckLevel++;
		ExperienceRequirement = int64(PlayerClassCDO->GetExperienceCurve()->GetFloatValue(CheckLevel));

		if (ExperienceRequirement > PlayerClassExperience)
		{
			PlayerClassLevel = CheckLevel - 1;
			break;
		}
	}

	const uint64 PreviousExperienceRequirement = PlayerClassCDO->GetExperienceCurve()->GetFloatValue(PlayerClassLevel);

	ExperienceProgress = PlayerClassExperience - PreviousExperienceRequirement;
	PercentProgress = float(ExperienceProgress) / float(ExperienceRequirement - PreviousExperienceRequirement);
}