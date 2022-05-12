// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/CoreUserWidget.h"
#include "ChatUserWidget.generated.h"

UENUM(BlueprintType)
enum class EChatEntryType : uint8
{
	Text,
	VoiceCommand,
	Notification,
	NotificationCritical
};

/**
 * 
 */
UCLASS()
class NAUSEA_API UChatUserWidget : public UCoreUserWidget
{
	GENERATED_UCLASS_BODY()

//~ Begin UUserWidget Interface
public:
	virtual bool Initialize() override;
//~ End UUserWidget Interface

protected:
	UFUNCTION()
	void ProcessLocalMessage(const FName& MessageType, int32 MessageIndex, const FString& MessageString, ACorePlayerState* PlayerStateA, ACorePlayerState* PlayerStateB, const UObject* OptionalObject);

	UFUNCTION()
	UChatEntryUserWidget* GetChatEntryInstance();
	
	UFUNCTION(BlueprintImplementableEvent, Category = ChatUserWidget)
	void AddChatEntry(UChatEntryUserWidget* ChatEntry);

protected:
	UPROPERTY(EditDefaultsOnly, Category = ChatUserWidget)
	TSubclassOf<UChatEntryUserWidget> ChatEntryWidgetClass = nullptr;
};

UCLASS()
class NAUSEA_API UChatEntryUserWidget : public UCoreUserWidget
{
	GENERATED_UCLASS_BODY()
	
public:
	UFUNCTION()
	void SetEntryData(ACorePlayerState* InCorePlayerState, EChatEntryType InType, const FString& InText);

	UFUNCTION(BlueprintImplementableEvent, Category = ChatEntryUserWidget)
	void ReceiveChatEntryData(ACorePlayerState* InCorePlayerState, EChatEntryType InType, const FString& InText);

protected:
	UPROPERTY()
	ACorePlayerState* CorePlayerState = nullptr;

	UPROPERTY()
	EChatEntryType Type = EChatEntryType::Text;

	UPROPERTY()
	FString Text = FString("");
};
