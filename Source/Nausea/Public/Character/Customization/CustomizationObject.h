// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/DataAsset.h"
#include "CustomizationObject.generated.h"

class ACorePlayerController;
class ACoreCharacter;
class UCoreCustomizationComponent;
class USkeletalMesh;
class UMaterialInstance;

UENUM(BlueprintType)
enum class ECustomizationSlot : uint8
{
	//Core mesh parts.
	Body = 0,
	Head = 1,
	Helmet = 3,
	Backpack = 4,

	//Accessories
	JacketShoulderMarks = 50,

	JacketArmPatchLeft = 55,
	JacketArmPatchRight = 56,
	
	JacketLapelBadgeLeft = 60,
	JacketLapelBadgeRight = 61,

	JacketCuffBadges = 65,

	JacketCollarBadge = 70,

	//Texture Overrides
	JacketTextureOverride = 100,
	UndershirtTextureOverride = 101,
	PantsTextureOverride = 102,
	GlovesTextureOverride = 103,
	BootsTextureOverride = 104,
	StrapsTextureOverride = 105,
	HairTextureOverride = 106,
	HelmetTextureOverride = 107,

	//Player Class Specific Texture Overrides
	PlayerClassATextureOverride = 130,
	PlayerClassBTextureOverride = 131,

	Invalid = 254,
	MAX = 255
};
extern const TArray<ECustomizationSlot> BodySlotList;
extern const TArray<ECustomizationSlot> AccessorySlotList;
extern const TArray<ECustomizationSlot> TextureSlotList;

/**
 * Holds data about a specific customization. Should be accessed via CDO and never modified. All new customizations should receive its own CustomziationFactory subclass (see CustomizationFactory.h).
 */
UCLASS(NotBlueprintable, BlueprintType)
class NAUSEA_API UCustomizationAsset : public UPrimaryDataAsset
{
	GENERATED_UCLASS_BODY()

public:
	static const FPrimaryAssetType CustomizationAssetType;

//~ Begin UObject Interface
public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
//~ End UObject Interface

public:
	virtual TSoftObjectPtr<USkeletalMesh> GetSkeletalMesh() const { return nullptr; }
	virtual TSoftObjectPtr<UTexture2D> GetTexture() const { return nullptr; }
	virtual TSoftObjectPtr<UStaticMesh> GetStaticMesh() const { return nullptr; }
	virtual TSoftObjectPtr<UMaterialInstance> GetMaterial() const { return nullptr; }
	bool NeedsMergeToBaseMesh() const { return bNeedsMergeToBaseMesh; }
	bool NeedsApply() const { return bNeedsApply; }
	virtual bool Apply(UCoreCustomizationComponent* CustomizationComponent, USkeletalMeshComponent* Component) const { return false; }

	ECustomizationSlot GetSlot() const { return CustomizationSlot; }
	FText GetSlotEditorName() const;

	static bool ValidateCustomization(ACorePlayerController* PlayerController, ACoreCharacter* Character, TSet<UCustomizationAsset*>& CustomizationSet);
	static bool ValidateCustomization(ACorePlayerController* PlayerController, ACoreCharacter* Character, TArray<UCustomizationAsset*>& CustomizationList);
	static bool ValidateCustomization(ACorePlayerController* PlayerController, ACoreCharacter* Character, TMap<ECustomizationSlot, UCustomizationAsset*>& CustomizationMap);

	static TArray<TSoftObjectPtr<USkeletalMesh>> GetMeshMergeListForCustomizationList(const TArray<UCustomizationAsset*>& CustomizationList);
	static void ApplyCustomizationList(UCoreCustomizationComponent* CustomizationComponent, USkeletalMeshComponent* SkeletalMeshComponent, const TArray<UCustomizationAsset*>& CustomizationList);

protected:
	//Set by child class constructor.
	UPROPERTY()
	ECustomizationSlot CustomizationSlot = ECustomizationSlot::Invalid;
	UPROPERTY()
	bool bNeedsMergeToBaseMesh = false;
	UPROPERTY()
	bool bNeedsApply = false;

	UPROPERTY(EditDefaultsOnly, Category = Customization)
	TSoftObjectPtr<UTexture2D> PreviewIcon = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = Customization)
	FText CustomizationName = FText();

};

UCLASS()
class NAUSEA_API UCustomizationAsset_Accessory : public UCustomizationAsset
{
	GENERATED_UCLASS_BODY()
};

UCLASS()
class NAUSEA_API UCustomizationAsset_Accessory_StaticMesh : public UCustomizationAsset_Accessory
{
	GENERATED_UCLASS_BODY()

//~ Begin UCustomizationAsset Interface
public:
	virtual TSoftObjectPtr<UStaticMesh> GetStaticMesh() const override { return StaticMesh; }
	virtual TSoftObjectPtr<UMaterialInstance> GetMaterial() const override { return MaterialOverride; }
	virtual bool Apply(UCoreCustomizationComponent* CustomizationComponent, USkeletalMeshComponent* SkeletalMeshComponent) const;
//~ End UCustomizationAsset Interface

protected:
	void OnStaticMeshLoaded(UCoreCustomizationComponent* CustomizationComponent, USkeletalMeshComponent* SkeletalMeshComponent) const;
	
protected:
	UPROPERTY(EditDefaultsOnly, Category = Customization)
	TSoftObjectPtr<UStaticMesh> StaticMesh = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = Customization)
	TSoftObjectPtr<UMaterialInstance> MaterialOverride = nullptr;
};

UCLASS()
class NAUSEA_API UCustomizationAsset_Accessory_SkeletalMesh : public UCustomizationAsset_Accessory
{
	GENERATED_UCLASS_BODY()

//~ Begin UCustomizationAsset Interface
public:
	virtual TSoftObjectPtr<USkeletalMesh> GetSkeletalMesh() const override { return SkeletalMesh; }
//~ End UCustomizationAsset Interface

protected:
	UPROPERTY(EditDefaultsOnly, Category = Customization)
	TSoftObjectPtr<USkeletalMesh> SkeletalMesh = nullptr;
};

UCLASS()
class NAUSEA_API UCustomizationAsset_TextureOverride : public UCustomizationAsset
{
	GENERATED_UCLASS_BODY()

//~ Begin UCustomizationAsset Interface		
public:
	virtual TSoftObjectPtr<UTexture2D> GetTexture() const override { return Texture; }
	virtual bool Apply(UCoreCustomizationComponent* CustomizationComponent, USkeletalMeshComponent* SkeletalMeshComponent) const;
//~ End UCustomizationAsset Interface

protected:
	void OnTextureLoaded(UCoreCustomizationComponent* CustomizationComponent, USkeletalMeshComponent* SkeletalMeshComponent) const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = Customization)
	TSoftObjectPtr<UTexture2D> Texture = nullptr;
};

UCLASS()
class NAUSEA_API UCustomizationAsset_CoreMesh : public UCustomizationAsset
{
	GENERATED_UCLASS_BODY()

//~ Begin UCustomizationAsset Interface
public:
	virtual TSoftObjectPtr<USkeletalMesh> GetSkeletalMesh() const override { return SkeletalMesh; }
//~ End UCustomizationAsset Interface

protected:
	//Mesh to merge for this core mesh.
	UPROPERTY(EditDefaultsOnly, Category = Customization)
	TSoftObjectPtr<USkeletalMesh> SkeletalMesh = nullptr;
};

//--------------------------------------------------------------
// CORE MESH CUSTOMIZATION OBJECTS

UCLASS(AutoCollapseCategories=("Customization|Default Texture Overrides", "Customization|Default Accessory Overrides"))
class NAUSEA_API UCustomizationAsset_Body : public UCustomizationAsset_CoreMesh
{
	GENERATED_UCLASS_BODY()

public:
	const TSubclassOf<UCustomizationListAsset>& GetCustomizationListForSlot(ECustomizationSlot CustomizationSlot) const;
	TSoftObjectPtr<UCustomizationAsset> GetDefaultCustomizationForSlot(ECustomizationSlot CustomizationSlot) const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = Customization, meta = (AllowedClasses = "CustomizationAsset_Head"))
	TSoftObjectPtr<UCustomizationAsset> DefaultHeadCustomization = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = Customization, meta = (AllowedClasses = "CustomizationAsset_Helmet"))
	TSoftObjectPtr<UCustomizationAsset> DefaultHelmetCustomization = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = Customization, meta = (AllowedClasses = "CustomizationAsset_Backpack"))
	TSoftObjectPtr<UCustomizationAsset> DefaultBackpackCustomization = nullptr;
	
	UPROPERTY(EditDefaultsOnly, Category = "Customization|Default Accessory Overrides", meta = (AllowedClasses = "CustomizationAsset_ShoulderMarks"))
	TSoftObjectPtr<UCustomizationAsset> DefaultShoulderMarksCustomization = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = "Customization|Default Accessory Overrides", meta = (AllowedClasses = "CustomizationAsset_ArmPatch_Left"))
	TSoftObjectPtr<UCustomizationAsset> DefaultArmPatchLeftCustomization = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = "Customization|Default Accessory Overrides", meta = (AllowedClasses = "CustomizationAsset_ArmPatch_Right"))
	TSoftObjectPtr<UCustomizationAsset> DefaultArmPatchRightCustomization = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = "Customization|Default Accessory Overrides", meta = (AllowedClasses = "CustomizationAsset_LapelBadge_Left"))
	TSoftObjectPtr<UCustomizationAsset> DefaultLapelBadgeLeftCustomization = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = "Customization|Default Accessory Overrides", meta = (AllowedClasses = "CustomizationAsset_LapelBadge_Right"))
	TSoftObjectPtr<UCustomizationAsset> DefaultLapelBadgeRightCustomization = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = "Customization|Default Accessory Overrides", meta = (AllowedClasses = "CustomizationAsset_CuffBadges"))
	TSoftObjectPtr<UCustomizationAsset> DefaultCuffBadgesCustomization = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = "Customization|Default Accessory Overrides", meta = (AllowedClasses = "CustomizationAsset_CollarBadge"))
	TSoftObjectPtr<UCustomizationAsset> DefaultCollarBadgeCustomization = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Customization|Default Texture Overrides", meta = (AllowedClasses = "CustomizationAsset_JacketTexture"))
	TSoftObjectPtr<UCustomizationAsset> DefaultJacketTextureCustomization = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = "Customization|Default Texture Overrides", meta = (AllowedClasses = "CustomizationAsset_UndershirtTexture"))
	TSoftObjectPtr<UCustomizationAsset> DefaultUndershirtTextureCustomization = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = "Customization|Default Texture Overrides", meta = (AllowedClasses = "CustomizationAsset_PantsTexture"))
	TSoftObjectPtr<UCustomizationAsset> DefaultPantsTextureCustomization = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = "Customization|Default Texture Overrides", meta = (AllowedClasses = "CustomizationAsset_GlovesTexture"))
	TSoftObjectPtr<UCustomizationAsset> DefaultGlovesTextureCustomization = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = "Customization|Default Texture Overrides", meta = (AllowedClasses = "CustomizationAsset_BootsTexture"))
	TSoftObjectPtr<UCustomizationAsset> DefaultBootsTextureCustomization = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = "Customization|Default Texture Overrides", meta = (AllowedClasses = "CustomizationAsset_StrapsTexture"))
	TSoftObjectPtr<UCustomizationAsset> DefaultStrapsTextureCustomization = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = "Customization|Default Texture Overrides", meta = (AllowedClasses = "CustomizationAsset_HairTexture"))
	TSoftObjectPtr<UCustomizationAsset> DefaultHairTextureCustomization = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = "Customization|Default Texture Overrides", meta = (AllowedClasses = "CustomizationAsset_HelmetTexture"))
	TSoftObjectPtr<UCustomizationAsset> DefaultHelmetTextureCustomization = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = "Customization|Default Texture Overrides", meta = (AllowedClasses = "CustomizationAsset_PlayerClassPrimaryTexture"))
	TSoftObjectPtr<UCustomizationAsset> DefaultPlayerClassPrimaryTextureCustomization = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = "Customization|Default Texture Overrides", meta = (AllowedClasses = "CustomizationAsset_PlayerClassSecondaryTexture"))
	TSoftObjectPtr<UCustomizationAsset> DefaultPlayerClassSecondaryTextureCustomization = nullptr;

	//Valid head meshes for this body.
	UPROPERTY(EditDefaultsOnly, Category = Customization, meta = (AllowedClasses = "CustomizationList_Head"))
	TSubclassOf<UCustomizationListAsset> HeadList = nullptr;
	//Valid helmet meshes for this body.
	UPROPERTY(EditDefaultsOnly, Category = Customization, meta = (AllowedClasses = "CustomizationList_Helmet"))
	TSubclassOf<UCustomizationListAsset> HelmetList = nullptr;
	//Valid helmet meshes for this body.
	UPROPERTY(EditDefaultsOnly, Category = Customization, meta = (AllowedClasses = "CustomizationList_Backpack"))
	TSubclassOf<UCustomizationListAsset> BackpackList = nullptr;

	//Valid accessories for this body.
	UPROPERTY(EditDefaultsOnly, Category = Customization, meta = (AllowedClasses = "CustomizationList_Accessory"))
	TSubclassOf<UCustomizationListAsset> AccessoryList = nullptr;
	//Valid texture overrides for this body.
	UPROPERTY(EditDefaultsOnly, Category = Customization, meta = (AllowedClasses = "CustomizationList_TextureOverride"))
	TSubclassOf<UCustomizationListAsset> TextureOverrideList = nullptr;
};

UCLASS()
class NAUSEA_API UCustomizationAsset_Head : public UCustomizationAsset_CoreMesh
{
	GENERATED_BODY()

	UCustomizationAsset_Head() : UCustomizationAsset_CoreMesh() { CustomizationSlot = ECustomizationSlot::Head; }
};

UCLASS()
class NAUSEA_API UCustomizationAsset_Helmet : public UCustomizationAsset_CoreMesh
{
	GENERATED_BODY()

	UCustomizationAsset_Helmet() : UCustomizationAsset_CoreMesh() { CustomizationSlot = ECustomizationSlot::Helmet; }
};

UCLASS()
class NAUSEA_API UCustomizationAsset_Backpack : public UCustomizationAsset_CoreMesh
{
	GENERATED_BODY()

	UCustomizationAsset_Backpack() : UCustomizationAsset_CoreMesh() { CustomizationSlot = ECustomizationSlot::Backpack; }
};

//--------------------------------------------------------------
// MESH CUSTOMIZATION OBJECTS

//SHOULDER MARKS
UCLASS()
class NAUSEA_API UCustomizationAsset_ShoulderMarks : public UCustomizationAsset_Accessory_SkeletalMesh
{
	GENERATED_BODY()

	UCustomizationAsset_ShoulderMarks() : UCustomizationAsset_Accessory_SkeletalMesh() { CustomizationSlot = ECustomizationSlot::JacketShoulderMarks; }
};

//ARM PATCHES
UCLASS()
class NAUSEA_API UCustomizationAsset_ArmPatch_Left : public UCustomizationAsset_Accessory_SkeletalMesh
{
	GENERATED_BODY()

	UCustomizationAsset_ArmPatch_Left() : UCustomizationAsset_Accessory_SkeletalMesh() { CustomizationSlot = ECustomizationSlot::JacketArmPatchLeft; }
};

UCLASS()
class NAUSEA_API UCustomizationAsset_ArmPatch_Right : public UCustomizationAsset_Accessory_SkeletalMesh
{
	GENERATED_BODY()

	UCustomizationAsset_ArmPatch_Right() : UCustomizationAsset_Accessory_SkeletalMesh() { CustomizationSlot = ECustomizationSlot::JacketArmPatchRight; }
};

//LAPEL BADGES
UCLASS()
class NAUSEA_API UCustomizationAsset_LapelBadge_Left : public UCustomizationAsset_Accessory_SkeletalMesh
{
	GENERATED_BODY()

	UCustomizationAsset_LapelBadge_Left() : UCustomizationAsset_Accessory_SkeletalMesh() { CustomizationSlot = ECustomizationSlot::JacketLapelBadgeLeft; }
};

UCLASS()
class NAUSEA_API UCustomizationAsset_LapelBadge_Right : public UCustomizationAsset_Accessory_SkeletalMesh
{
	GENERATED_BODY()

	UCustomizationAsset_LapelBadge_Right() : UCustomizationAsset_Accessory_SkeletalMesh() { CustomizationSlot = ECustomizationSlot::JacketLapelBadgeRight; }
};

//CUFF BADGES
UCLASS()
class NAUSEA_API UCustomizationAsset_CuffBadges : public UCustomizationAsset_Accessory_SkeletalMesh
{
	GENERATED_BODY()

	UCustomizationAsset_CuffBadges() : UCustomizationAsset_Accessory_SkeletalMesh() { CustomizationSlot = ECustomizationSlot::JacketCuffBadges; }
};

//COLLAR BADGE
UCLASS()
class NAUSEA_API UCustomizationAsset_CollarBadge : public UCustomizationAsset_Accessory_SkeletalMesh
{
	GENERATED_BODY()

	UCustomizationAsset_CollarBadge() : UCustomizationAsset_Accessory_SkeletalMesh() { CustomizationSlot = ECustomizationSlot::JacketCollarBadge; }
};

//--------------------------------------------------------------
// TEXTURE CUSTOMIZATION OBJECTS

UCLASS()
class NAUSEA_API UCustomizationAsset_JacketTexture : public UCustomizationAsset_TextureOverride
{
	GENERATED_BODY()

	UCustomizationAsset_JacketTexture() : UCustomizationAsset_TextureOverride() { CustomizationSlot = ECustomizationSlot::JacketTextureOverride; }
};

UCLASS()
class NAUSEA_API UCustomizationAsset_UndershirtTexture : public UCustomizationAsset_TextureOverride
{
	GENERATED_BODY()

	UCustomizationAsset_UndershirtTexture() : UCustomizationAsset_TextureOverride() { CustomizationSlot = ECustomizationSlot::UndershirtTextureOverride; }
};

UCLASS()
class NAUSEA_API UCustomizationAsset_PantsTexture : public UCustomizationAsset_TextureOverride
{
	GENERATED_BODY()

	UCustomizationAsset_PantsTexture() : UCustomizationAsset_TextureOverride() { CustomizationSlot = ECustomizationSlot::PantsTextureOverride; }
};

UCLASS()
class NAUSEA_API UCustomizationAsset_GlovesTexture : public UCustomizationAsset_TextureOverride
{
	GENERATED_BODY()

	UCustomizationAsset_GlovesTexture() : UCustomizationAsset_TextureOverride() { CustomizationSlot = ECustomizationSlot::GlovesTextureOverride; }
};

UCLASS()
class NAUSEA_API UCustomizationAsset_BootsTexture : public UCustomizationAsset_TextureOverride
{
	GENERATED_BODY()

	UCustomizationAsset_BootsTexture() : UCustomizationAsset_TextureOverride() { CustomizationSlot = ECustomizationSlot::BootsTextureOverride; }
};

UCLASS()
class NAUSEA_API UCustomizationAsset_StrapsTexture : public UCustomizationAsset_TextureOverride
{
	GENERATED_BODY()

	UCustomizationAsset_StrapsTexture() : UCustomizationAsset_TextureOverride() { CustomizationSlot = ECustomizationSlot::StrapsTextureOverride; }
};

UCLASS()
class NAUSEA_API UCustomizationAsset_HairTexture : public UCustomizationAsset_TextureOverride
{
	GENERATED_BODY()

	UCustomizationAsset_HairTexture() : UCustomizationAsset_TextureOverride() { CustomizationSlot = ECustomizationSlot::HairTextureOverride; }
};

UCLASS()
class NAUSEA_API UCustomizationAsset_HelmetTexture : public UCustomizationAsset_TextureOverride
{
	GENERATED_BODY()

	UCustomizationAsset_HelmetTexture() : UCustomizationAsset_TextureOverride() { CustomizationSlot = ECustomizationSlot::HelmetTextureOverride; }
};

UCLASS()
class NAUSEA_API UCustomizationAsset_PlayerClassPrimaryTexture : public UCustomizationAsset_TextureOverride
{
	GENERATED_BODY()

	UCustomizationAsset_PlayerClassPrimaryTexture() : UCustomizationAsset_TextureOverride() { CustomizationSlot = ECustomizationSlot::PlayerClassATextureOverride; }
};

UCLASS()
class NAUSEA_API UCustomizationAsset_PlayerClassSecondaryTexture : public UCustomizationAsset_TextureOverride
{
	GENERATED_BODY()

	UCustomizationAsset_PlayerClassSecondaryTexture() : UCustomizationAsset_TextureOverride() { CustomizationSlot = ECustomizationSlot::PlayerClassBTextureOverride; }
};


//--------------------------------------------------------------
// CUSTOMIZATION LISTS

UCLASS(NotBlueprintable, NotBlueprintType)
class NAUSEA_API UCustomizationListAsset : public UPrimaryDataAsset
{
	GENERATED_UCLASS_BODY()

public:
	static const FPrimaryAssetType CustomizationListAssetType;

//~ Begin UObject Interface
public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
//~ End UObject Interface

public:
	virtual const TArray<TSoftObjectPtr<UCustomizationAsset>>& GetCustomizationList() const
	{
		static TArray<TSoftObjectPtr<UCustomizationAsset>> InvalidList;
		return InvalidList;
	}

	virtual FText GetListEditorName() const { return FText::FromString("InvalidList"); }
};

UCLASS()
class NAUSEA_API UCustomizationList_Accessory : public UCustomizationListAsset
{
	GENERATED_BODY()

//~ Begin UCustomizationList Interface
public:
	virtual const TArray<TSoftObjectPtr<UCustomizationAsset>>& GetCustomizationList() const override { return AccessoryList; }
	virtual FText GetListEditorName() const override { return FText::FromString("Accessory List"); }
//~ End UCustomizationList Interface

protected:
	UPROPERTY(EditDefaultsOnly, Category = Customization, meta = (AllowedClasses = "CustomizationAsset_Accessory"))
	TArray<TSoftObjectPtr<UCustomizationAsset>> AccessoryList = TArray<TSoftObjectPtr<UCustomizationAsset>>();
};

UCLASS()
class NAUSEA_API UCustomizationList_TextureOverride : public UCustomizationListAsset
{
	GENERATED_BODY()

//~ Begin UCustomizationList Interface
public:
	virtual const TArray<TSoftObjectPtr<UCustomizationAsset>>& GetCustomizationList() const override { return TextureList; }
	virtual FText GetListEditorName() const override { return FText::FromString("Texture Override List"); }
//~ End UCustomizationList Interface

protected:
	UPROPERTY(EditDefaultsOnly, Category = Customization, meta = (AllowedClasses = "CustomizationAsset_TextureOverride"))
	TArray<TSoftObjectPtr<UCustomizationAsset>> TextureList = TArray<TSoftObjectPtr<UCustomizationAsset>>();
};

UCLASS()
class NAUSEA_API UCustomizationList_Head : public UCustomizationListAsset
{
	GENERATED_BODY()

//~ Begin UCustomizationList Interface
public:
	virtual const TArray<TSoftObjectPtr<UCustomizationAsset>>& GetCustomizationList() const override { return HeadList; }
	virtual FText GetListEditorName() const override { return FText::FromString("Head List"); }
//~ End UCustomizationList Interface

protected:
	UPROPERTY(EditDefaultsOnly, Category = Customization, meta = (AllowedClasses = "CustomizationAsset_Head"))
	TArray<TSoftObjectPtr<UCustomizationAsset>> HeadList = TArray<TSoftObjectPtr<UCustomizationAsset>>();
};

UCLASS(Blueprintable, NotBlueprintType)
class NAUSEA_API UCustomizationList_Helmet : public UCustomizationListAsset
{
	GENERATED_BODY()

//~ Begin UCustomizationList Interface
public:
	virtual const TArray<TSoftObjectPtr<UCustomizationAsset>>& GetCustomizationList() const override { return HelmetList; }
	virtual FText GetListEditorName() const override { return FText::FromString("Helmet List"); }
//~ End UCustomizationList Interface

protected:
	UPROPERTY(EditDefaultsOnly, Category = Customization, meta = (AllowedClasses = "CustomizationAsset_Helmet"))
	TArray<TSoftObjectPtr<UCustomizationAsset>> HelmetList = TArray<TSoftObjectPtr<UCustomizationAsset>>();
};

UCLASS()
class NAUSEA_API UCustomizationList_Backpack : public UCustomizationListAsset
{
	GENERATED_BODY()

//~ Begin UCustomizationList Interface
public:
	virtual const TArray<TSoftObjectPtr<UCustomizationAsset>>& GetCustomizationList() const override { return BackpackList; }
	virtual FText GetListEditorName() const override { return FText::FromString("Backpack List"); }
//~ End UCustomizationList Interface

protected:
	UPROPERTY(EditDefaultsOnly, Category = Customization, meta = (AllowedClasses = "CustomizationAsset_Backpack"))
	TArray<TSoftObjectPtr<UCustomizationAsset>> BackpackList = TArray<TSoftObjectPtr<UCustomizationAsset>>();
};