// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Gameplay/StatusInterface.h"
#include "Gameplay/StatusComponent.h"

DECLARE_CYCLE_STAT(TEXT("Requesting UStatusInterfaceStatics::RemoveDeadData call from within a const function"),
	STAT_FSimpleDelegateGraphTask_RequestingRemovalOfDeadStatusComponentData,
	STATGROUP_TaskGraphTasks);

UStatusInterface::UStatusInterface(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UStatusInterfaceStatics::UStatusInterfaceStatics(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UStatusComponent* UStatusInterfaceStatics::GetStatusComponent(TScriptInterface<IStatusInterface> Target)
{
	//Native interfaces should leverage the native implementation.
	if (IStatusInterface* NativeStatusInterface = Cast<IStatusInterface>(Target.GetObject()))
	{
		return NativeStatusInterface->GetStatusComponent();
	}

	UStatusInterfaceStatics* StatusInterfaceStaticCDO = UStatusInterfaceStatics::StaticClass()->GetDefaultObject<UStatusInterfaceStatics>();
	
	if (!StatusInterfaceStaticCDO)
	{
		return TSCRIPTINTERFACE_CALL_K2_FUNC_RET(Target, K2_GetStatusComponent, nullptr);
	}

	if (StatusInterfaceStaticCDO->CachedStatusComponentMap.Contains(Target.GetObject()))
	{
		return StatusInterfaceStaticCDO->CachedStatusComponentMap[Target.GetObject()].Get();
	}

	UStatusComponent* StatusComponent = TSCRIPTINTERFACE_CALL_K2_FUNC_RET(Target, K2_GetStatusComponent, nullptr);
	StatusInterfaceStaticCDO->CachedStatusComponentMap.Add(Target.GetObject()) = StatusComponent;

	FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
		FSimpleDelegateGraphTask::FDelegate::CreateUObject(StatusInterfaceStaticCDO, &UStatusInterfaceStatics::RemoveDeadData),
		GET_STATID(STAT_FSimpleDelegateGraphTask_RequestingRemovalOfDeadStatusComponentData), nullptr, ENamedThreads::GameThread);

	return StatusComponent;
}

bool UStatusInterfaceStatics::IsDead(TScriptInterface<IStatusInterface> Target)
{
	if (UStatusComponent* StatusComponent = GetStatusComponent(Target))
	{
		return StatusComponent->IsDead();
	}

	return false;
}

void UStatusInterfaceStatics::Kill(TScriptInterface<IStatusInterface> Target, float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (UStatusComponent* StatusComponent = GetStatusComponent(Target))
	{
		StatusComponent->Kill(Damage, DamageEvent, EventInstigator, DamageCauser);
	}
}

void UStatusInterfaceStatics::HealDamage(TScriptInterface<IStatusInterface> Target, float Amount, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (UStatusComponent* StatusComponent = GetStatusComponent(Target))
	{
		StatusComponent->HealDamage(Amount, DamageEvent, EventInstigator, DamageCauser);
	}
}

void UStatusInterfaceStatics::RemoveDeadData()
{
	TArray<TObjectKey<UObject>> DeadKeys = TArray<TObjectKey<UObject>>();
	for (const TPair<TObjectKey<UObject>, TWeakObjectPtr<UStatusComponent>>& Entry : CachedStatusComponentMap)
	{
		if (Entry.Key.ResolveObjectPtr() != nullptr)
		{
			continue;
		}

		DeadKeys.Add(Entry.Key);
	}

	for (const TObjectKey<UObject>& Key : DeadKeys)
	{
		CachedStatusComponentMap.Remove(Key);
	}
}