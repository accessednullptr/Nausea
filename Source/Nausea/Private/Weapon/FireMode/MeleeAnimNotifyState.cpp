// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Weapon/FireMode/MeleeAnimNotifyState.h"
#include "Components/SkeletalMeshComponent.h"
#include "Weapon/WeaponAnimInstance.h"

UMeleeAnimNotifyState::UMeleeAnimNotifyState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UMeleeAnimNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration);

	SendMeleeHitBox(MeshComp);
}

void UMeleeAnimNotifyState::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime);

	SendMeleeHitBox(MeshComp);
}

void UMeleeAnimNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	Super::NotifyEnd(MeshComp, Animation);

	SendMeleeHitBox(MeshComp);
}

void UMeleeAnimNotifyState::SendMeleeHitBox(USkeletalMeshComponent* MeshComp)
{
	UWeaponAnimInstance* WeaponAnimInstance = MeshComp ? Cast<UWeaponAnimInstance>(MeshComp->GetAnimInstance()) : nullptr;

	if (!WeaponAnimInstance)
	{
		return;
	}

	WeaponAnimInstance->ProcessMeleeNotifyHitBox(FMeleeNotifyData());
}