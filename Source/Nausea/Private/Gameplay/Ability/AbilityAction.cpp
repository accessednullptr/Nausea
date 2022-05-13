// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Gameplay/Ability/AbilityAction.h"
#include "GameFramework/GameState.h"
#include "Kismet/GameplayStatics.h"
#include "Character/CoreCharacter.h"
#include "Gameplay/AbilityComponent.h"
#include "Gameplay/CoreDamageType.h"
#include "Gameplay/StatusType.h"

UAbilityAction::UAbilityAction(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UWorld* UAbilityAction::GetWorld() const
{
	return GetAbilityComponent() ? GetAbilityComponent()->GetWorld() : nullptr;
}

void UAbilityAction::InitializeInstance(UAbilityComponent* AbilityComponent, const FAbilityInstanceData& AbilityInstance, const FAbilityTargetData& AbilityTargetData, EActionStage Stage)
{
	OwningAbilityComponent = AbilityComponent;
	AbilityInstanceHandle = AbilityInstance.GetHandle();
	AbilityTargetDataHandle = AbilityTargetData.GetHandle();
	ActionStage = Stage;
	OwningAbilityComponent->RegisterAbilityAction(AbilityTargetData.GetHandle(), this);
}

void UAbilityAction::Complete()
{
	bHasCompleted = true;
}

void UAbilityAction::Cleanup()
{
	if (bCompleteOnCleanup && !IsCompleted())
	{
		Complete();
	}

	if (OwningAbilityComponent.IsValid())
	{
		OwningAbilityComponent->OnAbilityActionCleanup(AbilityTargetDataHandle, this);
		OwningAbilityComponent = nullptr;
	}

	MarkPendingKill();
}

UAbilityActionDamage::UAbilityActionDamage(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

inline float GetServerWorldTimeFromAbilityComponent(UWorld* World)
{
	if (!World)
	{
		return -1.f;
	}

	AGameStateBase* GameState = World->GetGameState();

	if (!GameState)
	{
		return -1.f;
	}

	return GameState->GetServerWorldTimeSeconds();
}

void UAbilityActionDamage::PerformAction(UAbilityComponent* AbilityComponent, const FAbilityInstanceData& AbilityInstance, const FAbilityTargetData& AbilityTargetData, EActionStage Stage) const
{
	if (!AbilityComponent)
	{
		return;
	}

	switch (AbilityTargetData.GetTargetType())
	{
	case ETargetDataType::Actor:
	case ETargetDataType::ActorRelativeTransform:
		ApplyDamageAtActor(AbilityComponent, AbilityInstance, AbilityTargetData);
		break;
	case ETargetDataType::Transform:
	case ETargetDataType::MovingTransform:
		ApplyDamageAtLocation(AbilityComponent, AbilityInstance, AbilityTargetData);
		break;
	}
}

void UAbilityActionDamage::ApplyDamageAtActor(UAbilityComponent* AbilityComponent, const FAbilityInstanceData& AbilityInstance, const FAbilityTargetData& AbilityTargetData) const
{
	const UAbilityInfo* AbilityInfo = AbilityInstance.GetClassCDO();

	if (!AbilityInfo)
	{
		return;
	}

	AActor* TargetActor = AbilityTargetData.GetTargetActor();

	if (!TargetActor)
	{
		return;
	}

	bool bAppliedDamageToInstigator = false;
	if (TargetActor && (!bIgnoreInstigator || AbilityComponent->GetOwner() != TargetActor))
	{
		bAppliedDamageToInstigator = true;
		TargetActor->TakeDamage(DamageType.GetDefaultObject()->GetDamageAmount(), FDamageEvent(DamageType), AbilityComponent->GetOwningController(), AbilityComponent->GetOwner());
	}

	UWorld* World = GEngine->GetWorldFromContextObject(AbilityComponent, EGetWorldErrorMode::LogAndReturnNull);

	if (!World)
	{
		return;
	}

	const float ServerWorldTimeSeconds = GetServerWorldTimeFromAbilityComponent(World);

	const FVector2D& TargetSize = UAbilityInfo::GetAbilityTargetDataTargetSize(AbilityInstance, AbilityTargetData);

	if (TargetSize <= FVector2D(0))
	{
		return;
	}

	TArray<FOverlapResult> OverlapList;

	const FTransform& Transform = AbilityTargetData.GetTransform(ServerWorldTimeSeconds);

	FCollisionObjectQueryParams COQP = FCollisionObjectQueryParams::DefaultObjectQueryParam;
	FCollisionShape CollisionShape = FCollisionShape::MakeSphere(UAbilityInfo::GetAbilityTargetDataTargetSize(AbilityInstance, AbilityTargetData).X);
	FCollisionQueryParams CQP(SCENE_QUERY_STAT(ApplyDamageAtActor), false, (bIgnoreInstigator || bAppliedDamageToInstigator) ? AbilityComponent->GetOwner() : nullptr);

	World->OverlapMultiByObjectType(OverlapList, Transform.GetLocation(), Transform.GetRotation(), COQP, CollisionShape, CQP);

	TArray<AActor*> ActorList;
	{
		TSet<AActor*> ActorSet;
		ActorSet.Reserve(OverlapList.Num());
		for (const FOverlapResult& Overlap : OverlapList)
		{
			ActorSet.Add(Overlap.GetActor());
		}

		ActorList = ActorSet.Array();
	}
	ActorList.Shrink();

	FDamageEvent DamageEvent(DamageType);
	for (AActor* Actor : ActorList)
	{
		if (!AbilityInfo->CanTargetActor(AbilityComponent, AbilityTargetData, Actor, ServerWorldTimeSeconds))
		{
			continue;
		}

		Actor->TakeDamage(DamageType.GetDefaultObject()->GetDamageAmount(), DamageEvent, AbilityComponent->GetOwningController(), AbilityComponent->GetOwner());
	}
}

void UAbilityActionDamage::ApplyDamageAtLocation(UAbilityComponent* AbilityComponent, const FAbilityInstanceData& AbilityInstance, const FAbilityTargetData& AbilityTargetData) const
{
	const UAbilityInfo* AbilityInfo = AbilityInstance.GetClassCDO();

	if (!AbilityInfo)
	{
		return;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(AbilityComponent, EGetWorldErrorMode::LogAndReturnNull);

	if (!World)
	{
		return;
	}

	const float ServerWorldTimeSeconds = GetServerWorldTimeFromAbilityComponent(World);

	const FVector2D& TargetSize = UAbilityInfo::GetAbilityTargetDataTargetSize(AbilityInstance, AbilityTargetData);

	if (TargetSize <= FVector2D(0))
	{
		return;
	}

	TArray<FOverlapResult> OverlapList;

	const FTransform& Transform = AbilityTargetData.GetTransform(ServerWorldTimeSeconds);

	FCollisionObjectQueryParams COQP = FCollisionObjectQueryParams::DefaultObjectQueryParam;
	FCollisionShape CollisionShape = FCollisionShape::MakeSphere(UAbilityInfo::GetAbilityTargetDataTargetSize(AbilityInstance, AbilityTargetData).X);
	FCollisionQueryParams CQP(SCENE_QUERY_STAT(ApplyDamageAtActor), false, bIgnoreInstigator ? AbilityComponent->GetOwner() : nullptr);

	World->OverlapMultiByObjectType(OverlapList, Transform.GetLocation(), Transform.GetRotation(), COQP, CollisionShape, CQP);

	TArray<AActor*> ActorList;
	{
		TSet<AActor*> ActorSet;
		ActorSet.Reserve(OverlapList.Num());
		for (const FOverlapResult& Overlap : OverlapList)
		{
			ActorSet.Add(Overlap.GetActor());
		}

		ActorList = ActorSet.Array();
	}
	ActorList.Shrink();

	FDamageEvent DamageEvent(DamageType);
	for (AActor* Actor : ActorList)
	{
		if (!AbilityInfo->CanTargetActor(AbilityComponent, AbilityTargetData, Actor, ServerWorldTimeSeconds))
		{
			continue;
		}

		Actor->TakeDamage(DamageType.GetDefaultObject()->GetDamageAmount(), DamageEvent, AbilityComponent->GetOwningController(), AbilityComponent->GetOwner());
	}
}

UAbilityActionDamageOverTime::UAbilityActionDamageOverTime(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bNeedsNewInstance = true;
	bCompleteOnCleanup = true;
}

void UAbilityActionDamageOverTime::InitializeInstance(UAbilityComponent* AbilityComponent, const FAbilityInstanceData& AbilityInstance, const FAbilityTargetData& AbilityTargetData, EActionStage Stage)
{
	Super::InitializeInstance(AbilityComponent, AbilityInstance, AbilityTargetData, Stage);

	if (!GetWorld())
	{
		return;
	}

	ApplyDamageTimer();
}

void UAbilityActionDamageOverTime::Complete()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(DamageTickHandle);
	}

	Super::Complete();
}

void UAbilityActionDamageOverTime::ApplyDamageTimer()
{
	if (IsPendingKill())
	{
		return;
	}

	if (!GetWorld() || !GetAbilityComponent())
	{
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(DamageTickHandle);

	FAbilityInstanceData* OwningAbilityInstanceData = nullptr;
	FAbilityTargetData* OwningAbilityTargetData = nullptr;

	GetAbilityComponent()->GetAbilityInstanceAndTargetDataByID(AbilityInstanceHandle, AbilityTargetDataHandle, OwningAbilityInstanceData, OwningAbilityTargetData);

	if (!OwningAbilityTargetData || !OwningAbilityTargetData)
	{
		return;
	}

	switch (OwningAbilityTargetData->GetTargetType())
	{
	case ETargetDataType::Actor:
	case ETargetDataType::ActorRelativeTransform:
		ApplyDamageAtActor(GetAbilityComponent(), *OwningAbilityInstanceData, *OwningAbilityTargetData);
		break;
	case ETargetDataType::Transform:
	case ETargetDataType::MovingTransform:
		ApplyDamageAtLocation(GetAbilityComponent(), *OwningAbilityInstanceData, *OwningAbilityTargetData);
		break;
	}

	GetWorld()->GetTimerManager().SetTimer(DamageTickHandle, FTimerDelegate::CreateUObject(this, &UAbilityActionDamageOverTime::ApplyDamageTimer), DamageTickDuration, false);
}