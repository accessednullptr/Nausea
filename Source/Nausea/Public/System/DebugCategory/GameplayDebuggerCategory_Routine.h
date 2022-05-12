// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#if WITH_GAMEPLAY_DEBUGGER
#include "CoreMinimal.h"
#include "GameplayDebuggerCategory.h"

class AActor;
class APlayerController;
/**
 * 
 */
class NAUSEA_API FGameplayDebuggerCategory_Routine : public FGameplayDebuggerCategory
{
public:
	FGameplayDebuggerCategory_Routine();

	virtual void CollectData(APlayerController* OwnerPC, AActor* DebugActor) override;
	virtual void DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext) override;

	static TSharedRef<FGameplayDebuggerCategory> MakeInstance();

protected:
	struct FRepData
	{
		FString RoutineDescription;

		void Serialize(FArchive& Ar);
	};
	FRepData DataPack;
};
#endif // WITH_GAMEPLAY_DEBUGGER