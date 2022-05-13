// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Tickable.h"
#include "AITypes.h"
#include "Gameplay/StatusType.h"
#include "Player/PlayerClass/PlayerClassTypes.h"
#include "Gameplay/DamageLogInterface.h"
#include "System/ReplicatedObjectInterface.h"
#include "StatusEffectBase.generated.h"

class ACorePlayerState;
class UStatusComponent;
class UStatusEffectUserWidget;
class UAnimMontage;

UENUM(BlueprintType)
enum class EStatusEffectStatModifier : uint8
{
	MovementSpeed,
	RotationRate,
	DamageTaken,
	DamageDealt,
	StatusPowerTaken,
	StatusPowerDealt,
	ActionDisabled
};

UENUM(BlueprintType)
enum class EInstigationDirectionUpdateRule : uint8
{
	Never,
	Refresh,
	Always
};

USTRUCT(BlueprintType)
struct FStatusEffectDelegateEntry
{
	GENERATED_USTRUCT_BODY()

	FStatusEffectDelegateEntry() {}

public:
	FDelegateHandle StatusEffectDelegate;
	UPROPERTY()
	float Value = -1.f;
};

class UStatusEffectBase;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FStatusEffectBeginSignature, UStatusEffectBase*, StatusEffect, EStatusBeginType, BeginType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FStatusEffectEndSignature, UStatusEffectBase*, StatusEffect, EStatusEndType, EndType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FStatusEffectInstigatorUpdateSignature, UStatusEffectBase*, StatusEffect, AActor*, Instigator);

//Base of all status effects.
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, AutoExpandCategories = (StatusEffectObject), HideFunctions = (K2_GetDamageLogInstigatorName))
class NAUSEA_API UStatusEffectBase : public UObject, public IDamageLogInterface, public IReplicatedObjectInterface
{
	GENERATED_UCLASS_BODY()

//~ Begin UObject Interface
public:
	virtual void PostInitProperties() override;
	virtual void PreNetReceive() override;
	virtual void PostNetReceive() override;
	virtual void PreDestroyFromReplication() override;
	virtual void BeginDestroy() override;
	virtual UWorld* GetWorld() const override final { return (WorldPrivate ? WorldPrivate : GetWorld_Uncached()); } //UActorComponent's implementation
protected:
	virtual bool IsSupportedForNetworking() const override { return true; }
	virtual int32 GetFunctionCallspace(UFunction* Function, FFrame* Stack) override;
	virtual bool CallRemoteFunction(UFunction* Function, void* Parameters, FOutParmRec* OutParams, FFrame* Stack) override;
//~ End UObject Interface

//~ Begin IDamageLogInterface Interface
public:
	virtual FText GetDamageLogInstigatorName() const override { return GetStatusEffectName(); }
//~ End IDamageLogInterface Interface

public:
	virtual void Initialize(UStatusComponent* StatusComponent, ACorePlayerState* Instigator, float Power = -1.f, const FVector& InstigationDirection = FAISystem::InvalidDirection);
	bool IsInitialized() const { return OwningStatusComponent != nullptr; }

	virtual void OnDestroyed();

	virtual bool CanActivateStatus(ACorePlayerState* Instigator, float Power) const;
	virtual bool CanRefreshStatus(ACorePlayerState* Instigator, float Power) const;

	virtual void AddEffectPower(ACorePlayerState* Instigator, float Power = -1.f, const FVector& InstigationDirection = FAISystem::InvalidDirection);
	virtual void OnActivated(EStatusBeginType BeginType);
	virtual void OnDeactivated(EStatusEndType EndType);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = StatusEffect)
	virtual void UpdateInstigator(ACorePlayerState* Instigator);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = StatusEffect)
	virtual void UpdateInstigationDirection(const FVector& Direction);

	UFUNCTION(BlueprintCallable, Category = StatusEffect)
	bool IsAuthority() const;
	UFUNCTION(BlueprintCallable, Category = StatusEffect)
	bool IsAutonomousProxy() const;
	UFUNCTION(BlueprintCallable, Category = StatusEffect)
	bool IsSimulatedProxy() const;

	UFUNCTION(BlueprintCallable, Category = StatusEffect)
	UStatusComponent* GetOwningStatusComponent() const { return OwningStatusComponent; }

	UFUNCTION(BlueprintCallable, Category = StatusEffect)
	EStatusType GetStatusType() const { return StatusType; }
	UFUNCTION(BlueprintCallable, Category = StatusEffect)
	FText GetStatusEffectName() const { return Name; }
	UFUNCTION(BlueprintCallable, Category = StatusEffect)
	FText GetStatusEffectDescription() const { return Description; }

	UFUNCTION(BlueprintCallable, Category = StatusEffect)
	TSoftClassPtr<UStatusEffectUserWidget> GetStatusEffectWidget() const { return StatusEffectWidget; }
	UFUNCTION(BlueprintCallable, Category = StatusEffect)
	TSoftObjectPtr<UTexture2D> GetStatusEffectIcon() const { return StatusEffectIcon; }

	//Generic progress meter. Can describe how long until a status expires, how long until a stack decreases, how long until a new stack is applied, etc.
	UFUNCTION(BlueprintCallable, Category = StatusEffect)
	virtual float GetStatusEffectProgress() const { return -1.f; }
	UFUNCTION(BlueprintCallable, Category = StatusEffect)
	virtual void GetStatusTime(float& StartTime, float& EndTime) const { StartTime = -1.f; EndTime = -1.f; }
	UFUNCTION(BlueprintCallable, Category = StatusEffect)
	virtual float GetStatusTimeRemaining() const { return -1.f; }

	UFUNCTION(BlueprintCallable, Category = StatusEffect)
	void SetStatModifier(EStatusEffectStatModifier Stat, float InModifier);

	UFUNCTION(BlueprintCallable, Category = StatusEffect)
	float GetStatModifier(EStatusEffectStatModifier Stat) const;

	UFUNCTION(BlueprintCallable, Category = StatusEffect)
	void ClearStatModifiers();

	UFUNCTION(BlueprintCallable, Category = StatusEffect)
	void RequestMovementSpeedUpdate();
	UFUNCTION(BlueprintCallable, Category = StatusEffect)
	void RequestRotationRateUpdate();

	UFUNCTION(BlueprintCallable, Category = StatusEffect)
	void BlockCharacterActions(bool bInterruptCurrentAction);
	UFUNCTION(BlueprintCallable, Category = StatusEffect)
	void UnblockCharacterActions();

	UFUNCTION(BlueprintCallable, Category = StatusEffect, meta = (DisplayName="Get Instigation Direction", ScriptName="GetInstigationDirection"))
	FVector K2_GetInsitgationDirection() const { return GetInstigationDirection(); }
	const FVector_NetQuantizeNormal& GetInstigationDirection() const { return StatusEffectInstigationDirection; }

public:
	UPROPERTY(BlueprintAssignable, Category = StatusEffect)
	FStatusEffectBeginSignature OnEffectBegin;
	UPROPERTY(BlueprintAssignable, Category = StatusEffect)
	FStatusEffectEndSignature OnEffectEnd;

	UPROPERTY(BlueprintAssignable, Category = StatusEffect)
	FStatusEffectInstigatorUpdateSignature OnEffectInstigatorUpdate;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = StatusEffect, meta=(DisplayName="On Activated",ScriptName="OnActivated"))
	void K2_OnActivated(EStatusBeginType BeginType);
	UFUNCTION(BlueprintImplementableEvent, Category = StatusEffect, meta=(DisplayName="On Deactivated",ScriptName="OnDeactivated"))
	void K2_OnDeactivated(EStatusEndType EndType);
	UFUNCTION(BlueprintImplementableEvent, Category = StatusEffect, meta=(DisplayName="On Owner Died",ScriptName="OnOwnerDied"))
	void K2_OnOwnerDied(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser);
	UFUNCTION(BlueprintImplementableEvent, Category = StatusEffect, meta=(DisplayName="On Power Changed",ScriptName="OnPowerChanged"))
	void K2_OnPowerChanged(float Power);

	void BindStatModifier(EStatusEffectStatModifier Stat, FStatusEffectDelegateEntry& StatusEffectModifierEntry);
	void UpdateStatModifier(EStatusEffectStatModifier Stat);
	void UnbindStatModifier(EStatusEffectStatModifier Stat, FStatusEffectDelegateEntry& StatusEffectModifierEntry);

	UFUNCTION()
	virtual void OnOwnerDied(UStatusComponent* StatusComponent, float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser);

	UFUNCTION()
	virtual void OnRep_RefreshCounter();

	UFUNCTION()
	virtual void OnRep_StatusEffectInsitgator();

	UFUNCTION()
	virtual void OnRep_StatusEffectInstigationDirection();

protected:
	UPROPERTY(EditDefaultsOnly)
	EStatusType StatusType = EStatusType::Invalid;

	UPROPERTY(EditDefaultsOnly)
	FText Name;
	UPROPERTY(EditDefaultsOnly)
	FText Description;

	UPROPERTY(EditDefaultsOnly)
	TSoftClassPtr<UStatusEffectUserWidget> StatusEffectWidget;
	UPROPERTY(EditDefaultsOnly)
	TSoftObjectPtr<UTexture2D> StatusEffectIcon;

	UPROPERTY(ReplicatedUsing = OnRep_RefreshCounter)
	uint8 RefreshCounter = 0;

	UPROPERTY(Transient)
	TMap<EStatusEffectStatModifier, FStatusEffectDelegateEntry> StatusModificationMap;
	UPROPERTY(Transient)
	int32 BlockActionID = 0;

	UPROPERTY(ReplicatedUsing = OnRep_StatusEffectInsitgator)
	ACorePlayerState* StatusEffectInsitgator = nullptr;

	UPROPERTY(ReplicatedUsing = OnRep_StatusEffectInstigationDirection)
	FVector_NetQuantizeNormal StatusEffectInstigationDirection = FAISystem::InvalidDirection;
	UPROPERTY(EditDefaultsOnly)
	EInstigationDirectionUpdateRule InstigationUpdateDirectionRule = EInstigationDirectionUpdateRule::Never;


private:
	UWorld* GetWorld_Uncached() const;

private:
	UWorld* WorldPrivate = nullptr;

	UPROPERTY(Transient)
	UStatusComponent* OwningStatusComponent = nullptr;

	UPROPERTY(Transient)
	bool bDoneRemotePostNetReceiveActivation = false;
};

UENUM(BlueprintType)
enum class EBasicStatusEffectType : uint8
{
	Instant,
	Cumulative
};

UENUM(BlueprintType)
enum class EBasicStatusEffectInstigatorRule : uint8
{
	Never, //Will never update instigator.
	Refresh, //Will update instigator if instigator causes this status to refresh refresh.
	LargestAmount, //Will update instigator if instigator causes the most power added to status.
	Any //Will update instigator on any power added to status.
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FStatusTimeUpdateSignature, UStatusEffectBase*, StatusEffect, float, StatusStartTime, float, StatusEndTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPowerUpdateSignature, UStatusEffectBase*, StatusEffect, float, Power);

/**
 * 
 */
UCLASS()
class NAUSEA_API UStatusEffectBasic : public UStatusEffectBase, public FTickableGameObject
{
	GENERATED_UCLASS_BODY()

//~ Begin UObject Interface
public:
	virtual void PostInitProperties() override;
	virtual void BeginDestroy() override;
//~ End UObject Interface

//~ Begin UStatusEffectBase Interface
public:
	virtual void Initialize(UStatusComponent* StatusComponent, ACorePlayerState* Instigator, float Power, const FVector& InstigationDirection) override;
	virtual void OnDestroyed() override;
	virtual void OnActivated(EStatusBeginType BeginType) override;
	virtual void OnDeactivated(EStatusEndType EndType) override;
	virtual bool CanActivateStatus(ACorePlayerState* Instigator, float Power) const override;
	virtual bool CanRefreshStatus(ACorePlayerState* Instigator, float Power) const override;
	virtual void AddEffectPower(ACorePlayerState* Instigator, float Power, const FVector& InstigationDirection) override;
	virtual void GetStatusTime(float& StartTime, float& EndTime) const override;
	virtual float GetStatusTimeRemaining() const override;
//~ End UStatusEffectBase Interface

//~ Begin FTickableGameObject Interface
protected:
	virtual void Tick(float DeltaTime) override;
public:
	virtual ETickableTickType GetTickableTickType() const { return TickType; }
	virtual bool IsTickable() const { return bTickEnabled && !IsPendingKill(); }
	virtual TStatId GetStatId() const { return TStatId(); }
	virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
//~ End FTickableGameObject Interface

public:
	UFUNCTION(BlueprintCallable, Category = StatusEffect)
	float GetPowerRequirement() const { return EffectPowerRange.X; }

	UFUNCTION()
	virtual float GetDurationAtCurrentPower() const;

	UFUNCTION(BlueprintCallable, Category = StatusEffect)
	float GetPowerPercent() const;

	UFUNCTION(BlueprintCallable, Category = StatusEffect)
	float GetCurrentPower() const { return CurrentPower; }
	UFUNCTION(BlueprintCallable, Category = StatusEffect)
	float GetMaximumPower() const { return EffectPowerRange.Y; }

	UFUNCTION(BlueprintCallable, Category = StatusEffect)
	EBasicStatusEffectType GetStatusEffectType() const { return StatusEffectType; }

	UFUNCTION(BlueprintCallable, Category = StatusEffect)
	bool IsCriticalPointReached() const;
	
	UFUNCTION(BlueprintCallable, Category = StatusEffect)
	bool ShouldBindToProcessDamage() const { return IsAuthority() && (bBindProcessDamage || bK2ProcessDamageImplemented); }
	
	UFUNCTION(BlueprintCallable, Category = StatusEffect)
	bool ShouldBindToReceivedDamage() const { return IsAuthority() && (bBindReceivedDamage || bK2ReceivedDamageImplemented); }

public:
	UPROPERTY(BlueprintAssignable, Category = StatusEffect)
	FStatusTimeUpdateSignature OnStatusTimeUpdate;

	UPROPERTY(BlueprintAssignable, Category = StatusEffect)
	FPowerUpdateSignature OnPowerUpdate;

protected:
	UFUNCTION()
	void OnRep_StatusTime();

	UFUNCTION()
	virtual void OnRep_CurrentPower();

	UFUNCTION()
	virtual void ProcessDamage(UStatusComponent* Component, float& Damage, const struct FDamageEvent& DamageEvent, ACorePlayerState* Instigator);

	UFUNCTION()
	void ReceivedDamage(UStatusComponent* Component, float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser);

	UFUNCTION()
	void SetCriticalPointReached(bool bReached);

	UFUNCTION()
	void OnRep_CriticalPointReached();

	UFUNCTION(BlueprintImplementableEvent, Category = StatusEffect)
	void OnCriticalPointReached(bool bReached);

	UFUNCTION(BlueprintImplementableEvent, Category = StatusEffect, meta = (DisplayName="Tick",ScriptName="Tick"))
	void K2_Tick(float DeltaTime);

	UFUNCTION(BlueprintImplementableEvent, BlueprintAuthorityOnly, Category = StatusEffect, meta = (DisplayName = "Process Damage", ScriptName = "ProcessDamage"))
	float K2_ProcessDamage(UStatusComponent* Component, float Damage, const struct FDamageEvent& DamageEvent, ACorePlayerState* Instigator);

	UFUNCTION(BlueprintImplementableEvent, BlueprintAuthorityOnly, Category = StatusEffect, meta = (DisplayName = "Received Damage", ScriptName = "ReceivedDamage"))
	void K2_ReceivedDamage(UStatusComponent* Component, float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser);

	void UpdateInsitgatorCumulativePower(ACorePlayerState* Instigator, float Power);

protected:
	UPROPERTY(ReplicatedUsing = OnRep_StatusTime)
	FVector2D StatusTime = FVector2D(-1.f);

	UPROPERTY()
	FTimerHandle StatusTimer;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentPower)
	float CurrentPower = -1.f;

	UPROPERTY(EditDefaultsOnly)
	EBasicStatusEffectType StatusEffectType = EBasicStatusEffectType::Instant;

	UPROPERTY(EditDefaultsOnly)
	FVector2D EffectDuration = FVector2D(1.f);
	UPROPERTY(EditDefaultsOnly)
	FVector2D EffectPowerRange = FVector2D(1.f);

	UPROPERTY(EditDefaultsOnly)
	uint8 bNotifyWhenCriticalPointReached:1;

	UPROPERTY(ReplicatedUsing = OnRep_CriticalPointReached)
	uint8 bCriticalPointReached:1;

	UPROPERTY(EditDefaultsOnly)
	float PowerDecayDelay = 0.f;
	UPROPERTY(EditDefaultsOnly)
	float PowerDecayRate = 0.f;

	UPROPERTY(EditDefaultsOnly)
	EBasicStatusEffectInstigatorRule InstigatorUpdateRule = EBasicStatusEffectInstigatorRule::Never;
	UPROPERTY(Transient)
	TMap<ACorePlayerState*, float> CumulativePowerMap = TMap<ACorePlayerState*, float>();

	UPROPERTY()
	FTimerHandle PowerDecayTimer;

	UPROPERTY(Transient)
	bool bTickEnabled = false;

	UPROPERTY(Transient)
	bool bK2TickImplemented = false;

	UPROPERTY(EditDefaultsOnly)
	bool bBindProcessDamage = false;
	UPROPERTY(EditDefaultsOnly)
	bool bBindReceivedDamage = false;

	UPROPERTY(Transient)
	bool bK2ProcessDamageImplemented = false;
	UPROPERTY(Transient)
	bool bK2ReceivedDamageImplemented = false;

	FDelegateHandle ProcessDamageHandle;

private:
	ETickableTickType TickType = ETickableTickType::Never;
};

UENUM(BlueprintType)
enum class EStackStatusEffectInstigatorRule : uint8
{
	Never, //Will never update instigator.
	StackIncrease, //Will update instigator if instigator increases the stack.
	Any //Will update instigator any time an instigator attempts to increase stack.
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FStackCountUpdateSignature, UStatusEffectBase*, StatusEffect, uint8, StackCount);

UCLASS()
class NAUSEA_API UStatusEffectStack : public UStatusEffectBase
{
	GENERATED_UCLASS_BODY()

//~ Begin UStatusEffectBase Interface
public:
	virtual void Initialize(UStatusComponent* StatusComponent, ACorePlayerState* Instigator, float Power, const FVector& InstigationDirection) override;
	virtual void OnDestroyed() override;
	virtual void OnActivated(EStatusBeginType BeginType) override;
	virtual void OnDeactivated(EStatusEndType EndType) override;
	virtual bool CanActivateStatus(ACorePlayerState* Instigator, float Power) const override;
	virtual bool CanRefreshStatus(ACorePlayerState* Instigator, float Power) const override;
	virtual void AddEffectPower(ACorePlayerState* Instigator, float Power, const FVector& InstigationDirection) override;
//~ End UStatusEffectBase Interface

public:
	UFUNCTION(BlueprintCallable, Category = Stack)
	uint8 GetStackCount() const { return CurrentStackCount; }
	UFUNCTION(BlueprintCallable, Category = Stack)
	uint8 GetMaxStackCount() const { return MaxStackCount; }

public:
	UPROPERTY(BlueprintAssignable, Category = StatusEffect)
	FStatusTimeUpdateSignature OnStatusTimeUpdate;
	UPROPERTY(BlueprintAssignable, Category = StatusEffect)
	FStackCountUpdateSignature OnStackCountUpdate;

protected:
	UFUNCTION()
	void OnRep_StatusTime();

	UFUNCTION()
	void OnRep_CurrentStackCount();

	UFUNCTION()
	void OnStackExpired();

protected:
	UPROPERTY(ReplicatedUsing = OnRep_StatusTime)
	FVector2D StatusTime = FVector2D (-1.f);

	UPROPERTY()
	FTimerHandle StatusTimer;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentStackCount)
	uint8 CurrentStackCount = 0;

	UPROPERTY(EditDefaultsOnly, Category = Stack)
	uint8 MaxStackCount = 3;

	UPROPERTY(EditDefaultsOnly, Category = Stack)
	float StackDuration = 5.f;

	UPROPERTY(EditDefaultsOnly, Category = Stack)
	EStackStatusEffectInstigatorRule InstigatorUpdateRule = EStackStatusEffectInstigatorRule::Never;
};