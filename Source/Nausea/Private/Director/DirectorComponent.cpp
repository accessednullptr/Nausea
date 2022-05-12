#include "Director/DirectorComponent.h"

UDirectorComponent::UDirectorComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
}