// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "System/DebugCategory/GameplayDebuggerCategory_Routine.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "AI/CoreAIController.h"
#include "AI/RoutineManagerComponent.h"

FGameplayDebuggerCategory_Routine::FGameplayDebuggerCategory_Routine()
{
	SetDataPackReplication<FRepData>(&DataPack);
}

void FGameplayDebuggerCategory_Routine::CollectData(APlayerController* OwnerPC, AActor* DebugActor)
{
	APawn* MyPawn = Cast<APawn>(DebugActor);
	ACoreAIController* MyController = MyPawn ? Cast<ACoreAIController>(MyPawn->Controller) : nullptr;
	URoutineManagerComponent* RoutineComponent = MyController ? MyController->GetRoutineManagerComponent() : nullptr;

	if (RoutineComponent && !RoutineComponent->IsPendingKill())
	{
		DataPack.RoutineDescription = FString::Printf(TEXT("{yellow}%s\n{white}%s"),
			*RoutineComponent->GetName(),
			*RoutineComponent->DescribeRoutineManagerToGameplayDebugger());
	}
	else
	{
		DataPack.RoutineDescription = "";
	}
}

void FGameplayDebuggerCategory_Routine::DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext)
{
	if (DataPack.RoutineDescription.IsEmpty())
	{
		return;
	}

	CanvasContext.Printf(TEXT("%s"), *DataPack.RoutineDescription);
}

TSharedRef<FGameplayDebuggerCategory> FGameplayDebuggerCategory_Routine::MakeInstance()
{
	return MakeShareable(new FGameplayDebuggerCategory_Routine());
}

void FGameplayDebuggerCategory_Routine::FRepData::Serialize(FArchive& Ar)
{
	Ar << RoutineDescription;
}
#endif // WITH_GAMEPLAY_DEBUGGER