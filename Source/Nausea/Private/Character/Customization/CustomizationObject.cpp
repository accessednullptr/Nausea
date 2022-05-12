// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.


#include "Character/Customization/CustomizationObject.h"
#include "Engine/AssetManager.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Character/CoreCustomizationComponent.h"

const TArray<ECustomizationSlot> BodySlotList = {
	ECustomizationSlot::Body,
	ECustomizationSlot::Head,
	ECustomizationSlot::Helmet,
	ECustomizationSlot::Backpack
};

const TArray<ECustomizationSlot> AccessorySlotList = {
	ECustomizationSlot::JacketShoulderMarks,
	ECustomizationSlot::JacketArmPatchLeft,
	ECustomizationSlot::JacketArmPatchRight,
	ECustomizationSlot::JacketLapelBadgeLeft,
	ECustomizationSlot::JacketLapelBadgeRight,
	ECustomizationSlot::JacketCuffBadges,
	ECustomizationSlot::JacketCollarBadge
};

const TArray<ECustomizationSlot> TextureSlotList = {
	ECustomizationSlot::JacketTextureOverride,
	ECustomizationSlot::UndershirtTextureOverride,
	ECustomizationSlot::PantsTextureOverride,
	ECustomizationSlot::GlovesTextureOverride,
	ECustomizationSlot::BootsTextureOverride,
	ECustomizationSlot::StrapsTextureOverride,
	ECustomizationSlot::HairTextureOverride,
	ECustomizationSlot::HelmetTextureOverride,
	ECustomizationSlot::PlayerClassATextureOverride,
	ECustomizationSlot::PlayerClassBTextureOverride
};

const FPrimaryAssetType UCustomizationAsset::CustomizationAssetType = FName(TEXT("CustomizationAsset"));

UCustomizationAsset::UCustomizationAsset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

FPrimaryAssetId UCustomizationAsset::GetPrimaryAssetId() const
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return FPrimaryAssetId(UCustomizationAsset::StaticClass()->GetFName(), FPackageName::GetShortFName(GetOutermost()->GetName()));
	}

	return FPrimaryAssetId(UCustomizationAsset::StaticClass()->GetFName(), GetFName());
}

FText UCustomizationAsset::GetSlotEditorName() const
{
	switch (CustomizationSlot)
	{
	case ECustomizationSlot::Body:
		return FText::FromString("Body Customization");
	case ECustomizationSlot::Head:
		return FText::FromString("Head Customization");
	case ECustomizationSlot::Helmet:
		return FText::FromString("Helmet Customization");
	case ECustomizationSlot::Backpack:
		return FText::FromString("Backpack Customization");

	case ECustomizationSlot::JacketShoulderMarks:
		return FText::FromString("Shoulder Mark Customization");
	
	case ECustomizationSlot::JacketArmPatchLeft:
		return FText::FromString("Left Arm Patch Customization");
	case ECustomizationSlot::JacketArmPatchRight:
		return FText::FromString("Right Arm Patch Customization");
	
	case ECustomizationSlot::JacketLapelBadgeLeft:
		return FText::FromString("Left Lapel Badge Customization");
	case ECustomizationSlot::JacketLapelBadgeRight:
		return FText::FromString("Right Lapel Badge Customization");

	case ECustomizationSlot::JacketCuffBadges:
		return FText::FromString("Cuff Badges Customization");
	case ECustomizationSlot::JacketCollarBadge:
		return FText::FromString("Collar Badge Customization");

	case ECustomizationSlot::JacketTextureOverride:
		return FText::FromString("Jacket Texture Customization");
	case ECustomizationSlot::UndershirtTextureOverride:
		return FText::FromString("Undershirt Texture Customization");
	case ECustomizationSlot::PantsTextureOverride:
		return FText::FromString("Pants Texture Customization");
	case ECustomizationSlot::GlovesTextureOverride:
		return FText::FromString("Gloves Texture Customization");
	case ECustomizationSlot::BootsTextureOverride:
		return FText::FromString("Boots Texture Customization");
	case ECustomizationSlot::StrapsTextureOverride:
		return FText::FromString("Straps Texture Customization");
	case ECustomizationSlot::HairTextureOverride:
		return FText::FromString("Hair Texture Customization");
	case ECustomizationSlot::HelmetTextureOverride:
		return FText::FromString("Helmet Texture Customization");

	case ECustomizationSlot::PlayerClassATextureOverride:
		return FText::FromString("Class Primary Texture Customization");
	case ECustomizationSlot::PlayerClassBTextureOverride:
		return FText::FromString("Class Secondary Texture Customization");
	}

	return FText::FromString("Invalid");
}

bool UCustomizationAsset::ValidateCustomization(ACorePlayerController* PlayerController, ACoreCharacter* Character, TSet<UCustomizationAsset*>& CustomizationSet)
{
	TArray<UCustomizationAsset*> CustomizationList = CustomizationSet.Array();
	bool bResult = ValidateCustomization(PlayerController, Character, CustomizationList);

	CustomizationSet.Reset();
	for (UCustomizationAsset* Customization : CustomizationList)
	{
		if (!Customization)
		{
			continue;
		}

		CustomizationSet.Add(Customization);
	}

	return bResult;
}

bool UCustomizationAsset::ValidateCustomization(ACorePlayerController* PlayerController, ACoreCharacter* Character, TArray<UCustomizationAsset*>& CustomizationList)
{
	TMap<ECustomizationSlot, UCustomizationAsset*> CustomizationMap;
	CustomizationMap.Reserve(CustomizationList.Num());

	for (UCustomizationAsset* CustomizationObject : CustomizationList)
	{
		if (!CustomizationObject)
		{
			continue;
		}

		ECustomizationSlot Slot = CustomizationObject->GetSlot();

		if (Slot == ECustomizationSlot::Invalid || CustomizationMap.Contains(Slot))
		{
			continue;
		}

		CustomizationMap.Add(Slot) = CustomizationObject;
	}

	bool Result = ValidateCustomization(PlayerController, Character, CustomizationMap);

	CustomizationList.Reset();
	CustomizationMap.GenerateValueArray(CustomizationList);

	return Result;
}

inline void ValidateOrUpdateCustomizationFromBody(TMap<ECustomizationSlot, UCustomizationAsset*>& CustomizationMap, ECustomizationSlot Slot, const UCustomizationAsset_Body* BodyCustomization)
{
	const TSoftObjectPtr<UCustomizationAsset>& DefaultCustomizationObject = BodyCustomization->GetDefaultCustomizationForSlot(Slot);
	if (!CustomizationMap.Contains(Slot))
	{
		if (!DefaultCustomizationObject.IsNull())
		{
			CustomizationMap.Add(Slot) = DefaultCustomizationObject.Get();
		}

		return;
	}

	const TSubclassOf<UCustomizationListAsset>& CustomizationListClass = BodyCustomization->GetCustomizationListForSlot(Slot);

	if (!CustomizationListClass)
	{
		if (DefaultCustomizationObject != CustomizationMap[Slot])
		{
			CustomizationMap[Slot] = DefaultCustomizationObject.Get();
		}
		else
		{
			CustomizationMap[Slot] = nullptr;
		}

		return;
	}

	UCustomizationAsset*& Customization = CustomizationMap[Slot];
	const TArray<TSoftObjectPtr<UCustomizationAsset>> CustomizationList = CustomizationListClass.GetDefaultObject()->GetCustomizationList();
	if (!CustomizationList.Contains(Customization))
	{
		if (!DefaultCustomizationObject.IsNull())
		{
			Customization = DefaultCustomizationObject.Get();
		}
		else
		{
			CustomizationMap[Slot] = nullptr;
		}
	}
};

inline void ValidateOrUpdateCustomizationFromArray(TMap<ECustomizationSlot, UCustomizationAsset*>& CustomizationMap, ECustomizationSlot Slot, const UCustomizationAsset_Body* BodyCustomization, const TArray<TSoftObjectPtr<UCustomizationAsset>>& InCustomizationList)
{
	const TSoftObjectPtr<UCustomizationAsset>& DefaultCustomizationObject = BodyCustomization->GetDefaultCustomizationForSlot(Slot);
	if (!CustomizationMap.Contains(Slot))
	{
		if (!DefaultCustomizationObject.IsNull())
		{
			CustomizationMap.Add(Slot) = DefaultCustomizationObject.Get();
		}

		return;
	}

	if (InCustomizationList.Num() == 0)
	{
		if (DefaultCustomizationObject != CustomizationMap[Slot])
		{
			CustomizationMap[Slot] = DefaultCustomizationObject.Get();
		}

		return;
	}

	UCustomizationAsset*& Customization = CustomizationMap[Slot];
	if (!InCustomizationList.Contains(Customization))
	{
		if (!DefaultCustomizationObject.IsNull())
		{
			Customization = DefaultCustomizationObject.Get();
		}
		else
		{
			Customization = nullptr;
		}
	}
};

bool UCustomizationAsset::ValidateCustomization(ACorePlayerController* PlayerController, ACoreCharacter* Character, TMap<ECustomizationSlot, UCustomizationAsset*>& CustomizationMap)
{
	if (CustomizationMap.Num() == 0)
	{
		return false;
	}

	if (!CustomizationMap.Contains(ECustomizationSlot::Body))
	{
		return false;
	}

	//One of the few we'll need to directly cast to - this is the "core" customization that validates all others.
	const UCustomizationAsset_Body* BodyCustomization = Cast<UCustomizationAsset_Body>(CustomizationMap[ECustomizationSlot::Body]);

	if (!BodyCustomization)
	{
		return false;
	}

	ValidateOrUpdateCustomizationFromBody(CustomizationMap, ECustomizationSlot::Head, BodyCustomization);
	ValidateOrUpdateCustomizationFromBody(CustomizationMap, ECustomizationSlot::Helmet, BodyCustomization);
	ValidateOrUpdateCustomizationFromBody(CustomizationMap, ECustomizationSlot::Backpack, BodyCustomization);

	const TSubclassOf<UCustomizationListAsset>& AccessoryListClass = BodyCustomization->GetCustomizationListForSlot(ECustomizationSlot::JacketShoulderMarks);
	const TArray<TSoftObjectPtr<UCustomizationAsset>>& AccessoryCustomizationList = AccessoryListClass ? AccessoryListClass.GetDefaultObject()->GetCustomizationList() : TArray<TSoftObjectPtr<UCustomizationAsset>>();
	UCustomizationAsset* DefaultAccessory = nullptr;
	for (ECustomizationSlot Slot : AccessorySlotList)
	{
		ValidateOrUpdateCustomizationFromArray(CustomizationMap, Slot, BodyCustomization, AccessoryCustomizationList);
	}

	const TSubclassOf<UCustomizationListAsset>& TextureListClass = BodyCustomization->GetCustomizationListForSlot(ECustomizationSlot::JacketTextureOverride);
	const TArray<TSoftObjectPtr<UCustomizationAsset>>& TextureCustomizationList = TextureListClass ? TextureListClass.GetDefaultObject()->GetCustomizationList() : TArray<TSoftObjectPtr<UCustomizationAsset>>();
	UCustomizationAsset* DefaultTexture = nullptr;
	for (ECustomizationSlot Slot : TextureSlotList)
	{
		ValidateOrUpdateCustomizationFromArray(CustomizationMap, Slot, BodyCustomization, AccessoryCustomizationList);
	}

	TArray<ECustomizationSlot> NullSlotList;
	for (const TPair<ECustomizationSlot, UCustomizationAsset*> Entry : CustomizationMap)
	{
		if (!Entry.Value)
		{
			NullSlotList.Add(Entry.Key);
		}
	}

	for (ECustomizationSlot Key : NullSlotList)
	{
		CustomizationMap.Remove(Key);
	}

	return true;
}

TArray<TSoftObjectPtr<USkeletalMesh>> UCustomizationAsset::GetMeshMergeListForCustomizationList(const TArray<UCustomizationAsset*>& CustomizationList)
{
	TArray<TSoftObjectPtr<USkeletalMesh>> ResultMeshList;

	for (UCustomizationAsset* Customization : CustomizationList)
	{
		if (!Customization || !Customization->NeedsMergeToBaseMesh() || Customization->GetSkeletalMesh().IsNull())
		{
			continue;
		}

		if (Customization->GetSlot() == ECustomizationSlot::Body)
		{
			ResultMeshList.Insert(Customization->GetSkeletalMesh(), 0);
		}
		else
		{
			ResultMeshList.Add(Customization->GetSkeletalMesh());
		}
	}

	return ResultMeshList;
}

void UCustomizationAsset::ApplyCustomizationList(UCoreCustomizationComponent* CustomizationComponent, USkeletalMeshComponent* SkeletalMeshComponent, const TArray<UCustomizationAsset*>& CustomizationList)
{
	if (!CustomizationComponent || !SkeletalMeshComponent)
	{
		return;
	}

	for (UCustomizationAsset* Customization : CustomizationList)
	{
		if (!Customization || !Customization->NeedsApply())
		{
			continue;
		}

		Customization->Apply(CustomizationComponent, SkeletalMeshComponent);
	}
}

UCustomizationAsset_Accessory::UCustomizationAsset_Accessory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UCustomizationAsset_Accessory_StaticMesh::UCustomizationAsset_Accessory_StaticMesh(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bNeedsApply = true;
}

bool UCustomizationAsset_Accessory_StaticMesh::Apply(UCoreCustomizationComponent* CustomizationComponent, USkeletalMeshComponent* SkeletalMeshComponent) const
{
	if (StaticMesh.IsNull() || !CustomizationComponent || !SkeletalMeshComponent)
	{
		return false;
	}

	if (StaticMesh.IsValid())
	{
		OnStaticMeshLoaded(CustomizationComponent, SkeletalMeshComponent);
		return true;
	}

	if (!UAssetManager::IsValid())
	{
		return false;
	}

	FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();
	
	TWeakObjectPtr<UCustomizationAsset_Accessory_StaticMesh> WeakThis = const_cast<UCustomizationAsset_Accessory_StaticMesh*>(this);
	TWeakObjectPtr<UCoreCustomizationComponent> WeakCustomizationComponent = CustomizationComponent;
	TWeakObjectPtr<USkeletalMeshComponent> WeakSkeletalMeshComponent = SkeletalMeshComponent;

	TArray<FSoftObjectPath> SoftObjectPathList = TArray<FSoftObjectPath>();
	SoftObjectPathList.Add(StaticMesh.ToSoftObjectPath());
	
	if (!MaterialOverride.IsNull())
	{
		SoftObjectPathList.Add(MaterialOverride.ToSoftObjectPath());
	}

	TSharedPtr<FStreamableHandle> Handle = StreamableManager.RequestAsyncLoad(SoftObjectPathList, FStreamableDelegate::CreateWeakLambda(WeakThis.Get(), [WeakThis, WeakCustomizationComponent, WeakSkeletalMeshComponent]()
		{
			if (!WeakThis.IsValid() || !WeakCustomizationComponent.IsValid() || !WeakSkeletalMeshComponent.IsValid())
			{
				return;
			}

			WeakThis->OnStaticMeshLoaded(WeakCustomizationComponent.Get(), WeakSkeletalMeshComponent.Get());
		}));

	if (Handle->HasLoadCompleted())
	{
		OnStaticMeshLoaded(CustomizationComponent, SkeletalMeshComponent);
	}
	else
	{
		CustomizationComponent->AddCustomizationStreamHandle(Handle);
	}

	return true;
}

void UCustomizationAsset_Accessory_StaticMesh::OnStaticMeshLoaded(UCoreCustomizationComponent* CustomizationComponent, USkeletalMeshComponent* SkeletalMeshComponent) const
{
	UStaticMesh* LoadedStaticMesh = StaticMesh.Get();
	if (!StaticMesh)
	{
		return;
	}

	UMaterialInstance* LoadedMaterialOverride = MaterialOverride.Get();

	TArray<FName> AttachPointList;
	switch (CustomizationSlot)
	{
	case ECustomizationSlot::JacketShoulderMarks:
		AttachPointList.Add("JacketShoulderMarkLeft");
		AttachPointList.Add("JacketShoulderMarkRight");
		break;
	case ECustomizationSlot::JacketArmPatchLeft:
		AttachPointList.Add("JacketArmPatchLeft");
		break;
	case ECustomizationSlot::JacketArmPatchRight:
		AttachPointList.Add("JacketArmPatchRight");
		break;
	case ECustomizationSlot::JacketLapelBadgeLeft:
		AttachPointList.Add("JacketLapelBadgeLeft");
		break;
	case ECustomizationSlot::JacketLapelBadgeRight:
		AttachPointList.Add("JacketLapelBadgeRight");
		break;
	case ECustomizationSlot::JacketCuffBadges:
		AttachPointList.Add("JacketCuffBadgesLeft");
		AttachPointList.Add("JacketCuffBadgesRight");
		break;
	case ECustomizationSlot::JacketCollarBadge:
		AttachPointList.Add("JacketCollarBadge");
		break;
	}

	AActor* Owner = CustomizationComponent->GetOwner();
	for (const FName& AttachPoint : AttachPointList)
	{
		UStaticMeshComponent* StaticMeshComponent = NewObject<UStaticMeshComponent>(Owner, UStaticMeshComponent::StaticClass());
		StaticMeshComponent->RegisterComponent();
		StaticMeshComponent->AttachToComponent(SkeletalMeshComponent, FAttachmentTransformRules::SnapToTargetIncludingScale, AttachPoint);

		StaticMeshComponent->SetStaticMesh(StaticMesh.Get());

		if (LoadedMaterialOverride)
		{
			StaticMeshComponent->SetMaterial(0, LoadedMaterialOverride);
		}
	}
}

UCustomizationAsset_Accessory_SkeletalMesh::UCustomizationAsset_Accessory_SkeletalMesh(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bNeedsMergeToBaseMesh = true;
}

UCustomizationAsset_TextureOverride::UCustomizationAsset_TextureOverride(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bNeedsApply = true;
}

bool UCustomizationAsset_TextureOverride::Apply(UCoreCustomizationComponent* CustomizationComponent, USkeletalMeshComponent* SkeletalMeshComponent) const
{
	if (!CustomizationComponent || !SkeletalMeshComponent || Texture.IsNull())
	{
		return false;
	}

	if (Texture.IsValid())
	{
		OnTextureLoaded(CustomizationComponent, SkeletalMeshComponent);
		return true;
	}

	if (!UAssetManager::IsValid())
	{
		return false;
	}

	FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();

	TWeakObjectPtr<UCustomizationAsset_TextureOverride> WeakThis = const_cast<UCustomizationAsset_TextureOverride*>(this);
	TWeakObjectPtr<UCoreCustomizationComponent> WeakCustomizationComponent = CustomizationComponent;
	TWeakObjectPtr<USkeletalMeshComponent> WeakSkeletalMeshComponent = SkeletalMeshComponent;
	TSharedPtr<FStreamableHandle> Handle = StreamableManager.RequestAsyncLoad(Texture.ToSoftObjectPath(), FStreamableDelegate::CreateWeakLambda(WeakThis.Get(), [WeakThis, WeakCustomizationComponent, WeakSkeletalMeshComponent]()
		{
			if (!WeakThis.IsValid() || !WeakCustomizationComponent.IsValid() || !WeakSkeletalMeshComponent.IsValid())
			{
				return;
			}

			WeakThis->OnTextureLoaded(WeakCustomizationComponent.Get(), WeakSkeletalMeshComponent.Get());
		}));

	if (Handle->HasLoadCompleted())
	{
		OnTextureLoaded(CustomizationComponent, SkeletalMeshComponent);
	}
	else
	{
		CustomizationComponent->AddCustomizationStreamHandle(Handle);
	}

	return true;
}

void UCustomizationAsset_TextureOverride::OnTextureLoaded(UCoreCustomizationComponent* CustomizationComponent, USkeletalMeshComponent* SkeletalMeshComponent) const
{
	if (!Texture.IsValid())
	{
		return;
	}

	FName TextureParamName = NAME_None;

	switch (CustomizationSlot)
	{
	case ECustomizationSlot::JacketTextureOverride:
		TextureParamName = "JacketTexture";
		break;
	case ECustomizationSlot::UndershirtTextureOverride:
		TextureParamName = "UndershirtTexture";
		break;
	case ECustomizationSlot::PantsTextureOverride:
		TextureParamName = "PantsTexture";
		break;
	case ECustomizationSlot::GlovesTextureOverride:
		TextureParamName = "GlovesTexture";
		break;
	case ECustomizationSlot::BootsTextureOverride:
		TextureParamName = "BootsTexture";
		break;
	case ECustomizationSlot::StrapsTextureOverride:
		TextureParamName = "StrapsTexture";
		break;
	case ECustomizationSlot::HairTextureOverride:
		TextureParamName = "HairTexture";
		break;
	case ECustomizationSlot::HelmetTextureOverride:
		TextureParamName = "HelmetTexture";
		break;
	case ECustomizationSlot::PlayerClassATextureOverride:
		TextureParamName = "PlayerClassATexture";
		break;
	case ECustomizationSlot::PlayerClassBTextureOverride:
		TextureParamName = "PlayerClassBTexture";
		break;
	}

	TArray<UMaterialInterface*> MaterialList = SkeletalMeshComponent->GetMaterials();
	int32 Index = -1;

	for (UMaterialInterface* Material : MaterialList)
	{
		Index++;
		UMaterialInstance* MaterialInstance = Cast<UMaterialInstance>(Material);

		if (!MaterialInstance)
		{
			continue;
		}

		//We don't use any of the defined functions to get this data due to wanting to make this check as light as possible.
		const int32 ParameterIndex = MaterialInstance->TextureParameterValues.IndexOfByPredicate([&TextureParamName](const FTextureParameterValue& Parameter)
		{
			return Parameter.ParameterInfo.Name == TextureParamName;
		});

		if (ParameterIndex == INDEX_NONE)
		{
			continue;
		}

		UMaterialInstanceDynamic* MID = SkeletalMeshComponent->CreateDynamicMaterialInstance(Index);

		if (!MID)
		{
			continue;
		}

		MID->SetTextureParameterValue(TextureParamName, Texture.Get());
	}
}

UCustomizationAsset_CoreMesh::UCustomizationAsset_CoreMesh(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bNeedsMergeToBaseMesh = true;
}

UCustomizationAsset_Body::UCustomizationAsset_Body(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CustomizationSlot = ECustomizationSlot::Body;
}

const TSubclassOf<UCustomizationListAsset>& UCustomizationAsset_Body::GetCustomizationListForSlot(ECustomizationSlot Slot) const
{
	static TSubclassOf<UCustomizationListAsset> InvalidList = nullptr;

	switch (Slot)
	{
	case ECustomizationSlot::Body:
		return InvalidList;
	case ECustomizationSlot::Head:
		return HeadList;
	case ECustomizationSlot::Helmet:
		return HelmetList;
	case ECustomizationSlot::Backpack:
		return BackpackList;

	case ECustomizationSlot::JacketShoulderMarks:
	case ECustomizationSlot::JacketArmPatchLeft:
	case ECustomizationSlot::JacketArmPatchRight:
	case ECustomizationSlot::JacketLapelBadgeLeft:
	case ECustomizationSlot::JacketLapelBadgeRight:
	case ECustomizationSlot::JacketCuffBadges:
	case ECustomizationSlot::JacketCollarBadge:
		return AccessoryList;

	case ECustomizationSlot::JacketTextureOverride:
	case ECustomizationSlot::UndershirtTextureOverride:
	case ECustomizationSlot::PantsTextureOverride:
	case ECustomizationSlot::GlovesTextureOverride:
	case ECustomizationSlot::BootsTextureOverride:
	case ECustomizationSlot::StrapsTextureOverride:
	case ECustomizationSlot::HairTextureOverride:
	case ECustomizationSlot::HelmetTextureOverride:
	case ECustomizationSlot::PlayerClassATextureOverride:
	case ECustomizationSlot::PlayerClassBTextureOverride:
		return TextureOverrideList;
	}

	return InvalidList;
}

TSoftObjectPtr<UCustomizationAsset> UCustomizationAsset_Body::GetDefaultCustomizationForSlot(ECustomizationSlot Slot) const
{
	static TSoftObjectPtr<UCustomizationAsset> InvalidSlot = nullptr;

	switch (Slot)
	{
	case ECustomizationSlot::Body:
		return InvalidSlot;
	case ECustomizationSlot::Head:
		return DefaultHeadCustomization;
	case ECustomizationSlot::Helmet:
		return DefaultHelmetCustomization;
	case ECustomizationSlot::Backpack:
		return DefaultBackpackCustomization;

	case ECustomizationSlot::JacketShoulderMarks:
		return DefaultShoulderMarksCustomization;
	case ECustomizationSlot::JacketArmPatchLeft:
		return DefaultArmPatchLeftCustomization;
	case ECustomizationSlot::JacketArmPatchRight:
		return DefaultArmPatchRightCustomization;
	case ECustomizationSlot::JacketLapelBadgeLeft:
		return DefaultLapelBadgeLeftCustomization;
	case ECustomizationSlot::JacketLapelBadgeRight:
		return DefaultLapelBadgeRightCustomization;
	case ECustomizationSlot::JacketCuffBadges:
		return DefaultCuffBadgesCustomization;
	case ECustomizationSlot::JacketCollarBadge:
		return DefaultCollarBadgeCustomization;

	case ECustomizationSlot::JacketTextureOverride:
		return DefaultJacketTextureCustomization;
	case ECustomizationSlot::UndershirtTextureOverride:
		return DefaultUndershirtTextureCustomization;
	case ECustomizationSlot::PantsTextureOverride:
		return DefaultPantsTextureCustomization;
	case ECustomizationSlot::GlovesTextureOverride:
		return DefaultGlovesTextureCustomization;
	case ECustomizationSlot::BootsTextureOverride:
		return DefaultBootsTextureCustomization;
	case ECustomizationSlot::StrapsTextureOverride:
		return DefaultStrapsTextureCustomization;
	case ECustomizationSlot::HairTextureOverride:
		return DefaultHairTextureCustomization;
	case ECustomizationSlot::HelmetTextureOverride:
		return DefaultHelmetTextureCustomization;
	case ECustomizationSlot::PlayerClassATextureOverride:
		return DefaultPlayerClassPrimaryTextureCustomization;
	case ECustomizationSlot::PlayerClassBTextureOverride:
		return DefaultPlayerClassSecondaryTextureCustomization;
	}

	return InvalidSlot;
}

UCustomizationListAsset::UCustomizationListAsset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

FPrimaryAssetId UCustomizationListAsset::GetPrimaryAssetId() const
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return FPrimaryAssetId(UCustomizationListAsset::StaticClass()->GetFName(), FPackageName::GetShortFName(GetOutermost()->GetName()));
	}

	return FPrimaryAssetId(UCustomizationListAsset::StaticClass()->GetFName(), GetFName());
}