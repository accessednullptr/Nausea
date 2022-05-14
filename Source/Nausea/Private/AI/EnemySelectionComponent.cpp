// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "AI/EnemySelectionComponent.h"
#include "Engine/Public/EngineUtils.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "AI/CoreAIController.h"
#include "AI/EnemySelection/AITargetInterface.h"
#include "Character/CoreCharacter.h"

UEnemySelectionComponent::UEnemySelectionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UEnemySelectionComponent::OnPawnUpdated(ACoreAIController* AIController, ACoreCharacter* InCharacter)
{
	if (!InCharacter)
	{
		ClearEnemy(true);
		CurrentCharacter = nullptr;
	}
}

AActor* UEnemySelectionComponent::GetEnemy() const
{
	return Cast<AActor>(CurrentEnemy.GetObject());
}

AActor* UEnemySelectionComponent::FindBestEnemy() const
{
	if (!GetOwningCharacter())
	{
		return nullptr;
	}

	float ClosestDistanceSq = MAX_FLT;
	AActor* ClosestActor = nullptr;

	//Default behaviour is to just find closest pawn to us.
	for (TActorIterator<APawn> Itr(GetWorld()); Itr; ++Itr)
	{
		APawn* Pawn = *Itr;

		if (ETeamAttitude::Type::Hostile != FGenericTeamId::GetAttitude(GetOwner(), Pawn))
		{
			continue;
		}

		const float DistanceSq = GetOwningCharacter()->GetSquaredDistanceTo(Pawn);

		if (DistanceSq >= ClosestDistanceSq)
		{
			continue;
		}

		ClosestDistanceSq = DistanceSq;
		ClosestActor = Pawn;
	}

	return ClosestActor;
}


IAITargetInterface* UEnemySelectionComponent::GetEnemyInterface() const
{
	if (!CurrentEnemy.GetObject())
	{
		return nullptr;
	}

	return (IAITargetInterface*)CurrentEnemy.GetInterface();
}

bool UEnemySelectionComponent::SetEnemy(AActor* NewEnemy, bool bForce)
{
	if (!NewEnemy)
	{
		return ClearEnemy(bForce);
	}

	if (GetEnemy() == NewEnemy)
	{
		return true;
	}

	if (!bForce)
	{
		if (!CanChangeEnemy())
		{
			return false;
		}
		
		if (!CanTargetEnemy(NewEnemy))
		{
			return false;
		}
	}

	AActor* PreviousEnemy = GetEnemy();
	CurrentEnemy = NewEnemy;
	EnemyChanged(PreviousEnemy);
	return true;
}

bool UEnemySelectionComponent::ClearEnemy(bool bForce)
{
	if (!bForce)
	{
		if (!CanChangeEnemy())
		{
			return false;
		}
	}

	AActor* PreviousEnemy = GetEnemy();
	CurrentEnemy = nullptr;
	EnemyChanged(PreviousEnemy);
	return true;
}

bool UEnemySelectionComponent::CanChangeEnemy() const
{
	if (IsEnemySelectionLocked())
	{
		return false;
	}

	if (GetEnemy() && IsEnemyChangeCooldownActive())
	{
		return false;
	}

	return true;
}

bool UEnemySelectionComponent::CanTargetEnemy(AActor* NewEnemy) const
{
	IAITargetInterface* AITargetInterface = Cast<IAITargetInterface>(NewEnemy);

	if (!AITargetInterface)
	{
		return false;
	}

	if (!AITargetInterface->IsTargetable(GetOwner()))
	{
		return false;
	}

	return true;
}

void UEnemySelectionComponent::EnemyChanged(AActor* PreviousEnemy)
{
	if (bEnemyChangeCooldown)
	{
		GetWorld()->GetTimerManager().SetTimer(EnemyCooldownTimerHandle, EnemyChangeCooldown, false);
	}

	TScriptInterface<IAITargetInterface> PreviousEnemyInterface = PreviousEnemy;
	if (IAITargetInterface* PreviousTargetInterface = Cast<IAITargetInterface>(PreviousEnemy))
	{
		PreviousTargetInterface->GetTargetableStateChangedDelegate().RemoveDynamic(this, &UEnemySelectionComponent::OnEnemyTargetableStateChanged);
	}
	TSCRIPTINTERFACE_CALL_FUNC(PreviousEnemyInterface, OnEndTarget, K2_OnEndTarget, GetOwner());

	TScriptInterface<IAITargetInterface> CurrentEnemyInterface = CurrentEnemy;
	if (IAITargetInterface* NewTargetInterface = GetEnemyInterface())
	{
		NewTargetInterface->GetTargetableStateChangedDelegate().AddDynamic(this, &UEnemySelectionComponent::OnEnemyTargetableStateChanged);
	}
	TSCRIPTINTERFACE_CALL_FUNC(CurrentEnemyInterface, OnBecomeTarget, K2_OnBecomeTarget, GetOwner());
	
	OnEnemyChanged.Broadcast(this, GetEnemy(), PreviousEnemy);
	K2_OnEnemyChanged(PreviousEnemy);
}

float UEnemySelectionComponent::GetEnemyChangeCooldownRemaining() const
{
	return GetWorld()->GetTimerManager().GetTimerRemaining(EnemyCooldownTimerHandle);
}

bool UEnemySelectionComponent::IsEnemyChangeCooldownActive() const
{
	return GetWorld()->GetTimerManager().IsTimerActive(EnemyCooldownTimerHandle);
}

bool UEnemySelectionComponent::LockEnemySelection(UObject* Requester)
{
	if (EnemySelectionLockSet.Contains(Requester))
	{
		return false;
	}

	EnemySelectionLockSet.Add(Requester);
	return true;
}

bool UEnemySelectionComponent::UnlockEnemySelection(UObject* Requester)
{
	if (!EnemySelectionLockSet.Contains(Requester))
	{
		return false;
	}

	EnemySelectionLockSet.Remove(Requester);
	return true;
}

void UEnemySelectionComponent::ClearEnemySelectionLock()
{
	EnemySelectionLockSet.Empty();
}

bool UEnemySelectionComponent::IsEnemySelectionLocked() const
{
	return EnemySelectionLockSet.Num() > 0;
}

void UEnemySelectionComponent::GetAllEnemies(TArray<AActor*>& EnemyList) const
{
	if (!GetEnemy())
	{
		EnemyList.Empty(EnemyList.Max());
		return;
	}

	EnemyList.Empty(FMath::Max(1, EnemyList.Max()));
	EnemyList.Add(GetEnemy());
}

void UEnemySelectionComponent::OnEnemyTargetableStateChanged(AActor* Actor, bool bTargetable)
{
	if (Actor != GetEnemy())
	{
		return;
	}

	if (!bTargetable)
	{
		ClearEnemy(true);
	}
}