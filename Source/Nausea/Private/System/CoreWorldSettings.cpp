// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "System/CoreWorldSettings.h"
#include "IAnimationBudgetAllocator.h"
#include "AnimationBudgetAllocatorParameters.h"

ACoreWorldSettings::ACoreWorldSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void ACoreWorldSettings::NotifyBeginPlay()
{
	if(!IsNetMode(NM_DedicatedServer) && GetWorld())
	{
		if (IAnimationBudgetAllocator* AnimationBudgetAllocator = IAnimationBudgetAllocator::Get(GetWorld()))
		{
			FAnimationBudgetAllocatorParameters Parameters = FAnimationBudgetAllocatorParameters();
			Parameters.BudgetInMs = 0.75f;
			Parameters.MaxTickRate = 15;
			Parameters.WorkUnitSmoothingSpeed = 10.f;
			Parameters.AlwaysTickFalloffAggression = 0.5f;
			Parameters.InterpolationFalloffAggression = 0.6f;
			Parameters.MaxInterpolatedComponents = 20;
			Parameters.MaxTickedOffsreenComponents = 2;
			Parameters.StateChangeThrottleInFrames = 15;
			Parameters.BudgetFactorBeforeReducedWork = 1.3f;
			Parameters.BudgetPressureSmoothingSpeed = 2.f;
			Parameters.BudgetFactorBeforeAggressiveReducedWork = 1.75f;
			Parameters.ReducedWorkThrottleMaxPerFrame = 30;

			AnimationBudgetAllocator->SetParameters(Parameters);

			
			AnimationBudgetAllocator->SetEnabled(true);
		}
	}

	Super::NotifyBeginPlay();
}