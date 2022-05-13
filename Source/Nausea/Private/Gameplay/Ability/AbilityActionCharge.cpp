// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Gameplay/Ability/AbilityActionCharge.h"
#include "GameFramework/GameStateBase.h"
#include "Gameplay/AbilityComponent.h"
#include "Character/CoreCharacter.h"
#include "Character/CoreCharacterMovementComponent.h"

UAbilityActionSlowRotation::UAbilityActionSlowRotation(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bNeedsNewInstance = true;
}

void UAbilityActionSlowRotation::InitializeInstance(UAbilityComponent* AbilityComponent, const FAbilityInstanceData& AbilityInstance, const FAbilityTargetData& AbilityTargetData, EActionStage Stage)
{
	Super::InitializeInstance(AbilityComponent, AbilityInstance, AbilityTargetData, Stage);

	if (UCoreCharacterMovementComponent* MovementComponent = AbilityComponent->GetOwningCharacter()->GetCoreMovementComponent())
	{
		float Duration = -1.f;
			
		if (bProgressivelySlowRotation)
		{
			Duration = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
			switch (Stage)
			{
			case EActionStage::Startup:
				Duration = UAbilityInfo::GetAbilityTargetDataStartupTime(AbilityInstance, AbilityTargetData).Y - Duration;
			default:
				Duration = UAbilityInfo::GetAbilityTargetDataActivationTime(AbilityInstance, AbilityTargetData).Y - Duration;
			}

			if (Duration > 0.f)
			{
				GetWorld()->GetTimerManager().SetTimer(SlowDownTimerHandle, Duration, false);
			}
		}

		RotationRateDelegateHandle = MovementComponent->OnProcessRotationRate.AddWeakLambda(this, [&](const ACoreCharacter* Character, float& RotationRate) {
			if (!bProgressivelySlowRotation || !Character->GetWorld()->GetTimerManager().IsTimerActive(SlowDownTimerHandle))
			{
				RotationRate *= RotationMultiplier;
				return;
			}

			RotationRate *= FMath::Lerp(1.f, RotationMultiplier, Character->GetWorld()->GetTimerManager().GetTimerRemaining(SlowDownTimerHandle) / Character->GetWorld()->GetTimerManager().GetTimerRate(SlowDownTimerHandle));
		});
	}
}

void UAbilityActionSlowRotation::Cleanup()
{
	if (RotationRateDelegateHandle.IsValid())
	{
		if (UCoreCharacterMovementComponent* MovementComponent = GetAbilityComponent() ? GetAbilityComponent()->GetOwningCharacter()->GetCoreMovementComponent() : nullptr)
		{
			MovementComponent->OnProcessRotationRate.Remove(RotationRateDelegateHandle);
		}
	}

	Super::Cleanup();
}

UAbilityActionCharge::UAbilityActionCharge(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bNeedsNewInstance = true;
	bCompleteOnCleanup = true;
}

void UAbilityActionCharge::InitializeInstance(UAbilityComponent* AbilityComponent, const FAbilityInstanceData& AbilityInstance, const FAbilityTargetData& AbilityTargetData, EActionStage Stage)
{
	Super::InitializeInstance(AbilityComponent, AbilityInstance, AbilityTargetData, Stage);
}

void UAbilityActionCharge::Complete()
{
	Super::Complete();
}

void UAbilityActionCharge::TickAction(float DeltaTime)
{
	
}