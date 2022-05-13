// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Character/Customization/MainMenuCustomizationComponent.h"
#include "Engine/StreamableManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Character/CoreCharacter.h"
#include "Character/Customization/CustomizationObject.h"

UMainMenuCustomizationComponent::UMainMenuCustomizationComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UMainMenuCustomizationComponent::BeginPlay()
{
	for (ECustomizationSlot Slot : BodySlotList)
	{
		if (Slot == ECustomizationSlot::Body)
		{
			SkeletalMeshBodyComponents.FindOrAdd(Slot) = GetOwningCharacter()->GetMesh();
			continue;
		}

		USkeletalMeshComponent*& SkeletalMeshComponent = SkeletalMeshBodyComponents.FindOrAdd(Slot);
		SkeletalMeshComponent = NewObject<USkeletalMeshComponent>(this);
		SkeletalMeshComponent->RegisterComponent();
		SkeletalMeshComponent->AttachToComponent(SkeletalMeshBodyComponents[Slot], FAttachmentTransformRules::SnapToTargetIncludingScale);
		SkeletalMeshComponent->SetMasterPoseComponent(SkeletalMeshBodyComponents[Slot], true);
	}

	Super::BeginPlay();
}

void UMainMenuCustomizationComponent::UpdateCustomization()
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

	TMap<ECustomizationSlot, UCustomizationAsset*> CustomizationMap;
	for (UCustomizationAsset* Customization : CustomizationList)
	{
		if (!Customization)
		{
			continue;
		}

		CustomizationMap.Add(Customization->GetSlot()) = Customization;
	}

	for (ECustomizationSlot Slot : BodySlotList)
	{
		if (SkeletalMeshBodyComponents.Contains(Slot))
		{
			SkeletalMeshBodyComponents[Slot]->SetSkeletalMesh(CustomizationMap.Contains(Slot) ? CustomizationMap[Slot]->GetSkeletalMesh().LoadSynchronous() : nullptr);
		}
	}

	for (ECustomizationSlot Slot : AccessorySlotList)
	{
		if (!CustomizationMap.Contains(Slot))
		{
			continue;
		}

		if (!CustomizationMap[Slot]->GetStaticMesh().IsNull())
		{
			CustomizationMap[Slot]->GetStaticMesh().LoadSynchronous();
		}

		if (!CustomizationMap[Slot]->GetMaterial().IsNull())
		{
			CustomizationMap[Slot]->GetStaticMesh().LoadSynchronous();
		}

		if (!CustomizationMap[Slot]->GetSkeletalMesh().IsNull())
		{
			CustomizationMap[Slot]->GetSkeletalMesh().LoadSynchronous();
		}
	}

	for (ECustomizationSlot Slot : TextureSlotList)
	{
		if (CustomizationMap.Contains(Slot) && !CustomizationMap[Slot]->GetTexture().IsNull())
		{
			CustomizationMap[Slot]->GetTexture().LoadSynchronous();
		}
	}

	UCustomizationAsset::ApplyCustomizationList(this, GetOwningCharacter()->GetMesh(), CustomizationList);
}