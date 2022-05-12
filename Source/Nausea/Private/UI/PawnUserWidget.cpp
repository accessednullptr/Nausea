// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#include "UI/PawnUserWidget.h"
#include "Player/CorePlayerController.h"
#include "Character/CoreCharacter.h"

UPawnUserWidget::UPawnUserWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

bool UPawnUserWidget::Initialize()
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

	if (!GetOwningCorePlayerController()->OnPawnUpdated.IsAlreadyBound(this, &UPawnUserWidget::PossessedPawn))
	{
		GetOwningCorePlayerController()->OnPawnUpdated.AddDynamic(this, &UPawnUserWidget::PossessedPawn);
	}
	else
	{
		ensure(false);
	}

	if (ACoreCharacter* Pawn = GetOwningPlayerCharacter())
	{
		if (Pawn == GetOwningPlayer()->AcknowledgedPawn)
		{
			PossessedPawn(GetOwningCorePlayerController(), Pawn);
		}
	}

	return true;
}

void UPawnUserWidget::PossessedPawn(ACorePlayerController* PlayerController, ACoreCharacter* Pawn)
{
	K2_OnPossessedPawn(Pawn);
}