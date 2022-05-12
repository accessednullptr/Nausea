// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Player/PlayerPrompt/PlayerPromptTypes.h"
#include "CoreHUD.generated.h"

class ACorePlayerController;
class UPlayerStatisticsComponent;
class ACorePlayerState;
class UPlayerClassComponent;
class UPromptInfo;
class UWidget;

USTRUCT()
struct FWidgetPool
{
	GENERATED_USTRUCT_BODY()

	FWidgetPool() {}

public:
	void PushWidget(UWidget* Widget) { Pool.Push(Widget); }
	UWidget* PopWidget() { return Pool.Num() > 0 ? Pool.Pop(false) : nullptr; }

protected:
	UPROPERTY()
	TArray<UWidget*> Pool;
};


/**
 * 
 */
UCLASS()
class NAUSEA_API ACoreHUD : public AHUD
{
	GENERATED_UCLASS_BODY()
	
//~ Begin AActor Interface
public:
	virtual void PostInitializeComponents() override;
//~ End AActor Interface

public:
	UFUNCTION(BlueprintCallable, Category=HUD)
	ACorePlayerController* GetOwningCorePlayerController() const;

	void ReleaseWidgetToPool(UWidget* Widget);

	template<class TWidgetClass>
	TWidgetClass* GetWidgetFromPool(TSubclassOf<UWidget> WidgetClass)
	{
		if (TWidgetClass* Widget = Cast<TWidgetClass>(WidgetPool.FindOrAdd(WidgetClass).PopWidget()))
		{
			return Widget;
		}

		return nullptr;
	}

protected:
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = HUD)
	void OnPlayerDataReady(ACorePlayerController* PlayerController, UPlayerStatisticsComponent* PlayerStatisticsComponent);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = HUD)
	void OnReceivedPlayerState(ACorePlayerState* PlayerState);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = HUD)
	void OnReceivedPlayerClass(UPlayerClassComponent* PlayerClass);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = HUD)
	void OnReceivedPlayerPrompt(const FPromptHandle& PromptHandle);

	UFUNCTION()
	void ReceivedPlayerDataReady();

	UFUNCTION()
	void ReceivedPlayerState(ACorePlayerController* PlayerController, ACorePlayerState* PlayerState);

	UFUNCTION()
	void ReceivedPlayerClassChange(ACorePlayerState* PlayerState, UPlayerClassComponent* PlayerClass);

	UFUNCTION()
	void ReceivedPlayerPrompt(const FPromptHandle& PromptHandle);

private:
	UPROPERTY(Transient)
	TMap<TSubclassOf<UWidget>, FWidgetPool> WidgetPool;
};
