// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Player/CoreLocalMessage.h"
#include "Player/CorePlayerController.h"

UCoreLocalMessage::UCoreLocalMessage(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UCoreLocalMessage::ClientReceive(const FClientReceiveData& ClientData) const
{
	if (ShouldIgnoreData(ClientData))
	{
		return;
	}

	if (ACorePlayerController* CorePlayerController = Cast<ACorePlayerController>(ClientData.LocalPC))
	{
		CorePlayerController->ReceivedLocalMessage(ClientData);
	}
}

bool UCoreLocalMessage::ShouldIgnoreData(const FClientReceiveData& ClientData) const
{
	ERecipientFilter RecipientFilter = ERecipientFilter::Everyone;
	if (ClientData.MessageType == ACorePlayerController::MessageTypeSay)
	{
		RecipientFilter = SayRecipientFilter;
	}
	else if (ClientData.MessageType == ACorePlayerController::MessageTypeTeamSay)
	{
		RecipientFilter = TeamSayRecipientFilter;
	}

	IGenericTeamAgentInterface* SenderTeamInterface = Cast<IGenericTeamAgentInterface>(ClientData.RelatedPlayerState_1);
	IGenericTeamAgentInterface* ReceiverTeamInterface = Cast<IGenericTeamAgentInterface>(ClientData.LocalPC);

	FGenericTeamId SenderTeam = SenderTeamInterface ? SenderTeamInterface->GetGenericTeamId() : FGenericTeamId::NoTeam;
	FGenericTeamId ReceiverTeam = ReceiverTeamInterface ? ReceiverTeamInterface->GetGenericTeamId() : FGenericTeamId::NoTeam;
	ETeamAttitude::Type Attitude = FGenericTeamId::GetAttitude(SenderTeam, ReceiverTeam);

	switch (Attitude)
	{
	case ETeamAttitude::Friendly:
		return false;
	case ETeamAttitude::Neutral:
		return RecipientFilter > ERecipientFilter::AllyOrNeutralOnly;
	case ETeamAttitude::Hostile:
		return RecipientFilter > ERecipientFilter::Everyone;
	}

	return false;
}