// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Player/PlayerPromptComponent.h"
#include "Blueprint/UserWidget.h"
#include "Player/CorePlayerController.h"
#include "UI/PromptUserWidget.h"

bool FPromptStack::IsValidPromptHandle(const FPromptHandle& PromptHandle) const
{
	if (!PromptHandle.IsValid())
	{
		return false;
	}

	return PromptMap.Contains(PromptHandle);
}

const FPromptHandle& FPromptStack::PushPrompt(class UPlayerPromptComponent* PlayerPromptComponent, FPromptData&& InPromptData)
{
	const FPromptHandle& InPromptHandle = InPromptData.GetPromptHandle();

	const uint8 PushPriority = InPromptData.GetPriority();
	const EPromptDuplicateLogic PromptDuplicateLogic = InPromptData.GetPromptDuplicateLogic();
	
	if (PromptDuplicateLogic != EPromptDuplicateLogic::AllowDuplicate)
	{
		const TSubclassOf<UPromptInfo>& PromptInfoClass = InPromptData.GetPromptInfoClass();
		TArray<FPromptHandle> PromptHandleList;
		PromptMap.GenerateKeyArray(PromptHandleList);

		for (const FPromptHandle& PromptHandle : PromptHandleList)
		{
			if (PromptMap[PromptHandle].GetPromptInfoClass() != PromptInfoClass)
			{
				continue;
			}

			const FPromptHandle& DuplicatePromptHandle = PromptMap[PromptHandle].GetPromptHandle();

			switch (PromptDuplicateLogic)
			{
			case EPromptDuplicateLogic::ReplaceWithNewer:
				if (PlayerPromptComponent) { PlayerPromptComponent->DismissPrompt(DuplicatePromptHandle); }
				PromptMap.Remove(PromptHandle);
				PromptMap.Add(InPromptHandle) = MoveTemp(InPromptData);
				return InPromptHandle;
			case EPromptDuplicateLogic::Ignore:
				return FPromptHandle::InvalidHandle;
			}
		}
	}

	PromptMap.Add(InPromptHandle) = MoveTemp(InPromptData);
	return InPromptHandle;
}

bool FPromptStack::PopPrompt(class UPlayerPromptComponent* PlayerPromptComponent, const FPromptHandle& PromptHandle)
{
	if (!IsValidPromptHandle(PromptHandle))
	{
		return false;
	}

	if (PlayerPromptComponent && PromptMap[PromptHandle].IsMarkedDisplayed())
	{
		PlayerPromptComponent->DismissPrompt(PromptHandle);
	}

	PromptMap.Remove(PromptHandle);
	return true;
}

const FPromptData& FPromptStack::GetPromptData(const FPromptHandle& PromptHandle) const
{
	if (IsValidPromptHandle(PromptHandle))
	{
		return PromptMap[PromptHandle];
	}

	return FPromptData::InvalidPrompt;
}

FPromptData& FPromptStack::GetPromptDataMutable(const FPromptHandle& PromptHandle)
{
	return const_cast<FPromptData&>(GetPromptData(PromptHandle));
}

bool FPromptStack::IsMarkedDisplayed(const FPromptHandle& PromptHandle) const
{
	const FPromptData& PromptData = GetPromptData(PromptHandle);
	return PromptData.IsValid() && PromptData.IsMarkedDisplayed();
}

bool FPromptStack::MarkDisplayed(const FPromptHandle& PromptHandle)
{
	FPromptData& PromptData = GetPromptDataMutable(PromptHandle);

	if (!PromptData.IsValid())
	{
		return false;
	}

	PromptData.MarkDisplayed();
	return true;
}

const FPromptData& FPromptStack::GetTopData() const
{
	if (PromptMap.Num() == 0)
	{
		return FPromptData::InvalidPrompt;
	}

	TArray<FPromptHandle> PromptHandleList;
	PromptMap.GenerateKeyArray(PromptHandleList);

	FPromptHandle HighestPriorityHandle = FPromptHandle::InvalidHandle;

	for (const FPromptHandle& PromptHandle : PromptHandleList)
	{
		if (!HighestPriorityHandle.IsValid())
		{
			HighestPriorityHandle = PromptHandle;
		}

		if (PromptMap[PromptHandle] > PromptMap[HighestPriorityHandle])
		{
			HighestPriorityHandle = PromptHandle;
		}
	}

	if (IsValidPromptHandle(HighestPriorityHandle))
	{
		return PromptMap[HighestPriorityHandle];
	}

	return FPromptData::InvalidPrompt;
}

FPromptData& FPromptStack::GetTopDataMutable()
{
	return const_cast<FPromptData&>(GetTopData());
}

UPlayerPromptComponent::UPlayerPromptComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UPlayerPromptComponent::BeginPlay()
{
	Super::BeginPlay();

	if (IsLocalPlayerController())
	{
		FPromptHandle::ResetPromptHandleCounter();
	}
}

ACorePlayerController* UPlayerPromptComponent::GetOwningPlayerController() const
{
	return Cast<ACorePlayerController>(GetOwner());
}

bool UPlayerPromptComponent::IsLocalPlayerController() const
{
	if (!GetOwningPlayerController())
	{
		return false;
	}

	return GetOwningPlayerController()->IsLocalPlayerController();
}

const FPromptHandle& UPlayerPromptComponent::PushPlayerPrompt(TSubclassOf<UPromptInfo> PromptInfo, const FPromptResponseSignature& Delegate)
{
	if (!IsLocalPlayerController())
	{
		return FPromptHandle::InvalidHandle;
	}

	const FPromptHandle& PromptHandle = PromptStack.PushPrompt(this, FPromptData::GeneratePrompt(PromptInfo, Delegate));

	const FPromptData& TopPromptData = PromptStack.GetTopData();
	if (TopPromptData.IsValid() && !TopPromptData.IsMarkedDisplayed())
	{
		DisplayPrompt(TopPromptData.GetPromptHandle());
	}

	return PromptHandle;
}

bool UPlayerPromptComponent::PopPlayerPrompt(const FPromptHandle& PromptHandle)
{
	if (!IsLocalPlayerController())
	{
		return false;
	}

	bool bPoppedHandle = PromptStack.PopPrompt(this, PromptHandle);

	FPromptData& TopPromptData = PromptStack.GetTopDataMutable();
	if (TopPromptData.IsValid() && !TopPromptData.IsMarkedDisplayed())
	{
		DisplayPrompt(TopPromptData.GetPromptHandle());
	}

	return bPoppedHandle;
}

bool UPlayerPromptComponent::IsValidPromptHandle(const FPromptHandle& PromptHandle) const
{
	if (!PromptHandle.IsValid())
	{
		return false;
	}

	return PromptStack.IsValidPromptHandle(PromptHandle);
}

const FPromptData& UPlayerPromptComponent::GetPromptData(const FPromptHandle& PromptHandle) const
{
	return PromptStack.GetPromptData(PromptHandle);
}

FPromptData& UPlayerPromptComponent::GetPromptDataMutable(const FPromptHandle& PromptHandle)
{
	return const_cast<FPromptData&>(GetPromptData(PromptHandle));
}

void UPlayerPromptComponent::AddPromptUserWidget(const FPromptHandle& PromptHandle, UPromptUserWidget* PromptWidget)
{
	if (!PromptWidget)
	{
		return;
	}

	PromptWidget->OnPromptResponseEvent.AddUObject(this, &UPlayerPromptComponent::OnPromptResponse);
	PromptWidget->SetPromptData(GetPromptData(PromptHandle));
	PromptWidgetMap.Add(PromptHandle) = TWeakObjectPtr<UPromptUserWidget>(PromptWidget);
}

void UPlayerPromptComponent::DisplayPrompt(const FPromptHandle& PromptHandle)
{
	if (!PromptStack.IsValidPromptHandle(PromptHandle) || PromptStack.IsMarkedDisplayed(PromptHandle))
	{
		return;
	}

	PromptStack.MarkDisplayed(PromptHandle);
	OnRequestDisplayPrompt.Broadcast(PromptHandle);
}

void UPlayerPromptComponent::DismissPrompt(const FPromptHandle& PromptHandle)
{
	if (!PromptStack.IsValidPromptHandle(PromptHandle) || !PromptStack.IsMarkedDisplayed(PromptHandle))
	{
		return;
	}

	if (PromptWidgetMap.Contains(PromptHandle) && PromptWidgetMap[PromptHandle].IsValid())
	{
		PromptWidgetMap[PromptHandle]->DismissPrompt();
		PromptWidgetMap.Remove(PromptHandle);
	}
}

void UPlayerPromptComponent::OnPromptResponse(const FPromptHandle& PromptHandle, bool bAccepted)
{
	if (!IsValidPromptHandle(PromptHandle))
	{
		return;
	}

	FPromptData& PromptData = GetPromptDataMutable(PromptHandle);

	if (const UPromptInfo* PromptInfo = PromptData.GetPromptInfo())
	{
		PromptInfo->NotifyPromptResponse(GetOwningPlayerController(), bAccepted);
	}

	PromptData.BroadcastResponse(bAccepted);
	PopPlayerPrompt(PromptHandle);
}