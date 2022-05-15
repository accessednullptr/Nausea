// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Engine/NetSerialization.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "NauseaHelpers.h"
#include "Character/CoreCharacterComponent.h"
#include "GenericTeamAgentInterface.h"
#include "Gameplay/Ability/AbilityTypes.h"
#include "AbilityComponent.generated.h"

class IAbilityObjectInterface;
class UAbilityAction;
class UAbilityInfo;
class UActionBrainDataObject;
class UActionBrainComponentAction;
class UAnimMontage;

UENUM(BlueprintType)
enum class EAbilityRequestResponse : uint8
{
	Success,
	ActionBlocked,
	NoChargeRemaining,
	NotInitialized,
	InvalidAbilityInstance
};

USTRUCT()
struct FAbilityObjectContainer
{
	GENERATED_USTRUCT_BODY()

	FAbilityObjectContainer() {}

public:
	void Add(TScriptInterface<IAbilityObjectInterface> Instance);
	void Remove(TScriptInterface<IAbilityObjectInterface> Instance);

	void Activated();
	void Completed();
	void Cleanup();

protected:
	UPROPERTY()
	TArray<UObject*> AbilityObjectArray = TArray<UObject*>();
};

USTRUCT()
struct FAbilityTargetDataHandleArray
{
	GENERATED_USTRUCT_BODY()

	FAbilityTargetDataHandleArray() {}

public:
	UPROPERTY()
	TArray<FAbilityTargetDataHandle> TargetDataHandleArray = TArray<FAbilityTargetDataHandle>();
};

USTRUCT()
struct FAbilityInstanceTargetDataMap
{
	GENERATED_USTRUCT_BODY()

	FAbilityInstanceTargetDataMap() {}

public:
	UPROPERTY()
	TMap<FAbilityInstanceHandle, FAbilityTargetDataHandleArray> AbilityInstanceMap = TMap<FAbilityInstanceHandle, FAbilityTargetDataHandleArray>();
};

USTRUCT()
struct FAbilityActionList
{
	GENERATED_USTRUCT_BODY()

	FAbilityActionList() {}

public:
	UPROPERTY()
	TArray<UAbilityAction*> AbilityList = TArray<UAbilityAction*>();
};

class UAbilityDecalComponent;

UENUM(BlueprintType)
enum class EAbilityEventType : uint8
{
	Started,
	Interrupted,
	Completed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAbilityDataUpdateSignature, UAbilityComponent*, Component, const FAbilityData&, AbilityData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FAbilityDataChangeUpdateSignature, UAbilityComponent*, Component, const FAbilityData&, AbilityData, const FAbilityData&, PreviousAbilityData);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), HideCategories = (ComponentTick, Collision, Tags, Variable, Activation, ComponentReplication, Cooking, Sockets, UserAssetData))
class NAUSEA_API UAbilityComponent : public UCoreCharacterComponent
{
	GENERATED_UCLASS_BODY()

	friend FAbilityInstanceContainer;
	friend UAbilityAction;

//~ Begin UActorComponent Interface 
protected:
	virtual void InitializeComponent() override;
	virtual void BeginPlay() override;
public:
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
//~ End UActorComponent Interface

//~ Begin UCoreCharacterComponent Interface
protected:
	virtual void OnOwningCharacterDied(UStatusComponent* Component, TSubclassOf<UDamageType> DamageType, float Damage, FVector_NetQuantize HitLocation, FVector_NetQuantize HitMomentum) override;
//~ End UCoreCharacterComponent Interface 

public:
	TMap<FAbilityTargetDataHandle, FAbilityObjectContainer>& GetAbilityTargetDataObjectMap() { return AbilityTargetDataObjectMap; }

	EAbilityRequestResponse CanPerformAbility(TSubclassOf<UAbilityInfo> AbilityClass) const;
	EAbilityRequestResponse CanPerformAbility(const FAbilityInstanceData& AbilityInstanceData) const;

	EAbilityRequestResponse PerformAbility(FAbilityInstanceData&& AbilityInstanceData, bool bNotify = true);

	UFUNCTION(BlueprintCallable, Category = AbilityComponent)
	void RegisterAbilityObjectInstance(TScriptInterface<IAbilityObjectInterface> AbilityObject, const FAbilityInstanceData& AbilityInstance, const FAbilityTargetData& AbilityTargetData);

	void RegisterAbilityAction(FAbilityTargetDataHandle AbilityTargetDataHandle, UAbilityAction* AbilityAction);
	void OnAbilityActionCleanup(FAbilityTargetDataHandle AbilityTargetDataHandle, UAbilityAction* AbilityAction);

	UFUNCTION(BlueprintCallable, Category = AbilityComponent)
	void AsyncLoadAbilityObjectClass(TSoftClassPtr<UObject> SoftClass);
	UFUNCTION(BlueprintCallable, Category = AbilityComponent)
	void AsyncLoadAbilityObject(TSoftObjectPtr<UObject> SoftObject);

	//Called when something has interrupted casting.
	UFUNCTION()
	void InterruptAbilityComponent();

	UFUNCTION()
	bool InterruptAbility(FAbilityInstanceHandle AbilityHandle);

	UFUNCTION()
	void OnAbilitySoftClassLoadComplete(TSubclassOf<UObject> ObjectClass);
	UFUNCTION()
	void OnAbilitySoftObjectLoadComplete(UObject* Object);

	UFUNCTION()
	bool IsHandleValid(FAbilityInstanceHandle InstanceHandle) const;

	UFUNCTION()
	void TestRequestAbility(TSubclassOf<UAbilityInfo> AbilityClass);

	//NOTE: Never add a new ability instance or target data while working with returned const ref/pointer ability instance or target data.
	inline void GetAbilityInstanceAndTargetDataByID(FAbilityInstanceHandle InstanceHandle, FAbilityTargetDataHandle TargetDataHandle, FAbilityInstanceData*& OwningAbilityInstanceData, FAbilityTargetData*& OwningAbilityTargetData);
	const FAbilityTargetData& GetAbilityTargetDataByHandle(FAbilityInstanceHandle InstanceHandle, FAbilityTargetDataHandle TargetDataHandle) const;

	const FAbilityActionList& GetTargetDataAbilityActionList(FAbilityTargetDataHandle TargetDataHandle) const;

	//Used to gate playing of montages. Makes sure a montage started by a new ability is not canceled by one started by a previous one stopping.
	void PlayAnimationMontage(UAnimMontage* Montage, FAbilityInstanceHandle InstanceHandle, float InStartTime);
	void StopAnimationMontage(UAnimMontage* Montage, FAbilityInstanceHandle InstanceHandle);

public:
	UPROPERTY(BlueprintAssignable, Category = AbilityComponent)
	FAbilityDataUpdateSignature OnAbilityDataAdded;
	UPROPERTY(BlueprintAssignable, Category = AbilityComponent)
	FAbilityDataUpdateSignature OnAbilityDataRemoved;
	UPROPERTY(BlueprintAssignable, Category = AbilityComponent)
	FAbilityDataChangeUpdateSignature OnAbilityDataChanged;


	DECLARE_EVENT_TwoParams(UAbilityComponent, FAbilityInstanceEventSignature, const UAbilityComponent*, FAbilityInstanceHandle)
	FAbilityInstanceEventSignature OnAbilityInstanceStartupBegin;
	FAbilityInstanceEventSignature OnAbilityInstanceStartupComplete;
	FAbilityInstanceEventSignature OnAbilityInstanceComplete;
	FAbilityInstanceEventSignature OnAbilityInstanceInterrupted;

	DECLARE_EVENT_ThreeParams(UAbilityComponent, FAbilityInstanceUpdateSignature, const UAbilityComponent*, FAbilityInstanceHandle, EAbilityEventType)
	FAbilityInstanceUpdateSignature OnAbilityInstanceStartupUpdate;

protected:
	inline FAbilityInstanceData* GetAbilityInstanceByHandle(FAbilityInstanceHandle InstanceHandle);

	UFUNCTION()
	void OnRep_AbilityDataList();

	UFUNCTION()
	void InitializeAbilityClass(TSubclassOf<UAbilityInfo> AbilityClass, bool bNotify = true);

	UFUNCTION()
	void OnAbilityClassInitialized(TSubclassOf<UAbilityInfo> AbilityClass);
	
	UFUNCTION()
	void RechargeTimerCompleted(UClass* AbilityClass);

	//Finds FAbilityData of a given UAbilityInfo. Can return FAbilityData::InvalidAbilityData.
	FAbilityData& GetAbilityDataByClass(TSubclassOf<UAbilityInfo> AbilityClass);
	const FAbilityData& GetAbilityDataByClass(TSubclassOf<UAbilityInfo> AbilityClass) const;

	UFUNCTION()
	void ProcessInstanceDataAdded(const FAbilityInstanceData& InstanceData);
	UFUNCTION()
	void ProcessInstanceDataRemoved(const FAbilityInstanceData& InstanceData);
	UFUNCTION()
	void ProcessInstanceDataChanged(const FAbilityInstanceData& InstanceData);

	UFUNCTION()
	void OnInstanceDataCastComplete(FAbilityInstanceHandle InstanceHandle);
	
	UFUNCTION()
	void ProcessInstanceTargetDataAdded(const FAbilityInstanceData& InstanceData, const FAbilityTargetData& AbilityTargetData);
	UFUNCTION()
	void ProcessInstanceTargetDataRemoved(const FAbilityInstanceData& InstanceData, const FAbilityTargetData& AbilityTargetData);

	//Server-side timer handling an ability instance's startup time.
	UFUNCTION()
	void OnTargetDataStartupComplete(FAbilityInstanceHandle InstanceHandle, FAbilityTargetDataHandle TargetDataID);

	//Server-side timer handling an ability instance's activation time.
	UFUNCTION()
	void OnTargetDataActivationComplete(FAbilityInstanceHandle InstanceHandle, FAbilityTargetDataHandle TargetDataID);

	//Server-side timer handling an ability's instance's deferred removal (allowing for clients to properly fully simulate even if a bit behind the server).
	UFUNCTION()
	void OnTargetDataDestructionReady(FAbilityInstanceHandle InstanceHandle, FAbilityTargetDataHandle TargetDataID);

	UFUNCTION()
	void CleanupAbilityComponent();

	UFUNCTION()
	void OnActionInterrupt(const UStatusComponent* Component);

protected:
	UPROPERTY(ReplicatedUsing = OnRep_AbilityDataList)
	TArray<FAbilityData> AbilityDataList = TArray<FAbilityData>();
	UPROPERTY()
	TMap<TSubclassOf<UAbilityInfo>, FAbilityData> PreviousAbilityDataMap = TMap<TSubclassOf<UAbilityInfo>, FAbilityData>();

	//Populated via OnRep_AbilityDataList to help fast index lookup.
	UPROPERTY()
	TMap<TSubclassOf<UAbilityInfo>, int32> AbilityDataIndexMap = TMap<TSubclassOf<UAbilityInfo>, int32>();

	UPROPERTY(Replicated)
	FAbilityInstanceContainer AbilityInstanceDataContainer = FAbilityInstanceContainer();

	//List of arbitrary object classes this ability component has loaded for use (used primarily for initialized ability's decal classes).
	UPROPERTY()
	TSet<TSubclassOf<UObject>> LoadedClassSet = TSet<TSubclassOf<UObject>>();
	UPROPERTY()
	TSet<UObject*> LoadedObjectSet = TSet<UObject*>();
	
	UPROPERTY()
	TMap<FAbilityTargetDataHandle, FAbilityObjectContainer> AbilityTargetDataObjectMap = TMap<FAbilityTargetDataHandle, FAbilityObjectContainer>();

	UPROPERTY()
	TMap<FAbilityInstanceHandle, FTimerHandle> AbilityInstanceTimerMap = TMap<FAbilityInstanceHandle, FTimerHandle>();
	UPROPERTY()
	TMap<FAbilityTargetDataHandle, FTimerHandle> AbilityTargetDataTimerMap = TMap<FAbilityTargetDataHandle, FTimerHandle>();
	
	UPROPERTY()
	TArray<UAbilityAction*> AbilityActionList = TArray<UAbilityAction*>();
	UPROPERTY()
	TMap<FAbilityTargetDataHandle, FAbilityActionList> AbilityActionMap = TMap<FAbilityTargetDataHandle, FAbilityActionList>();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = AbilityComponent)
	TArray<TSubclassOf<UAbilityInfo>> AbilityClassList = TArray<TSubclassOf<UAbilityInfo>>();

	UPROPERTY(Transient)
	TWeakObjectPtr<UStatusComponent> OwningStatusComponent = nullptr;

	UPROPERTY(Transient)
	TMap<UAnimMontage*, FAbilityInstanceHandle> LastKnownAnimationAbilityInstanceHandleMap;
};

UCLASS(Blueprintable, BlueprintType, ClassGroup = (Custom))
class NAUSEA_API UAbilityInfo : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = Ability)
	float GetCastDuration() const { return CastDuration; }

	UFUNCTION(BlueprintCallable, Category = Ability)
	float GetAbilityDuration() const { return AbilityDuration; }

	UFUNCTION(BlueprintCallable, Category = Ability)
	int32 GetInitialChargeCount() const { return InitialChargeCount; }

	UFUNCTION(BlueprintCallable, Category = Ability)
	int32 GetMaxChargeCount() const { return InitialChargeCount; }

	UFUNCTION(BlueprintCallable, Category = Ability)
	float GetRechargeDuration() const { return InitialChargeCount; }

	UFUNCTION(BlueprintCallable, Category = Ability)
	const FVector2D& GetTargetSize() const { return TargetSize; }

	UFUNCTION(BlueprintCallable, Category = Ability)
	TSoftClassPtr<UAbilityDecalComponent> GetAbilityDecalClass() const { return AbilityDecalClass; }
	UFUNCTION(BlueprintCallable, Category = Ability)
	TSoftObjectPtr<UAnimMontage> GetAbilityCastAnimMontage() const { return AbilityCastMontage; }
	UFUNCTION(BlueprintCallable, Category = Ability)
	TSoftObjectPtr<UAnimMontage> GetAbilityActivationAnimMontage() const { return AbilityActivationMontage; }
	UFUNCTION(BlueprintCallable, Category = Ability)
	bool ShouldStopCastMontageOnActivation() const { return bStopCastMontageOnActivation; }

	UFUNCTION(BlueprintCallable, Category = Ability)
	int32 GetMaxTargetCount() const { return MaxTargetCount; }

	UFUNCTION(BlueprintCallable, Category = Ability)
	bool NeedsLineOfSight() const { return bNeedLineOfSightToTarget; }
	UFUNCTION(BlueprintCallable, Category = Ability)
	ECollisionChannel GetLineOfSightCollisionChannel() const { return LineOfSightCollisionChannel; }

	UFUNCTION(BlueprintCallable, Category = Ability)
	bool HasMoveActionBrainDataObject() const { return MoveActionBrainDataObject != nullptr; }
	UFUNCTION(BlueprintCallable, Category = Ability)
	UActionBrainDataObject* CreateMoveActionBrainDataObject(UObject* Outer) const;

	UFUNCTION(BlueprintCallable, Category = Ability)
	bool HasTargetActionBrainDataObject() const { return TargetActionBrainDataObject != nullptr; }
	UFUNCTION(BlueprintCallable, Category = Ability)
	UActionBrainDataObject* CreateTargetActionBrainDataObject(UObject* Outer) const;

	UFUNCTION(BlueprintCallable, Category = Ability)
	TSubclassOf<UActionBrainComponentAction> GetMoveAndPerformAbilityClass() const { return MoveAndPerformAbilityClass; }
	UFUNCTION(BlueprintCallable, Category = Ability)
	TSubclassOf<UActionBrainComponentAction> GetPerformAbilityClass() const { return PerformAbilityClass; }

	UFUNCTION(BlueprintCallable, Category = AI)
	bool ShouldAIStopMovement() const { return bStopMovement; }
	UFUNCTION(BlueprintCallable, Category = AI)
	bool ShouldAIWaitForLanding() const { return bWaitForLanding; }
	UFUNCTION(BlueprintCallable, Category = AI)
	float GetAICastStartDelay() const { return CastStartDelay; }
	UFUNCTION(BlueprintCallable, Category = AI)
	ECompleteCondition GetAICompleteCondition() const { return CompleteCondition; }
	UFUNCTION(BlueprintCallable, Category = AI)
	bool ShouldStopMoveRequestOnBlock() const { return bStopMoveRequestOnBlock; }

	void AbilityInitializedOnComponent(UAbilityComponent* AbilityComponent) const;

	void GenerateTargetData(UAbilityComponent* AbilityComponent, const UActionBrainDataObject* DataObject, TArray<FAbilityTargetData>& TargetDataList) const;
	void GenerateTargetDataFromDataObject(const UActionBrainDataObject* DataObject, TArray<FAbilityTargetData>& TargetDataList) const;
	void GenerateTargetDataFromAbilityComponent(UAbilityComponent* AbilityComponent, TArray<FAbilityTargetData>& TargetDataList) const;

	//Called as a final pass on generated target data list.
	void ProcessTargetData(UAbilityComponent* AbilityComponent, TArray<FAbilityTargetData>& TargetDataList) const;

	//Called on server and client on each target data added/removed.
	void OnTargetDataAdded(UAbilityComponent* AbilityComponent, const FAbilityInstanceData& InstanceData, const FAbilityTargetData& InstanceTargetData) const;
	void OnTargetDataRemoved(UAbilityComponent* AbilityComponent, const FAbilityInstanceData& InstanceData, const FAbilityTargetData& InstanceTargetData) const;

	//Allows Blueprint to perform a final pass on targeting data before it's executed as an ability.
	UFUNCTION(BlueprintImplementableEvent, Category = Ability, meta = (DisplayName = "Process Target Data"))
	void K2_ProcessTargetData(UAbilityComponent* AbilityComponent, TArray<FAbilityTargetData>& InstanceTargetData) const;

	//Batched target data handling for Blueprint. Stops needing to constantly hop between native and Blueprint per check.
	UFUNCTION(BlueprintImplementableEvent, Category = Ability, meta = (DisplayName = "On Target Data"))
	void K2_OnTargetDataAdded(UAbilityComponent* AbilityComponent, const FAbilityInstanceData& InstanceData, const TArray<FAbilityTargetData>& InstanceTargetData) const;
	UFUNCTION(BlueprintImplementableEvent, Category = Ability, meta = (DisplayName = "On Target Data"))
	void K2_OnTargetDataRemoved(UAbilityComponent* AbilityComponent, const FAbilityInstanceData& InstanceData, const TArray<FAbilityTargetData>& InstanceTargetData) const;

	void OnAbilityTargetStartup(UAbilityComponent* AbilityComponent, const FAbilityInstanceData& InstanceData, const FAbilityTargetData& InstanceTargetData) const;
	void OnAbilityTargetActivation(UAbilityComponent* AbilityComponent, const FAbilityInstanceData& InstanceData, const FAbilityTargetData& InstanceTargetData) const;

	bool CanTargetActor(UAbilityComponent* AbilityComponent, const FAbilityTargetData& AbilityTargetData, AActor* Target, float WorldTimeSeconds) const;

	bool PlayAbilityCast(UAbilityComponent* AbilityComponent, const FAbilityInstanceData& InstanceData, float WorldTime, float CastStartWorldTime) const;
	bool StopPlayAbilityCast(UAbilityComponent* AbilityComponent, const FAbilityInstanceData& InstanceData, float WorldTimeSeconds) const;
	bool PlayAbilityActivation(UAbilityComponent* AbilityComponent, const FAbilityInstanceData& InstanceData, float WorldTime, float ActivationStartWorldTime) const;
	bool StopPlayAbilityActivation(UAbilityComponent* AbilityComponent, const FAbilityInstanceData& InstanceData, float WorldTimeSeconds) const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = Ability)
	float CastDuration = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = Ability)
	int32 InitialChargeCount = 0;
	UPROPERTY(EditDefaultsOnly, Category = Ability)
	int32 MaxChargeCount = 1;
	UPROPERTY(EditDefaultsOnly, Category = Ability)
	float RechargeDuration = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = Ability)
	FVector2D TargetSize = FVector2D(-1.f);
	UPROPERTY(EditDefaultsOnly, Category = Ability)
	FVector2D TargetSizeVariance = FVector2D(1.f);

	//How long the target area will exist for once casted.
	UPROPERTY(EditDefaultsOnly, Category = Ability)
	float AbilityDuration = -1.f;

	UPROPERTY(EditDefaultsOnly, Category = Ability)
	int32 MaxTargetCount = 1;

	UPROPERTY(EditDefaultsOnly, Category = Ability)
	TSoftClassPtr<UAbilityDecalComponent> AbilityDecalClass = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = Ability)
	TSoftObjectPtr<UAnimMontage> AbilityCastMontage = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = Ability)
	TSoftObjectPtr<UAnimMontage> AbilityActivationMontage = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = Ability)
	bool bStopCastMontageOnActivation = true;

	UPROPERTY(EditDefaultsOnly, Instanced, Category = Ability)
	TArray<UAbilityAction*> StartupAbilityActions;

	UPROPERTY(EditDefaultsOnly, Instanced, Category = Ability)
	TArray<UAbilityAction*> ActivationAbilityActions;

	UPROPERTY(EditDefaultsOnly, Category = Ability)
	bool bNeedLineOfSightToTarget = false;
	UPROPERTY(EditDefaultsOnly, Category = Action)
	TEnumAsByte<ECollisionChannel> LineOfSightCollisionChannel = ECC_AbilityTrace;

	UPROPERTY(EditDefaultsOnly, Category = Ability)
	float AbilityRange = -1.f;

	UPROPERTY(EditDefaultsOnly, Category = Ability)
	TSubclassOf<UActionBrainComponentAction> MoveAndPerformAbilityClass = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = Ability)
	TSubclassOf<UActionBrainComponentAction> PerformAbilityClass = nullptr;

	UPROPERTY(EditDefaultsOnly, Instanced, Category = AI)
	UActionBrainDataObject* MoveActionBrainDataObject = nullptr;
	UPROPERTY(EditDefaultsOnly, Instanced, Category = AI)
	UActionBrainDataObject* TargetActionBrainDataObject = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = AI)
	bool bStopMovement = true;
	UPROPERTY(EditDefaultsOnly, Category = AI)
	bool bWaitForLanding = true;
	UPROPERTY(EditDefaultsOnly, Category = AI)
	float CastStartDelay = 0.f;
	UPROPERTY(EditDefaultsOnly, Category = AI)
	ECompleteCondition CompleteCondition = ECompleteCondition::CastComplete;
	UPROPERTY(EditDefaultsOnly, Category = AI)
	bool bStopMoveRequestOnBlock = false;

public:
	static const FVector2D& GetAbilityTargetDataStartupTime(const FAbilityInstanceData& InstanceData, const FAbilityTargetData& TargetData);
	static const FVector2D& GetAbilityTargetDataActivationTime(const FAbilityInstanceData& InstanceData, const FAbilityTargetData& TargetData);
	static const FVector2D& GetAbilityTargetDataTargetSize(const FAbilityInstanceData& InstanceData, const FAbilityTargetData& TargetData);

	UFUNCTION(BlueprintCallable, Category = Ability, meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
	static FVector2D GetAdjustedAbilityTargetDataStartupTime(const UObject* WorldContextObject, const FAbilityInstanceData& InstanceData, const FAbilityTargetData& TargetData);

	UFUNCTION(BlueprintCallable, Category = Ability, meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
	static FVector2D GetAdjustedAbilityTargetDataActivationTime(const UObject* WorldContextObject, const FAbilityInstanceData& InstanceData, const FAbilityTargetData& TargetData);

	UFUNCTION(BlueprintCallable, Category = Ability, meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
	static FVector2D GetAdjustedAbilityTargetDataTargetSize(const UObject* WorldContextObject, const FAbilityInstanceData& InstanceData, const FAbilityTargetData& TargetData);
};