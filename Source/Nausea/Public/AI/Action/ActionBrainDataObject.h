// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "LatentActions.h"
#include "Engine/LatentActionManager.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "ActionBrainDataObject.generated.h"

class AAIController;

UENUM(BlueprintType)
enum class EDataSelectionMode : uint8
{
	ActorOnly, //Will only provide actor list. Location list will always be 0 sized.
	LocationOnly, //Will only provide location list. Actor list will always be 0 sized.
	PreferActor, //Will only provide actor list if non 0 sized. If actor list is 0 sized, location list will be provided.
	PreferLocation, //Will only provide location list if non 0 sized. If location list is 0 sized, actor list will be provided.
	Both //Will provide both actor and location list.
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FActionBrainDataObjectReady, UActionBrainDataObject*, ActionBrainDataObject);

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class NAUSEA_API UActionBrainDataObject : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	//Called on UActionBrainComponentAction::Activate before an owning action is started.
	UFUNCTION()
	virtual void Initialize(AAIController* InOwningController) { OwningController = InOwningController; }
	UFUNCTION()
	virtual void CleanUp();

	//Used to update cached data, if relevant. Called on UActionBrainComponentAction::Activate after UActionBrainDataObject::Activate. Can be called by other actions if an update is relevant.
	UFUNCTION(BlueprintCallable, Category = ActionDataObject)
	virtual void UpdateDataObject() {}

	//Used to notify this data object that the entity that has queried it no longer wants it.
	UFUNCTION(BlueprintCallable, Category = ActionDataObject)
	virtual void AbortRequest() {}
	
	UFUNCTION(BlueprintCallable, Category = ActionDataObject)
	AAIController* GetOwningController() const { return OwningController; }

	//Gets actor from data actor list. Providing an invalid index will return a random element from the list.
	UFUNCTION(BlueprintCallable, Category = ActionDataObject)
	virtual AActor* GetActor(int32 Index = 0) const;
	//Gets location from data location list. Providing an invalid index will return a random element from the list.
	UFUNCTION(BlueprintCallable, Category = ActionDataObject)
	virtual FVector GetLocation(int32 Index = 0) const;

	UFUNCTION(BlueprintCallable, Category = ActionDataObject)
	virtual void GetListOfActors(TArray<AActor*>& ActorList) const;
	UFUNCTION(BlueprintCallable, Category = ActionDataObject)
	virtual void GetListOfLocations(TArray<FVector>& LocationList) const;

	UFUNCTION(BlueprintImplementableEvent, Category = ActionDataObject)
	void K2_GetListOfActors(TArray<AActor*>& ActorList) const;
	UFUNCTION(BlueprintImplementableEvent, Category = ActionDataObject)
	void K2_GetListOfLocations(TArray<FVector>& LocationList) const;

	UFUNCTION(BlueprintCallable, Category = ActionDataObject, meta = (Latent, LatentInfo = "LatentInfo"))
	void WaitForDataObject(struct FLatentActionInfo LatentInfo);

	UFUNCTION(BlueprintCallable, Category = ActionDataObject)
	bool IsReady() const { return bIsReady; }
	
	UFUNCTION(BlueprintCallable, Category = ActionDataObject)
	FString GetDebugInfoString() const;

	UFUNCTION(BlueprintCallable, Category = ActionDataObject)
	EDataSelectionMode GetSelectionMode() const { return SelectionMode; }

	static FString GetSelectionModeString(EDataSelectionMode InSelectionMode);

protected:
	UFUNCTION(BlueprintCallable, Category = ActionDataObject)
	void SetReady();

public:
	UPROPERTY()
	FActionBrainDataObjectReady OnActionBrainDataReady;

protected:
	UPROPERTY()
	AAIController* OwningController = nullptr;
	UPROPERTY()
	bool bIsReady = true;

	UPROPERTY(EditDefaultsOnly, Category = ActionDataObject)
	EDataSelectionMode SelectionMode = EDataSelectionMode::PreferActor;
};

//We don't support blackboard so we're omitting that.
USTRUCT(BlueprintType)
struct NAUSEA_API FSimpleAIDynamicParam
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = EQS)
	FName ParamName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = EQS)
	EAIParamType ParamType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = EQS)
	float Value;

	FSimpleAIDynamicParam()
	{
		ParamType = EAIParamType::Float;
		Value = 0.f;
	}

	FSimpleAIDynamicParam(const FAIDynamicParam& Other)
	{
		ParamName = Other.ParamName;
		ParamType = Other.ParamType;
		Value = Other.Value;
	}
};


UCLASS()
class NAUSEA_API UActionBrainDataObject_EQS : public UActionBrainDataObject
{
	GENERATED_UCLASS_BODY()

//~ Begin UObject Interface
#if WITH_EDITOR
protected:
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif //WITH_EDITOR
//~ End UObject Interface

//~ Begin UActionBrainDataObject Interface
public:
	virtual void Initialize(AAIController* InOwningController) override;
	virtual void CleanUp() override;
	virtual void UpdateDataObject() override;
	virtual void AbortRequest() override;
	virtual void GetListOfActors(TArray<AActor*>& ActorList) const override;
	virtual void GetListOfLocations(TArray<FVector>& LocationList) const override;
//~ Begin UActionBrainDataObject Interface

public:
	int32 Execute(AActor* QueryOwner, FQueryFinishedSignature& InQueryFinishedDelegate) const;

protected:
	void OnQueryComplete(TSharedPtr<FEnvQueryResult> Result);

protected:
	UPROPERTY(Category = Node, EditAnywhere)
	UEnvQuery* QueryTemplate;
	
	UPROPERTY(Category = Node, EditAnywhere)
	TEnumAsByte<EEnvQueryRunMode::Type> RunMode;

	UPROPERTY(Category = Node, EditAnywhere)
	TArray<FSimpleAIDynamicParam> QueryConfig;

	FQueryFinishedSignature QueryFinishedDelegate;

	TSharedPtr<FEnvQueryResult> QueryResult;

	UPROPERTY(Transient)
	int32 QueryID = INDEX_NONE;
};

class FWaitOnDataObjectLatentAction : public FPendingLatentAction
{
	FName ExecutionFunction;
	int32 OutputLink;
	FWeakObjectPtr CallbackTarget;
	TWeakObjectPtr<UActionBrainDataObject> OwningActionBrainData;

public:
	FWaitOnDataObjectLatentAction(UActionBrainDataObject* OwningActionBrainData, const FLatentActionInfo& LatentInfo)
		: ExecutionFunction(LatentInfo.ExecutionFunction)
		, OutputLink(LatentInfo.Linkage)
		, CallbackTarget(LatentInfo.CallbackTarget)
		, OwningActionBrainData(TWeakObjectPtr<UActionBrainDataObject>(OwningActionBrainData))
	{
	}

	virtual void UpdateOperation(FLatentResponse& Response) override
	{
		if (!OwningActionBrainData.IsValid())
		{
			UE_LOG(LogTemp, Error, TEXT("FWaitOnDataObjectLatentAction completed with invalid OwningActionBrainData."));
			Response.FinishAndTriggerIf(true, ExecutionFunction, OutputLink, CallbackTarget);
			return;
		}

		Response.FinishAndTriggerIf(OwningActionBrainData->IsReady(), ExecutionFunction, OutputLink, CallbackTarget);
	}
};