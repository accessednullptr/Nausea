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
class NAUSEA_API FGameplayDebuggerCategory_Mission : public FGameplayDebuggerCategory
{
public:
	FGameplayDebuggerCategory_Mission();

	virtual void CollectData(APlayerController* OwnerPC, AActor* DebugActor) override;
	virtual void DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext) override;

	static TSharedRef<FGameplayDebuggerCategory> MakeInstance();

protected:
	struct FRepData
	{
		TArray<FString> MissionDescriptionList;

		struct FObjectiveData
		{
			FObjectiveData() {}

			FObjectiveData(int32 Index, FString Description)
			{
				MissionIndex = Index;
				ObjectiveDescription = Description;
			}

			static TArray<FObjectiveData> GetArrayFromDescriptionList(int32 Index, TArray<FString> DescriptionList)
			{
				TArray<FObjectiveData> Result;
				Result.Reserve(DescriptionList.Num());

				for (FString Description : DescriptionList)
				{
					Result.Add(FObjectiveData(Index, Description));
				}

				return Result;
			}

			int32 MissionIndex = 0;
			FString ObjectiveDescription = "";
		};
		TArray<FObjectiveData> ObjectiveDescriptionList;

		void Serialize(FArchive& Ar);
	};
	FRepData DataPack;
};
#endif // WITH_GAMEPLAY_DEBUGGER
