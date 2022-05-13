// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "AI/EnemyPhaseComponent.h"
#include "System/NetHelper.h"

UEnemyPhaseComponent::UEnemyPhaseComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UEnemyPhaseComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_WITH_PARAMS_FAST(UEnemyPhaseComponent, CurrentPhase, PushReplicationParams::Default);
}

void UEnemyPhaseComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UEnemyPhaseComponent::SetPhase(int32 NewPhase)
{
	if (NewPhase == CurrentPhase)
	{
		return;
	}

	if (bIgnoreSetToPreviousPhase && NewPhase < CurrentPhase)
	{
		return;
	}

	const int32 PreviousPhase = CurrentPhase;
	CurrentPhase = NewPhase;
	OnPhaseSet(NewPhase, PreviousPhase);
	OnRep_CurrentPhase(PreviousPhase);
	MARK_PROPERTY_DIRTY_FROM_NAME(UEnemyPhaseComponent, CurrentPhase, this);
}

void UEnemyPhaseComponent::OnRep_CurrentPhase(int32 PreviousPhase)
{
	OnPhaseUpdate(GetCurrentPhase(), PreviousPhase);
}