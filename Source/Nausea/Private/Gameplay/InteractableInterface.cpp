// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Gameplay/InteractableInterface.h"
#include "Gameplay/InteractableComponent.h"

DECLARE_CYCLE_STAT(TEXT("Requesting UInteractableInterfaceStatics::RemoveDeadData call from within a const function"),
	STAT_FSimpleDelegateGraphTask_RequestingRemovalOfDeadInteractableData,
	STATGROUP_TaskGraphTasks);

UInteractableInterface::UInteractableInterface(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UInteractableInterfaceStatics::UInteractableInterfaceStatics(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UInteractableComponent* UInteractableInterfaceStatics::GetInteractableComponent(TScriptInterface<IInteractableInterface> Target)
{
	UInteractableInterfaceStatics* InteractableInterfaceStaticCDO = UInteractableInterfaceStatics::StaticClass()->GetDefaultObject<UInteractableInterfaceStatics>();

	if (!InteractableInterfaceStaticCDO)
	{
		return TSCRIPTINTERFACE_CALL_FUNC_RET(Target, GetInteractableComponent, K2_GetInteractableComponent, nullptr);
	}

	static auto ContainsDeadData = [](const TMap<TObjectKey<UObject>, TWeakObjectPtr<UInteractableComponent>>& Map)->bool
	{
		for (TMap<TObjectKey<UObject>, TWeakObjectPtr<UInteractableComponent>>::TConstIterator DataIt = TMap<TObjectKey<UObject>, TWeakObjectPtr<UInteractableComponent>>::TConstIterator(Map); DataIt; ++DataIt)
		{
			if (DataIt->Key.ResolveObjectPtr() == nullptr)
			{
				return true;
			}

			if (!DataIt->Value.IsValid())
			{
				return true;
			}
		}

		return false;
	};

	if (ContainsDeadData(InteractableInterfaceStaticCDO->CachedInteractableComponentMap))
	{
		FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
			FSimpleDelegateGraphTask::FDelegate::CreateUObject(InteractableInterfaceStaticCDO, &UInteractableInterfaceStatics::RemoveDeadData),
			GET_STATID(STAT_FSimpleDelegateGraphTask_RequestingRemovalOfDeadInteractableData), nullptr, ENamedThreads::GameThread);
	}

	if (InteractableInterfaceStaticCDO->CachedInteractableComponentMap.Contains(Target.GetObject()) && InteractableInterfaceStaticCDO->CachedInteractableComponentMap[Target.GetObject()].IsValid())
	{
		return InteractableInterfaceStaticCDO->CachedInteractableComponentMap[Target.GetObject()].Get();
	}

	UInteractableComponent* InteractableComponent = TSCRIPTINTERFACE_CALL_FUNC_RET(Target, GetInteractableComponent, K2_GetInteractableComponent, nullptr);
	InteractableInterfaceStaticCDO->CachedInteractableComponentMap.Add(Target.GetObject()) = InteractableComponent;
	return InteractableComponent;
}

bool UInteractableInterfaceStatics::CanInteract(TScriptInterface<IInteractableInterface> Target, UPawnInteractionComponent* InstigatorComponent, const FVector& Location, const FVector& Direction)
{
	return TSCRIPTINTERFACE_CALL_FUNC_RET(Target, CanInteract, K2_CanInteract, false, InstigatorComponent, Location, Direction);
}

void UInteractableInterfaceStatics::RemoveDeadData()
{
	TArray<TObjectKey<UObject>> DeadKeys = TArray<TObjectKey<UObject>>();
	for (const TPair<TObjectKey<UObject>, TWeakObjectPtr<UInteractableComponent>>& Entry : CachedInteractableComponentMap)
	{
		if (Entry.Key.ResolveObjectPtr() != nullptr)
		{
			continue;
		}

		DeadKeys.Add(Entry.Key);
	}

	for (const TObjectKey<UObject>& Key : DeadKeys)
	{
		CachedInteractableComponentMap.Remove(Key);
	}
}