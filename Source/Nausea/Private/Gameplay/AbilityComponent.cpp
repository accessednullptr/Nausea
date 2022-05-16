// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Gameplay/AbilityComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NauseaNetDefines.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Gameplay/Ability/AbilityDecalComponent.h"
#include "Character/CoreCharacter.h"
#include "Gameplay/StatusComponent.h"
#include "Gameplay/Ability/AbilityAction.h"
#include "AI/Action/ActionBrainDataObject.h"
#include "AI/ActionBrainComponentAction.h"
#include "AI/Action/ActionPerformAbility.h"

void FAbilityObjectContainer::Add(TScriptInterface<IAbilityObjectInterface> Instance)
{
	AbilityObjectArray.Add(Instance.GetObject());
}

void FAbilityObjectContainer::Remove(TScriptInterface<IAbilityObjectInterface> Instance)
{
	AbilityObjectArray.Remove(Instance.GetObject());
}

void FAbilityObjectContainer::Activated()
{
	for (UObject* Object : AbilityObjectArray)
	{
		TScriptInterface<IAbilityObjectInterface> ObjectInterface = Object;
		TSCRIPTINTERFACE_CALL_FUNC(ObjectInterface, OnAbilityActivate, K2_OnAbilityActivate);
	}
}

void FAbilityObjectContainer::Completed()
{
	for (UObject* Object : AbilityObjectArray)
	{
		TScriptInterface<IAbilityObjectInterface> ObjectInterface = Object;
		TSCRIPTINTERFACE_CALL_FUNC(ObjectInterface, OnAbilityComplete, K2_OnAbilityComplete);
	}
}

void FAbilityObjectContainer::Cleanup()
{
	for (UObject* Object : AbilityObjectArray)
	{
		TScriptInterface<IAbilityObjectInterface> ObjectInterface = Object;
		TSCRIPTINTERFACE_CALL_FUNC(ObjectInterface, OnAbilityCleanup, K2_OnAbilityCleanup);
	}

	AbilityObjectArray.Empty(AbilityObjectArray.Num());
}

UAbilityComponent::UAbilityComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	bWantsInitializeComponent = true;

	bBindToCharacterDied = true;

	SetIsReplicatedByDefault(true);
}

void UAbilityComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_WITH_PARAMS_FAST(UAbilityComponent, AbilityDataList, PushReplicationParams::Default);
	DOREPLIFETIME_WITH_PARAMS_FAST(UAbilityComponent, AbilityInstanceDataContainer, PushReplicationParams::Default);
}

void UAbilityComponent::InitializeComponent()
{
	Super::InitializeComponent();

	UpdateOwningCharacter();

	AbilityInstanceDataContainer.SetOwningAbilityComponent(this);

	//This can happen in editor when compiling (and spawning the editor preview actor).
	if (!GetWorld() || !GetWorld()->GetGameState())
	{
		return;
	}

	if (UStatusComponent* StatusComponent = Cast<UStatusComponent>(GetOwningCharacter()->GetStatusComponent()))
	{
		OwningStatusComponent = StatusComponent;
		StatusComponent->OnActionInterrupt.AddUObject(this, &UAbilityComponent::OnActionInterrupt);
	}

	if (!IsAuthority())
	{
		return;
	}

	AbilityDataList.Reserve(AbilityClassList.Num());
	for (TSubclassOf<UAbilityInfo> AbilityInstanceClass : AbilityClassList)
	{
		if (!AbilityInstanceClass)
		{
			continue;
		}

		InitializeAbilityClass(AbilityInstanceClass, false);
	}
	AbilityDataList.Shrink();
	MARK_PROPERTY_DIRTY_FROM_NAME(UAbilityComponent, AbilityDataList, this);

	OnRep_AbilityDataList();
}

void UAbilityComponent::BeginPlay()
{
	Super::BeginPlay();

	if (IsAuthority() && AbilityClassList.Num() > 0)
	{
		for (TSubclassOf<UAbilityInfo> AbilityClass : AbilityClassList)
		{
			InitializeAbilityClass(AbilityClass);
		}
	}
	
	if (!IsAuthority())
	{
		for (const FAbilityInstanceData& AbilityInstance : *AbilityInstanceDataContainer)
		{
			ProcessInstanceDataAdded(AbilityInstance);
		}
	}
}

void UAbilityComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (DeltaTime == 0.f)
	{
		return;
	}

	for (UAbilityAction* Ability : AbilityActionList)
	{
		if (!ensure(Ability && !Ability->IsPendingKill()))
		{
			continue;
		}

		Ability->Tick(DeltaTime);
	}
}

void UAbilityComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	CleanupAbilityComponent();
}

void UAbilityComponent::OnOwningCharacterDied(UStatusComponent* Component, TSubclassOf<UDamageType> DamageType, float Damage, FVector_NetQuantize HitLocation, FVector_NetQuantize HitMomentum)
{
	CleanupAbilityComponent();
}

EAbilityRequestResponse UAbilityComponent::CanPerformAbility(TSubclassOf<UAbilityInfo> AbilityClass) const
{
	if (OwningStatusComponent.IsValid() && OwningStatusComponent->IsBlockingAction())
	{
		return EAbilityRequestResponse::ActionBlocked;
	}

	const FAbilityData& AbilityDataEntry = GetAbilityDataByClass(AbilityClass);

	if (!AbilityDataEntry.IsValid())
	{
		return EAbilityRequestResponse::NotInitialized;
	}

	if (!AbilityDataEntry.HasCharge())
	{
		return EAbilityRequestResponse::NoChargeRemaining;
	}

	return EAbilityRequestResponse::Success;
}

EAbilityRequestResponse UAbilityComponent::CanPerformAbility(const FAbilityInstanceData& AbilityInstanceData) const
{
	if (!AbilityInstanceData.IsValid())
	{
		return EAbilityRequestResponse::InvalidAbilityInstance;
	}

	return CanPerformAbility(TSubclassOf<UAbilityInfo>(AbilityInstanceData.GetClass()));
}

EAbilityRequestResponse UAbilityComponent::PerformAbility(FAbilityInstanceData&& AbilityInstanceData, bool bNotify)
{
	const EAbilityRequestResponse Response = CanPerformAbility(AbilityInstanceData);

	if (Response != EAbilityRequestResponse::Success)
	{
		return Response;
	}

	FAbilityData& AbilityDataEntry = GetAbilityDataByClass(AbilityInstanceData.GetClass());

	AbilityDataEntry.ConsumeCharge();

	AbilityInstanceData.InitializeAbilityInstance(this, AbilityDataEntry); //Initialize all timing and other relevant data now.
	FAbilityInstanceData& Instance = AbilityInstanceDataContainer->Add_GetRef(MoveTemp(AbilityInstanceData));
	ProcessInstanceDataAdded(Instance);
	AbilityInstanceDataContainer.MarkItemDirty(Instance);

	MARK_PROPERTY_DIRTY_FROM_NAME(UAbilityComponent, AbilityDataList, this);
	MARK_PROPERTY_DIRTY_FROM_NAME(UAbilityComponent, AbilityInstanceDataContainer, this);

	return EAbilityRequestResponse::Success;
}

void UAbilityComponent::RegisterAbilityObjectInstance(TScriptInterface<IAbilityObjectInterface> AbilityObject, const FAbilityInstanceData& AbilityInstance, const FAbilityTargetData& AbilityTargetData)
{
	if (!TSCRIPTINTERFACE_IS_VALID(AbilityObject))
	{
		return;
	}

	FAbilityObjectContainer& AbilityInstanceObjectContainer = GetAbilityTargetDataObjectMap().FindOrAdd(AbilityTargetData.GetHandle());
	AbilityInstanceObjectContainer.Add(AbilityObject);
	TSCRIPTINTERFACE_CALL_FUNC(AbilityObject, InitializeForAbilityData, K2_InitializeForAbilityData, this, AbilityInstance, AbilityTargetData);
}

void UAbilityComponent::RegisterAbilityAction(FAbilityTargetDataHandle AbilityTargetDataHandle, UAbilityAction* AbilityAction)
{
	ensure(!AbilityActionList.Contains(AbilityAction));
	AbilityActionList.Add(AbilityAction);
	AbilityActionMap.FindOrAdd(AbilityTargetDataHandle).AbilityList.Add(AbilityAction);
	SetComponentTickEnabled(true);
}

void UAbilityComponent::OnAbilityActionCleanup(FAbilityTargetDataHandle AbilityTargetDataHandle, UAbilityAction* AbilityAction)
{
	AbilityActionList.Remove(AbilityAction);
	if (AbilityActionMap.Contains(AbilityTargetDataHandle))
	{
		AbilityActionMap[AbilityTargetDataHandle].AbilityList.Remove(AbilityAction);

		if (AbilityActionMap[AbilityTargetDataHandle].AbilityList.Num() == 0)
		{
			AbilityActionMap.Remove(AbilityTargetDataHandle);
		}
	}

	SetComponentTickEnabled(AbilityActionList.Num() != 0);
}

void UAbilityComponent::AsyncLoadAbilityObjectClass(TSoftClassPtr<UObject> SoftClass)
{
	if (SoftClass.Get())
	{
		LoadedClassSet.Add(SoftClass.Get());
		return;
	}

	UAssetManager* AssetManager = UAssetManager::GetIfValid();

	if (!AssetManager)
	{
		return;
	}

	FStreamableManager& StreamableManager = AssetManager->GetStreamableManager();
	TSharedPtr<FStreamableHandle> StreamableHandle = StreamableManager.RequestAsyncLoad(SoftClass.ToSoftObjectPath());

	if (!StreamableHandle.IsValid())
	{
		return;
	}

	if (StreamableHandle->HasLoadCompleted())
	{
		LoadedClassSet.Add(SoftClass.Get());
		return;
	}

	TWeakObjectPtr<UAbilityComponent> WeakThis = TWeakObjectPtr<UAbilityComponent>(this);
	auto StreamClassCompleteDelegate = [WeakThis, StreamableHandle] {
		if (!WeakThis.IsValid() || !StreamableHandle.IsValid())
		{
			return;
		}

		WeakThis->OnAbilitySoftClassLoadComplete(TSubclassOf<UObject>(Cast<UClass>(StreamableHandle->GetLoadedAsset())));
	};
	
	StreamableHandle->BindCompleteDelegate(FStreamableDelegate::CreateWeakLambda(this, StreamClassCompleteDelegate));
}

void UAbilityComponent::AsyncLoadAbilityObject(TSoftObjectPtr<UObject> SoftObject)
{
	if (SoftObject.IsNull())
	{
		return;
	}

	if (SoftObject.Get())
	{
		LoadedObjectSet.Add(SoftObject.Get());
		return;
	}

	UAssetManager* AssetManager = UAssetManager::GetIfValid();

	if (!AssetManager)
	{
		return;
	}

	FStreamableManager& StreamableManager = AssetManager->GetStreamableManager();
	TSharedPtr<FStreamableHandle> StreamableHandle = StreamableManager.RequestAsyncLoad(SoftObject.ToSoftObjectPath());

	if (!StreamableHandle.IsValid())
	{
		return;
	}

	if (StreamableHandle->HasLoadCompleted())
	{
		LoadedObjectSet.Add(SoftObject.Get());
		return;
	}

	TWeakObjectPtr<UAbilityComponent> WeakThis = TWeakObjectPtr<UAbilityComponent>(this);
	auto StreamClassCompleteDelegate = [WeakThis, StreamableHandle] {
		if (!WeakThis.IsValid() || !StreamableHandle.IsValid())
		{
			return;
		}

		WeakThis->OnAbilitySoftObjectLoadComplete(StreamableHandle->GetLoadedAsset());
	};

	StreamableHandle->BindCompleteDelegate(FStreamableDelegate::CreateWeakLambda(this, StreamClassCompleteDelegate));
}

void UAbilityComponent::InterruptAbilityComponent()
{
	bool bRemovedInstance = false;

	for (int32 Index = AbilityInstanceDataContainer->Num() - 1; Index >= 0; Index--)
	{
		if (AbilityInstanceDataContainer[Index].IsStartupComplete())
		{
			continue;
		}

		OnAbilityInstanceInterrupted.Broadcast(this, AbilityInstanceDataContainer[Index].GetHandle());
		AbilityInstanceDataContainer->RemoveAt(Index);
		AbilityInstanceDataContainer.MarkArrayDirty();
		bRemovedInstance = true;
	}

	if (bRemovedInstance)
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(UAbilityComponent, AbilityInstanceDataContainer, this);
	}
}

bool UAbilityComponent::InterruptAbility(FAbilityInstanceHandle AbilityInstanceHandle)
{
	for (int32 Index = AbilityInstanceDataContainer->Num() - 1; Index >= 0; Index--)
	{
		if (AbilityInstanceDataContainer[Index].IsStartupComplete() && AbilityInstanceDataContainer[Index].GetHandle() == AbilityInstanceHandle)
		{
			continue;
		}

		OnAbilityInstanceInterrupted.Broadcast(this, AbilityInstanceHandle);
		AbilityInstanceDataContainer->RemoveAt(Index);
		AbilityInstanceDataContainer.MarkArrayDirty();
		MARK_PROPERTY_DIRTY_FROM_NAME(UAbilityComponent, AbilityInstanceDataContainer, this);
		return true;
	}

	return false;
}

void UAbilityComponent::OnAbilitySoftClassLoadComplete(TSubclassOf<UObject> ObjectClass)
{
	LoadedClassSet.Add(ObjectClass);
}

void UAbilityComponent::OnAbilitySoftObjectLoadComplete(UObject* Object)
{
	LoadedObjectSet.Add(Object);
}

bool UAbilityComponent::IsHandleValid(FAbilityInstanceHandle InstanceHandle) const
{
	for (const FAbilityInstanceData& AbilityInstanceData : *AbilityInstanceDataContainer)
	{
		if (AbilityInstanceData.GetHandle() == InstanceHandle)
		{
			return true;
		}
	}

	return false;
}

void UAbilityComponent::TestRequestAbility(TSubclassOf<UAbilityInfo> AbilityClass)
{
	if (!IsLocallyOwned())
	{
		return;
	}

	TArray<FAbilityTargetData> AbilityTargetData;
	AbilityTargetData.Add(FAbilityTargetData::GenerateTargetLocationData(GetOwningCharacter()->GetActorTransform()));
	AbilityClass.GetDefaultObject()->ProcessTargetData(this, AbilityTargetData);
	PerformAbility(FAbilityInstanceData::GenerateInstanceData(AbilityClass, AbilityTargetData));

	FTimerHandle Dummy;
	GetWorld()->GetTimerManager().SetTimer(Dummy, FTimerDelegate::CreateUObject(this, &UAbilityComponent::TestRequestAbility, AbilityClass), 2.f, false);
}

inline void UAbilityComponent::GetAbilityInstanceAndTargetDataByID(FAbilityInstanceHandle InstanceHandle, FAbilityTargetDataHandle TargetDataHandle, FAbilityInstanceData*& OwningAbilityInstanceData, FAbilityTargetData*& OwningAbilityTargetData)
{
	OwningAbilityInstanceData = nullptr;
	OwningAbilityTargetData = nullptr;

	for (FAbilityInstanceData& AbilityInstanceData : *AbilityInstanceDataContainer)
	{
		if (AbilityInstanceData.GetHandle() == InstanceHandle)
		{
			OwningAbilityInstanceData = &AbilityInstanceData;
			break;
		}
	}

	if (!OwningAbilityInstanceData)
	{
		return;
	}

	TArray<FAbilityTargetData>& TargetDataList = OwningAbilityInstanceData->GetTargetData().GetTargetDataList();
	FAbilityTargetData* OwningTargetData = nullptr;

	for (FAbilityTargetData& AbilityTargetData : TargetDataList)
	{
		if (AbilityTargetData.GetHandle() == TargetDataHandle)
		{
			OwningAbilityTargetData = &AbilityTargetData;
			break;
		}
	}
}

const FAbilityTargetData& UAbilityComponent::GetAbilityTargetDataByHandle(FAbilityInstanceHandle InstanceHandle, FAbilityTargetDataHandle TargetDataHandle) const
{
	for (const FAbilityInstanceData& AbilityInstanceData : *AbilityInstanceDataContainer)
	{
		if (AbilityInstanceData.GetHandle() == InstanceHandle)
		{
			const TArray<FAbilityTargetData>& TargetDataList = AbilityInstanceData.GetTargetData().GetTargetDataList();
			for (const FAbilityTargetData& AbilityTargetData : TargetDataList)
			{
				return AbilityTargetData;
			}
		}
	}

	return FAbilityTargetData::InvalidTargetData;
}

const FAbilityActionList& UAbilityComponent::GetTargetDataAbilityActionList(FAbilityTargetDataHandle TargetDataHandle) const
{
	static FAbilityActionList InvalidAbilityActionList = FAbilityActionList();

	if (AbilityActionMap.Contains(TargetDataHandle))
	{
		return AbilityActionMap[TargetDataHandle];
	}

	return InvalidAbilityActionList;
}

void UAbilityComponent::PlayAnimationMontage(UAnimMontage* Montage, FAbilityInstanceHandle InstanceHandle, float InStartTime)
{
	if (!Montage)
	{
		return;
	}

	LastKnownAnimationAbilityInstanceHandleMap.FindOrAdd(Montage) = InstanceHandle;

	if (GetOwningCharacter()->GetMesh() && GetOwningCharacter()->GetMesh()->GetAnimInstance())
	{
		GetOwningCharacter()->GetMesh()->GetAnimInstance()->Montage_Play(Montage, 1.f, EMontagePlayReturnType::MontageLength, InStartTime);
	}
}

void UAbilityComponent::StopAnimationMontage(UAnimMontage* Montage, FAbilityInstanceHandle InstanceHandle)
{
	if (!Montage)
	{
		return;
	}

	if (LastKnownAnimationAbilityInstanceHandleMap.Contains(Montage))
	{
		if (LastKnownAnimationAbilityInstanceHandleMap[Montage] != InstanceHandle)
		{
			return;
		}
	}

	if (GetOwningCharacter()->GetMesh() && GetOwningCharacter()->GetMesh()->GetAnimInstance())
	{
		GetOwningCharacter()->GetMesh()->GetAnimInstance()->Montage_Stop(0.1f, Montage);
	}
}

inline FAbilityInstanceData* UAbilityComponent::GetAbilityInstanceByHandle(FAbilityInstanceHandle InstanceHandle)
{
	for (FAbilityInstanceData& AbilityInstanceData : *AbilityInstanceDataContainer)
	{
		if (AbilityInstanceData.GetHandle() == InstanceHandle)
		{
			return &AbilityInstanceData;
		}
	}

	return nullptr;
}

void UAbilityComponent::OnRep_AbilityDataList()
{
	TMap<TSubclassOf<UAbilityInfo>, FAbilityData> NewAbilityDataMap;
	NewAbilityDataMap.Reserve(AbilityDataList.Num());
	AbilityDataIndexMap.Empty(AbilityDataList.Num());
	int32 Index = 0;
	for (const FAbilityData& AbilityData : AbilityDataList)
	{
		NewAbilityDataMap.Add(TSubclassOf<UAbilityInfo>(AbilityData.GetClass())) = AbilityData;
		AbilityDataIndexMap.Add(TSubclassOf<UAbilityInfo>(AbilityData.GetClass())) = Index;
		Index++;
	}

	for (const TPair<TSubclassOf<UAbilityInfo>, FAbilityData>& NewAbilityEntry : NewAbilityDataMap)
	{
		const FAbilityData& NewAbilityData = NewAbilityEntry.Value;

		if (!PreviousAbilityDataMap.Contains(NewAbilityData.GetClass()))
		{
			OnAbilityClassInitialized(NewAbilityData.GetClass());
			OnAbilityDataAdded.Broadcast(this, NewAbilityData);
			continue;
		}

		const FAbilityData& PreviousAbilityData = PreviousAbilityDataMap[NewAbilityData.GetClass()];

		if (NewAbilityData == PreviousAbilityData)
		{
			continue;
		}

		OnAbilityDataChanged.Broadcast(this, NewAbilityData, PreviousAbilityData);
	}

	for (const TPair<TSubclassOf<UAbilityInfo>, FAbilityData>& PreviousAbilityEntry : PreviousAbilityDataMap)
	{
		if (!NewAbilityDataMap.Contains(PreviousAbilityEntry.Value.GetClass()))
		{
			OnAbilityDataRemoved.Broadcast(this, PreviousAbilityEntry.Value);
		}
	}

	PreviousAbilityDataMap = NewAbilityDataMap;
}

void UAbilityComponent::InitializeAbilityClass(TSubclassOf<UAbilityInfo> AbilityClass, bool bNotify)
{
	if (!IsAuthority())
	{
		return;
	}

	if (!AbilityClass || AbilityDataList.Contains(AbilityClass))
	{
		return;
	}

	const UAbilityInfo* AbilityInfoCDO = AbilityClass.GetDefaultObject();

	FAbilityData PendingAbilityData = FAbilityData(AbilityClass, GetWorld()->GetGameState()->GetServerWorldTimeSeconds());

	if (!PendingAbilityData.IsValid())
	{
		return;
	}

	FAbilityData& AbilityData = AbilityDataList[AbilityDataList.Add(MoveTemp(PendingAbilityData))];
	MARK_PROPERTY_DIRTY_FROM_NAME(UAbilityComponent, AbilityDataList, this);

	if (AbilityData.GetRechargeTime() != FVector2D(-1.f))
	{
		GetWorld()->GetTimerManager().SetTimer(AbilityData.GetRechargeTimerHandle(), FTimerDelegate::CreateUObject(this, &UAbilityComponent::RechargeTimerCompleted, AbilityData.GetClass()),
			AbilityInfoCDO->GetRechargeDuration(), false);
	}

	if (bNotify)
	{
		OnRep_AbilityDataList();
	}
}

void UAbilityComponent::OnAbilityClassInitialized(TSubclassOf<UAbilityInfo> AbilityClass)
{
	const UAbilityInfo* AbilityInfoCDO = AbilityClass.GetDefaultObject();
	check(AbilityInfoCDO);
	AbilityInfoCDO->AbilityInitializedOnComponent(this);
}

void UAbilityComponent::RechargeTimerCompleted(UClass* AbilityClass)
{
	FAbilityData& AbilityDataEntry = GetAbilityDataByClass(AbilityClass);

	if (!AbilityDataEntry.IsValid())
	{
		return;
	}

	AbilityDataEntry.IncrementChargeCount();
	MARK_PROPERTY_DIRTY_FROM_NAME(UAbilityComponent, AbilityDataList, this);

	const UAbilityInfo* AbilityInfoCDO = AbilityDataEntry.GetClassCDO();

	if (!AbilityInfoCDO)
	{
		return;
	}

	if (AbilityDataEntry.GetChargeCount() < AbilityInfoCDO->GetMaxChargeCount())
	{
		AbilityDataEntry.SetRechargeTime(FVector2D(GetWorld()->GetGameState()->GetServerWorldTimeSeconds(), GetWorld()->GetGameState()->GetServerWorldTimeSeconds() + AbilityInfoCDO->GetRechargeDuration()));

		GetWorld()->GetTimerManager().SetTimer(AbilityDataEntry.GetRechargeTimerHandle(), FTimerDelegate::CreateUObject(this, &UAbilityComponent::RechargeTimerCompleted, AbilityDataEntry.GetClass()),
			AbilityInfoCDO->GetRechargeDuration(), false);
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(UAbilityComponent, AbilityDataList, this);
}

FAbilityData& UAbilityComponent::GetAbilityDataByClass(TSubclassOf<UAbilityInfo> AbilityClass)
{
	if (AbilityDataIndexMap.Contains(AbilityClass))
	{
		const int32 FastLookupIndex = AbilityDataIndexMap[AbilityClass];
		
		if (AbilityDataList.IsValidIndex(FastLookupIndex) && AbilityDataList[FastLookupIndex].GetClass() == AbilityClass)
		{
			return AbilityDataList[FastLookupIndex];
		}
	}
	
	for (int32 Index = 0; Index < AbilityClassList.Num(); Index++)
	{
		if (AbilityDataList[Index].GetClass() == AbilityClass)
		{
			AbilityDataIndexMap.FindOrAdd(AbilityClass) = Index;
			return AbilityDataList[Index];
		}
	}

	return FAbilityData::InvalidAbilityData;
}

const FAbilityData& UAbilityComponent::GetAbilityDataByClass(TSubclassOf<UAbilityInfo> AbilityClass) const
{
	if (AbilityDataIndexMap.Contains(AbilityClass))
	{
		const int32 FastLookupIndex = AbilityDataIndexMap[AbilityClass];

		if (AbilityDataList.IsValidIndex(FastLookupIndex) && AbilityDataList[FastLookupIndex].GetClass() == AbilityClass)
		{
			return AbilityDataList[FastLookupIndex];
		}
	}

	for (int32 Index = 0; Index < AbilityDataList.Num(); Index++)
	{
		if (AbilityDataList[Index].GetClass() == AbilityClass)
		{
			return AbilityDataList[Index];
		}
	}

	return FAbilityData::InvalidAbilityData;
}

void UAbilityComponent::ProcessInstanceDataAdded(const FAbilityInstanceData& InstanceData)
{
	if (!HasBegunPlay())
	{
		return;
	}

	if (!InstanceData.GetClass())
	{
		return;
	}

	const UAbilityInfo* AbilityInfoCDO = InstanceData.GetClassCDO();

	if (!AbilityInfoCDO)
	{
		return;
	}

	const FVector2D& StartupTime = InstanceData.GetStartupTime();

	if (GetWorld() && GetWorld()->GetGameState())
	{
		AbilityInfoCDO->PlayAbilityCast(this, InstanceData, GetWorld()->GetGameState()->GetServerWorldTimeSeconds(), StartupTime.X);
	}

	OnAbilityInstanceStartupBegin.Broadcast(this, InstanceData.GetHandle());

	const float WorldTimeSeconds = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
	if (StartupTime != FVector2D(-1.f) && WorldTimeSeconds < StartupTime.Y)
	{
		const float StartupTimeRemaining = StartupTime.Y - WorldTimeSeconds;

		FTimerHandle& AbilityTimerHandle = AbilityInstanceTimerMap.FindOrAdd(InstanceData.GetHandle());
		GetWorld()->GetTimerManager().ClearTimer(AbilityTimerHandle);

		GetWorld()->GetTimerManager().SetTimer(AbilityTimerHandle,
			FTimerDelegate::CreateUObject(this, &UAbilityComponent::OnInstanceDataCastComplete, InstanceData.GetHandle()),
			StartupTimeRemaining, false);
	}
	else
	{
		OnInstanceDataCastComplete(InstanceData.GetHandle());
	}

	const TArray<FAbilityTargetData>& TargetDataList = InstanceData.GetTargetData().GetTargetDataList();
	for (const FAbilityTargetData& AbilityTargetData : TargetDataList)
	{
		ProcessInstanceTargetDataAdded(InstanceData, AbilityTargetData);
		AbilityInfoCDO->OnTargetDataAdded(this, InstanceData, AbilityTargetData);
	}

	AbilityInfoCDO->K2_OnTargetDataAdded(this, InstanceData, TargetDataList);
}

void UAbilityComponent::ProcessInstanceDataRemoved(const FAbilityInstanceData& InstanceData)
{
	if (!InstanceData.GetClass())
	{
		return;
	}

	const UAbilityInfo* AbilityInfoCDO = InstanceData.GetClassCDO();

	if (!AbilityInfoCDO)
	{
		return;
	}

	if (GetWorld() && GetWorld()->GetGameState())
	{
		const float WorldTimeSeconds = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
		AbilityInfoCDO->StopPlayAbilityCast(this, InstanceData, WorldTimeSeconds);
		AbilityInfoCDO->StopPlayAbilityActivation(this, InstanceData, WorldTimeSeconds);
	}

	const TArray<FAbilityTargetData>& TargetDataList = InstanceData.GetTargetData().GetTargetDataList();
	for (const FAbilityTargetData& AbilityTargetData : TargetDataList)
	{
		ProcessInstanceTargetDataRemoved(InstanceData, AbilityTargetData);
		AbilityInfoCDO->OnTargetDataRemoved(this, InstanceData, AbilityTargetData);
	}

	AbilityInfoCDO->K2_OnTargetDataRemoved(this, InstanceData, TargetDataList);
}

void UAbilityComponent::ProcessInstanceDataChanged(const FAbilityInstanceData& InstanceData)
{
	if (!InstanceData.GetClass())
	{
		return;
	}
	
	const UAbilityInfo* AbilityInfoCDO = InstanceData.GetClassCDO();

	if (!AbilityInfoCDO)
	{
		return;
	}

	TArray<FAbilityTargetData> AddedTargetData;
	TArray<FAbilityTargetData> RemovedTargetData;
	InstanceData.GetTargetData().UpdateTargetDataCache(AddedTargetData, RemovedTargetData);

	for (const FAbilityTargetData& AbilityTargetData : AddedTargetData)
	{
		ProcessInstanceTargetDataAdded(InstanceData, AbilityTargetData);
		AbilityInfoCDO->OnTargetDataAdded(this, InstanceData, AbilityTargetData);
	}
	AbilityInfoCDO->K2_OnTargetDataAdded(this, InstanceData, AddedTargetData);

	for (const FAbilityTargetData& AbilityTargetData : RemovedTargetData)
	{
		ProcessInstanceTargetDataRemoved(InstanceData, AbilityTargetData);
		AbilityInfoCDO->OnTargetDataRemoved(this, InstanceData, AbilityTargetData);
	}
	AbilityInfoCDO->K2_OnTargetDataRemoved(this, InstanceData, RemovedTargetData);
}

void UAbilityComponent::OnInstanceDataCastComplete(FAbilityInstanceHandle InstanceHandle)
{
	FAbilityInstanceData* AbilityInstanceData = GetAbilityInstanceByHandle(InstanceHandle);

	if (!AbilityInstanceData)
	{
		return;
	}

	AbilityInstanceData->MarkStartupComplete();

	const FVector2D& StartupTime = AbilityInstanceData->GetStartupTime();

	if (const UAbilityInfo* AbilityInfoCDO = AbilityInstanceData->GetClassCDO())
	{
		if (GetWorld() && GetWorld()->GetGameState())
		{
			const float WorldTimeSeconds = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
			AbilityInfoCDO->PlayAbilityActivation(this, *AbilityInstanceData, WorldTimeSeconds, StartupTime.X);
		}
	}

	OnAbilityInstanceStartupComplete.Broadcast(this, InstanceHandle);
}

void UAbilityComponent::ProcessInstanceTargetDataAdded(const FAbilityInstanceData& InstanceData, const FAbilityTargetData& AbilityTargetData)
{
	FVector2D StartupTime = UAbilityInfo::GetAbilityTargetDataStartupTime(InstanceData, AbilityTargetData);

	const float WorldTimeSeconds = GetOwningCharacter()->GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
	const float StartupTimeRemaining = StartupTime.Y - WorldTimeSeconds;

	if (const UAbilityInfo* AbilityInfoCDO = InstanceData.GetClassCDO())
	{
		AbilityInfoCDO->OnAbilityTargetStartup(this, InstanceData, AbilityTargetData);
	}

	if (StartupTimeRemaining <= 0.f)
	{
		OnTargetDataStartupComplete(InstanceData.GetHandle(), AbilityTargetData.GetHandle());
		return;
	}
	
	if (AbilityTargetDataTimerMap.Contains(AbilityTargetData.GetHandle()))
	{
		GetWorld()->GetTimerManager().ClearTimer(AbilityTargetDataTimerMap[AbilityTargetData.GetHandle()]);
	}

	FTimerHandle& AbilityTimerHandle = AbilityTargetDataTimerMap.FindOrAdd(AbilityTargetData.GetHandle());

	GetWorld()->GetTimerManager().SetTimer(AbilityTimerHandle,
		FTimerDelegate::CreateUObject(this, &UAbilityComponent::OnTargetDataStartupComplete, InstanceData.GetHandle(), AbilityTargetData.GetHandle()),
		StartupTimeRemaining, false);
}

void UAbilityComponent::ProcessInstanceTargetDataRemoved(const FAbilityInstanceData& InstanceData, const FAbilityTargetData& AbilityTargetData)
{
	if (AbilityTargetDataTimerMap.Contains(AbilityTargetData.GetHandle()))
	{
		GetWorld()->GetTimerManager().ClearTimer(AbilityTargetDataTimerMap[AbilityTargetData.GetHandle()]);
	}
}

void UAbilityComponent::OnTargetDataStartupComplete(FAbilityInstanceHandle InstanceHandle, FAbilityTargetDataHandle TargetDataHandle)
{
	FAbilityInstanceData* OwningAbilityInstanceData = nullptr;
	FAbilityTargetData* OwningAbilityTargetData = nullptr;

	GetAbilityInstanceAndTargetDataByID(InstanceHandle, TargetDataHandle, OwningAbilityInstanceData, OwningAbilityTargetData);

	if (!OwningAbilityInstanceData || !OwningAbilityTargetData)
	{
		return;
	}

	OnAbilityInstanceStartupBegin.Broadcast(this, InstanceHandle);

	FAbilityObjectContainer* AbilityObjectContainer = AbilityTargetDataObjectMap.Find(OwningAbilityTargetData->GetHandle());
	if (AbilityObjectContainer)
	{
		AbilityObjectContainer->Activated();
	}

	const UAbilityInfo* AbilityInfoCDO = OwningAbilityInstanceData->GetClassCDO();

	if (AbilityInfoCDO)
	{
		AbilityInfoCDO->OnAbilityTargetActivation(this, *OwningAbilityInstanceData, *OwningAbilityTargetData);
	}

	const float WorldTimeSeconds = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
	const FVector2D& ActivationTime = UAbilityInfo::GetAbilityTargetDataActivationTime(*OwningAbilityInstanceData, *OwningAbilityTargetData);

	const float ActivationTimeRemaining = ActivationTime.Y - WorldTimeSeconds;

	if (ActivationTimeRemaining > 0.f)
	{
		if (AbilityTargetDataTimerMap.Contains(OwningAbilityTargetData->GetHandle()))
		{
			GetWorld()->GetTimerManager().ClearTimer(AbilityTargetDataTimerMap[TargetDataHandle]);
		}

		FTimerHandle& AbilityTimerHandle = AbilityTargetDataTimerMap.FindOrAdd(TargetDataHandle);

		GetWorld()->GetTimerManager().SetTimer(AbilityTimerHandle,
			FTimerDelegate::CreateUObject(this, &UAbilityComponent::OnTargetDataActivationComplete, InstanceHandle, TargetDataHandle),
			ActivationTimeRemaining, false);
		return;
	}

	OnTargetDataActivationComplete(InstanceHandle, TargetDataHandle);
}

void UAbilityComponent::OnTargetDataActivationComplete(FAbilityInstanceHandle InstanceHandle, FAbilityTargetDataHandle TargetDataHandle)
{
	FAbilityInstanceData* OwningAbilityInstanceData = nullptr;
	FAbilityTargetData* OwningAbilityTargetData = nullptr;

	GetAbilityInstanceAndTargetDataByID(InstanceHandle, TargetDataHandle, OwningAbilityInstanceData, OwningAbilityTargetData);

	if (!OwningAbilityInstanceData || !OwningAbilityTargetData)
	{
		return;
	}

	FAbilityObjectContainer* AbilityObjectContainer = AbilityTargetDataObjectMap.Find(OwningAbilityTargetData->GetHandle());
	if (AbilityObjectContainer)
	{
		AbilityObjectContainer->Completed();
	}

	if (AbilityTargetDataTimerMap.Contains(OwningAbilityTargetData->GetHandle()))
	{
		GetWorld()->GetTimerManager().ClearTimer(AbilityTargetDataTimerMap[TargetDataHandle]);
		AbilityTargetDataTimerMap.Remove(TargetDataHandle);
	}

	if (const UAbilityInfo* AbilityInfoCDO = OwningAbilityInstanceData->GetClassCDO())
	{
		if (GetWorld() && GetWorld()->GetGameState())
		{
			const float WorldTimeSeconds = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
			AbilityInfoCDO->StopPlayAbilityCast(this, *OwningAbilityInstanceData, WorldTimeSeconds);
			AbilityInfoCDO->StopPlayAbilityActivation(this, *OwningAbilityInstanceData, WorldTimeSeconds);
		}
	}

	if (AbilityActionMap.Contains(OwningAbilityTargetData->GetHandle()))
	{
		TArray<UAbilityAction*> AbilityList = AbilityActionMap[OwningAbilityTargetData->GetHandle()].AbilityList;
		for (UAbilityAction* AbilityAction : AbilityList)
		{
			if (!AbilityAction || AbilityAction->IsCompleted())
			{
				continue;
			}

			AbilityAction->Complete();
		}
		for (UAbilityAction* AbilityAction : AbilityList)
		{
			if (!AbilityAction)
			{
				continue;
			}

			AbilityAction->Cleanup();
		}

		AbilityActionMap.Remove(OwningAbilityTargetData->GetHandle());
	}

	//Authority is the only one who manages target data removal.
	if (!IsAuthority())
	{
		return;
	}

	FTimerHandle& AbilityTimerHandle = AbilityTargetDataTimerMap.FindOrAdd(TargetDataHandle);

	OnAbilityInstanceComplete.Broadcast(this, InstanceHandle);

	GetWorld()->GetTimerManager().SetTimer(AbilityTimerHandle,
		FTimerDelegate::CreateUObject(this, &UAbilityComponent::OnTargetDataDestructionReady, InstanceHandle, TargetDataHandle),
		2.f, false);
}

void UAbilityComponent::OnTargetDataDestructionReady(FAbilityInstanceHandle InstanceHandle, FAbilityTargetDataHandle TargetDataHandle)
{
	int32 OwningAbilityInstanceIndex = INDEX_NONE;

	for (int32 Index = AbilityInstanceDataContainer->Num() - 1; Index >= 0; Index--)
	{
		if (AbilityInstanceDataContainer[Index].GetHandle() == InstanceHandle)
		{
			OwningAbilityInstanceIndex = Index;
			break;
		}
	}

	if (OwningAbilityInstanceIndex == INDEX_NONE)
	{
		return;
	}

	bool bStaleAbilityInstance = true;
	TArray<FAbilityTargetData>& TargetDataList = AbilityInstanceDataContainer[OwningAbilityInstanceIndex].GetTargetData().GetTargetDataList();
	for (FAbilityTargetData& AbilityTargetData : TargetDataList)
	{
		if (AbilityTargetData.GetHandle() == TargetDataHandle)
		{
			AbilityTargetData.MarkStale();
			continue;
		}

		if (!AbilityTargetData.IsStale())
		{
			bStaleAbilityInstance = false;
			break;
		}
	}

	if (bStaleAbilityInstance)
	{
		ProcessInstanceDataRemoved(AbilityInstanceDataContainer[OwningAbilityInstanceIndex]);
		AbilityInstanceDataContainer->RemoveAt(OwningAbilityInstanceIndex);
		AbilityInstanceDataContainer.MarkArrayDirty();
	}
	else
	{
		AbilityInstanceDataContainer.MarkItemDirty(AbilityInstanceDataContainer[OwningAbilityInstanceIndex]);
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(UAbilityComponent, AbilityInstanceDataContainer, this);
}

void UAbilityComponent::CleanupAbilityComponent()
{
	for (TPair<FAbilityTargetDataHandle, FAbilityObjectContainer>& AbilityTargetDataObject : AbilityTargetDataObjectMap)
	{
		AbilityTargetDataObject.Value.Cleanup();
	}
	AbilityTargetDataObjectMap.Empty();
	
	TMap<FAbilityTargetDataHandle, FAbilityActionList> CurrentAbilityActionMap = AbilityActionMap;
	for (TPair<FAbilityTargetDataHandle, FAbilityActionList>& Entry : CurrentAbilityActionMap)
	{
		TArray<UAbilityAction*> ActionArray = Entry.Value.AbilityList;
		for (UAbilityAction* AbilityAction : ActionArray)
		{
			if (!AbilityAction || AbilityAction->IsPendingKill())
			{
				continue;
			}

			AbilityAction->Cleanup();
		}
	}
	CurrentAbilityActionMap.Empty();
	AbilityActionMap.Empty();

	for (const FAbilityInstanceData& AbilityInstance : *AbilityInstanceDataContainer)
	{
		ProcessInstanceDataRemoved(AbilityInstance);
	}
	AbilityInstanceDataContainer->Empty();
	AbilityInstanceDataContainer.MarkArrayDirty();

	MARK_PROPERTY_DIRTY_FROM_NAME(UAbilityComponent, AbilityInstanceDataContainer, this);
}

void UAbilityComponent::OnActionInterrupt(const UStatusComponent* Component)
{
	InterruptAbilityComponent();
}

UAbilityInfo::UAbilityInfo(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	MoveAndPerformAbilityClass = UActionSequencePerformAbility::StaticClass();
	PerformAbilityClass = UActionPerformAbility::StaticClass();
}

UActionBrainDataObject* UAbilityInfo::CreateMoveActionBrainDataObject(UObject* Outer) const
{
	if (!MoveActionBrainDataObject || !Outer)
	{
		return nullptr;
	}

	return Cast<UActionBrainDataObject>(StaticDuplicateObject(MoveActionBrainDataObject, Outer->GetWorld()));
}

UActionBrainDataObject* UAbilityInfo::CreateTargetActionBrainDataObject(UObject* Outer) const
{
	if (!TargetActionBrainDataObject || !Outer)
	{
		return nullptr;
	}

	return Cast<UActionBrainDataObject>(StaticDuplicateObject(TargetActionBrainDataObject, Outer->GetWorld()));
}

void UAbilityInfo::AbilityInitializedOnComponent(UAbilityComponent* AbilityComponent) const
{
	check(AbilityComponent);
	AbilityComponent->AsyncLoadAbilityObjectClass(GetAbilityDecalClass());
	AbilityComponent->AsyncLoadAbilityObject(GetAbilityCastAnimMontage());
	AbilityComponent->AsyncLoadAbilityObject(GetAbilityActivationAnimMontage());
}

void UAbilityInfo::GenerateTargetData(UAbilityComponent* AbilityComponent, const UActionBrainDataObject* DataObject, TArray<FAbilityTargetData>& TargetDataList) const
{
	GenerateTargetDataFromDataObject(DataObject, TargetDataList);
	GenerateTargetDataFromAbilityComponent(AbilityComponent, TargetDataList);
	TargetDataList.Shrink();
	ProcessTargetData(AbilityComponent, TargetDataList);
}

void UAbilityInfo::GenerateTargetDataFromDataObject(const UActionBrainDataObject* DataObject, TArray<FAbilityTargetData>& TargetDataList) const
{
	if (GetMaxTargetCount() == 0)
	{
		TargetDataList.Empty();
		return;
	}

	TArray<AActor*> ActorList;
	DataObject->GetListOfActors(ActorList);

	if (ActorList.Num() > 0)
	{
		TargetDataList.Reserve(ActorList.Num());
		for (AActor* Actor : ActorList)
		{
			TargetDataList.Add(FAbilityTargetData::GenerateTargetActorData(Actor));
		}
		return;
	}

	TArray<FVector> LocationList;
	DataObject->GetListOfLocations(LocationList);
	
	if (LocationList.Num() > 0)
	{
		TargetDataList.Reserve(LocationList.Num());
		for (const FVector& Location : LocationList)
		{
			TargetDataList.Add(FAbilityTargetData::GenerateTargetLocationData(FTransform(Location)));
		}
	}
	
}

void UAbilityInfo::GenerateTargetDataFromAbilityComponent(UAbilityComponent* AbilityComponent, TArray<FAbilityTargetData>& TargetDataList) const
{

}

void UAbilityInfo::ProcessTargetData(UAbilityComponent* AbilityComponent, TArray<FAbilityTargetData>& TargetDataList) const
{
	if (!GetWorld() || !GetWorld()->GetGameState())
	{
		return;
	}

	if (GetTargetSize() < FVector2D(-1.f) || TargetSizeVariance != FVector2D(1.f) || TargetSizeVariance <= FVector2D(0.f))
	{
		for (FAbilityTargetData& AbilityTargetData : TargetDataList)
		{
			AbilityTargetData.ApplyTargetSizeVariance(TargetSizeVariance);
		}
	}
	
	K2_ProcessTargetData(AbilityComponent, TargetDataList);
}

void UAbilityInfo::OnTargetDataAdded(UAbilityComponent* AbilityComponent, const FAbilityInstanceData& InstanceData, const FAbilityTargetData& InstanceTargetData) const
{
	if (!AbilityComponent)
	{
		return;
	}

	if (AbilityComponent->IsNetMode(NM_DedicatedServer))
	{
		return;
	}

	if (!GetAbilityDecalClass().Get())
	{
		return;
	}

	TScriptInterface<IAbilityObjectInterface> AbilityObjectInterface = UAbilityDecalComponent::SpawnAbilityDecal(AbilityComponent, GetAbilityDecalClass().Get(), InstanceTargetData);
	AbilityComponent->RegisterAbilityObjectInstance(AbilityObjectInterface, InstanceData, InstanceTargetData);
}

void UAbilityInfo::OnTargetDataRemoved(UAbilityComponent* AbilityComponent, const FAbilityInstanceData& InstanceData, const FAbilityTargetData& InstanceTargetData) const
{
	if (!AbilityComponent)
	{
		return;
	}

	if (!AbilityComponent->GetAbilityTargetDataObjectMap().Contains(InstanceTargetData.GetHandle()))
	{
		return;
	}

	FAbilityObjectContainer& AbilityInstanceObjectContainer = AbilityComponent->GetAbilityTargetDataObjectMap()[InstanceTargetData.GetHandle()];
	AbilityInstanceObjectContainer.Cleanup();

	AbilityComponent->GetAbilityTargetDataObjectMap().Remove(InstanceTargetData.GetHandle());
}

void UAbilityInfo::OnAbilityTargetStartup(UAbilityComponent* AbilityComponent, const FAbilityInstanceData& InstanceData, const FAbilityTargetData& InstanceTargetData) const
{
	const ENetRole Role = AbilityComponent->GetOwnerRole();

	for (UAbilityAction* AbilityAction : StartupAbilityActions)
	{
		if (!AbilityAction)
		{
			continue;
		}

		if (Role != ROLE_Authority && !(AbilityAction->ShouldPerformOnAutonomous() && Role == ROLE_AutonomousProxy))
		{
			continue;
		}

		if (!AbilityAction->ShouldInstance())
		{
			AbilityAction->PerformAction(AbilityComponent, InstanceData, InstanceTargetData, EActionStage::Startup);
		}
		else
		{
			UAbilityAction* AbilityActionInstance = Cast<UAbilityAction>(StaticDuplicateObject(AbilityAction, AbilityComponent));
			AbilityActionInstance->InitializeInstance(AbilityComponent, InstanceData, InstanceTargetData, EActionStage::Startup);
		}
	}
}

void UAbilityInfo::OnAbilityTargetActivation(UAbilityComponent* AbilityComponent, const FAbilityInstanceData& InstanceData, const FAbilityTargetData& InstanceTargetData) const
{
	const ENetRole Role = AbilityComponent->GetOwnerRole();
	FAbilityTargetDataHandle TargetHandle = InstanceTargetData.GetHandle();

	{
		const TArray<UAbilityAction*>& ActionList = AbilityComponent->GetTargetDataAbilityActionList(TargetHandle).AbilityList;
		for (UAbilityAction* Action : ActionList)
		{
			if (!Action || Action->GetActionStage() != EActionStage::Startup)
			{
				continue;
			}

			Action->Complete();
		}
	}

	for (UAbilityAction* AbilityAction : ActivationAbilityActions)
	{
		if (!AbilityAction)
		{
			continue;
		}

		if (Role != ROLE_Authority && !(AbilityAction->ShouldPerformOnAutonomous() && Role == ROLE_AutonomousProxy))
		{
			continue;
		}

		if (!AbilityAction->ShouldInstance())
		{
			AbilityAction->PerformAction(AbilityComponent, InstanceData, InstanceTargetData, EActionStage::Activation);
		}
		else
		{
			UAbilityAction* AbilityActionInstance = Cast<UAbilityAction>(StaticDuplicateObject(AbilityAction, AbilityComponent));
			AbilityActionInstance->InitializeInstance(AbilityComponent, InstanceData, InstanceTargetData, EActionStage::Activation);
		}
	}
}

bool UAbilityInfo::CanTargetActor(UAbilityComponent* AbilityComponent, const FAbilityTargetData& AbilityTargetData, AActor* Target, float WorldTimeSeconds) const
{
	if (!AbilityComponent || !Target)
	{
		return false;
	}

	if (NeedsLineOfSight())
	{
		const FVector StartPoint = AbilityTargetData.GetTransform(WorldTimeSeconds).GetLocation();
		const FVector EndPoint = Target->GetActorLocation();

		FHitResult HitResult;
		FCollisionQueryParams CQP = FCollisionQueryParams();
		CQP.AddIgnoredActor(AbilityComponent->GetOwner());
		if (AbilityComponent->GetWorld()->LineTraceSingleByChannel(HitResult, StartPoint, EndPoint, GetLineOfSightCollisionChannel(), CQP) && HitResult.GetActor() != Target)
		{
			return false;
		}
	}

	return true;
}

bool UAbilityInfo::PlayAbilityCast(UAbilityComponent* AbilityComponent, const FAbilityInstanceData& InstanceData, float WorldTime, float CastStartWorldTime) const
{
	ACoreCharacter* OwningCharacter = AbilityComponent->GetOwningCharacter();

	if (!OwningCharacter)
	{
		return false;
	}

	if (UAnimMontage* CastAnimation = GetAbilityCastAnimMontage().Get())
	{
		AbilityComponent->PlayAnimationMontage(CastAnimation, InstanceData.GetHandle(), FMath::Max(WorldTime - CastStartWorldTime, 0.f));
	}

	return true;
}

bool UAbilityInfo::StopPlayAbilityCast(UAbilityComponent* AbilityComponent, const FAbilityInstanceData& InstanceData, float WorldTime) const
{
	ACoreCharacter* OwningCharacter = AbilityComponent->GetOwningCharacter();

	if (!OwningCharacter)
	{
		return false;
	}

	if (UAnimMontage* CastAnimation = GetAbilityCastAnimMontage().Get())
	{
		AbilityComponent->StopAnimationMontage(CastAnimation, InstanceData.GetHandle());
	}

	return true;
}

bool UAbilityInfo::PlayAbilityActivation(UAbilityComponent* AbilityComponent, const FAbilityInstanceData& InstanceData, float WorldTime, float ActivationStartWorldTime) const
{
	ACoreCharacter* OwningCharacter = AbilityComponent->GetOwningCharacter();

	if (!OwningCharacter)
	{
		return false;
	}

	if (ShouldStopCastMontageOnActivation())
	{
		StopPlayAbilityCast(AbilityComponent, InstanceData, WorldTime);
	}

	if (UAnimMontage* ActivationAnimation = GetAbilityActivationAnimMontage().Get())
	{
		const float StartTime = FMath::Max(WorldTime - ActivationStartWorldTime, 0.f);
		AbilityComponent->PlayAnimationMontage(ActivationAnimation, InstanceData.GetHandle(), FMath::Max(WorldTime - ActivationStartWorldTime, 0.f));
	}

	return true;
}

bool UAbilityInfo::StopPlayAbilityActivation(UAbilityComponent* AbilityComponent, const FAbilityInstanceData& InstanceData, float WorldTime) const
{
	ACoreCharacter* OwningCharacter = AbilityComponent->GetOwningCharacter();

	if (!OwningCharacter)
	{
		return false;
	}

	if (UAnimMontage* ActivationAnimation = GetAbilityActivationAnimMontage().Get())
	{
		AbilityComponent->StopAnimationMontage(ActivationAnimation, InstanceData.GetHandle());
	}

	return true;
}

const FVector2D& UAbilityInfo::GetAbilityTargetDataStartupTime(const FAbilityInstanceData& InstanceData, const FAbilityTargetData& TargetData)
{
	const FVector2D& ActivationTime = TargetData.GetStartupTime();
	if (ActivationTime != FVector2D(-1.f))
	{
		return ActivationTime;
	}

	return InstanceData.GetStartupTime();
}

const FVector2D& UAbilityInfo::GetAbilityTargetDataActivationTime(const FAbilityInstanceData& InstanceData, const FAbilityTargetData& TargetData)
{
	const FVector2D& ActivationTime = TargetData.GetActivationTime();
	if (ActivationTime != FVector2D(-1.f))
	{
		return ActivationTime;
	}

	return InstanceData.GetActivationTime();
}

const FVector2D& UAbilityInfo::GetAbilityTargetDataTargetSize(const FAbilityInstanceData& InstanceData, const FAbilityTargetData& TargetData)
{
	const FVector2D& TargetSize = TargetData.GetTargetSize();
	if (TargetSize != FVector2D(-1.f) || !InstanceData.GetClass())
	{
		return TargetSize;
	}

	const UAbilityInfo* AbilityInfoCDO = InstanceData.GetClassCDO();

	if (!AbilityInfoCDO)
	{
		return TargetSize;
	}

	return AbilityInfoCDO->GetTargetSize();
}

FVector2D UAbilityInfo::GetAdjustedAbilityTargetDataStartupTime(const UObject* WorldContextObject, const FAbilityInstanceData& InstanceData, const FAbilityTargetData& TargetData)
{
	if (!WorldContextObject)
	{
		return FVector2D(-1.f);
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	if (!World)
	{
		return FVector2D(-1.f);
	}

	const float ServerTime = World->GetGameState()->GetServerWorldTimeSeconds();
	const float WorldTime = World->GetTimeSeconds();
	const float TimeAdjustment = WorldTime - ServerTime;
	const FVector2D& StartupTime = UAbilityInfo::GetAbilityTargetDataStartupTime(InstanceData, TargetData);

	return FVector2D(StartupTime.X + TimeAdjustment, StartupTime.Y + TimeAdjustment);
}

FVector2D UAbilityInfo::GetAdjustedAbilityTargetDataActivationTime(const UObject* WorldContextObject, const FAbilityInstanceData& InstanceData, const FAbilityTargetData& TargetData)
{
	if (!WorldContextObject)
	{
		return FVector2D(-1.f);
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	if (!World)
	{
		return FVector2D(-1.f);
	}

	const float ServerTime = World->GetGameState()->GetServerWorldTimeSeconds();
	const float WorldTime = World->GetTimeSeconds();
	const float TimeAdjustment = WorldTime - ServerTime;
	const FVector2D& ActivationTime = UAbilityInfo::GetAbilityTargetDataActivationTime(InstanceData, TargetData);

	return FVector2D(ActivationTime.X + TimeAdjustment, ActivationTime.Y + TimeAdjustment);
}

FVector2D UAbilityInfo::GetAdjustedAbilityTargetDataTargetSize(const UObject* WorldContextObject, const FAbilityInstanceData& InstanceData, const FAbilityTargetData& TargetData)
{
	if (!WorldContextObject)
	{
		return FVector2D(-1.f);
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	if (!World)
	{
		return FVector2D(-1.f);
	}

	const float ServerTime = World->GetGameState()->GetServerWorldTimeSeconds();
	const float WorldTime = World->GetTimeSeconds();
	const float TimeAdjustment = WorldTime - ServerTime;
	const FVector2D& TargetSize = UAbilityInfo::GetAbilityTargetDataTargetSize(InstanceData, TargetData);

	return FVector2D(TargetSize.X + TimeAdjustment, TargetSize.Y + TimeAdjustment);
}