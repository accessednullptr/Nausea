// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#include "Character/CoreCustomizationComponent.h"
#include "SkeletalMeshMerge.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/Skeleton.h"
#include "Components/SkeletalMeshComponent.h"
#include "Character/CoreCharacter.h"
#include "Character/Customization/CustomizationObject.h"

UCoreCustomizationComponent::UCoreCustomizationComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UCoreCustomizationComponent::BeginPlay()
{
    Super::BeginPlay();
    SetCustomization(DefaultCustomizationList);
}

void UCoreCustomizationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

void UCoreCustomizationComponent::SetCustomization(const TArray<UCustomizationAsset*>& InCustomizationList)
{
    CustomizationList = InCustomizationList;
    ValidateCustomization(CustomizationList);
    UpdateCustomization();
}

void UCoreCustomizationComponent::AddCustomizationStreamHandle(TSharedPtr<FStreamableHandle> StreamHandle)
{
    CustomizationStreamableHandleList.Add(StreamHandle);
}

void UCoreCustomizationComponent::AddCustomizationComponent(UActorComponent* Component)
{
    CustomizationComponentList.Add(Component);
}

void UCoreCustomizationComponent::UpdateCustomization()
{
    if (IsNetMode(NM_DedicatedServer))
    {
        return;
    }

    for (TWeakObjectPtr<UActorComponent> ActorComponent : CustomizationComponentList)
    {
        if (!ActorComponent.IsValid())
        {
            continue;
        }

        ActorComponent->DestroyComponent();
    }
    CustomizationComponentList.Reset();

    for (TSharedPtr<FStreamableHandle> Handle : CustomizationStreamableHandleList)
    {
        if (!Handle.IsValid())
        {
            continue;
        }

        Handle->CancelHandle();
    }
    CustomizationStreamableHandleList.Reset();

    if (!bPerformMergeInEditor || CustomizationList.Num() == 0)
    {
        return;
    }

    TArray<TSoftObjectPtr<USkeletalMesh>> MeshList = UCustomizationAsset::GetMeshMergeListForCustomizationList(CustomizationList);

    if (MeshList.Num() == 0)
    {
        return;
    }

    MeshMergeHandle = UCustomizationMergeManager::RequestMeshMerge(this, FMeshList(MeshList), FCustomizationMergeRequestDelegate::CreateUObject(this, &UCoreCustomizationComponent::OnMeshMergeComplete));

    if (MeshMergeHandle.IsValid() && UCustomizationMergeManager::IsMeshMergeComplete(this, MeshMergeHandle))
    {
        OnMeshMergeComplete();
    }
}

void UCoreCustomizationComponent::ValidateCustomization(TArray<UCustomizationAsset*>& InCustomizationList)
{
	UCustomizationAsset::ValidateCustomization(Cast<ACorePlayerController>(GetOwningController()), GetOwningCharacter(), InCustomizationList);
}

void UCoreCustomizationComponent::OnMeshMergeComplete()
{
	if (IsNetMode(NM_DedicatedServer))
	{
		return;
	}

    FMergedMesh MergedMesh = UCustomizationMergeManager::GetMergedMesh(this, MeshMergeHandle);
    
    if (ensure(MergedMesh.IsValid()))
	{
		GetOwningCharacter()->GetMesh()->SetSkeletalMesh(MergedMesh.GetSkeletalMesh());
        UCustomizationAsset::ApplyCustomizationList(this, GetOwningCharacter()->GetMesh(), CustomizationList);
    }
}