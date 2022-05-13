// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Gameplay/StatusType.h"
#include "CombatLogWidget.generated.h"

class UTextBlock;

USTRUCT()
struct FCombatLog
{
	GENERATED_USTRUCT_BODY()

	FCombatLog() {}

	FCombatLog(UTextBlock* Widget, int32 ModifierCount)
	{
		WidgetList.Reserve(ModifierCount + 1);
		WidgetList.Add(Widget);
	}

public:
	FORCEINLINE void AddSubWidget(UTextBlock* Widget) { WidgetList.Add(Widget); }

	bool FadeOut(float DeltaTime);
	bool TickLifeTime(float DeltaTime) { LifeTime -= DeltaTime; return LifeTime <= 0.f; }

	TArray<UTextBlock*>& GetWidgetList() { return WidgetList; }

protected:
	UPROPERTY()
	TArray<UTextBlock*> WidgetList = TArray<UTextBlock*>();
	UPROPERTY()
	float LifeTime = 1.f;
};

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class NAUSEA_API UCombatLogWidget : public UUserWidget
{
	GENERATED_UCLASS_BODY()
	
//~ Begin UCombatLogWidget Interface
protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
//~ End UCombatLogWidget Interface

public:
	UFUNCTION(BlueprintCallable, Category = CombatLogWidget)
	void BindToCombatHistoryComponent(class UCombatHistoryComponent* CombatHistoryComponent);

	UFUNCTION(BlueprintCallable, Category = CombatLogWidget)
	void UnbindToCombatHistoryComponent(class UCombatHistoryComponent* CombatHistoryComponent);

	UFUNCTION(BlueprintCallable, Category = CombatLogWidget)
	bool IsVerbose() const { return true; }

	UFUNCTION(BlueprintCallable, Category = CombatLogWidget)
	int32 GetVisibleEntries() const { return 4; }

	UFUNCTION(BlueprintCallable, Category = CombatLogWidget)
	int32 GetEntryLimit() const { return 6; }

protected:
	UFUNCTION()
	void OnReceivedDamageLogEvent(UCombatHistoryComponent* Component, const FDamageLogEvent& DamageLogEvent);

	UFUNCTION()
	UTextBlock* CreateTextBlock();
	UFUNCTION()
	void AddToCombatLogPool(FCombatLog& CombatLog);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = CombatLogWidget)
	FSlateFontInfo EventFont;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = CombatLogWidget)
	FSlateFontInfo ModifierFont;

	UPROPERTY()
	class UVerticalBox* VerticalBox = nullptr;

	UPROPERTY()
	TArray<FCombatLog> CombatLogList = TArray<FCombatLog>();

private:
	UPROPERTY()
	TArray<UTextBlock*> TextBlockPool = TArray<UTextBlock*>();
};