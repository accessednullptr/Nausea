// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.


#include "UI/NauseaWidgetComponent.h"
#include "UI/CoreUserWidget.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"

UNauseaWidgetComponent::UNauseaWidgetComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UNauseaWidgetComponent::InitWidget()
{
	if (ShouldHideWidget())
	{
		return;
	}

	Super::InitWidget();

	if (UCoreUserWidget* CoreUserWidget = Cast<UCoreUserWidget>(GetWidget()))
	{
		CoreUserWidget->InitializeWidgetComponent(this);
	}
}

bool UNauseaWidgetComponent::ShouldHideWidget() const
{
	if (const APawn* PawnOwner = Cast<APawn>(GetOwner()))
	{
		if (const AController* ControllerOwner = PawnOwner->GetController())
		{
			if (ControllerOwner->IsLocalPlayerController())
			{
				return true;
			}
		}
	}

	return false;
}