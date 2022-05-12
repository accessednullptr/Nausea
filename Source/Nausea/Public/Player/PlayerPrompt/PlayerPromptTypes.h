// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "PlayerPromptTypes.generated.h"

class UPromptUserWidget;

UENUM(BlueprintType)
enum class EPromptDuplicateLogic : uint8
{
	ReplaceWithNewer,
	Ignore,
	AllowDuplicate,
	Invalid
};

UENUM(BlueprintType)
enum class EPromptResponseDisplay : uint8
{
	Default,
	AcceptOnly,
	DeclineOnly,
	TimedDisplay
};

USTRUCT(BlueprintType)
struct NAUSEA_API FPromptHandle
{
	GENERATED_USTRUCT_BODY()

	FPromptHandle();
	static void ResetPromptHandleCounter();

public:
	static const FPromptHandle InvalidHandle;

	operator uint32() const { return PromptHandle; }
	bool IsValid() const { return PromptHandle != 0; }

	bool operator==(const FPromptHandle& Other) const { return PromptHandle == Other.PromptHandle; }
	bool operator!=(const FPromptHandle& Other) const { return PromptHandle != Other.PromptHandle; }
	bool operator<(const FPromptHandle& Other) const { return PromptHandle < Other.PromptHandle; } //Used to check if this handle was made before another.

	FORCEINLINE friend uint32 GetTypeHash(const FPromptHandle& Other)
	{
		return GetTypeHash(Other.PromptHandle);
	}

protected:
	UPROPERTY()
	uint32 PromptHandle = 0;

private:
	FPromptHandle(uint32 InPromptHandle) { PromptHandle = InPromptHandle; }
};

DECLARE_DYNAMIC_DELEGATE_TwoParams(FPromptResponseSignature, const FPromptHandle&, PromptHandle, bool, bAccepted);

USTRUCT()
struct NAUSEA_API FPromptData
{
	GENERATED_USTRUCT_BODY()

public:
	FPromptData() {}
	static const FPromptData InvalidPrompt;
	static FPromptData GeneratePrompt(const TSubclassOf<UPromptInfo> InPromptInfoClass, const FPromptResponseSignature& Delegate);

	bool IsValid() const { return PromptHandle.IsValid(); }
	const FPromptHandle& GetPromptHandle() const { return PromptHandle; }

	const TSubclassOf<UPromptInfo>& GetPromptInfoClass() const { return PromptInfoClass; }
	const UPromptInfo* GetPromptInfo() const { return PromptInfoClass.GetDefaultObject(); }

	operator const FPromptHandle&() const { return PromptHandle; }
	bool operator==(const FPromptData& Other) const { return (PromptHandle == Other.PromptHandle); }
	bool operator>(const FPromptData& Other) const;

	bool operator==(const FPromptHandle& OtherID) const { return (PromptHandle == OtherID); }
	bool operator!=(const FPromptHandle& OtherID) const { return (PromptHandle != OtherID); }


	uint8 GetPriority() const;
	EPromptDuplicateLogic GetPromptDuplicateLogic() const;

	bool IsMarkedDisplayed() const { return bDisplayed; }
	void MarkDisplayed() { bDisplayed = true; }

	void BroadcastResponse(bool bResponse);

protected:
	UPROPERTY()
	FPromptHandle PromptHandle = FPromptHandle::InvalidHandle;

	UPROPERTY()
	bool bDisplayed = false;

	UPROPERTY()
	TSubclassOf<UPromptInfo> PromptInfoClass;

	UPROPERTY()
	TArray<FPromptResponseSignature> ResponseDelegateList;
};

UCLASS(BlueprintType, Blueprintable, AutoExpandCategories = (PromptInfo))
class NAUSEA_API UPromptInfo : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = PromptInfo)
	static EPromptDuplicateLogic GetPromptDuplicateLogic(TSubclassOf<UPromptInfo> PromptInfoClass);

	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = PromptInfo)
	static FText GetPromptText(TSubclassOf<UPromptInfo> PromptInfoClass);
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = PromptInfo)
	static FText GetPromptAcceptText(TSubclassOf<UPromptInfo> PromptInfoClass);
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = PromptInfo)
	static FText GetPromptDeclineText(TSubclassOf<UPromptInfo> PromptInfoClass);

	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = PromptInfo)
	static uint8 GetPromptPriority(TSubclassOf<UPromptInfo> PromptInfoClass);

	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = PromptInfo)
	static EPromptResponseDisplay GetPromptResponseDisplay(TSubclassOf<UPromptInfo> PromptInfoClass);

	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = PromptInfo)
	uint8 GetPriority() const { return PromptPriority; }
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = PromptInfo)
	EPromptDuplicateLogic GetDuplicateLogic() const { return DuplicateLogic; }
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = PromptInfo)
	const TSoftClassPtr<UPromptUserWidget>& GetUserWidgetClass() const { return PromptWidgetClass; }

	UFUNCTION()
	virtual void NotifyPromptResponse(ACorePlayerController* PlayerController, bool bResponse) const { return OnPromptResponse(PlayerController, bResponse); }

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = PromptInfo)
	void OnPromptResponse(ACorePlayerController* PlayerController, bool bResponse) const;

protected:
	//Determines what to do when a this prompt info is being pushed to the prompt list while another prompt with the same UPromptInfo is already in the list.
	UPROPERTY(EditDefaultsOnly, Category = PromptInfo)
	EPromptDuplicateLogic DuplicateLogic = EPromptDuplicateLogic::Ignore;

	UPROPERTY(EditDefaultsOnly, Category = PromptInfo)
	FText PromptText = FText();

	UPROPERTY(EditDefaultsOnly, Category = PromptInfo)
	FText PromptAcceptText = FText();

	UPROPERTY(EditDefaultsOnly, Category = PromptInfo)
	FText PromptDeclineText = FText();

	UPROPERTY(EditDefaultsOnly, Category = PromptInfo)
	TSoftClassPtr<UPromptUserWidget> PromptWidgetClass = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = PromptInfo)
	uint8 PromptPriority = 0;

	UPROPERTY(EditDefaultsOnly, Category = PromptInfo)
	EPromptResponseDisplay ResponseDisplay = EPromptResponseDisplay::Default;
};

UCLASS(BlueprintType, Blueprintable, AutoExpandCategories = (PromptInfo))
class NAUSEA_API UPromptInfoStatResetBase : public UPromptInfo
{
	GENERATED_UCLASS_BODY()

//~ Begin UPromptInfo Interface
public:
	virtual void NotifyPromptResponse(ACorePlayerController* PlayerController, bool bResponse) const override;
//~ End UPromptInfo Interface
};