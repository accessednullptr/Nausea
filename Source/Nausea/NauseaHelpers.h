#pragma once

#include "CoreMinimal.h"
#include "NauseaHelpers.generated.h"

namespace CoreNoiseTag
{
	extern NAUSEA_API const FName Generic;

	extern NAUSEA_API const FName InventoryPickup;
	extern NAUSEA_API const FName InventoryDrop;
	extern NAUSEA_API const FName WeaponEquip;
	extern NAUSEA_API const FName WeaponPutDown;
	extern NAUSEA_API const FName WeaponFire;
	extern NAUSEA_API const FName WeaponReload;

	extern NAUSEA_API const FName MeleeSwing;
	extern NAUSEA_API const FName MeleeHit;

	extern NAUSEA_API const FName Damage;
	extern NAUSEA_API const FName Death;
}

#define IS_K2_FUNCTION_IMPLEMENTED(Object, FunctionName)\
(Object ? (Object->FindFunction(GET_FUNCTION_NAME_CHECKED(std::remove_pointer<decltype(Object)>::type, FunctionName)) ? Object->FindFunction(GET_FUNCTION_NAME_CHECKED(std::remove_pointer<decltype(Object)>::type, FunctionName))->IsInBlueprint() : false) : false)

class FNauseaHelpers
{
public:
	template<typename ElementType>
	FORCEINLINE static TArray<TSubclassOf<ElementType>> ConvertFromSoftClass(const TArray<TSoftClassPtr<ElementType>>& SoftClassList)
	{
		TArray<TSubclassOf<ElementType>> ClassList;
		ClassList.Reserve(SoftClassList.Num());

		for (const TSoftClassPtr<ElementType>& SoftClass : SoftClassList)
		{
			TSubclassOf<ElementType> LoadedClass = SoftClass.Get();

			if (!LoadedClass) //Is false if not a valid class, or if not loaded.
			{
				continue;
			}

			ClassList.Add(LoadedClass);
		}

		return ClassList;
	}

	FORCEINLINE static bool __IsK2FunctionImplementedInternal(UObject* Object, const FName& FunctionName)
	{
		if (!Object)
		{
			return false;
		}

		if (UFunction* TickFunction = Object->FindFunction(FunctionName))
		{
			if (TickFunction->IsInBlueprint())
			{
				return true;
			}
		}

		return false;
	}
};

USTRUCT(BlueprintType)
struct FCoreNoiseParams
{
	GENERATED_USTRUCT_BODY()

	FCoreNoiseParams() {}
	
	FCoreNoiseParams(const float InLoudness, const float InMaxRadius)
	{
		Loudness = InLoudness;
		MaxRadius = InMaxRadius;
	}

	FCoreNoiseParams(const FName& InTag, const float InLoudness = 1.f, const float InMaxRadius = 0.f)
	{
		Tag = InTag;
		Loudness = InLoudness;
		MaxRadius = InMaxRadius;
	}

	bool MakeNoise(AActor* Instigator, const FVector& Location = FVector::ZeroVector, float LoudnessMultiplier = 1.f) const;

public:
	UPROPERTY(EditDefaultsOnly)
	float Loudness = 1.f;
	UPROPERTY(EditDefaultsOnly)
	float MaxRadius = 0.f;

	UPROPERTY(EditDefaultsOnly)
	FName Tag = NAME_None;
};