// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "CoreUserWidget.generated.h"

class AActor;
class ACorePlayerController;
class UPlayerStatisticsComponent;
class ACorePlayerState;
class UPlayerClassComponent;
class ACoreCharacter;
class UCoreWidgetComponent;

/**
 * 
 */
UCLASS()
class NAUSEA_API UCoreUserWidget : public UUserWidget
{
	GENERATED_UCLASS_BODY()
	
//~ Begin UUserWidget Interface
public:
	virtual bool Initialize() override;
//~ End UUserWidget Interface

public:
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "Widget")
	ACorePlayerController* GetOwningCorePlayerController() const;

	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "Widget")
	ACoreCharacter* GetOwningPlayerCharacter() const;

	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "Widget")
	void K2_SetWidgetMinimumDesiredSize(const FVector2D& InMinimumDesiredSize);

	virtual void InitializeWidgetComponent(UCoreWidgetComponent* OwningComponent);

	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "Widget")
	UCoreWidgetComponent* GetWidgetComponent() const;

	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "Widget")
	bool IsWidgetComponentWidget() const;

	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = "Widget")
	AActor* GetWidgetComponentOwner() const;

	void ReleaseWidgetToPool();

protected:
	UFUNCTION()
	void ReceivedPlayerDataReady();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Widget")
	void OnPlayerDataReady(ACorePlayerController* PlayerController, UPlayerStatisticsComponent* PlayerStatisticsComponent);

	UFUNCTION()
	void ReceivedPlayerState(ACorePlayerController* PlayerController, ACorePlayerState* PlayerState);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Widget")
	void OnReceivedPlayerState(ACorePlayerState* PlayerState);

	UFUNCTION()
	void ReceivedPlayerClassChange(ACorePlayerState* PlayerState, UPlayerClassComponent* PlayerClass);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Widget")
	void OnReceivedPlayerClass(UPlayerClassComponent* PlayerClass);

	//Returns this widget to the widget pool. This may not be used to destroy a component that is owned by an actor unless the owning actor is calling the function.
	UFUNCTION(BlueprintCallable, Category="Widget", meta=(BlueprintProtected, Keywords = "Release", HidePin="Object", DefaultToSelf="Object", DisplayName = "Release Widget To Pool", ScriptName = "ReleaseWidgetToPool"))
	void K2_ReleaseWidgetToPool(UObject* Object);

	UFUNCTION(BlueprintImplementableEvent, Category="Widget")
	void OnWidgetReleasedToPool();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = "Widget")
	void OnReceivedWidgetComponent(UCoreWidgetComponent* WidgetComponent);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Widget")
	bool bNotifyOnPlayerDataReady = false;

	UPROPERTY(EditDefaultsOnly, Category = "Widget")
	bool bNotifyOnReceivePlayerState = false;

	UPROPERTY(EditDefaultsOnly, Category = "Widget")
	bool bNotifyOnPlayerClassChanged = false;

private:
	UPROPERTY(Transient)
	UCoreWidgetComponent* OwningWidgetComponent = nullptr;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCoreWidgetLoadedSignature, UObject*, Object, TSubclassOf<UCoreUserWidget>, WidgetClass);

UCLASS()
class NAUSEA_API UCoreWidgetAsyncAction : public UBlueprintAsyncActionBase
{
	GENERATED_UCLASS_BODY()
	
//~ Begin UBlueprintAsyncActionBase Interface
	virtual void Activate() override;
	virtual void SetReadyToDestroy() override;
//~ End UBlueprintAsyncActionBase Interface

public:
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true"), Category = UI)
	static UCoreWidgetAsyncAction* LoadCoreWidget(UObject* Object, TSoftClassPtr<UCoreUserWidget> WidgetClass);

public:
	UPROPERTY(BlueprintAssignable)
	FOnCoreWidgetLoadedSignature OnCoreWidgetLoaded;

protected:
	UPROPERTY()
	TWeakObjectPtr<UObject> OwningObject = nullptr;

	UPROPERTY()
	TSoftClassPtr<UCoreUserWidget> SoftWidgetClass = nullptr;

	TSharedPtr<FStreamableHandle> StreamableHandle;

	UPROPERTY()
	bool bFailed = false;
};
