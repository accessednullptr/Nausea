// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "UI/CoreWidgetComponent.h"
#include "UI/CoreUserWidget.h"

UCoreWidgetComponent::UCoreWidgetComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UCoreWidgetComponent::InitWidget()
{
	Super::InitWidget();

	if (UCoreUserWidget* CoreUserWidget = Cast<UCoreUserWidget>(GetWidget()))
	{
		CoreUserWidget->InitializeWidgetComponent(this);
	}
}