// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Player/PlayerOwnershipInterfaceTypes.h"

const FGenericTeamId UPlayerOwnershipInterfaceTypes::Alpha = FGenericTeamId(uint8(ETeam::Alpha));
const FGenericTeamId UPlayerOwnershipInterfaceTypes::Bravo = FGenericTeamId(uint8(ETeam::Bravo));
const FGenericTeamId UPlayerOwnershipInterfaceTypes::Charlie = FGenericTeamId(uint8(ETeam::Charlie));
const FGenericTeamId UPlayerOwnershipInterfaceTypes::Delta = FGenericTeamId(uint8(ETeam::Delta));
const FGenericTeamId UPlayerOwnershipInterfaceTypes::Epsilon = FGenericTeamId(uint8(ETeam::Epsilon));
const FGenericTeamId UPlayerOwnershipInterfaceTypes::IncursionTeam = FGenericTeamId(uint8(ETeam::IncursionTeam));

namespace
{
	ETeamAttitude::Type CoreTeamAttitudeSolver(FGenericTeamId A, FGenericTeamId B)
	{
		return (A == FGenericTeamId::NoTeam || A != B) ? ETeamAttitude::Hostile : ETeamAttitude::Friendly;
	}
}

UPlayerOwnershipInterfaceTypes::UPlayerOwnershipInterfaceTypes(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	FGenericTeamId::SetAttitudeSolver(&CoreTeamAttitudeSolver);
}

const FGenericTeamId& UPlayerOwnershipInterfaceTypes::GetGenericTeamIdFromTeamEnum(ETeam Team)
{
	switch (Team)
	{
	case ETeam::Alpha:
		return UPlayerOwnershipInterfaceTypes::Alpha;
	case ETeam::Bravo:
		return UPlayerOwnershipInterfaceTypes::Bravo;
	case ETeam::Charlie:
		return UPlayerOwnershipInterfaceTypes::Charlie;
	case ETeam::Delta:
		return UPlayerOwnershipInterfaceTypes::Delta;
	case ETeam::Epsilon:
		return UPlayerOwnershipInterfaceTypes::Epsilon;

	case ETeam::IncursionTeam:
		return UPlayerOwnershipInterfaceTypes::IncursionTeam;

	case ETeam::NoTeam:
		return FGenericTeamId::NoTeam;
	}

	return FGenericTeamId::NoTeam;
}

ETeam UPlayerOwnershipInterfaceTypes::GetTeamEnumFromGenericTeamId(const FGenericTeamId& TeamId)
{
	return ETeam(TeamId.GetId());
}

const FGenericTeamId& UPlayerOwnershipInterfaceTypes::GetNoTeamGenericTeamId()
{
	return FGenericTeamId::NoTeam;
}