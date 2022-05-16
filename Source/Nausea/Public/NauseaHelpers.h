#pragma once

#include "CoreMinimal.h"

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