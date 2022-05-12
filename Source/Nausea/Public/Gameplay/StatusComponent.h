// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GenericTeamAgentInterface.h"
#include "NauseaHelpers.h"
#include "System/NetHelper.h"
#include "StatusType.h"
#include "Player/PlayerOwnershipInterfaceTypes.h"
#include "System/ReplicatedObjectInterface.h"
#include "StatusComponent.generated.h"

class UCurveFloat;
class IStatusInterface;
class UStatusEffectBase;
class AActor;
class AController;
class ACorePlayerState;
class ACoreCharacter;
class UDamageType;
class UNauseaDamageType;

//Helper struct for health and anything else that would want to be clamped between 0 and a specified maximum. 
USTRUCT(BlueprintType)
struct FStatStruct
{
	GENERATED_USTRUCT_BODY()

	FStatStruct()
	{

	}

	FStatStruct(const float& InValue, const float& InMaxValue)
		: DefaultValue(InValue), DefaultMaxValue(InMaxValue) {}

	FStatStruct(const float& InMaxValue)
		: FStatStruct(InMaxValue, InMaxValue) {}

	operator float() const { return GetValue(); }

	FStatStruct& operator= (const float& InValue) { this->SetValue(InValue); return *this; }
	float operator+ (const float& InValue) const { return this->GetValue() + InValue; }
	FStatStruct& operator+= (const float& InValue) { this->SetValue(this->GetValue() + InValue); return *this; }
	float operator- (const float& InValue) const { return this->GetValue() - InValue; }
	FStatStruct& operator-= (const float& InValue) { this->SetValue(this->GetValue() - InValue); return *this; }

	bool IsIdenticalTo(const FStatStruct& Other) const { return GetValue() == Other.GetValue() && GetMaxValue() == Other.GetMaxValue(); }

	//Multiplication scales the entire struct (specifically the Value and MaxValue) together.
	FStatStruct operator* (const float& InValue) const { FStatStruct Result = *this; Result.Value *= InValue; Result.MaxValue *= InValue; return Result; }
	FStatStruct& operator*= (const float& InValue) { this->MaxValue *= InValue; this->Value *= InValue; return *this; }

public:
	FStatStruct Initialize() { MaxValue = DefaultMaxValue; SetValue(DefaultValue < 0.f ? MaxValue : DefaultValue); return *this; }

	float GetValue() const { return Value; }
	float GetMaxValue() const { return MaxValue; }
	float GetPercentValue() const { return Value / MaxValue; }

	float SetValue(float InValue) { Value = FMath::Clamp(InValue, 0.f, MaxValue); return Value; }

	float AddValue(float Delta) { this->SetValue(this->GetValue() + Delta); return Value; }
	float SetMaxValue(float InMaxValue) { MaxValue = InMaxValue; return SetValue(Value); }

	bool IsValid() const { return DefaultValue == -1.f && DefaultMaxValue == -1.f; }

protected:
	UPROPERTY()
	float Value = 0.f;
	UPROPERTY()
	float MaxValue = 100.f;
	
public:
	UPROPERTY(NotReplicated, EditAnywhere, BlueprintReadOnly)
	float DefaultValue = 0.f;
	UPROPERTY(NotReplicated, EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Max Value"))
	float DefaultMaxValue = 100.f;

	static FStatStruct InvalidStat;
};

USTRUCT(BlueprintType)
struct FPartStatStruct : public FFastArraySerializerItem
{
	GENERATED_USTRUCT_BODY()

public:
	FPartStatStruct()
	{

	}

	FPartStatStruct(const float& InValue, const float& InMaxValue)
		: StatStruct(InValue, InMaxValue) {}

	operator float() const { return StatStruct; }

	FPartStatStruct& operator= (const float& InValue) { this->StatStruct.SetValue(InValue); return *this; }
	float operator+ (const float& InValue) const { return this->StatStruct + InValue; }
	FPartStatStruct& operator+= (const float& InValue) { this->StatStruct += InValue; return *this; }
	float operator- (const float& InValue) const { return this->StatStruct - InValue; }
	FPartStatStruct& operator-= (const float& InValue) { this->StatStruct -= InValue; return *this; }

	float GetValue() const { return this->StatStruct.GetValue(); }
	float GetMaxValue() const { return this->StatStruct.GetMaxValue(); }
	float GetPercentValue() const { return this->StatStruct.GetPercentValue(); }

	bool IsValid() const { return PartName != NAME_None; }
	bool IsDestroyed() const { return CanTakeDamage() && StatStruct.GetValue() <= 0.f; }
	bool CanTakeDamage() const { return StatStruct.GetMaxValue() > 0.f; }

public:
	//Part name returned by UStatusComponent::GetHitBodyPart when provided a hit bone.
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName PartName = NAME_None;

	//List of bones that will correspond to the part above.
	UPROPERTY(NotReplicated, EditAnywhere, BlueprintReadOnly)
	TArray<FName> BoneList = TArray<FName>();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (PinHiddenByDefault, InlineEditConditionToggle))
	bool bApplyFlatDamageOnPartKilled = false;
	//Flat damage to apply back to main status component health once this part is killed.
	UPROPERTY(NotReplicated, EditAnywhere, BlueprintReadOnly, meta = (EditCondition = bApplyFlatDamageOnPartKilled))
	float FlatDamageOnPartKilled = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (PinHiddenByDefault, InlineEditConditionToggle))
	bool bApplyPercentDamageOnPartKilled = false;
	//Percent of max health damage to apply back to main status component health once this part is killed.
	UPROPERTY(NotReplicated, EditAnywhere, BlueprintReadOnly, meta = (EditCondition = bApplyPercentDamageOnPartKilled))
	float PercentDamageOnPartKilled = 0.f;

	//Multiplies incoming damage for this part.
	UPROPERTY(NotReplicated, EditAnywhere, BlueprintReadOnly)
	float DamageMultiplier = 1.f;

	//If true this part will never receive health scaling, even if the status configuration object says parts should.
	UPROPERTY(NotReplicated, EditAnywhere, BlueprintReadOnly)
	bool bAlwaysIgnoresHealthScaling = false;

	UPROPERTY(NotReplicated, EditAnywhere, BlueprintReadOnly)
	TMap<EDamageHitDescriptor, float> HitTypeDamageMultiplier = TMap<EDamageHitDescriptor, float>();
	UPROPERTY(NotReplicated, EditAnywhere, BlueprintReadOnly)
	TMap<EDamageElementalDescriptor, float> ElementalTypeDamageMultiplier = TMap<EDamageElementalDescriptor, float>();

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FStatStruct StatStruct = FStatStruct(0.f, 0.f);

	static FPartStatStruct InvalidPartStat;
};

USTRUCT(BlueprintType)
struct FPartStatContainer : public FFastArraySerializer
{
	GENERATED_USTRUCT_BODY()

	FAST_ARRAY_SERIALIZER_OPERATORS(FPartStatStruct, InstanceList);

public:
	FPartStatContainer() {}

	FORCEINLINE void SetOwningStatusComponent(UStatusComponent* InOwningStatusComponent) { OwningStatusComponent = InOwningStatusComponent; }

protected:
	UPROPERTY()
	TArray<FPartStatStruct> InstanceList;
		
	UPROPERTY(NotReplicated)
	UStatusComponent* OwningStatusComponent = nullptr;

public:
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FPartStatStruct, FPartStatContainer>(InstanceList, DeltaParms, *this);
	}

	void PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize);
};

template<>
struct TStructOpsTypeTraits< FPartStatContainer > : public TStructOpsTypeTraitsBase2< FPartStatContainer >
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};

USTRUCT(BlueprintType)
struct FPartDestroyedEvent
{
	GENERATED_USTRUCT_BODY()

	FPartDestroyedEvent() {}

public:
	UPROPERTY()
	TSubclassOf<UDamageType> DamageType = nullptr;
	UPROPERTY()
	float Damage = 0.f;
};

USTRUCT(BlueprintType)
struct FHitEventContainer : public FFastArraySerializer
{
	GENERATED_USTRUCT_BODY()

	FAST_ARRAY_SERIALIZER_OPERATORS(FHitEvent, InstanceList);

public:
	FHitEventContainer() {}
	FORCEINLINE void SetOwningStatusComponent(UStatusComponent* InOwningStatusComponent) { OwningStatusComponent = InOwningStatusComponent; }

protected:
	UPROPERTY()
	TArray<FHitEvent> InstanceList;
		
	UPROPERTY(NotReplicated)
	UStatusComponent* OwningStatusComponent = nullptr;

public:
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FHitEvent, FHitEventContainer>(InstanceList, DeltaParms, *this);
	}

	void PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize);
};

template<>
struct TStructOpsTypeTraits< FHitEventContainer > : public TStructOpsTypeTraitsBase2< FHitEventContainer >
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};

USTRUCT(BlueprintType)
struct FDeathEvent
{
	GENERATED_USTRUCT_BODY()

	FDeathEvent() {}

public:
	UPROPERTY()
	TSubclassOf<UDamageType> DamageType = nullptr;
	UPROPERTY()
	float Damage = 0.f;
	UPROPERTY()
	FVector_NetQuantize HitLocation;
	UPROPERTY()
	FVector_NetQuantize HitMomentum;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FHealthChangedSignature, UStatusComponent*, Component, float, Health, float, PreviousHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FMaxHealthChangedSignature, UStatusComponent*, Component, float, MaxHealth, float, PreviousMaxHealth);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FArmourChangedSignature, UStatusComponent*, Component, float, Armour, float, PreviousArmour);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FMaxArmourChangedSignature, UStatusComponent*, Component, float, MaxArmour, float, PreviousMaxArmour);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FPartHealthChangedSignature, UStatusComponent*, Component, const FName&, PartName, float, Health, float, PreviousHealth);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FStatusComponentEffectBeginSignature, UStatusComponent*, Component, UStatusEffectBase*, StatusEffect, EStatusBeginType, BeginType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FStatusComponentEffectEndSignature, UStatusComponent*, Component, UStatusEffectBase*, StatusEffect, EStatusEndType, EndType);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FDamageReceiveSignature, UStatusComponent*, Component, float, Damage, struct FDamageEvent const&, DamageEvent, AController*, EventInstigator, AActor*, DamageCauser);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FDiedSignature, UStatusComponent*, Component, float, Damage, struct FDamageEvent const&, DamageEvent, AController*, EventInstigator, AActor*, DamageCauser);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FDeathEventSignature, UStatusComponent*, Component, TSubclassOf<UDamageType>, DamageType, float, Damage, FVector_NetQuantize, HitLocation, FVector_NetQuantize, HitMomentum);

UCLASS( ClassGroup=(Custom), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent), HideCategories = (ComponentTick, Collision, Tags, Variable, Activation, ComponentReplication, Cooking, Sockets, UserAssetData))
class NAUSEA_API UStatusComponent : public UActorComponent, public IGenericTeamAgentInterface, public IReplicatedObjectInterface
{
	GENERATED_UCLASS_BODY()

	//Configuration will perform the initialization so it needs access to all the internals of this class.
	friend class UStatusComponentConfigObject;
	//Callbacks need to be used but are protected (and should obviously not be public).
	friend FHitEventContainer;

//~ Begin UActorComponent Interface 
protected:
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
public:
	virtual bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
//~ End UActorComponent Interface

//~ Begin IGenericTeamAgentInterface Interface
public:
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	virtual FGenericTeamId GetGenericTeamId() const override { return TeamId; }
//~ End IGenericTeamAgentInterface Interface

public:
	UFUNCTION()
	virtual void SetPlayerDefaults();
	UFUNCTION()
	virtual void InitializeStatusComponent();

	UFUNCTION(BlueprintCallable, Category = StatusComponent)
	TScriptInterface<IStatusInterface> GetOwnerInterface() const { return StatusInterface; }

	UFUNCTION(BlueprintCallable, Category = StatusComponent)
	float GetHealth() const { return Health; }
	UFUNCTION(BlueprintCallable, Category = StatusComponent)
	float GetHealthMax() const { return Health.GetMaxValue(); }
	UFUNCTION(BlueprintCallable, Category = StatusComponent)
	float GetHealthPercent() const { return Health.GetPercentValue(); }

	UFUNCTION(BlueprintCallable, Category = StatusComponent)
	bool IsHealthBelowPercent(float Percent) const { return Health.GetPercentValue() < Percent; }

	UFUNCTION(BlueprintCallable, Category = StatusComponent)
	float GetArmour() const { return Armour; }
	UFUNCTION(BlueprintCallable, Category = StatusComponent)
	float GetArmourMax() const { return Armour.GetMaxValue(); }
	UFUNCTION(BlueprintCallable, Category = StatusComponent)
	float GetArmourPercent() const { return Armour.GetPercentValue(); }

	UFUNCTION(BlueprintCallable, Category = StatusComponent)
	bool IsDead() const { return Health <= 0.f; }

	UFUNCTION(BlueprintCallable, Category = StatusComponent)
	const FDeathEvent& GetDeathEvent() const { return DeathEvent; }

	UFUNCTION(BlueprintCallable, Category = StatusComponent)
	float GetMovementSpeedModifier() const;

	UFUNCTION(BlueprintCallable, Category = StatusComponent)
	float GetRotationRateModifier() const;

	UFUNCTION(BlueprintCallable, Category = StatusComponent)
	bool CanDisplayStatus() const { return bDisplayStatus; }

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	virtual float HealDamage(float HealAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser);

	UFUNCTION()
	virtual void Kill(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser);

	UFUNCTION()
	virtual void Died(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser);

	UFUNCTION(BlueprintCallable, Category = StatusComponent)
	FName GetHitBodyPartName(const FName& BoneName) const;

	UFUNCTION(BlueprintCallable, Category = StatusComponent)
	FPartStatStruct& GetPartHealthForBone(const FName& BoneName);

	UFUNCTION()
	int32 GetPartHealthIndexForBone(const FName& BoneName) const;

	void RequestMovementSpeedUpdate() { bUpdateMovementSpeedModifier = true; }
	void RequestRotationRateUpdate() { bUpdateRotationRateModifier = true; }

	void OnReceivedPartHealthUpdate(const FPartStatStruct& PartStat);

	void OnReceivedHitEvent(const FHitEvent& HitEvent);

	void RequestActionBlock(int32& BlockID, bool bInterruptCurrentAction);
	void RevokeActionBlock(int32& BlockID);
	bool UpdateActionBlock();
	bool IsBlockingAction() const { return bIsBlockingAction; }

	void InterruptCharacterActions();

	UFUNCTION(BlueprintCallable, Category = StatusComponent)
	const TArray<UStatusEffectBase*>& GetStatusEffectList() const { return StatusEffectList; }

	UFUNCTION()
	void OnStatusEffectAdded(UStatusEffectBase* StatusEffect);
	UFUNCTION()
	void OnStatusEffectRemoved(UStatusEffectBase* StatusEffect);

public:
	UPROPERTY(BlueprintAssignable, Category = StatusComponent)
	FHealthChangedSignature OnHealthChanged;
	UPROPERTY(BlueprintAssignable, Category = StatusComponent)
	FMaxHealthChangedSignature OnMaxHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = StatusComponent)
	FArmourChangedSignature OnArmourChanged;
	UPROPERTY(BlueprintAssignable, Category = StatusComponent)
	FMaxArmourChangedSignature OnMaxArmourChanged;
	
	UPROPERTY(BlueprintAssignable, Category = StatusComponent)
	FPartHealthChangedSignature OnPartHealthChanged;
	
	//Event that broadcasts damage logs once the log is completed.
	DECLARE_EVENT_TwoParams(UStatusComponent, FDamageLogPoppedSignature, const UStatusComponent*, const FDamageLogEvent&)
	FDamageLogPoppedSignature OnDamageLogPopped;

	//Event called post-damage application. Distinct to UStatusComponent::OnProcessDamageTaken as this one is not meant to modifiy damage, only as a notification once damage has been applied.
	UPROPERTY(BlueprintAssignable, Category = StatusComponent)
	FDamageReceiveSignature OnDamageReceived;

	UPROPERTY(BlueprintAssignable, Category = StatusComponent)
	FDiedSignature OnDied;

	UPROPERTY(BlueprintAssignable, Category = StatusComponent)
	FDeathEventSignature OnDeathEvent;

	UPROPERTY(BlueprintAssignable, Category = StatusComponent)
	FStatusComponentEffectBeginSignature OnStatusEffectBegin;
	UPROPERTY(BlueprintAssignable, Category = StatusComponent)
	FStatusComponentEffectEndSignature OnStatusEffectEnd;

	DECLARE_EVENT_TwoParams(UStatusComponent, FHitEventSignature, const UStatusComponent*, const FHitEvent&)
	FHitEventSignature OnHitEvent;
	
//Status effect hooks.
public:
	DECLARE_EVENT_OneParam(UStatusComponent, FStatusComponentEventSignature, const UStatusComponent*)
	FStatusComponentEventSignature OnActionInterrupt;

	DECLARE_EVENT_TwoParams(UStatusComponent, FStatusComponentValueModifierSignature, const UStatusComponent*, float&)
	FStatusComponentValueModifierSignature OnProcessMovementSpeed;
	FStatusComponentValueModifierSignature OnProcessRotationRate;

	DECLARE_EVENT_FourParams(UStatusComponent, FStatusComponentDamageModifierSignature, UStatusComponent*, float&, const struct FDamageEvent&, ACorePlayerState*)
	FStatusComponentDamageModifierSignature OnProcessDamageTaken;
	FStatusComponentDamageModifierSignature OnProcessDamageDealt;

	DECLARE_EVENT_FourParams(UStatusComponent, FStatusComponentStatusPowerModifierSignature, AActor*, float&, const struct FDamageEvent&, EStatusType)
	FStatusComponentStatusPowerModifierSignature OnProcessStatusPowerTaken;
	FStatusComponentStatusPowerModifierSignature OnProcessStatusPowerDealt;

	DECLARE_EVENT_TwoParams(UStatusComponent, FStatusComponentBooleanValueModifierSignature, const UStatusComponent*, bool&)
	FStatusComponentBooleanValueModifierSignature OnProcessActionDisabled;

	DECLARE_EVENT_FourParams(UStatusComponent, FStatusComponentThreadModifierSignature, UStatusComponent*, float&, const struct FDamageEvent&, ACorePlayerState*)
	FStatusComponentThreadModifierSignature OnProcessThreatApplied;

	DECLARE_EVENT_TwoParams(UStatusComponent, FStatusComponentBooleanValueUpdateSignature, const UStatusComponent*, bool)
	FStatusComponentBooleanValueUpdateSignature OnBlockingActionUpdate;

	DECLARE_EVENT_TwoParams(UStatusComponent, FStatusComponentHitEventSignature, const UStatusComponent*, const FHitEvent&)
	FStatusComponentHitEventSignature OnHitEventReceived;

protected:
	float SetHealth(float InHealth);
	float SetMaxHealth(float InMaxHealth);
	float SetArmour(float InArmour);
	float SetMaxArmour(float InMaxArmour);

	UFUNCTION()
	virtual void TakeDamage(AActor* Actor, float& DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser);

	UFUNCTION()
	virtual void HandlePointDamage(AActor* Actor, float& DamageAmount, struct FPointDamageEvent const& PointDamageEvent, AController* EventInstigator, AActor* DamageCauser);
	UFUNCTION()
	virtual void HandleRadialDamage(AActor* Actor, float& DamageAmount, struct FRadialDamageEvent const& RadialDamageEvent, AController* EventInstigator, AActor* DamageCauser);

	UFUNCTION(BlueprintImplementableEvent, Category = StatusComponent, meta = (DisplayName="Handle Point Damage"))
	void K2_HandlePointDamage(AActor* Actor, float DamageAmount, float& Return, const struct FPointDamageEvent& PointDamageEvent, AController* EventInstigator, AActor* DamageCauser);
	UFUNCTION(BlueprintImplementableEvent, Category = StatusComponent, meta = (DisplayName = "Handle Radial Damage"))
	void K2_HandleRadialDamage(AActor* Actor, float DamageAmount, float& Return, const struct FRadialDamageEvent& PointDamageEvent, AController* EventInstigator, AActor* DamageCauser);

	UFUNCTION()
	void HandleHitPartDestroyed(const FPartStatStruct& DestroyedPart, float& DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser);

	UFUNCTION()
	void HandleArmourDamage(float& DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser);

	virtual void HandleDamageTypeStatus(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = StatusComponent)
	virtual UStatusEffectBase* AddStatusEffect(TSubclassOf<UStatusEffectBase> StatusEffectClass, struct FDamageEvent const& DamageEvent, AController* EventInstigator, float Power = -1.f);

	UFUNCTION()
	virtual void UpdateDeathEvent(float DamageAmount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser);

	inline void PushDamageLog(FDamageLogEvent&& Entry);
	inline FDamageLogEvent PopDamageLog(float DamageDealt);

	void GenerateHitEvent(FHitEvent&& InHitEvent);
	UFUNCTION()
	void CleanupHitEvent(uint64 ID);
	UFUNCTION()
	void PlayHitEffect(const FHitEvent& HitEvent);

public:
	UFUNCTION(BlueprintCallable, Category = StatusComponent)
	bool ShouldPerformDamageLog() const { return bPerformDamageLog; }

	inline void MarkDeathEvent();
	inline void PushDamageLogModifier(FDamageLogEventModifier&& Modifier);

protected:
	UFUNCTION()
	virtual void OnRep_Health();

	UFUNCTION()
	virtual void OnRep_Armour();

	UFUNCTION()
	void OnRep_ReplicatedStatusListUpdateCounter();

	UFUNCTION()
	virtual void OnRep_TeamId();

	UFUNCTION()
	virtual void OnRep_DeathEvent();
	UFUNCTION()
	virtual void OnRep_PartDestroyedEventList();

protected:
	UPROPERTY(Transient, ReplicatedUsing = OnRep_Health)
	FStatStruct Health;
	UPROPERTY(Transient, Replicated)
	FVector2D HealthMovementSpeedModifier;
	UPROPERTY(Transient)
	FStatStruct PreviousHealth;

	UPROPERTY(Transient)
	TMap<EDamageHitDescriptor, float> HitTypeDamageMultiplier = TMap<EDamageHitDescriptor, float>();
	UPROPERTY(Transient)
	TMap<EDamageElementalDescriptor, float> ElementalTypeDamageMultiplier = TMap<EDamageElementalDescriptor, float>();

	UPROPERTY(Transient, Replicated)
	FPartStatContainer PartHealthList;
	UPROPERTY(Transient)
	TMap<FName, float> PreviousPartHealthMap;
	UPROPERTY(Transient)
	TMap<FName, int32> BonePartIndexMap;
	
	UPROPERTY(Transient, ReplicatedUsing = OnRep_Armour)
	FStatStruct Armour;
	UPROPERTY(Transient)
	FStatStruct PreviousArmour;
	UPROPERTY(Transient, Replicated)
	FVector2D ArmourAbsorption = FVector2D(1.f, 0.5f);
	UPROPERTY(Transient, Replicated)
	FVector2D ArmourDecay = FVector2D(0.667f, 0.333f);

	UPROPERTY(Transient)
	TArray<UStatusEffectBase*> StatusEffectList;

	UPROPERTY(Transient)
	TMap<EStatusType, TSubclassOf<UStatusEffectBase>> GenericStatusEffectMap;

	UPROPERTY(Transient)
	TMap<EStatusType, float> GenericStatusEffectMultiplierMap;

	UPROPERTY(Transient)
	int32 BlockingActionCounter = 0;
	UPROPERTY(Transient)
	TSet<int32> BlockingActionSet = TSet<int32>();
	UPROPERTY(Transient)
	bool bIsBlockingAction = false;

	UPROPERTY(EditDefaultsOnly, Category = StatusComponent)
	TSubclassOf<UStatusComponentConfigObject> StatusConfig;

	//If false, this status component will not perform damage log events.
	UPROPERTY(EditDefaultsOnly, Category = DamageLog)
	bool bPerformDamageLog = true;

	//Used to describe armor damage modifications to the damage log.
	UPROPERTY(EditDefaultsOnly, Category = DamageLog)
	TSubclassOf<class UDamageLogModifierObject> ArmorDamageLogModifierClass = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = DamageLog)
	TSubclassOf<class UDamageLogModifierObject> PartDestroyedFlatDamageLogModifierClass = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = DamageLog)
	TSubclassOf<class UDamageLogModifierObject> PartDestroyedPercentDamageLogModifierClass = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = DamageLog)
	TSubclassOf<class UDamageLogModifierObject> WeaknessDamageLogModifierClass = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = DamageLog)
	TSubclassOf<class UDamageLogModifierObject> ResistanceDamageLogModifierClass = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = DamageLog)
	TSubclassOf<class UDamageLogModifierObject> PartWeaknessDamageLogModifierClass = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = DamageLog)
	TSubclassOf<class UDamageLogModifierObject> PartResistanceDamageLogModifierClass = nullptr;

	//Internal property.
	UPROPERTY()
	bool bHideStatusComponentTeam = false;

	UPROPERTY(EditDefaultsOnly, Category = StatusComponent)
	ETeam StatusComponentTeam;

	UPROPERTY(EditDefaultsOnly, Category = AI, meta = (PinHiddenByDefault, InlineEditConditionToggle))
	bool bMakeNoiseOnDamage = true;
	UPROPERTY(EditDefaultsOnly, Category = AI, meta = (EditCondition = "bMakeNoiseOnDamage", DisplayName = "Make Noise On Damaged"))
	FCoreNoiseParams DamageNoise = FCoreNoiseParams(CoreNoiseTag::Damage, 0.75f, 750.f);

	UPROPERTY(EditDefaultsOnly, Category = AI, meta = (PinHiddenByDefault, InlineEditConditionToggle))
	bool bMakeNoiseOnDeath = true;
	UPROPERTY(EditDefaultsOnly, Category = AI, meta = (EditCondition = "bMakeNoiseOnDeath", DisplayName = "Make Noise On Death"))
	FCoreNoiseParams DeathNoise = FCoreNoiseParams(CoreNoiseTag::Death, 1.25f, 2000.f);

	UPROPERTY(EditDefaultsOnly, Category = StatusComponent)
	bool bDisplayStatus = true;

	UPROPERTY(EditDefaultsOnly, Category = StatusComponent)
	bool bReplicateHitEvents = false;

	UPROPERTY(ReplicatedUsing = OnRep_TeamId)
	FGenericTeamId TeamId = FGenericTeamId::NoTeam;

	UPROPERTY(Transient, ReplicatedUsing = OnRep_DeathEvent)
	FDeathEvent DeathEvent;
	UPROPERTY(Transient, ReplicatedUsing = OnRep_PartDestroyedEventList)
	TArray<FPartDestroyedEvent> PartDestroyedEventList;
	UPROPERTY(Transient, Replicated)
	FHitEventContainer HitEventList;

	UPROPERTY(Transient)
	FDamageLogStack DamageLogStack;

	UPROPERTY(Transient)
	bool bAutomaticallyInitialize = true;

	UPROPERTY(Transient)
	mutable float CachedMovementSpeedModifier = 1.f;
	UPROPERTY(Transient)
	mutable bool bUpdateMovementSpeedModifier = false;

	UPROPERTY(Transient)
	mutable float CachedRotationRateModifier = 1.f;
	UPROPERTY(Transient)
	mutable bool bUpdateRotationRateModifier = false;

private:
	UPROPERTY(Transient)
	TScriptInterface<IStatusInterface> StatusInterface = nullptr;
};

//Configuration object of a given UStatusComponent. Gives us the ability to leverage OOP principles while configuring/designing UStatusComponent configurations.
UCLASS(BlueprintType, Blueprintable, AutoExpandCategories = (Default, Basic))
class NAUSEA_API UStatusComponentConfigObject : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	virtual void ConfigureStatusComponent(UStatusComponent* StatusComponent) const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = Basic)
	FStatStruct Health = FStatStruct(100.f, 100.f);
	UPROPERTY(EditDefaultsOnly, Category = Basic)
	FVector2D HealthMovementSpeedModifier = FVector2D(1.f, 1.f);

	UPROPERTY(EditDefaultsOnly, Category = Basic)
	FStatStruct Armour = FStatStruct(100.f, 100.f);
	UPROPERTY(EditDefaultsOnly, Category = Basic)
	FVector2D ArmourAbsorption = FVector2D(1.f, 0.5f);
	UPROPERTY(EditDefaultsOnly, Category = Basic)
	FVector2D ArmourDecay = FVector2D(0.667f, 0.333f);

	UPROPERTY(EditDefaultsOnly, Category = Basic)
	TMap<EDamageHitDescriptor, float> HitTypeDamageMultiplier = TMap<EDamageHitDescriptor, float>();
	UPROPERTY(EditDefaultsOnly, Category = Basic)
	TMap<EDamageElementalDescriptor, float> ElementalTypeDamageMultiplier = TMap<EDamageElementalDescriptor, float>();

	UPROPERTY(EditDefaultsOnly, Category = Advanced)
	TArray<FPartStatStruct> PartHealthList;
	
	UPROPERTY(EditDefaultsOnly, Category = Status)
	TMap<EStatusType, TSubclassOf<UStatusEffectBase>> GenericStatusEffectMap;

	UPROPERTY(EditDefaultsOnly, Category = Status)
	TMap<EStatusType, float> GenericStatusEffectMultiplierMap;

	UPROPERTY(EditDefaultsOnly, Category = Difficulty)
	UCurveFloat* HealthDifficultyScalingCurve = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = Difficulty)
	UCurveFloat* HealthPlayerCountScalingCurve = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = Difficulty)
	bool bHealthScalingEffectsPartHealth = true;

	UPROPERTY(EditDefaultsOnly, Category = Difficulty)
	UCurveFloat* ArmourDifficultyScalingCurve = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = Difficulty)
	UCurveFloat* ArmourPlayerCountScalingCurve = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = Difficulty)
	UCurveFloat* StatusDifficultyScalingCurve = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = Difficulty)
	UCurveFloat* StatusPlayerCountScalingCurve = nullptr;
};