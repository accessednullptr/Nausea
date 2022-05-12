#pragma once

#include "GenericTeamAgentInterface.h"
#include "AITypes.h"
#include "StatusType.generated.h"

UENUM(BlueprintType)
enum class EStatusType : uint8
{
	Invalid,
	Slow,
	Stun,
	Flinch,
	Burn,
	Poison,
	NonGeneric,
	MAX
};

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EDamageHitDescriptor : uint8
{
	None = 0 UMETA(Hidden),
	Ballistic = 1 << 0,
	Slashing = 1 << 1,
	Piercing = 1 << 2,
	Blunt = 1 << 3,
	ExplosiveImpact = 1 << 4
};
ENUM_CLASS_FLAGS(EDamageHitDescriptor);

UENUM(BlueprintType, meta = (Bitflags, UseEnumValuesAsMaskValuesInEditor = "true"))
enum class EDamageElementalDescriptor : uint8
{
	None = 0 UMETA(Hidden),
	Fire = 1 << 1,
	Electric = 1 << 1,
	Water = 1 << 2,
	Ice = 1 << 3,
	Explosive = 1 << 4
};
ENUM_CLASS_FLAGS(EDamageElementalDescriptor);

UENUM(BlueprintType)
enum class EStatusBeginType : uint8
{
	Invalid,
	Initial,
	Refresh
};

UENUM(BlueprintType)
enum class EStatusEndType : uint8
{
	Invalid,
	Expired,
	Interrupted
};

UENUM(BlueprintType)
enum class EDamageEventType : uint8
{
	Event,
	PointEvent,
	RadialEvent,
	CoreEvent,
	CorePointEvent,
	CoreRadialEvent
};

USTRUCT(BlueprintType)
struct FDamageLogEventModifier
{
	GENERATED_USTRUCT_BODY()

	FDamageLogEventModifier() {}

	FDamageLogEventModifier(UObject* InDataObject, UObject* InInstigatorObject, float InModifier)
	{
		DataObject = TWeakObjectPtr<UObject>(InDataObject);
		InstigatorObject = TWeakObjectPtr<UObject>(InInstigatorObject);
		Modifier = InModifier;
	}

	UPROPERTY()
	TWeakObjectPtr<UObject> DataObject;
	UPROPERTY()
	TWeakObjectPtr<UObject> InstigatorObject;
	UPROPERTY()
	float Modifier = 1.f;
};

class AController;
class UCoreDamageType;

USTRUCT(BlueprintType)
struct FDamageLogEvent
{
	GENERATED_USTRUCT_BODY()

	FDamageLogEvent() {}

	FDamageLogEvent(float InTimeStamp, float InDamageInitial, UObject* InInstigator, const FGenericTeamId& InInstigatorTeam,
		UObject* InInstigatorWeaponClass, UObject* InInstigatorFireMode, TSubclassOf<UCoreDamageType> InInstigatorDamageType)
	{
		TimeStamp = InTimeStamp;
		DamageInitial = InDamageInitial;

		Instigator = InInstigator;
		InstigatorWeaponClass = InInstigatorWeaponClass;
		InstigatorFireModeClass = InInstigatorFireMode;
		InstigatorDamageType = InInstigatorDamageType;

		InstigatorTeam = InInstigatorTeam;
		ModifierList = TArray<FDamageLogEventModifier>();
	}

public:
	bool PushModifier(FDamageLogEventModifier&& Modifier) { ModifierList.Add(MoveTemp(Modifier)); return true; }

	void FinalizeDamageLog(float InDamageDealt) { DamageDealt = InDamageDealt; }

	inline void CombineWithEvent(const FDamageLogEvent& Other)
	{
		TimeStamp = FMath::Max(TimeStamp, Other.TimeStamp);
		DamageDealt += Other.DamageDealt;
	}

	FORCEINLINE float GetTimeStamp() const { return TimeStamp; }
	FORCEINLINE float GetTimeSince(float Time) const { return Time - TimeStamp; }

	//Has the time this event can be in the buffer for expired?
	FORCEINLINE float IsExpired(float Time) const { return (Time - TimeStamp) > MaxMergeDeltaTime; }

	static constexpr float MaxMergeDeltaTime = 1.f;

	FORCEINLINE void MarkDeathEvent() { bDeathEvent = true; }
	FORCEINLINE bool IsDeathEvent() const { return bDeathEvent; }

public:
	UPROPERTY()
	float DamageInitial = -1.f;
	UPROPERTY()
	float DamageDealt = -1.f;

	UPROPERTY()
	float TimeStamp = -1.f;

	UPROPERTY()
	TWeakObjectPtr<UObject> Instigator = nullptr;
	UPROPERTY()
	TWeakObjectPtr<UObject> InstigatorWeaponClass = nullptr;
	UPROPERTY()
	TWeakObjectPtr<UObject> InstigatorFireModeClass = nullptr;
	UPROPERTY()
	TSubclassOf<UCoreDamageType> InstigatorDamageType = nullptr;
	UPROPERTY()
	FGenericTeamId InstigatorTeam = FGenericTeamId::NoTeam;

	UPROPERTY()
	TArray<FDamageLogEventModifier> ModifierList = TArray<FDamageLogEventModifier>();

	UPROPERTY()
	bool bDeathEvent = false;
};

USTRUCT(BlueprintType)
struct FDamageLogStack
{
	GENERATED_USTRUCT_BODY()

	FDamageLogStack() {}

public:
	inline FDamageLogEvent& Push(FDamageLogEvent&& Entry) { Stack.Add(MoveTemp(Entry)); return Stack.Last(); }
	inline bool PushEntryModifier(FDamageLogEventModifier&& EntryModifier) { return Stack.Num() != 0 ? Stack.Last().PushModifier(MoveTemp(EntryModifier)) : false; }
	inline bool MarkDeathEvent() { if (Stack.Num() != 0) { Stack.Last().MarkDeathEvent(); return true; } return false; }
	inline FDamageLogEvent Pop() { return Stack.Pop(false); }

protected:
	UPROPERTY()
	TArray<FDamageLogEvent> Stack = TArray<FDamageLogEvent>();
};

class UWeapon;
class UFireMode;
class UDamageType;

USTRUCT()
struct NAUSEA_API FCoreDamageEvent : public FDamageEvent
{
	GENERATED_USTRUCT_BODY()

	FCoreDamageEvent() {}

	FCoreDamageEvent(FDamageEvent const& InDamageEvent)
		: FDamageEvent(InDamageEvent)
	{}

	explicit FCoreDamageEvent(TSubclassOf<UDamageType> InDamageTypeClass)
		: FDamageEvent(InDamageTypeClass)
	{}

	UPROPERTY()
	TSubclassOf<UWeapon> WeaponClass = nullptr;

	UPROPERTY()
	TSubclassOf<UFireMode> FireModeClass = nullptr;

	static const int32 ClassID = 3;
	
	virtual int32 GetTypeID() const override { return FCoreDamageEvent::ClassID; };
	virtual bool IsOfType(int32 InID) const override { return (FCoreDamageEvent::ClassID == InID) || FDamageEvent::IsOfType(InID); };
};

USTRUCT()
struct NAUSEA_API FCorePointDamageEvent : public FPointDamageEvent
{
	GENERATED_USTRUCT_BODY()

	FCorePointDamageEvent() : FPointDamageEvent() {}
	FCorePointDamageEvent(float InDamage, struct FHitResult const& InHitInfo, FVector const& InShotDirection, TSubclassOf<class UDamageType> InDamageTypeClass)
		: FPointDamageEvent(InDamage, InHitInfo, InShotDirection, InDamageTypeClass)
	{}

	UPROPERTY()
	TSubclassOf<UWeapon> WeaponClass = nullptr;

	UPROPERTY()
	TSubclassOf<UFireMode> FireModeClass = nullptr;

	static const int32 ClassID = 4;
	
	virtual int32 GetTypeID() const override { return FCorePointDamageEvent::ClassID; };
	virtual bool IsOfType(int32 InID) const override { return (FCorePointDamageEvent::ClassID == InID) || FPointDamageEvent::IsOfType(InID); };
};

USTRUCT()
struct NAUSEA_API FCoreRadialDamageEvent : public FRadialDamageEvent
{
	GENERATED_USTRUCT_BODY()

	FCoreRadialDamageEvent() : FRadialDamageEvent() {}

	UPROPERTY()
	TSubclassOf<UWeapon> WeaponClass = nullptr;

	UPROPERTY()
	TSubclassOf<UFireMode> FireModeClass = nullptr;

	static const int32 ClassID = 5;
	
	virtual int32 GetTypeID() const override { return FCoreRadialDamageEvent::ClassID; };
	virtual bool IsOfType(int32 InID) const override { return (FCoreRadialDamageEvent::ClassID == InID) || FRadialDamageEvent::IsOfType(InID); };
};

UENUM(BlueprintType)
enum class EHitReactionDirection : uint8
{
	Invalid,
	Front,
	Back,
	Left,
	Right,
	MAX
};

UENUM(BlueprintType)
enum class EHitReactionStrength : uint8
{
	Invalid,
	Light,
	Medium,
	Heavy,
	MAX
};

USTRUCT(BlueprintType)
struct FHitEvent : public FFastArraySerializerItem
{
	GENERATED_USTRUCT_BODY()

	FHitEvent() {}

	FHitEvent(const TSubclassOf<UCoreDamageType>& InDamageType, float InDamage, const FVector& InHitDirection, const FVector& InHitLocation, float InWorldTime, const FRandomStream& InRandom)
		: DamageType(InDamageType), Damage(InDamage), HitDirection(InHitDirection), HitLocation(InHitLocation), WorldTime(InWorldTime), Random(InRandom)
	{
		IDCounter++;
		ID = IDCounter;
	}

public:
	UPROPERTY()
	TSubclassOf<UCoreDamageType> DamageType = nullptr;
	UPROPERTY()
	float Damage = 0.f;
	UPROPERTY()
	FVector_NetQuantizeNormal HitDirection = FAISystem::InvalidDirection;
	UPROPERTY()
	FVector_NetQuantize HitLocation = FAISystem::InvalidLocation;
	UPROPERTY()
	float WorldTime = -1.f;
	UPROPERTY()
	FRandomStream Random;
	UPROPERTY()
	uint64 ID = 0;

	static uint64 IDCounter;
};