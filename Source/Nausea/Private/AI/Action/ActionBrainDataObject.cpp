// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.


#include "AI/Action/ActionBrainDataObject.h"
#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "Engine/Engine.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"
#include "EnvironmentQuery/EnvQueryTypes.h"

UActionBrainDataObject::UActionBrainDataObject(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UActionBrainDataObject::CleanUp()
{
	OwningController = nullptr;
	MarkPendingKill();
}

AActor* UActionBrainDataObject::GetActor(int32 Index) const
{
	TArray<AActor*> ActorList;
	GetListOfActors(ActorList);

	if (ActorList.Num() == 0)
	{
		return nullptr;
	}

	if (ActorList.Num() == 1)
	{
		return ActorList[0];
	}

	if (ActorList.IsValidIndex(Index))
	{
		return ActorList[Index];
	}
	else
	{
		return ActorList[FMath::RandHelper(ActorList.Num())];
	}
}

FVector UActionBrainDataObject::GetLocation(int32 Index) const
{
	TArray<FVector> VectorList;
	GetListOfLocations(VectorList);

	if (VectorList.Num() == 0)
	{
		return FAISystem::InvalidLocation;
	}

	if (VectorList.Num() == 1)
	{
		return VectorList[0];
	}

	if (VectorList.IsValidIndex(Index))
	{
		return VectorList[Index];
	}
	else
	{
		return VectorList[FMath::RandHelper(VectorList.Num())];
	}
}

void UActionBrainDataObject::GetListOfActors(TArray<AActor*>& ActorList) const
{
	ActorList.Empty(ActorList.Num());
	switch (SelectionMode)
	{
	case EDataSelectionMode::LocationOnly:
		return;
	case EDataSelectionMode::PreferLocation:
		TArray<FVector> LocationList;
		GetListOfLocations(LocationList);
		if (LocationList.Num() > 0) { return; }
	}

	K2_GetListOfActors(ActorList);
}

void UActionBrainDataObject::GetListOfLocations(TArray<FVector>& LocationList) const
{
	LocationList.Empty(LocationList.Num());
	switch (SelectionMode)
	{
	case EDataSelectionMode::ActorOnly:
		return;
	case EDataSelectionMode::PreferActor:
		TArray<AActor*> ActorList;
		GetListOfActors(ActorList);
		if (ActorList.Num() > 0) { return; }
	}

	K2_GetListOfLocations(LocationList);
}

void UActionBrainDataObject::WaitForDataObject(struct FLatentActionInfo LatentInfo)
{
	UWorld* World = GEngine->GetWorldFromContextObject(this, EGetWorldErrorMode::LogAndReturnNull);

	if (!World)
	{
		return;
	}

	FLatentActionManager& LatentActionManager = World->GetLatentActionManager();

	ensure(LatentActionManager.FindExistingAction<FWaitOnDataObjectLatentAction>(LatentInfo.CallbackTarget, LatentInfo.UUID) == nullptr);

	LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID, new FWaitOnDataObjectLatentAction(this, LatentInfo));
}

FString UActionBrainDataObject::GetDebugInfoString() const
{
	if (!IsReady())
	{
		return FString::Printf(TEXT("%s {yellow}[Not Ready]{white}"), *GetSelectionModeString(GetSelectionMode()));
	}

	TArray<AActor*> ActorList;
	TArray<FVector> LocationList;
	GetListOfActors(ActorList);
	GetListOfLocations(LocationList);

	FString PropertyList;
	if (ActorList.Num() == 0 && LocationList.Num() == 0)
	{
		PropertyList = "{red}[No Results]";
	}
	else if (ActorList.Num() > 0)
	{
		PropertyList += "[";

		for (AActor* Actor : ActorList)
		{
			if (!Actor)
			{
				continue;
			}

			PropertyList += Actor->GetName();

			if (Actor != ActorList.Last())
			{
				PropertyList += " | ";
			}
		}

		PropertyList += "]";
	}
	else if (LocationList.Num() > 0)
	{
		PropertyList += "[";

		for (const FVector& Location : LocationList)
		{
			if (Location == FAISystem::InvalidLocation)
			{
				continue;
			}

			PropertyList += Location.ToString();

			if (Location != LocationList.Last())
			{
				PropertyList += " | ";
			}
		}

		PropertyList += "]";
	}

	return FString::Printf(TEXT("%s %s{white}"), *GetSelectionModeString(GetSelectionMode()), *PropertyList);
}

FString UActionBrainDataObject::GetSelectionModeString(EDataSelectionMode InSelectionMode)
{
	switch (InSelectionMode)
	{
	case EDataSelectionMode::ActorOnly:
		return "Actor Only";
	case EDataSelectionMode::LocationOnly:
		return "Location Only";
	case EDataSelectionMode::PreferActor:
		return "Prefer Actor";
	case EDataSelectionMode::PreferLocation:
		return "Prefer Location";
	case EDataSelectionMode::Both:
		return "Both";
	}

	return "Invalid";
}

void UActionBrainDataObject::SetReady()
{
	if (!bIsReady)
	{
		bIsReady = true;
		OnActionBrainDataReady.Broadcast(this);
	} 
}

UActionBrainDataObject_EQS::UActionBrainDataObject_EQS(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsReady = false;
}

inline void FromComplexToSimple(const TArray<FAIDynamicParam>& ComplexList, TArray<FSimpleAIDynamicParam>& SimpleList)
{
	SimpleList.Reserve(ComplexList.Num());
	for (const FAIDynamicParam& Complex : ComplexList)
	{
		SimpleList.Add(FSimpleAIDynamicParam(Complex));
	}
}

inline void FromSimpleToComplex(const TArray<FSimpleAIDynamicParam>& SimpleList, TArray<FAIDynamicParam>& ComplexList)
{
	ComplexList.Reserve(SimpleList.Num());
	for (const FSimpleAIDynamicParam& Simple : SimpleList)
	{
		FAIDynamicParam Param;
		Param.ParamName = Simple.ParamName;
		Param.ParamType = Simple.ParamType;
		Param.Value = Simple.Value;
		ComplexList.Add(Param);
	}
}

#if WITH_EDITOR
void UActionBrainDataObject_EQS::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	check(PropertyChangedEvent.MemberProperty);
	check(PropertyChangedEvent.Property);

	if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(FEQSParametrizedQueryExecutionRequest, QueryTemplate))
	{
		if (QueryTemplate)
		{
			TArray<FAIDynamicParam> TemplateQueryConfig;
			QueryTemplate->CollectQueryParams(*this, TemplateQueryConfig);
			FromComplexToSimple(TemplateQueryConfig, QueryConfig);
		}
		else
		{
			QueryConfig.Reset();
		}
	}
}
#endif //WITH_EDITOR

void UActionBrainDataObject_EQS::Initialize(AAIController* InOwningController)
{
	if (OwningController)
	{
		return;
	}

	Super::Initialize(InOwningController);

	QueryFinishedDelegate = FQueryFinishedSignature::CreateUObject(this, &UActionBrainDataObject_EQS::OnQueryComplete);
}

void UActionBrainDataObject_EQS::CleanUp()
{
	if (QueryFinishedDelegate.IsBound())
	{
		QueryFinishedDelegate.Unbind();
	}
	
	if (QueryResult.IsValid())
	{
		QueryResult.Reset();
	}
	
	Super::CleanUp();
}

void UActionBrainDataObject_EQS::UpdateDataObject()
{
	bIsReady = false;
	
	if (QueryResult.IsValid())
	{
		QueryResult.Reset();
	}

	QueryID = INDEX_NONE;

	if (GetOwningController())
	{
		if (GetOwningController()->GetPawn())
		{
			QueryID = Execute(GetOwningController()->GetPawn(), QueryFinishedDelegate);
		}
		else
		{
			QueryID = Execute(GetOwningController(), QueryFinishedDelegate);
		}
	}
}

void UActionBrainDataObject_EQS::AbortRequest()
{
	if (QueryID == INDEX_NONE)
	{
		return;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(this, EGetWorldErrorMode::ReturnNull);

	if (!World)
	{
		return;
	}

	UEnvQueryManager* EnvQueryManager = UEnvQueryManager::GetCurrent(World);
	
	if (!EnvQueryManager)
	{
		return;
	}

	EnvQueryManager->AbortQuery(QueryID);
}

template <typename InElementType>
inline void PickRandomEntry(TArray<InElementType>& InArray)
{
	if (InArray.Num() == 1)
	{
		return;
	}

	InArray[0] = InArray[FMath::RandHelper(InArray.Num())];
	InArray.SetNum(1);
}

template <typename InElementType>
inline void ProcessListForRunMode(TEnumAsByte<EEnvQueryRunMode::Type> RunMode, TArray<InElementType>& InArray)
{
	if (InArray.Num() <= 1)
	{
		return;
	}

	switch (RunMode)
	{
	case EEnvQueryRunMode::SingleResult:
		InArray.SetNum(1);
		break;
	case EEnvQueryRunMode::RandomBest5Pct:
		InArray.SetNum(FMath::Min(1, FMath::RoundToInt(float(InArray.Num()) * 0.05f)));
		PickRandomEntry(InArray);
		break;
	case EEnvQueryRunMode::RandomBest25Pct:
		InArray.SetNum(FMath::Min(1, FMath::RoundToInt(float(InArray.Num()) * 0.25f)));
		PickRandomEntry(InArray);
		break;
	case EEnvQueryRunMode::AllMatching:
		break;
	}
}

void UActionBrainDataObject_EQS::GetListOfActors(TArray<AActor*>& ActorList) const
{
	ActorList.Empty(ActorList.Num());
	if (!QueryResult.IsValid() || QueryResult->GetRawStatus() != EEnvQueryStatus::Success)
	{
		return;
	}

	switch (SelectionMode)
	{
	case EDataSelectionMode::LocationOnly:
		return;
	case EDataSelectionMode::PreferLocation:
		TArray<FVector> LocationList;
		GetListOfLocations(LocationList);
		if (LocationList.Num() > 0) { return; }
	}

	QueryResult->GetAllAsActors(ActorList);
	ProcessListForRunMode(RunMode, ActorList);
}

void UActionBrainDataObject_EQS::GetListOfLocations(TArray<FVector>& LocationList) const
{
	LocationList.Empty(LocationList.Num());
	if (!QueryResult.IsValid() || QueryResult->GetRawStatus() != EEnvQueryStatus::Success)
	{
		return;
	}

	switch (SelectionMode)
	{
	case EDataSelectionMode::ActorOnly:
		return;
	case EDataSelectionMode::PreferActor:
		TArray<AActor*> ActorList;
		GetListOfActors(ActorList);
		if (ActorList.Num() > 0) { return; }
	}

	QueryResult->GetAllAsLocations(LocationList);
	ProcessListForRunMode(RunMode, LocationList);
}

int32 UActionBrainDataObject_EQS::Execute(AActor* QueryOwner, FQueryFinishedSignature& InQueryFinishedDelegate) const
{
	if (!QueryTemplate || !QueryOwner)
	{
		return INDEX_NONE;
	}

	FEnvQueryRequest QueryRequest(QueryTemplate, QueryOwner);

	for (const FSimpleAIDynamicParam& RuntimeParam : QueryConfig)
	{
		switch (RuntimeParam.ParamType)
		{
		case EAIParamType::Float:
			QueryRequest.SetFloatParam(RuntimeParam.ParamName, RuntimeParam.Value);
			break;
		case EAIParamType::Int:
			QueryRequest.SetIntParam(RuntimeParam.ParamName, RuntimeParam.Value);
			break;
		case EAIParamType::Bool:
			{
				bool Result = RuntimeParam.Value > 0;
				QueryRequest.SetBoolParam(RuntimeParam.ParamName, Result);
			}
			break;
		default:
			checkNoEntry();
			break;
		}
	}

	return QueryRequest.Execute(RunMode, InQueryFinishedDelegate);
}

void UActionBrainDataObject_EQS::OnQueryComplete(TSharedPtr<FEnvQueryResult> Result)
{
	QueryResult = Result;
	SetReady();
}