// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.


#include "Character/CoreCharacterAnimInstanceTypes.h"
#include "Components/SkeletalMeshComponent.h"
#include "Character/CoreCharacterAnimInstance.h"
#include "Gameplay/StatusEffect/StatusEffectBase.h"
#include "Animation/AnimMontage.h"

FWeaponMontagePair FWeaponMontagePair::InvalidMontagePair = FWeaponMontagePair();

UAnimMontage* FAnimationMontageContainer::GetRandomMontage(const FRandomStream& Seed) const
{
	return MontageList.Num() == 0 ? nullptr : MontageList[Seed.RandHelper(MontageList.Num())];
}

UAnimMontage* FHitReactionAnimationContainer::GetMontage(EHitReactionStrength Strength) const
{
	return GetMontageForStream(Strength, FRandomStream(FMath::Rand()));
}

UAnimMontage* FHitReactionAnimationContainer::GetMontageForStream(EHitReactionStrength Strength, const FRandomStream& Seed) const
{
	switch (Strength)
	{
		case EHitReactionStrength::Light:
			return LightHitReaction.GetRandomMontage(FRandomStream(Seed.GetInitialSeed()));
		case EHitReactionStrength::Medium:
			return MediumHitReaction.GetRandomMontage(FRandomStream(Seed.GetInitialSeed()));
		case EHitReactionStrength::Heavy:
			return HeavyHitReaction.GetRandomMontage(FRandomStream(Seed.GetInitialSeed()));
	}

	return nullptr;
}

FName UAnimationObject::StatusLoopSection = "Loop";
FName UAnimationObject::StatusLoopEndSection = "LoopEnd";
UAnimationObject::UAnimationObject(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

#if WITH_EDITOR
void UAnimationObject::PostEditImport()
{
	Super::PostEditImport();
	UpdateVisibleProperties();
}

void UAnimationObject::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	UpdateVisibleProperties();
}

void UAnimationObject::UpdateVisibleProperties()
{
	if (bHasLoopingReload)
	{
		GetClass()->SetMetaData(TEXT("HideCategories"), *FString("Looping Reload Animation Set"));
	}
	else
	{
		GetClass()->SetMetaData(TEXT("HideCategories"), *FString(""));
	}
}
#endif //WITH_EDITOR

const FWeaponMontagePair& UAnimationObject::GetFireMontage(EFireMode FireMode) const
{
	if (FireMontage.Contains(FireMode))
	{
		return FireMontage[FireMode];
	}

	return FWeaponMontagePair::InvalidMontagePair;
}

float UAnimationObject::GetFireMontageAnimRate(EFireMode FireMode) const
{
	if (FireMontageAnimRate.Contains(FireMode))
	{
		return FireMontageAnimRate[FireMode];
	}

	return 1.f;
}

const FWeaponMontagePair& UAnimationObject::GetReloadMontage(EFireMode FireMode) const
{
	if (ReloadMontage.Contains(FireMode))
	{
		return ReloadMontage[FireMode];
	}

	return FWeaponMontagePair::InvalidMontagePair;
}

float UAnimationObject::GetReloadMontageAnimRate(EFireMode FireMode) const
{
	if (ReloadMontageAnimRate.Contains(FireMode))
	{
		return ReloadMontageAnimRate[FireMode];
	}

	return 1.f;
}

inline bool PlayMontagePair(const FWeaponMontagePair& InMontagePair, USkeletalMeshComponent* PlayerMesh, USkeletalMeshComponent* WeaponMesh, float Duration, float AnimRate)
{
	bool bMontagePlayed = false;

	if (InMontagePair.HasPlayerMontage() && PlayerMesh && PlayerMesh->GetAnimInstance())
	{
		const float PlaybackRate = (InMontagePair.PlayerMontage->GetPlayLength() / Duration) * AnimRate;
		PlayerMesh->GetAnimInstance()->Montage_Play(InMontagePair.PlayerMontage, PlaybackRate);
		bMontagePlayed = true;
	}

	if (InMontagePair.HasWeaponMontage() && WeaponMesh && WeaponMesh->GetAnimInstance())
	{
		const float PlaybackRate = (InMontagePair.WeaponMontage->GetPlayLength() / Duration) * AnimRate;
		WeaponMesh->GetAnimInstance()->Montage_Play(InMontagePair.WeaponMontage, PlaybackRate);
		bMontagePlayed = true;
	}

	return bMontagePlayed;
}

bool UAnimationObject::PlayEquipMontage(USkeletalMeshComponent* PlayerMesh, USkeletalMeshComponent* WeaponMesh, float Duration) const
{
	return PlayMontagePair(EquipMontage, PlayerMesh, WeaponMesh, Duration, EquipMontageAnimRate);
}

bool UAnimationObject::PlayPutDownMontage(USkeletalMeshComponent* PlayerMesh, USkeletalMeshComponent* WeaponMesh, float Duration) const
{
	return PlayMontagePair(PutDownMontage, PlayerMesh, WeaponMesh, Duration, PutDownMontageAnimRate);
}

bool UAnimationObject::PlayFireMontage(USkeletalMeshComponent* PlayerMesh, USkeletalMeshComponent* WeaponMesh, float Duration, EFireMode FireMode) const
{
	return PlayMontagePair(GetFireMontage(FireMode), PlayerMesh, WeaponMesh, Duration, GetFireMontageAnimRate(FireMode));
}

static FName ReloadStartSection = TEXT("Default");
static FName ReloadEndSection = TEXT("ReloadEnd");

inline bool IsReloadAtEndpoint(UAnimMontage* InStartMontage, UAnimMontage* InLoopMontage, UAnimInstance* AnimInstance)
{
	float StartTime, EndTime, CurrentTime;

	if (AnimInstance->Montage_IsPlaying(InStartMontage))
	{
		InStartMontage->GetSectionStartAndEndTime(InStartMontage->GetSectionIndex(ReloadEndSection), StartTime, EndTime);
		CurrentTime = AnimInstance->Montage_GetPosition(InStartMontage);
	}
	else if(AnimInstance->Montage_IsPlaying(InLoopMontage))
	{
		InLoopMontage->GetSectionStartAndEndTime(InLoopMontage->GetSectionIndex(ReloadEndSection), StartTime, EndTime);
		CurrentTime = AnimInstance->Montage_GetPosition(InLoopMontage);
	}
	else
	{
		return false;
	}
	
	if (FMath::Abs(CurrentTime - StartTime) < 0.2f)
	{
		return true;
	}

	return false;
}

inline bool PlayLoopingReload(UAnimMontage* InStartMontage, UAnimMontage* InLoopMontage, UAnimInstance* AnimInstance, float Duration)
{
	if (IsReloadAtEndpoint(InStartMontage, InLoopMontage, AnimInstance))
	{
		const float SectionDuration = InLoopMontage->GetSectionLength(InLoopMontage->GetSectionIndex(ReloadStartSection));
		AnimInstance->Montage_Stop(0.1f, InStartMontage);
		AnimInstance->Montage_Play(InLoopMontage, SectionDuration / Duration);
		return true;
	}

	const float MontageDuration = InStartMontage->GetPlayLength();
	const float SectionDuration = InStartMontage->GetSectionLength(InStartMontage->GetSectionIndex(ReloadStartSection));
	ensure(MontageDuration != SectionDuration);
	AnimInstance->Montage_Stop(0.1f, InLoopMontage);
	AnimInstance->Montage_Play(InStartMontage, SectionDuration / Duration);
	return true;
}

bool UAnimationObject::PlayReloadMontage(USkeletalMeshComponent* PlayerMesh, USkeletalMeshComponent* WeaponMesh, float Duration, EFireMode FireMode) const
{
	const FWeaponMontagePair& ReloadMontagePair = GetReloadMontage(FireMode);

	if (!ReloadMontagePair.IsValid())
	{
		return false;
	}

	if (!bHasLoopingReload)
	{
		return PlayMontagePair(ReloadMontagePair, PlayerMesh, WeaponMesh, Duration, GetReloadMontageAnimRate(FireMode));
	}

	const FWeaponMontagePair& LoopingReloadMontagePair = GetLoopingReloadMontage(FireMode);

	//If we have no looping reload montage pair despite being marked as such, ignore the flag.
	if (!LoopingReloadMontagePair.IsValid())
	{
		return PlayMontagePair(GetReloadMontage(FireMode), PlayerMesh, WeaponMesh, Duration, GetReloadMontageAnimRate(FireMode));
	}

	bool bMontagePlayed = false;
	const float MontageRate = GetReloadMontageAnimRate(FireMode);

	if (LoopingReloadMontagePair.HasPlayerMontage() && PlayerMesh && PlayerMesh->GetAnimInstance())
	{
		bMontagePlayed = PlayLoopingReload(ReloadMontagePair.PlayerMontage, LoopingReloadMontagePair.PlayerMontage, PlayerMesh->GetAnimInstance(), Duration);
	}

	if (LoopingReloadMontagePair.HasWeaponMontage() && WeaponMesh && WeaponMesh->GetAnimInstance())
	{
		bMontagePlayed = PlayLoopingReload(ReloadMontagePair.WeaponMontage, LoopingReloadMontagePair.WeaponMontage, WeaponMesh->GetAnimInstance(), Duration);
	}

	return bMontagePlayed;
}

static FName FrontHitSection("Front");
static FName BackHitSection("Back");
static FName LeftHitSection("Left");
static FName RightHitSection("Right");
bool UAnimationObject::PlayHitReactionMontage(UCoreCharacterAnimInstance* AnimInstance, EHitReactionStrength Strength, EHitReactionDirection Direction, const FRandomStream& Seed) const
{
	if (!AnimInstance)
	{
		return false;
	}

	UAnimMontage* HitMontage = HitReactionAnimations.GetMontageForStream(Strength, Seed);
	
	if (!HitMontage)
	{
		return false;
	}

	AnimInstance->Montage_Play(HitMontage, 1.f, EMontagePlayReturnType::MontageLength, 0.f, false);

	FName SectionName = NAME_None;
	switch (Direction)
	{
	case EHitReactionDirection::Front:
		SectionName = HitMontage->IsValidSectionName(FrontHitSection) ? FrontHitSection : NAME_None;
		break;
	case EHitReactionDirection::Back:
		SectionName = HitMontage->IsValidSectionName(BackHitSection) ? BackHitSection : NAME_None;
		break;
	case EHitReactionDirection::Left:
		SectionName = HitMontage->IsValidSectionName(LeftHitSection) ? LeftHitSection : NAME_None;
		break;
	case EHitReactionDirection::Right:
		SectionName = HitMontage->IsValidSectionName(RightHitSection) ? RightHitSection : NAME_None;
		break;
	}
	
	if (SectionName != NAME_None)
	{
		AnimInstance->Montage_JumpToSection(SectionName, HitMontage);
	}

	return true;
}

bool UAnimationObject::PlayStatusStartMontage(UStatusEffectBase* StatusEffect, UCoreCharacterAnimInstance* AnimInstance, EStatusBeginType BeginType, EHitReactionDirection Direction) const
{
	if (!AnimInstance)
	{
		return false;
	}

	if (!StatusEffectAnimations.StatusMap.Contains(StatusEffect->GetStatusType()))
	{
		return false;
	}

	UAnimMontage* StatusMontage = StatusEffectAnimations.StatusMap[StatusEffect->GetStatusType()].GetRandomMontage(0);
	const bool bRestartOnRefresh = StatusEffectAnimations.StatusMap[StatusEffect->GetStatusType()].bRestartAnimationOnRefresh;

	if (!StatusMontage)
	{
		return false;
	}

	if (!AnimInstance->DoesMontageHaveRegisteredLoopTimerHandle(StatusMontage) || BeginType == EStatusBeginType::Initial || bRestartOnRefresh)
	{
		AnimInstance->Montage_Stop(0.2f, StatusMontage);
		AnimInstance->Montage_Play(StatusMontage, 1.f, EMontagePlayReturnType::MontageLength, 0.f, false);
	}

	AnimInstance->RevokeMontageLoopTimerHandle(StatusMontage);
	FName SectionName = NAME_None;
	switch (Direction)
	{
	case EHitReactionDirection::Front:
		SectionName = StatusMontage->IsValidSectionName(FrontHitSection) ? FrontHitSection : NAME_None;
		break;
	case EHitReactionDirection::Back:
		SectionName = StatusMontage->IsValidSectionName(BackHitSection) ? BackHitSection : NAME_None;
		break;
	case EHitReactionDirection::Left:
		SectionName = StatusMontage->IsValidSectionName(LeftHitSection) ? LeftHitSection : NAME_None;
		break;
	case EHitReactionDirection::Right:
		SectionName = StatusMontage->IsValidSectionName(RightHitSection) ? RightHitSection : NAME_None;
		break;
	}

	if (SectionName != NAME_None)
	{
		AnimInstance->Montage_JumpToSection(SectionName, StatusMontage);
	}

	int32 SectionIndex = StatusMontage->GetSectionIndex(StatusLoopEndSection);

	if (SectionIndex == INDEX_NONE)
	{
		return true;
	}

	auto PlayMontageEnd = [](TWeakObjectPtr<UStatusEffectBase> StatusEffect, TWeakObjectPtr<UAnimMontage> AnimMontage, TWeakObjectPtr<UCoreCharacterAnimInstance> AnimInstance)
	{
		if (!StatusEffect.IsValid() || !AnimInstance.IsValid() || !AnimMontage.IsValid())
		{
			return;
		}

		AnimInstance->Montage_Stop(0.25f, AnimMontage.Get());
		AnimInstance->Montage_Play(AnimMontage.Get(), 1.f, EMontagePlayReturnType::MontageLength, 0.f, false);
		AnimInstance->Montage_JumpToSection(StatusLoopEndSection, AnimMontage.Get());
		AnimInstance->RevokeMontageLoopTimerHandle(AnimMontage.Get());
	};

	const float StatusTimeRemaining = StatusEffect->GetStatusTimeRemaining();

	//If we have no idea when this montage ends assume it's being manually controlled elsewhere.
	if (StatusTimeRemaining == -1.f)
	{
		return true;
	}

	const float TimeUntilStartLoopEnd = StatusTimeRemaining - StatusMontage->GetSectionLength(SectionIndex);

	//If this effect is already about to end then start montage end immediately.
	if (TimeUntilStartLoopEnd <= 0.f)
	{
		PlayMontageEnd(StatusEffect, StatusMontage, AnimInstance);
		return true;
	}

	FTimerHandle LoopEndHandle;
	StatusEffect->GetWorld()->GetTimerManager().SetTimer(LoopEndHandle,
		FTimerDelegate::CreateWeakLambda(StatusEffect->GetTypedOuter<AActor>(), PlayMontageEnd, TWeakObjectPtr<UStatusEffectBase>(StatusEffect), TWeakObjectPtr<UAnimMontage>(StatusMontage), TWeakObjectPtr<UCoreCharacterAnimInstance>(AnimInstance)),
		TimeUntilStartLoopEnd, false);
	
	AnimInstance->RegisterMontageLoopTimerHandle(StatusMontage, LoopEndHandle);
	return true;
}

bool UAnimationObject::PlayStatusEndMontage(UStatusEffectBase* StatusEffect, UCoreCharacterAnimInstance* AnimInstance, EStatusEndType EndType) const
{
	if (!AnimInstance)
	{
		return false;
	}

	if (!StatusEffectAnimations.StatusMap.Contains(StatusEffect->GetStatusType()))
	{
		return false;
	}

	UAnimMontage* StatusMontage = StatusEffectAnimations.StatusMap[StatusEffect->GetStatusType()].GetRandomMontage(0);

	if (!StatusMontage)
	{
		return false;
	}

	AnimInstance->RevokeMontageLoopTimerHandle(StatusMontage);
	AnimInstance->Montage_Stop(0.1f, StatusMontage);

	return true;
}

const FWeaponMontagePair& UAnimationObject::GetLoopingReloadMontage(EFireMode FireMode) const
{
	if (!bHasLoopingReload)
	{
		return FWeaponMontagePair::InvalidMontagePair;
	}

	if (LoopingReloadMontage.Contains(FireMode))
	{
		return LoopingReloadMontage[FireMode];
	}

	return FWeaponMontagePair::InvalidMontagePair;
}