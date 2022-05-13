// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/VerticalBox.h"
#include "SortableVerticalBox.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FVerticalBoxChildChangedSignature, USortableVerticalBox*, VerticalBox, UWidget*, Child);

/**
 * 
 */
UCLASS()
class NAUSEA_API USortableVerticalBox : public UVerticalBox
{
	GENERATED_UCLASS_BODY()

//~ Begin UPanelWidget Interface
protected:
	virtual void OnSlotAdded(UPanelSlot* AddedSlot) override;
	virtual void OnSlotRemoved(UPanelSlot* Slot) override;
//~ End UPanelWidget Interface


public:
	UVerticalBoxSlot* AddChildToSortableVerticalBox(UWidget* Content, bool bNotify = true);
	UFUNCTION(BlueprintCallable, Category=VerticalBox)
	UVerticalBoxSlot* AddChildToSortableVerticalBoxAtIndex(UWidget* Content, int32 Index = 0, bool bNotify = true);

	UFUNCTION(BlueprintCallable, Category=VerticalBox)
	void SetChildWidgetsInOrder(const TArray<UWidget*>& InWidgetOrder);

public:
	UPROPERTY(BlueprintAssignable)
	FVerticalBoxChildChangedSignature OnVerticalBoxChildAdded;
	UPROPERTY(BlueprintAssignable)
	FVerticalBoxChildChangedSignature OnVerticalBoxChildRemoved;

private:
	UPROPERTY(Transient)
	bool bNotifyOnChildAdded = true;
	UPROPERTY(Transient)
	bool bPerformingSort = false;
};