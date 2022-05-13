// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "UI/StatusEffectUserWidget.h"
#include "Gameplay/StatusEffect/StatusEffectBase.h"

UStatusEffectUserWidget::UStatusEffectUserWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UStatusEffectUserWidget::NativePreConstruct()
{
	if (!bHasPerformedFirstPreConsturct)
	{
		if (StatusEffect)
		{
			StatusEffect->OnEffectEnd.AddDynamic(this, &UStatusEffectUserWidget::ReceiveStatusEffectEnd);
		}
		bHasPerformedFirstPreConsturct = true;
	}

	Super::NativePreConstruct();
}

UBaseStatusEffectUserWidget::UBaseStatusEffectUserWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UBasicStatusEffectUserWidget::UBasicStatusEffectUserWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UStatusEffectBasic* UBasicStatusEffectUserWidget::GetStatusEffect() const
{
	return Cast<UStatusEffectBasic>(StatusEffect);
}

void UBasicStatusEffectUserWidget::ReceiveStatusEffectEnd(UStatusEffectBase* Status, EStatusEndType EndType)
{
	OnStatusEffectEnd(Cast<UStatusEffectBasic>(Status), EndType);
}