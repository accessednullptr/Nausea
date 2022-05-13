// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "UI/MainMenuScreenUserWidget.h"
#include "Player/CorePlayerController.h"
#include "Player/CorePlayerState.h"
#include "System/CoreGameState.h"
#include "System/MainMenuGameState.h"

UMainMenuScreenUserWidget::UMainMenuScreenUserWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

bool UMainMenuScreenUserWidget::Initialize()
{
	if (!Super::Initialize())
	{
		return false;
	}

	if (!GetPlayerContext().IsValid() || !GetPlayerContext().IsInitialized())
	{
		return true;
	}

	if (!ensure(GetOwningCorePlayerController()))
	{
		return true;
	}

	if (ACoreGameState* CoreGameState = GetOwningCorePlayerController()->GetWorld()->GetGameState<ACoreGameState>())
	{
		if (!CoreGameState->OnPlayerStateAdded.IsAlreadyBound(this, &UMainMenuScreenUserWidget::OnPlayerStateAdded))
		{
			CoreGameState->OnPlayerStateAdded.AddDynamic(this, &UMainMenuScreenUserWidget::OnPlayerStateAdded);
			CoreGameState->OnPlayerStateRemoved.AddDynamic(this, &UMainMenuScreenUserWidget::OnPlayerStateRemoved);
		}
	}

	if (AMainMenuGameState* MainMenuGameState = GetOwningCorePlayerController()->GetWorld()->GetGameState<AMainMenuGameState>())
	{
		if (!MainMenuGameState->OnSelectedMapDataChanged.IsAlreadyBound(this, &UMainMenuScreenUserWidget::OnSelectedMapDataChanged))
		{
			MainMenuGameState->OnSelectedMapDataChanged.AddDynamic(this, &UMainMenuScreenUserWidget::OnSelectedMapDataChanged);
		}

		OnSelectedMapDataChanged(MainMenuGameState, MainMenuGameState->GetSelectedMapData());
	}

	return true;
}

void UMainMenuScreenUserWidget::OnPlayerStateAdded(bool bIsPlayer, ACorePlayerState* PlayerState)
{
	K2_OnPlayerStateAdded(PlayerState ? PlayerState->IsActivePlayer() : false, PlayerState);
}

void UMainMenuScreenUserWidget::OnPlayerStateRemoved(bool bIsPlayer, ACorePlayerState* PlayerState)
{
	K2_OnPlayerStateRemoved(PlayerState ? PlayerState->IsActivePlayer() : false, PlayerState);
}

void UMainMenuScreenUserWidget::OnSelectedMapDataChanged(AMainMenuGameState* MainMenuGameState, const UMapDataAsset* SelectedMapData)
{
	K2_OnSelectedMapDataChanged(SelectedMapData);
}