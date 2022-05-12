// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/LocalMessage.h"
#include "CoreLocalMessage.generated.h"

UENUM(BlueprintType)
enum class ERecipientFilter : uint8
{
	AllyOnly,
	AllyOrNeutralOnly,
	Everyone
};

/**
 * 
 */
UCLASS()
class NAUSEA_API UCoreLocalMessage : public ULocalMessage
{
	GENERATED_UCLASS_BODY()
	
//~ Begin ULocalMessage Interface
public:
	virtual void ClientReceive(const FClientReceiveData& ClientData) const override;
//~ End ULocalMessage Interface

protected:
	virtual bool ShouldIgnoreData(const FClientReceiveData& ClientData) const;

	UPROPERTY(EditDefaultsOnly, Category = LocalMessage)
	ERecipientFilter SayRecipientFilter = ERecipientFilter::Everyone;
	UPROPERTY(EditDefaultsOnly, Category = LocalMessage)
	ERecipientFilter TeamSayRecipientFilter = ERecipientFilter::AllyOnly;
};
