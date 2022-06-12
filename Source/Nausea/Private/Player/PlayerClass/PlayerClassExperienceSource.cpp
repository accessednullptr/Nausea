// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Player/PlayerClass/PlayerClassExperienceSource.h"
#include "Player/CorePlayerController.h"
#include "Player/PlayerClassComponent.h"
#include "Player/PlayerStatistics/PlayerStatisticsComponent.h"
#include "Player/PlayerStatistics/PlayerStatisticsTypes.h"

UPlayerClassExperienceSource::UPlayerClassExperienceSource(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bReplicates = false;
	bHideReplicates = true;
}

void UPlayerClassExperienceSource::GrantExperience(uint64 Amount)
{
	OnGrantedExperience.Broadcast(this, Amount);
}

UPlayerClassSimpleExperienceSource::UPlayerClassSimpleExperienceSource(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UPlayerClassSimpleExperienceSource::Initialize(UPlayerClassComponent* OwningComponent)
{
	Super::Initialize(OwningComponent);

	if (!GetPlayerClassComponent())
	{
		return;
	}

	ACorePlayerController* PlayerController = Cast<ACorePlayerController>(GetPlayerClassComponent()->GetOwningController());

	if (!PlayerController)
	{
		return;
	}

	PlayerController->GetPlayerStatisticsComponent()->OnPlayerStatisticsUpdate.AddDynamic(this, &UPlayerClassSimpleExperienceSource::OnStatIncreased);
}

void UPlayerClassSimpleExperienceSource::OnStatIncreased(UPlayerStatisticsComponent* PlayerStatistics, EPlayerStatisticType StatisticType, uint64 Delta)
{
	CurrentStatProgress += Delta;

	if (CurrentStatProgress < StatRequiredPerGrant)
	{
		return;
	}

	CurrentStatProgress -= StatRequiredPerGrant;

	UPlayerStatisticHelpers::IncrementPlayerExperience(GetPlayerClassComponent(), GetPlayerClassComponent()->GetClass(), GetPlayerClassComponent()->GetVariant(), ExperienceToGrant);
}