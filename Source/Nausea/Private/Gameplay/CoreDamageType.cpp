// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Gameplay/CoreDamageType.h"
#include "GenericTeamAgentInterface.h"

UCoreDamageType::UCoreDamageType(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

const TArray<EDamageHitDescriptor>& UCoreDamageType::GetDamageHitDescriptorList() const
{
	if (bInitializedTypeLists)
	{
		return DamageHitDescriptorList;
	}

	UCoreDamageType* MutableThis = const_cast<UCoreDamageType*>(this);

	uint8 BitMask = 1;
	while (BitMask != 0)
	{
		if ((DamageHitDescriptors & BitMask) != 0)
		{
			EDamageHitDescriptor HitType = EDamageHitDescriptor(BitMask);
			MutableThis->DamageHitDescriptorList.Add(HitType);
		}

		if ((DamageElementalDescriptors & BitMask) != 0)
		{
			EDamageElementalDescriptor ElementType = EDamageElementalDescriptor(BitMask);
			MutableThis->DamageElementalDescriptorList.Add(ElementType);
		}

		BitMask *= 2;
		if (BitMask == 0)
		{
			break;
		}
	}

	MutableThis->bInitializedTypeLists = true;
	return DamageHitDescriptorList;
}

const TArray<EDamageElementalDescriptor>& UCoreDamageType::GetDamageElementalDescriptorList() const
{
	if (bInitializedTypeLists)
	{
		return DamageElementalDescriptorList;
	}

	UCoreDamageType* MutableThis = const_cast<UCoreDamageType*>(this);

	uint8 BitMask = 1;
	while (BitMask != 0)
	{
		if ((DamageHitDescriptors & BitMask) != 0)
		{
			EDamageHitDescriptor HitType = EDamageHitDescriptor(BitMask);
			MutableThis->DamageHitDescriptorList.Add(HitType);
		}

		if ((DamageElementalDescriptors & BitMask) != 0)
		{
			EDamageElementalDescriptor ElementType = EDamageElementalDescriptor(BitMask);
			MutableThis->DamageElementalDescriptorList.Add(ElementType);
		}

		BitMask *= 2;
		if (BitMask == 0)
		{
			break;
		}
	}

	MutableThis->bInitializedTypeLists = true;
	return DamageElementalDescriptorList;
}

EApplicationResult UCoreDamageType::GetDamageApplicationResult(AActor* Instigator, AActor* Target) const
{
	if (GetDamageApplicationLogic() == EApplicationLogic::All)
	{
		return EApplicationResult::Full;
	}

	ETeamAttitude::Type Attitude = FGenericTeamId::GetAttitude(Instigator, Target);

	switch (GetDamageApplicationLogic())
	{
	case EApplicationLogic::EnemyOnly:
		return Attitude != ETeamAttitude::Friendly ? EApplicationResult::Full : EApplicationResult::None;
	case EApplicationLogic::AllyOnly:
		return Attitude == ETeamAttitude::Friendly ? EApplicationResult::Full : EApplicationResult::None;
	case EApplicationLogic::Enemy:
		return Attitude != ETeamAttitude::Friendly ? EApplicationResult::Full : EApplicationResult::FriendlyFire;
	}

	return EApplicationResult::None;
}

EApplicationResult UCoreDamageType::GetStatusApplicationResult(AActor* Instigator, AActor* Target) const
{
	if (GetStatusApplicationLogic() == EApplicationLogic::All)
	{
		return EApplicationResult::Full;
	}

	ETeamAttitude::Type Attitude = FGenericTeamId::GetAttitude(Instigator, Target);

	switch (GetStatusApplicationLogic())
	{
	case EApplicationLogic::EnemyOnly:
		return Attitude != ETeamAttitude::Friendly ? EApplicationResult::Full : EApplicationResult::None;
	case EApplicationLogic::AllyOnly:
		return Attitude == ETeamAttitude::Friendly ? EApplicationResult::Full : EApplicationResult::None;
	case EApplicationLogic::Enemy:
		return Attitude != ETeamAttitude::Friendly ? EApplicationResult::Full : EApplicationResult::FriendlyFire;
	}

	return EApplicationResult::None;
}

EHitReactionStrength UCoreDamageType::GetHitReactionStrength(const FHitEvent& HitEvent) const
{
	if (!HitReactionCurve)
	{
		return EHitReactionStrength::Invalid;
	}

	return EHitReactionStrength(FMath::Min(FMath::FloorToInt(HitReactionCurve->GetFloatValue(DamageAmount)), int32(EHitReactionStrength::MAX) - 1));
}