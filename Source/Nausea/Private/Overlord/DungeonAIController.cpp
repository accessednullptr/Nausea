// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Overlord/DungeonAIController.h"
#include "AI/DungeonPathFollowingComponent.h"

ADungeonAIController::ADungeonAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UDungeonPathFollowingComponent>(TEXT("PathFollowingComponent")))
{
#if WITH_EDITOR
	//UNavigationSystemV1::SetNavigationAutoUpdateEnabled(false, nullptr);
#endif // WITH_EDITOR
}