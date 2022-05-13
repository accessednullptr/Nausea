// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


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