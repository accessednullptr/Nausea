// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Player/PlayerPrompt/PlayerPromptTypes.h"
#include "Internationalization/StringTableRegistry.h"
#include "Player/CorePlayerController.h"

const FPromptHandle FPromptHandle::InvalidHandle = FPromptHandle(0);

static uint32 GlobalPromptIDCounter = 0;
FPromptHandle::FPromptHandle()
{
	GlobalPromptIDCounter++;
	if (GlobalPromptIDCounter == 0) { GlobalPromptIDCounter++; }
	PromptHandle = GlobalPromptIDCounter;
}

void FPromptHandle::ResetPromptHandleCounter()
{
	GlobalPromptIDCounter = 0;
}

const FPromptData FPromptData::InvalidPrompt = FPromptData();

FPromptData FPromptData::GeneratePrompt(const TSubclassOf<UPromptInfo> InPromptInfoClass, const FPromptResponseSignature& Delegate)
{
	FPromptData Prompt = FPromptData();
	Prompt.PromptHandle = FPromptHandle();
	Prompt.PromptInfoClass = InPromptInfoClass;

	if (Delegate.IsBound())
	{
		Prompt.ResponseDelegateList.Add(Delegate);
	}

	return Prompt;
}

bool FPromptData::operator>(const FPromptData& Other) const
{
	if (GetPriority() == Other.GetPriority())
	{
		return PromptHandle < Other.PromptHandle;
	}

	return GetPriority() > Other.GetPriority();
}

uint8 FPromptData::GetPriority() const
{
	if (const UPromptInfo* PromptInfo = GetPromptInfo())
	{
		return PromptInfo->GetPriority();
	}

	return 0;
}

EPromptDuplicateLogic FPromptData::GetPromptDuplicateLogic() const
{
	if (const UPromptInfo* PromptInfo = GetPromptInfo())
	{
		return PromptInfo->GetDuplicateLogic();
	}

	return EPromptDuplicateLogic::Ignore;
}

void FPromptData::BroadcastResponse(bool bResponse)
{
	for (const FPromptResponseSignature& Delegate : ResponseDelegateList)
	{
		Delegate.ExecuteIfBound(GetPromptHandle(), bResponse);
	}

	ResponseDelegateList.Empty(ResponseDelegateList.Num());
}

UPromptInfo::UPromptInfo(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PromptAcceptText = LOCTABLE("/Game/Localization/PromptStringTable.PromptStringTable", "Prompt_Generic_Accept");
	PromptDeclineText = LOCTABLE("/Game/Localization/PromptStringTable.PromptStringTable", "Prompt_Generic_Cancel");
}

EPromptDuplicateLogic UPromptInfo::GetPromptDuplicateLogic(TSubclassOf<UPromptInfo> PlayerInfoClass)
{
	if (const UPromptInfo* PromptInfo = PlayerInfoClass.GetDefaultObject())
	{
		return PromptInfo->DuplicateLogic;
	}

	return EPromptDuplicateLogic::Invalid;
}

FText UPromptInfo::GetPromptText(TSubclassOf<UPromptInfo> PlayerInfoClass)
{
	if (const UPromptInfo* PromptInfo = PlayerInfoClass.GetDefaultObject())
	{
		return PromptInfo->PromptText;
	}

	return FText();
}

FText UPromptInfo::GetPromptAcceptText(TSubclassOf<UPromptInfo> PlayerInfoClass)
{
	if (const UPromptInfo* PromptInfo = PlayerInfoClass.GetDefaultObject())
	{
		return PromptInfo->PromptAcceptText;
	}

	return FText();
}

FText UPromptInfo::GetPromptDeclineText(TSubclassOf<UPromptInfo> PlayerInfoClass)
{
	if (const UPromptInfo* PromptInfo = PlayerInfoClass.GetDefaultObject())
	{
		return PromptInfo->PromptDeclineText;
	}

	return FText();
}

uint8 UPromptInfo::GetPromptPriority(TSubclassOf<UPromptInfo> PlayerInfoClass)
{
	if (const UPromptInfo* PromptInfo = PlayerInfoClass.GetDefaultObject())
	{
		return PromptInfo->PromptPriority;
	}

	return 0;
}

EPromptResponseDisplay UPromptInfo::GetPromptResponseDisplay(TSubclassOf<UPromptInfo> PromptInfoClass)
{
	if (const UPromptInfo* PromptInfo = PromptInfoClass.GetDefaultObject())
	{
		return PromptInfo->ResponseDisplay;
	}

	return EPromptResponseDisplay::Default;
}

UPromptInfoStatResetBase::UPromptInfoStatResetBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PromptAcceptText = LOCTABLE("/Game/Localization/PromptStringTable.PromptStringTable", "Prompt_Generic_Accept");
	PromptDeclineText = LOCTABLE("/Game/Localization/PromptStringTable.PromptStringTable", "Prompt_Generic_Cancel");
}

void UPromptInfoStatResetBase::NotifyPromptResponse(ACorePlayerController* PlayerController, bool bResponse) const
{
	if (!bResponse || !PlayerController)
	{
		return;
	}

	PlayerController->ResetPlayerStats();
}