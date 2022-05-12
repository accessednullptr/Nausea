// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.


#include "UI/SortableVerticalBox.h"
#include "Components/VerticalBoxSlot.h"

USortableVerticalBox::USortableVerticalBox(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void USortableVerticalBox::OnSlotAdded(UPanelSlot* AddedSlot)
{
	Super::OnSlotAdded(AddedSlot);

	if (!bPerformingSort && bNotifyOnChildAdded)
	{
		OnVerticalBoxChildAdded.Broadcast(this, AddedSlot ? AddedSlot->Content : nullptr);
	}
}

void USortableVerticalBox::OnSlotRemoved(UPanelSlot* RemovedSlot)
{
	if (!bPerformingSort)
	{
		OnVerticalBoxChildRemoved.Broadcast(this, RemovedSlot ? RemovedSlot->Content : nullptr);
	}

	Super::OnSlotRemoved(RemovedSlot);

	for (UPanelSlot* ChildSlot : Slots)
	{
		if (!ChildSlot || !ChildSlot->Content)
		{
			continue;
		}

		ChildSlot->Content->InvalidateLayoutAndVolatility();
	}
}

UVerticalBoxSlot* USortableVerticalBox::AddChildToSortableVerticalBox(UWidget* Content, bool bNotify)
{
	return AddChildToSortableVerticalBoxAtIndex(Content, Slots.Num(), bNotify);
}

UVerticalBoxSlot* USortableVerticalBox::AddChildToSortableVerticalBoxAtIndex(UWidget* Content, int32 Index, bool bNotify)
{
	if (!Content)
	{
		return nullptr;
	}

	if (Content->GetParent() == this)
	{
		Slots.Remove(Content->Slot);
		Slots.Insert(Content->Slot, Index);
		bNotifyOnChildAdded = bNotify;
		OnSlotAdded(Content->Slot);
		bNotifyOnChildAdded = true;
		InvalidateLayoutAndVolatility();
		return Cast<UVerticalBoxSlot>(Content->Slot);
	}

	Content->RemoveFromParent();

	EObjectFlags NewObjectFlags = RF_Transactional;
	if (HasAnyFlags(RF_Transient))
	{
		NewObjectFlags |= RF_Transient;
	}

	UVerticalBoxSlot* PanelSlot = NewObject<UVerticalBoxSlot>(this, GetSlotClass(), NAME_None, NewObjectFlags);
	PanelSlot->Content = Content;
	PanelSlot->Parent = this;
	Content->Slot = PanelSlot;

	Index = FMath::Clamp(Index, 0, Slots.Num());
	Slots.Insert(PanelSlot, Index);

	bNotifyOnChildAdded = bNotify;
	OnSlotAdded(PanelSlot);
	bNotifyOnChildAdded = true;

	InvalidateLayoutAndVolatility();

	return PanelSlot;
}

void USortableVerticalBox::SetChildWidgetsInOrder(const TArray<UWidget*>& InWidgetOrder)
{
	bPerformingSort = true;
	EObjectFlags NewObjectFlags = RF_Transactional;
	if (HasAnyFlags(RF_Transient))
	{
		NewObjectFlags |= RF_Transient;
	}

	for (int32 Index = InWidgetOrder.Num() - 1; Index >= 0; Index--)
	{
		UWidget* Content = InWidgetOrder[Index];

		if (!Content)
		{
			continue;
		}

		Content->RemoveFromParent();

		//If one of the new widgets is not parented to this container, reparent it now.
		if (Content->GetParent() != this)
		{
			UVerticalBoxSlot* PanelSlot = NewObject<UVerticalBoxSlot>(this, GetSlotClass(), NAME_None, NewObjectFlags);
			PanelSlot->Content = Content;
			PanelSlot->Parent = this;
			Content->Slot = PanelSlot;
		}

		Slots.Remove(Content->Slot);
		Slots.Insert(Content->Slot, Index);
		OnSlotAdded(Content->Slot);
	}

	InvalidateLayoutAndVolatility();
	bPerformingSort = false;
}