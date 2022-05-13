// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#include "Gameplay/Ability/AbilityTypes.h"
#include "Gameplay/AbilityComponent.h"
#include "GameFramework/GameState.h"

FAbilityData FAbilityData::InvalidAbilityData = FAbilityData();
FAbilityTargetData FAbilityTargetData::InvalidTargetData = FAbilityTargetData();

uint64 FAbilityInstanceHandle::HandleIDCounter = MAX_uint64;
uint64 FAbilityTargetDataHandle::HandleIDCounter = MAX_uint64;

FAbilityData::FAbilityData(UClass* InAbilityClass, float WorldTimeSeconds)
{
	if (!InAbilityClass)
	{
		return;
	}

	const UAbilityInfo* AbilityInfoCDO = InAbilityClass->GetDefaultObject<UAbilityInfo>();

	if (!AbilityInfoCDO)
	{
		return;
	}

	AbilityClass = InAbilityClass;

	const int32 CDOCharges = AbilityInfoCDO->GetInitialChargeCount();
	ChargeCount = CDOCharges > 0 ? CDOCharges : -1;

	if (ChargeCount != -1 && AbilityInfoCDO->GetMaxChargeCount() > ChargeCount)
	{
		RechargeTime = FVector2D(WorldTimeSeconds, WorldTimeSeconds + AbilityInfoCDO->GetRechargeDuration());
	}
}

const UAbilityInfo* FAbilityData::GetClassCDO() const
{
	return AbilityClass ? AbilityClass->GetDefaultObject<UAbilityInfo>() : nullptr;
}

FAbilityTargetData FAbilityTargetData::GenerateTargetActorData(AActor* InActor, const FTransform& InRelativeTransform)
{
	if (!InActor)
	{
		return FAbilityTargetData();
	}
	
	FAbilityTargetData TargetData;
	TargetData.TargetDataHandle = FAbilityTargetDataHandle::GenerateHandle();

	if (InRelativeTransform.Equals(FTransform::Identity))
	{
		TargetData.TargetDataType = ETargetDataType::Actor;
	}
	else
	{
		TargetData.TargetDataType = ETargetDataType::ActorRelativeTransform;
		TargetData.TargetTransform = InRelativeTransform;
	}
	
	TargetData.TargetActor = InActor;
	TargetData.TargetTransform = InRelativeTransform;
	return TargetData;
}

FAbilityTargetData FAbilityTargetData::GenerateTargetLocationData(const FTransform& InTransform)
{
	FAbilityTargetData TargetData;
	TargetData.TargetDataHandle = FAbilityTargetDataHandle::GenerateHandle();

	TargetData.TargetDataType = ETargetDataType::Transform;

	TargetData.TargetTransform = InTransform;
	return TargetData;
}

FAbilityTargetData FAbilityTargetData::GenerateTargetMovingLocationData(const FTransform& InTransform, const FTransform& InDestinationTransform, const FVector2D& InMoveTime)
{
	FAbilityTargetData TargetData = GenerateTargetLocationData(InTransform);
	TargetData.TargetDataType = ETargetDataType::MovingTransform;
	TargetData.DestinationTargetTransform = InDestinationTransform;
	TargetData.MoveTime = InMoveTime;
	return TargetData;
}

FAbilityTargetData& FAbilityTargetData::PullAbilityInstanceData(const FAbilityInstanceData& InData)
{
	if (StartupTime != FVector2D(-1.f))
	{
		StartupTime = InData.GetStartupTime();
	}

	if (ActivationTime != FVector2D(-1.f))
	{
		ActivationTime = InData.GetActivationTime();
	}

	if (TargetSize == FVector2D(-1.f) && InData.GetClass())
	{
		TargetSize = InData.GetClassCDO()->GetTargetSize();
	}

	return *this;
}

bool FAbilityTargetData::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	Ar << TargetDataHandle;
	Ar << TargetDataType;

	switch (TargetDataType)
	{
	case ETargetDataType::Actor:
		Ar << TargetActor;
		break;
	case ETargetDataType::ActorRelativeTransform:
		Ar << TargetActor;
		Ar << TargetTransform;
		break;
	case ETargetDataType::Transform:
		Ar << TargetTransform;
		break;
	case ETargetDataType::MovingTransform:
		Ar << TargetTransform;
		Ar << DestinationTargetTransform;
		Ar << MoveTime;
		break;
	default:
		check(false);
		break;
	}

	const bool bIsSaving = Ar.IsSaving();
	SerializeOptionalValue<FVector2D>(bIsSaving, Ar, StartupTime, FVector2D(-1.f));
	SerializeOptionalValue<FVector2D>(bIsSaving, Ar, ActivationTime, FVector2D(-1.f));
	SerializeOptionalValue<FVector2D>(bIsSaving, Ar, TargetSize, FVector2D(-1.f));

	return true;
}

void FAbilityTargetDataContainer::UpdateTargetDataCache(TArray<FAbilityTargetData>& AddedTargetData, TArray<FAbilityTargetData>& RemovedTargetData) const
{
	TMap<FAbilityTargetDataHandle, FAbilityTargetData> NewTargetDataListCache;
	NewTargetDataListCache.Reserve(TargetDataList.Num());
	for (const FAbilityTargetData& TargetData : TargetDataList)
	{
		NewTargetDataListCache.Add(TargetData.GetHandle()) = TargetData;
	}

	if (TargetDataListCache.Num() == 0)
	{
		AddedTargetData = TargetDataList;
	}
	else
	{
		AddedTargetData.Reserve(NewTargetDataListCache.Num());
		for (const TPair<FAbilityTargetDataHandle, FAbilityTargetData>& NewEntry : NewTargetDataListCache)
		{
			if (!TargetDataListCache.Contains(NewEntry.Key))
			{
				AddedTargetData.Add(NewEntry.Value);
			}
		}
		AddedTargetData.Shrink();

		RemovedTargetData.Reserve(TargetDataListCache.Num());
		for (const TPair<FAbilityTargetDataHandle, FAbilityTargetData>& CachedEntry : TargetDataListCache)
		{
			if (!NewTargetDataListCache.Contains(CachedEntry.Key))
			{
				RemovedTargetData.Add(CachedEntry.Value);
			}
		}
		RemovedTargetData.Shrink();
	}

	const_cast<FAbilityTargetDataContainer*>(this)->TargetDataListCache = NewTargetDataListCache;
}

const UAbilityInfo* FAbilityInstanceData::GetClassCDO() const
{
	return AbilityClass ? AbilityClass->GetDefaultObject<UAbilityInfo>() : nullptr;
}

FAbilityInstanceData FAbilityInstanceData::GenerateInstanceData(UClass* InAbilityClass, const TArray<FAbilityTargetData>& InTargetData, FAbilityInstanceHandle* OutHandle)
{
	FAbilityInstanceData AbilityInstanceData = FAbilityInstanceData(InAbilityClass, InTargetData);

	AbilityInstanceData.InstanceHandle = FAbilityInstanceHandle::GenerateHandle();

	if (OutHandle)
	{
		*OutHandle = AbilityInstanceData.InstanceHandle;
	}

	return AbilityInstanceData;
}

bool FAbilityInstanceData::InitializeAbilityInstance(UAbilityComponent* OwningAbilityComponent, FAbilityData& AbilityData)
{
	if (!OwningAbilityComponent || !AbilityClass || !OwningAbilityComponent->GetWorld() || !OwningAbilityComponent->GetWorld()->GetGameState())
	{
		return false;
	}

	const UAbilityInfo* AbilityInfoCDO = GetClassCDO();

	if (!AbilityInfoCDO)
	{
		return false;
	}

	const float WorldTimeSeconds = OwningAbilityComponent->GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
	const float CastDuration = AbilityInfoCDO->GetCastDuration();
	if (CastDuration > 0.f)
	{
		StartupTime = FVector2D(WorldTimeSeconds, WorldTimeSeconds + CastDuration);
	}

	const float ActivationDuration = AbilityInfoCDO->GetAbilityDuration();
	if (ActivationDuration > 0.f)
	{
		const float ActivationStartTime = CastDuration > 0.f ? StartupTime.Y : WorldTimeSeconds; //If CastDuration is 0, then we default to current time as activation start.
		ActivationTime = FVector2D(ActivationStartTime, ActivationStartTime + ActivationDuration);
	}

	return true;
}

void FAbilityInstanceContainer::PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize)
{
	if (!OwningAbilityComponent)
	{
		return;
	}

	for (const int32& Index : AddedIndices)
	{
		OwningAbilityComponent->ProcessInstanceDataAdded(InstanceList[Index]);
	}
}

void FAbilityInstanceContainer::PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize)
{
	if (!OwningAbilityComponent)
	{
		return;
	}

	for (const int32& Index : RemovedIndices)
	{
		OwningAbilityComponent->ProcessInstanceDataRemoved(InstanceList[Index]);
	}
}

void FAbilityInstanceContainer::PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize)
{
	if (!OwningAbilityComponent)
	{
		return;
	}

	for (const int32& Index : ChangedIndices)
	{
		OwningAbilityComponent->ProcessInstanceDataChanged(InstanceList[Index]);
	}
}