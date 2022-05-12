// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.


#include "UI/CombatLogWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Gameplay/DamageLogInterface.h"
#include "Character/CombatHistoryComponent.h"

bool FCombatLog::FadeOut(float DeltaTime)
{
	if (WidgetList.Num() == 0)
	{
		return true;
	}

	bool bAllFadedOut = true;
	for (UWidget* Widget : WidgetList)
	{
		if (!Widget)
		{
			continue;
		}

		const float Opacity = FMath::Clamp(Widget->GetRenderOpacity() - (DeltaTime * 2.f), 0.f, 1.f);
		Widget->SetRenderOpacity(Opacity);

		bAllFadedOut &= (Opacity == 0.f);
	}

	return bAllFadedOut;
}

UCombatLogWidget::UCombatLogWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UCombatLogWidget::NativeOnInitialized()
{
	VerticalBox = WidgetTree->ConstructWidget<UVerticalBox>();
	WidgetTree->RootWidget = VerticalBox;
	Super::NativeOnInitialized();
}

void UCombatLogWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	int32 Index = 0;
	while (Index < CombatLogList.Num())
	{
		if (CombatLogList[Index].TickLifeTime(InDeltaTime))
		{
			if (CombatLogList[Index].FadeOut(InDeltaTime))
			{
				AddToCombatLogPool(CombatLogList[Index]);
				CombatLogList.RemoveAt(Index, 1, false);
			}
		}

		Index++;
	}

	if (CombatLogList.Num() <= GetVisibleEntries())
	{
		return;
	}

	Index = 0;
	while (Index < CombatLogList.Num() - GetVisibleEntries())
	{
		if (CombatLogList[Index].FadeOut(InDeltaTime))
		{
			AddToCombatLogPool(CombatLogList[Index]);
			CombatLogList.RemoveAt(Index, 1, false);
		}

		Index++;
	}
}

void UCombatLogWidget::BindToCombatHistoryComponent(UCombatHistoryComponent* CombatHistoryComponent)
{
	if (!CombatHistoryComponent || CombatHistoryComponent->OnReceivedDamageLogEvent.IsAlreadyBound(this, &UCombatLogWidget::OnReceivedDamageLogEvent))
	{
		return;
	}

	CombatHistoryComponent->OnReceivedDamageLogEvent.AddDynamic(this, &UCombatLogWidget::OnReceivedDamageLogEvent);
}

void UCombatLogWidget::UnbindToCombatHistoryComponent(UCombatHistoryComponent* CombatHistoryComponent)
{
	if (!CombatHistoryComponent || !CombatHistoryComponent->OnReceivedDamageLogEvent.IsAlreadyBound(this, &UCombatLogWidget::OnReceivedDamageLogEvent))
	{
		return;
	}

	CombatHistoryComponent->OnReceivedDamageLogEvent.RemoveDynamic(this, &UCombatLogWidget::OnReceivedDamageLogEvent);
}

void UCombatLogWidget::OnReceivedDamageLogEvent(UCombatHistoryComponent* Component, const FDamageLogEvent& DamageLogEvent)
{
	if (!WidgetTree || !VerticalBox)
	{
		return;
	}

	const FText EventLogText = UDamageLogStatics::GenerateDamageLogEventText(DamageLogEvent);

	//If we've hit entry limit, immediately get rid of top-most entry and return it to the pool so we can use it.
	if (CombatLogList.Num() != 0 && CombatLogList.Num() >= GetEntryLimit())
	{
		AddToCombatLogPool(CombatLogList[0]);
		CombatLogList.RemoveAt(0, 1, false);
	}

	if (EventLogText.IsEmpty())
	{
		return;
	}

	UTextBlock* EventText = CreateTextBlock();
	EventText->SetFont(EventFont);
	EventText->SetText(EventLogText);

	CombatLogList.Push(FCombatLog(EventText, DamageLogEvent.ModifierList.Num()));

	UVerticalBoxSlot* EventSlot = VerticalBox->AddChildToVerticalBox(EventText);
	EventSlot->SetHorizontalAlignment(HAlign_Right);
	EventSlot->SetVerticalAlignment(VAlign_Bottom);

	if (IsVerbose() && DamageLogEvent.ModifierList.Num() > 0)
	{
		FCombatLog& CombatLogWidget = CombatLogList.Last();

		for (const FDamageLogEventModifier& Modifier : DamageLogEvent.ModifierList)
		{
			const FText ModifierLogText = UDamageLogStatics::GenerateDamageLogEventModifierText(Modifier);

			if (ModifierLogText.IsEmpty())
			{
				continue;
			}

			UTextBlock* ModifierText = CreateTextBlock();
			ModifierText->SetFont(ModifierFont);
			ModifierText->SetText(ModifierLogText);

			UVerticalBoxSlot* ModifierSlot = VerticalBox->AddChildToVerticalBox(ModifierText);
			ModifierSlot->SetHorizontalAlignment(HAlign_Right);
			ModifierSlot->SetVerticalAlignment(VAlign_Bottom);
			Invalidate(EInvalidateWidgetReason::Layout);

			CombatLogWidget.AddSubWidget(ModifierText);
		}
	}
}

UTextBlock* UCombatLogWidget::CreateTextBlock()
{
	if (!WidgetTree)
	{
		return nullptr;
	}

	if (TextBlockPool.Num() != 0)
	{
		UTextBlock* TextBlock = TextBlockPool.Pop(false);
		TextBlock->SetRenderOpacity(1.f);
	}

	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>();
	return TextBlock;
}

void UCombatLogWidget::AddToCombatLogPool(FCombatLog& CombatLog)
{
	TArray<UTextBlock*>& WidgetList = CombatLog.GetWidgetList();

	if (WidgetList.Num() == 0)
	{
		return;
	}

	for (UTextBlock* Widget : WidgetList)
	{
		TextBlockPool.Add(Widget);
		VerticalBox->RemoveChild(Widget);
	}

	WidgetList.Empty();
}