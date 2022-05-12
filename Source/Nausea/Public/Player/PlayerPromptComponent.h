// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Player/PlayerPrompt/PlayerPromptTypes.h"
#include "PlayerPromptComponent.generated.h"

USTRUCT()
struct NAUSEA_API FPromptStack
{
	GENERATED_USTRUCT_BODY()

public:
	bool IsValidPromptHandle(const FPromptHandle& PromptHandle) const;

	const FPromptHandle& PushPrompt(class UPlayerPromptComponent* PlayerPromptComponent, FPromptData&& PromptData);
	bool PopPrompt(class UPlayerPromptComponent* PlayerPromptComponent, const FPromptHandle& PromptHandle);

	const FPromptData& GetPromptData(const FPromptHandle& PromptHandle) const;
	FPromptData& GetPromptDataMutable(const FPromptHandle& PromptHandle);

	bool IsMarkedDisplayed(const FPromptHandle& PromptHandle) const;
	bool MarkDisplayed(const FPromptHandle& PromptHandle);

	const FPromptData& GetTopData() const;
	FPromptData& GetTopDataMutable();

private:
	TMap<FPromptHandle, FPromptData> PromptMap = TMap<FPromptHandle, FPromptData>();
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRequestDisplayPromptSignature, const FPromptHandle&, PromptHandle);

UCLASS(ClassGroup=(Custom))
class NAUSEA_API UPlayerPromptComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

	friend struct FPromptStack;

//~ Begin UActorComponent Interface
public:
	virtual void BeginPlay() override;
//~ End UActorComponent Interface

public:
	UFUNCTION()
	ACorePlayerController* GetOwningPlayerController() const;
	UFUNCTION()
	bool IsLocalPlayerController() const;

	//Returns true if a prompt was successfully pushed to the prompt stack.
	UFUNCTION(BlueprintCallable, Category = PlayerPromptComponent)
	const FPromptHandle& PushPlayerPrompt(TSubclassOf<UPromptInfo> PromptInfo, const FPromptResponseSignature& Delegate);

	UFUNCTION(BlueprintCallable, Category = PlayerPromptComponent)
	bool PopPlayerPrompt(const FPromptHandle& PromptHandle);

	UFUNCTION(BlueprintCallable, Category = PlayerPromptComponent)
	bool IsValidPromptHandle(const FPromptHandle& PromptHandle) const;

	const FPromptData& GetPromptData(const FPromptHandle& PromptHandle) const;
	FPromptData& GetPromptDataMutable(const FPromptHandle& PromptHandle);

	void AddPromptUserWidget(const FPromptHandle& PromptHandle, UPromptUserWidget* PromptWidget);

public:
	UPROPERTY(BlueprintAssignable, Category = PlayerPromptComponent)
	FRequestDisplayPromptSignature OnRequestDisplayPrompt;

protected:
	UFUNCTION()
	void DisplayPrompt(const FPromptHandle& PromptHandle);
	UFUNCTION()
	void DismissPrompt(const FPromptHandle& PromptHandle);

	UFUNCTION()
	void OnPromptResponse(const FPromptHandle& PromptHandle, bool bAccepted);

protected:
	UPROPERTY()
	TMap<FPromptHandle, TWeakObjectPtr<UPromptUserWidget>> PromptWidgetMap = TMap<FPromptHandle, TWeakObjectPtr<UPromptUserWidget>>();

private:
	UPROPERTY()
	FPromptStack PromptStack;
};