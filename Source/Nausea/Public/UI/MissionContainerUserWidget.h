// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/PawnUserWidget.h"
#include "MissionContainerUserWidget.generated.h"

class UMissionComponent;

/**
 * Used to display multiple child objectives for a given mission. We will likely not have too many different kinds of these.
 */
UCLASS()
class NAUSEA_API UMissionContainerUserWidget : public UPawnUserWidget
{
	GENERATED_UCLASS_BODY()
	
//~ Begin UUserWidget Interface	
public:
	virtual void NativePreConstruct() override;
//~ End UUserWidget Interface

protected:
	UFUNCTION()
	void ReceivedMissionStatusChanged(UMissionComponent* Mission, EMissionStatus Status);

	UFUNCTION(BlueprintImplementableEvent, Category = "Mission Status User Widget")
	void OnMissionStatusChanged(UMissionComponent* Mission, EMissionStatus Status);

protected:
	UPROPERTY(BlueprintReadOnly, meta = (ExposeOnSpawn = true))
	UMissionComponent* OwningMission = nullptr;

private:
	UPROPERTY()
	bool bHasPerformedFirstPreConsturct = false;
};
