// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#include "Character/CoreCharacterAnimInstance.h"
#include "Character/CoreCharacter.h"
#include "Character/CoreCharacterMovementComponent.h"
#include "Weapon/InventoryManagerComponent.h"
#include "Gameplay/StatusComponent.h"
#include "Gameplay/StatusEffect/StatusEffectBase.h"
#include "Gameplay/CoreDamageType.h"
#include "Weapon/Weapon.h"


FCoreCharacterAnimInstanceProxy::FCoreCharacterAnimInstanceProxy(UAnimInstance* Instance)
	: FAnimInstanceProxy(Instance)
{

}

void FCoreCharacterAnimInstanceProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	if (UCoreCharacterAnimInstance* CoreAnimInstance = Cast<UCoreCharacterAnimInstance>(InAnimInstance))
	{
		CoreAnimInstance->SetLocomotionAnimation(CoreAnimInstance->GetLocomotionAnimation());

		if (ACoreCharacter* CoreCharacter = CoreAnimInstance->GetCharacter())
		{
			const FVector& Velocity = CoreCharacter->GetVelocity();

			MovementSpeed = CoreCharacter->GetVelocity().Size2D();
			MovementDirection = CoreAnimInstance->CalculateDirection(Velocity, CoreCharacter->GetActorRotation());

			bMoving = MovementSpeed > CoreAnimInstance->MinMoveSpeed;
			bRunning = MovementSpeed > CoreAnimInstance->MinRunMoveSpeed;

			bCrouching = CoreCharacter->bIsCrouched;

			bJumping = CoreCharacter->bPressedJump || CoreCharacter->IsFalling();
			bRunJumping = bJumping && bRunning;
		}
	}

	Super::PreUpdate(InAnimInstance, DeltaSeconds);
}

UCoreCharacterAnimInstance::UCoreCharacterAnimInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

ACoreCharacter* UCoreCharacterAnimInstance::GetCharacter() const
{
	return Cast<ACoreCharacter>(GetOwningActor());
}

const FLocomotionAnimationContainer& UCoreCharacterAnimInstance::GetLocomotionAnimation() const
{
	if (WeaponLocomationAnimations.IsValid())
	{
		return WeaponLocomationAnimations;
	}

	return DefaultLocomationAnimations;
}

void UCoreCharacterAnimInstance::SetLocomotionAnimation(const FLocomotionAnimationContainer& InLocomationAnimations)
{
	LocomationAnimations = InLocomationAnimations;
}

FAnimInstanceProxy* UCoreCharacterAnimInstance::CreateAnimInstanceProxy()
{
	return &CoreCharacterProxy;
}

void UCoreCharacterAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy)
{

}

void UCoreCharacterAnimInstance::BeginDestroy()
{
	if (GetWorld())
	{
		FTimerManager& TimerManager = GetWorld()->GetTimerManager();

		for (TPair<UAnimMontage*, FTimerHandle>& Entry : StatusMontageLoopTimerMap)
		{
			TimerManager.ClearTimer(Entry.Value);
		}
	}

	Super::BeginDestroy();
}

const UAnimationObject* UCoreCharacterAnimInstance::GetCharacterAnimationObject() const
{
	if (!GetCharacter())
	{
		return DefaultAnimationObject.GetDefaultObject();
	}

	if (bIsThirdPerson)
	{
		return GetCharacter()->GetThirdPersonAnimObject();
	}
	else
	{
		return GetCharacter()->GetFirstPersonAnimObject();
	}
}

bool UCoreCharacterAnimInstance::DoesMontageHaveRegisteredLoopTimerHandle(UAnimMontage* Montage) const
{
	return StatusMontageLoopTimerMap.Contains(Montage);
}

void UCoreCharacterAnimInstance::RegisterMontageLoopTimerHandle(UAnimMontage* Montage, FTimerHandle Handle)
{
	ensure(!StatusMontageLoopTimerMap.Contains(Montage));
	StatusMontageLoopTimerMap.Add(Montage) = Handle;
}

void UCoreCharacterAnimInstance::RevokeMontageLoopTimerHandle(UAnimMontage* Montage)
{
	if (!StatusMontageLoopTimerMap.Contains(Montage))
	{
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(StatusMontageLoopTimerMap[Montage]);
	StatusMontageLoopTimerMap.Remove(Montage);
}

void UCoreCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	if (ACoreCharacter* CoreCharacter = Cast<ACoreCharacter>(GetOwningActor()))
	{
		SetCoreCharacterMovementComponent(CoreCharacter->GetCoreMovementComponent());

		if (CoreCharacter->GetInventoryManager() && !CoreCharacter->GetInventoryManager()->OnCurrentWeaponUpdate.IsAlreadyBound(this, &UCoreCharacterAnimInstance::OnWeaponChanged))
		{
			CoreCharacter->GetInventoryManager()->OnCurrentWeaponUpdate.AddDynamic(this, &UCoreCharacterAnimInstance::OnWeaponChanged);
			OnWeaponChanged(nullptr);
		}

		if (CoreCharacter->GetStatusComponent() && !CoreCharacter->GetStatusComponent()->OnStatusEffectBegin.IsAlreadyBound(this, &UCoreCharacterAnimInstance::OnStatusEffectStart))
		{
			CoreCharacter->GetStatusComponent()->OnStatusEffectBegin.AddDynamic(this, &UCoreCharacterAnimInstance::OnStatusEffectStart);
			CoreCharacter->GetStatusComponent()->OnStatusEffectEnd.AddDynamic(this, &UCoreCharacterAnimInstance::OnStatusEffectEnd);
		}

		if (!HitEventDelegate.IsValid())
		{
			HitEventDelegate = CoreCharacter->GetStatusComponent()->OnHitEventReceived.AddUObject(this, &UCoreCharacterAnimInstance::OnReceiveHitEvent);
		}
	}

	if (const UAnimationObject* LocomotionCDO = DefaultAnimationObject ? DefaultAnimationObject.GetDefaultObject() : nullptr)
	{
		DefaultLocomationAnimations = LocomotionCDO->LocomontionAnimations;
	}
}

void UCoreCharacterAnimInstance::OnWeaponChanged(UWeapon* Weapon)
{
	if (!GetCharacter() || !GetCharacter()->GetInventoryManager() || !GetCharacter()->GetInventoryManager()->GetCurrentWeapon() || !GetCharacter()->GetInventoryManager()->GetCurrentWeapon()->HasBegunPlay())
	{
		WeaponLocomationAnimations = FLocomotionAnimationContainer();
		GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UCoreCharacterAnimInstance::OnWeaponChanged, Weapon));
		return;
	}

	//Reestablish current weapon pointer.
	Weapon = GetCharacter()->GetInventoryManager()->GetCurrentWeapon();

	WeaponLocomationAnimations = GetCharacterAnimationObject() ? GetCharacterAnimationObject()->LocomontionAnimations : FLocomotionAnimationContainer();
}

void UCoreCharacterAnimInstance::OnReceiveHitEvent(const UStatusComponent* StatusComponent, const FHitEvent& HitEvent)
{
	const UAnimationObject* AnimObject = GetCharacterAnimationObject();
	if (!AnimObject)
	{
		return;
	}

	const UCoreDamageType* CoreDamageType = HitEvent.DamageType.GetDefaultObject();

	if (!CoreDamageType)
	{
		return;
	}

	const float FrontDot = (FVector::DotProduct(HitEvent.HitDirection, GetCharacter()->GetActorForwardVector()));
	const float RightDot = (FVector::DotProduct(HitEvent.HitDirection, GetCharacter()->GetActorRightVector()));

	const bool bFrontHit = FrontDot < 0.f;
	const bool bRightHit = RightDot < 0.f;

	EHitReactionDirection Direction = EHitReactionDirection::Front;
	if (FMath::Abs(FrontDot) >= FMath::Abs(RightDot))
	{
		Direction = bFrontHit ? EHitReactionDirection::Front : EHitReactionDirection::Back;
	}
	else
	{
		Direction = bRightHit ? EHitReactionDirection::Right : EHitReactionDirection::Left;
	}

	AnimObject->PlayHitReactionMontage(this, CoreDamageType->GetHitReactionStrength(HitEvent), Direction, HitEvent.Random);
}

void UCoreCharacterAnimInstance::OnStatusEffectStart(UStatusComponent* StatusComponent, UStatusEffectBase* StatusEffect, EStatusBeginType BeginType)
{
	const UAnimationObject* AnimObject = GetCharacterAnimationObject();
	if (!AnimObject)
	{
		return;
	}

	const FVector_NetQuantizeNormal& InstigationDirection = StatusEffect->GetInstigationDirection();
	EHitReactionDirection Direction = EHitReactionDirection::Front;

	if (InstigationDirection != FAISystem::InvalidDirection)
	{
		const float FrontDot = (FVector::DotProduct(InstigationDirection, GetCharacter()->GetActorForwardVector()));
		const float RightDot = (FVector::DotProduct(InstigationDirection, GetCharacter()->GetActorRightVector()));

		const bool bFrontHit = FrontDot < 0.f;
		const bool bRightHit = RightDot < 0.f;

		if (FMath::Abs(FrontDot) >= FMath::Abs(RightDot))
		{
			Direction = bFrontHit ? EHitReactionDirection::Front : EHitReactionDirection::Back;
		}
		else
		{
			Direction = bRightHit ? EHitReactionDirection::Right : EHitReactionDirection::Left;
		}
	}

	AnimObject->PlayStatusStartMontage(StatusEffect, this, BeginType, Direction);
}

void UCoreCharacterAnimInstance::OnStatusEffectEnd(UStatusComponent* StatusComponent, UStatusEffectBase* StatusEffect, EStatusEndType EndType)
{
	const UAnimationObject* AnimObject = GetCharacterAnimationObject();
	if (!AnimObject)
	{
		return;
	}

	AnimObject->PlayStatusEndMontage(StatusEffect, this, EndType);
}