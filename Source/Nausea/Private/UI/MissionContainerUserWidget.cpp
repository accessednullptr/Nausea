// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.


#include "UI/MissionContainerUserWidget.h"
#include "Objective/MissionComponent.h"

UMissionContainerUserWidget::UMissionContainerUserWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UMissionContainerUserWidget::NativePreConstruct()
{
	if (!bHasPerformedFirstPreConsturct)
	{
		if (OwningMission)
		{
			OwningMission->OnMissionStatusChanged.AddDynamic(this, &UMissionContainerUserWidget::ReceivedMissionStatusChanged);
		}
		bHasPerformedFirstPreConsturct = true;
	}

	Super::NativePreConstruct();
}

void UMissionContainerUserWidget::ReceivedMissionStatusChanged(UMissionComponent* Mission, EMissionStatus Status)
{
	OnMissionStatusChanged(Mission, Status);
}