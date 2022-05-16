#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "Engine/NetSerialization.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/Actor.h"
#include "NauseaNetDefines.h"
#include "AbilityTypes.generated.h"

class UAbilityInfo;
class UAbilityComponent;

//Contains data about a given ability. Cast and recharge timings as well as charge count is stored here.
USTRUCT(BlueprintType)
struct FAbilityData
{
	GENERATED_USTRUCT_BODY()

	FAbilityData() {}
	
	FAbilityData(UClass* InAbilityClass, float WorldTimeSeconds);

public:
	FORCEINLINE bool IsValid() const { return AbilityClass != nullptr; }
	FORCEINLINE UClass* GetClass() const { return AbilityClass; }
	const UAbilityInfo* GetClassCDO() const;

	FORCEINLINE int32 HasCharge() const { return ChargeCount == -1 || ChargeCount != 0; }
	FORCEINLINE int32 GetChargeCount() const { return ChargeCount; }
	int32 IncrementChargeCount() { return ++ChargeCount; }

	bool ConsumeCharge()
	{
		if (ChargeCount == INDEX_NONE)
		{
			return true;
		}

		if (ChargeCount != 0) 
		{
			--ChargeCount;
			return true;
		} 
		
		return false;
	}

	FORCEINLINE const FVector2D& GetRechargeTime() const { return RechargeTime; }
	void SetRechargeTime(const FVector2D& InRechargeTime) { RechargeTime = InRechargeTime; }

	FORCEINLINE FTimerHandle& GetRechargeTimerHandle() { return RechargeHandle; }

	FORCEINLINE bool operator== (const FAbilityData& Other) const
	{
		return AbilityClass == Other.AbilityClass && ChargeCount == Other.ChargeCount && RechargeTime == Other.RechargeTime;
	}

	FORCEINLINE bool operator== (const UClass* InClass) const
	{
		return this->AbilityClass == InClass;
	}

protected:
	UPROPERTY()
	UClass* AbilityClass = nullptr;

	UPROPERTY()
	int32 ChargeCount = INDEX_NONE;
	
	UPROPERTY()
	FVector2D RechargeTime = FVector2D(-1.f);

	UPROPERTY(NotReplicated)
	FTimerHandle RechargeHandle;

public:
	static FAbilityData InvalidAbilityData;
};

USTRUCT()
struct FAbilityTargetDataHandle
{
	GENERATED_USTRUCT_BODY()

public:
	FAbilityTargetDataHandle() {}

	FORCEINLINE bool operator== (FAbilityTargetDataHandle InData) const { return Handle == InData.Handle; }
	FORCEINLINE bool operator== (uint64 InID) const { return this->Handle == InID; }
	friend FArchive& operator<<(FArchive& Ar, FAbilityTargetDataHandle& AbilityTargetDataHandle) { Ar << AbilityTargetDataHandle.Handle; return Ar; }

	bool IsValid() const { return Handle != MAX_uint64; }

	static FAbilityTargetDataHandle GenerateHandle()
	{
		if (++FAbilityTargetDataHandle::HandleIDCounter == MAX_uint64)
		{
			FAbilityTargetDataHandle::HandleIDCounter++;
		}

		return FAbilityTargetDataHandle(FAbilityTargetDataHandle::HandleIDCounter);
	}

	FORCEINLINE friend uint64 GetTypeHash(FAbilityTargetDataHandle Other)
	{
		return GetTypeHash(Other.Handle);
	}

protected:
	FAbilityTargetDataHandle(int64 InHandle) { Handle = InHandle; }

	UPROPERTY()
	uint64 Handle = MAX_uint64;

	static uint64 HandleIDCounter;
};

UENUM(BlueprintType)
enum class ETargetDataType : uint8
{
	Actor,
	ActorRelativeTransform,
	Transform,
	MovingTransform,
	Invalid
};

USTRUCT(BlueprintType)
struct FAbilityTargetData
{
	GENERATED_USTRUCT_BODY()

	FAbilityTargetData() {}

	static FAbilityTargetData GenerateTargetActorData(AActor* InActor, const FTransform& InTransform = FTransform::Identity);
	static FAbilityTargetData GenerateTargetLocationData(const FTransform& InTransform);
	static FAbilityTargetData GenerateTargetMovingLocationData(const FTransform& InTransform, const FTransform& InDestinationTransform, const FVector2D& InMoveTime);

	//Used typically by clients when they want to create a cached standalone FAbilityTargetData without the need to also cache a FAbilityInstanceData.
	FORCEINLINE FAbilityTargetData& PullAbilityInstanceData(const FAbilityInstanceData& InData);

	FORCEINLINE FAbilityTargetData& SetTargetSize(const FVector2D& InTargetSize) { TargetSize = InTargetSize; return *this; }
	FORCEINLINE FAbilityTargetData& SetStartupTime(const FVector2D& InStartupTime) { StartupTime = InStartupTime; return *this; }
	FORCEINLINE FAbilityTargetData& SetActivationTime(const FVector2D& InActivationTime) { ActivationTime = InActivationTime; return *this; }

	FORCEINLINE bool IsValid() const { return TargetDataHandle.IsValid() && TargetDataType != ETargetDataType::Invalid; }
	FORCEINLINE FAbilityTargetDataHandle GetHandle() const { return TargetDataHandle; }

	FORCEINLINE const FVector2D& GetStartupTime() const { return StartupTime; }
	FORCEINLINE const FVector2D& GetActivationTime() const { return ActivationTime; }

	FORCEINLINE const FVector2D& GetTargetSize() const { return TargetSize; }
	FORCEINLINE void ApplyTargetSizeVariance(const FVector2D& Variance) { TargetSize *= Variance; }

	FORCEINLINE AActor* GetTargetActor() const { return TargetActor.Get(); }

	FORCEINLINE FTransform GetTransform(const float WorldTimeSeconds = -1.f) const
	{
		switch (GetTargetType())
		{
		case ETargetDataType::Actor:
			return GetTargetActor() ? GetTargetActor()->GetActorTransform() : FTransform::Identity;
		case ETargetDataType::ActorRelativeTransform:
			return GetTargetActor() ? GetTargetActor()->GetActorTransform().GetRelativeTransformReverse(TargetTransform) : FTransform::Identity;
		case ETargetDataType::Transform:
			return TargetTransform;
		case ETargetDataType::MovingTransform:

			const float Alpha = FMath::Clamp((WorldTimeSeconds - MoveTime.X) / (MoveTime.Y - MoveTime.X), 0.f, 1.f);
			return UKismetMathLibrary::TLerp(TargetTransform, DestinationTargetTransform, Alpha);
		}
		//We can't really handle ETargetDataType::MovingTransform here since that requires a worldtime.
		ensure(false);
		return FTransform::Identity;
	}

	FORCEINLINE const FTransform& GetTargetTransform() const { return TargetTransform; }
	FORCEINLINE const FTransform& GetDestinationTargetTransform() const { return DestinationTargetTransform; }
	FORCEINLINE const FVector2D& GetMoveTime() const { return MoveTime; }

	FORCEINLINE ETargetDataType GetTargetType() const { return TargetDataType; }

	FORCEINLINE bool MarkStale() { return bMarkedStale = true; }
	FORCEINLINE bool IsStale() const { return bMarkedStale; }

	static FAbilityTargetData InvalidTargetData;

protected:
	UPROPERTY()
	ETargetDataType TargetDataType = ETargetDataType::Invalid;

	UPROPERTY()
	FVector2D StartupTime = FVector2D(-1.f);
	UPROPERTY()
	FVector2D ActivationTime = FVector2D(-1.f);

	UPROPERTY()
	FVector2D TargetSize = FVector2D(-1.f);
	
	UPROPERTY()
	TWeakObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY()
	FTransform TargetTransform = FTransform::Identity;

	UPROPERTY()
	FTransform DestinationTargetTransform = FTransform::Identity;
	UPROPERTY()
	FVector2D MoveTime = FVector2D(-1.f);

	UPROPERTY()
	FAbilityTargetDataHandle TargetDataHandle;

	//Target data is marked stale once the it has completed activation and is no longer blocking the owning FAbilityInstance's cleanup.
	UPROPERTY(NotReplicated)
	bool bMarkedStale = false;

public:
	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits< FAbilityTargetData > : public TStructOpsTypeTraitsBase2< FAbilityTargetData >
{
	enum
	{
		WithNetSerializer = true,
	};
};

USTRUCT(BlueprintType)
struct FAbilityTargetDataContainer
{
	GENERATED_USTRUCT_BODY()

	FAbilityTargetDataContainer() {}

	FAbilityTargetDataContainer(const TArray<FAbilityTargetData>& InTargetDataList)
	{
		TargetDataList = InTargetDataList;
	}

	FORCEINLINE TArray<FAbilityTargetData>& GetTargetDataList() { return TargetDataList; }
	FORCEINLINE const TArray<FAbilityTargetData>& GetTargetDataList() const { return TargetDataList; }

	void UpdateTargetDataCache(TArray<FAbilityTargetData>& AddedTargetData, TArray<FAbilityTargetData>& RemovedTargetData) const;

protected:
	UPROPERTY()
	TArray<FAbilityTargetData> TargetDataList;
	UPROPERTY(NotReplicated)
	TMap<FAbilityTargetDataHandle, FAbilityTargetData> TargetDataListCache;
};

USTRUCT()
struct FAbilityInstanceHandle
{
	GENERATED_USTRUCT_BODY()

public:
	FAbilityInstanceHandle()
	{
		Handle = MAX_uint64;
	}

	FORCEINLINE bool operator== (const FAbilityInstanceHandle& InData) const { return Handle == InData.Handle; }
	FORCEINLINE bool operator!= (const FAbilityInstanceHandle& InData) const { return Handle != InData.Handle; }
	FORCEINLINE bool operator== (uint64 InID) const { return this->Handle == InID; }
	FORCEINLINE bool operator!= (uint64 InID) const { return this->Handle != InID; }

	bool IsValid() const { return Handle != MAX_uint64; }

	static FAbilityInstanceHandle GenerateHandle()
	{
		if (++FAbilityInstanceHandle::HandleIDCounter == MAX_uint64)
		{
			FAbilityInstanceHandle::HandleIDCounter++;
		}

		return FAbilityInstanceHandle(FAbilityInstanceHandle::HandleIDCounter);
	}

	FORCEINLINE friend uint32 GetTypeHash(FAbilityInstanceHandle Other)
	{
		return GetTypeHash(Other.Handle);
	}

	friend FArchive& operator<<(FArchive& Ar, FAbilityInstanceHandle& AbilityInstanceHandle)
	{
		Ar << AbilityInstanceHandle.Handle;
		return Ar;
	}

protected:
	FAbilityInstanceHandle(uint64 InHandle) { Handle = InHandle; }

	UPROPERTY()
	uint64 Handle = MAX_uint64;

	static uint64 HandleIDCounter;
};

USTRUCT(BlueprintType)
struct FAbilityInstanceData : public FFastArraySerializerItem
{
	GENERATED_USTRUCT_BODY()

	FAbilityInstanceData() {}

protected:
	FAbilityInstanceData(UClass* InAbilityClass, const TArray<FAbilityTargetData>& InTargetData)
	{
		AbilityClass = InAbilityClass;
		TargetData = FAbilityTargetDataContainer(InTargetData);
	}

public:
	FORCEINLINE FAbilityInstanceData operator= (const FAbilityInstanceData& InData)
	{
		return FAbilityInstanceData();
	}

	FORCEINLINE bool operator== (const FAbilityInstanceData& InData) const { return InstanceHandle == InData.InstanceHandle; }
	FORCEINLINE bool operator== (uint64 InID) const { return InstanceHandle == InID; }

	FORCEINLINE bool IsValid() const { return InstanceHandle.IsValid() && AbilityClass != nullptr; }
	FORCEINLINE FAbilityInstanceHandle GetHandle() const { return InstanceHandle; }
	FORCEINLINE UClass* GetClass() const { return AbilityClass; }
	const UAbilityInfo* GetClassCDO() const;

	FORCEINLINE const FVector2D& GetStartupTime() const { return StartupTime; }
	FORCEINLINE const FVector2D& GetActivationTime() const { return ActivationTime; }

	FORCEINLINE const FAbilityTargetDataContainer& GetTargetData() const { return TargetData; }
	FORCEINLINE FAbilityTargetDataContainer& GetTargetData() { return TargetData; }

	static FAbilityInstanceData GenerateInstanceData(UClass* InAbilityClass, const TArray<FAbilityTargetData>& InTargetData, FAbilityInstanceHandle* OutHandle = nullptr);
	bool InitializeAbilityInstance(UAbilityComponent* OwningAbilityComponent, FAbilityData& AbilityData);

	void MarkStartupComplete() { bStartupCompleted = true; }
	bool IsStartupComplete() const { return bStartupCompleted; }

protected:
	UPROPERTY()
	FAbilityInstanceHandle InstanceHandle;

	UPROPERTY()
	UClass* AbilityClass = nullptr;

	UPROPERTY()
	FVector2D StartupTime = FVector2D(-1.f);

	UPROPERTY()
	FVector2D ActivationTime = FVector2D(-1.f);

	UPROPERTY()
	FAbilityTargetDataContainer TargetData;

	//Marks this ability as "started" and no longer affected by interrupts.
	UPROPERTY(NotReplicated)
	bool bStartupCompleted = false;
};

USTRUCT(BlueprintType)
struct FAbilityInstanceContainer : public FFastArraySerializer
{
	GENERATED_USTRUCT_BODY()

	FAST_ARRAY_SERIALIZER_OPERATORS(FAbilityInstanceData, InstanceList);

public:
	FAbilityInstanceContainer() {}

	FORCEINLINE void SetOwningAbilityComponent(UAbilityComponent* InOwningAbilityComponent) { OwningAbilityComponent = InOwningAbilityComponent; }

private:
	UPROPERTY()
	TArray<FAbilityInstanceData> InstanceList;
		
	UPROPERTY(NotReplicated)
	UAbilityComponent* OwningAbilityComponent = nullptr;

public:
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FAbilityInstanceData, FAbilityInstanceContainer>(InstanceList, DeltaParms, *this);
	}

	void PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize);
	void PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize);
};

template<>
struct TStructOpsTypeTraits< FAbilityInstanceContainer > : public TStructOpsTypeTraitsBase2< FAbilityInstanceContainer >
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};

UENUM(BlueprintType)
enum class ECompleteCondition : uint8
{
	CastComplete,
	ActivationComplete
};