// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "AssetToolsModule.h"
#include "Character/Customization/CustomizationObject.h"
#include "CustomizationFactory.generated.h"

/**
 * 
 */
UCLASS()
class NAUSEAEDITOR_API UCustomizationFactory : public UFactory
{
	GENERATED_BODY()

public:
	UCustomizationFactory()
		: UFactory()
	{
		bCreateNew = true;
		bEditAfterNew = true;
		SupportedClass = nullptr;
	}

//~ Begin UFactory Interface
public:
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override
	{
		check(Class->IsChildOf(UCustomizationAsset::StaticClass()));
		return NewObject<UCustomizationAsset>(InParent, Class, Name, Flags);
	}

	virtual FText GetDisplayName() const
	{
		if (!SupportedClass)
		{
			return FText::FromString("Invalid");
		}

		const UCustomizationAsset* CustomizationObject = Cast<UCustomizationAsset>(SupportedClass.GetDefaultObject());

		if (!CustomizationObject)
		{
			return FText::FromString("Invalid");
		}

		return CustomizationObject->GetSlotEditorName();
	}
//~ End UFactory Interface
};

//---------------------------------
//CORE

UCLASS()
class NAUSEAEDITOR_API UCustomizationFactory_Body : public UCustomizationFactory
{
	GENERATED_BODY()

public:
	UCustomizationFactory_Body() : UCustomizationFactory() { SupportedClass = UCustomizationAsset_Body::StaticClass(); }
};

UCLASS()
class NAUSEAEDITOR_API UCustomizationFactory_Head : public UCustomizationFactory
{
	GENERATED_BODY()

public:
	UCustomizationFactory_Head() : UCustomizationFactory() { SupportedClass = UCustomizationAsset_Head::StaticClass(); }
};

UCLASS()
class NAUSEAEDITOR_API UCustomizationFactory_Helmet : public UCustomizationFactory
{
	GENERATED_BODY()

public:
	UCustomizationFactory_Helmet() : UCustomizationFactory() { SupportedClass = UCustomizationAsset_Helmet::StaticClass(); }
};

UCLASS()
class NAUSEAEDITOR_API UCustomizationFactory_Backpack : public UCustomizationFactory
{
	GENERATED_BODY()

public:
	UCustomizationFactory_Backpack() : UCustomizationFactory() { SupportedClass = UCustomizationAsset_Backpack::StaticClass(); }
};

//---------------------------------
//ACCESSORIES

UCLASS()
class NAUSEAEDITOR_API UCustomizationFactory_ShoulderMarks : public UCustomizationFactory
{
	GENERATED_BODY()

public:
	UCustomizationFactory_ShoulderMarks() : UCustomizationFactory() { SupportedClass = UCustomizationAsset_ShoulderMarks::StaticClass(); }
};

UCLASS()
class NAUSEAEDITOR_API UCustomizationFactory_ArmPatch_Left : public UCustomizationFactory
{
	GENERATED_BODY()

public:
	UCustomizationFactory_ArmPatch_Left() : UCustomizationFactory() { SupportedClass = UCustomizationAsset_ArmPatch_Left::StaticClass(); }
};

UCLASS()
class NAUSEAEDITOR_API UCustomizationFactory_ArmPatch_Right : public UCustomizationFactory
{
	GENERATED_BODY()

public:
	UCustomizationFactory_ArmPatch_Right() : UCustomizationFactory() { SupportedClass = UCustomizationAsset_ArmPatch_Right::StaticClass(); }
};

UCLASS()
class NAUSEAEDITOR_API UCustomizationFactory_LapelBadge_Left : public UCustomizationFactory
{
	GENERATED_BODY()

public:
	UCustomizationFactory_LapelBadge_Left() : UCustomizationFactory() { SupportedClass = UCustomizationAsset_LapelBadge_Left::StaticClass(); }
};

UCLASS()
class NAUSEAEDITOR_API UCustomizationFactory_LapelBadge_Right : public UCustomizationFactory
{
	GENERATED_BODY()

public:
	UCustomizationFactory_LapelBadge_Right() : UCustomizationFactory() { SupportedClass = UCustomizationAsset_LapelBadge_Right::StaticClass(); }
};

UCLASS()
class NAUSEAEDITOR_API UCustomizationFactory_CuffBadges : public UCustomizationFactory
{
	GENERATED_BODY()

public:
	UCustomizationFactory_CuffBadges() : UCustomizationFactory() { SupportedClass = UCustomizationAsset_CuffBadges::StaticClass(); }
};

UCLASS()
class NAUSEAEDITOR_API UCustomizationFactory_CollarBadge : public UCustomizationFactory
{
	GENERATED_BODY()

public:
	UCustomizationFactory_CollarBadge() : UCustomizationFactory() { SupportedClass = UCustomizationAsset_CollarBadge::StaticClass(); }
};

//---------------------------------
//TEXTURE OVERRIDES

UCLASS()
class NAUSEAEDITOR_API UCustomizationFactory_JacketTexture : public UCustomizationFactory
{
	GENERATED_BODY()

public:
	UCustomizationFactory_JacketTexture() : UCustomizationFactory() { SupportedClass = UCustomizationAsset_JacketTexture::StaticClass(); }
};

UCLASS()
class NAUSEAEDITOR_API UCustomizationFactory_UndershirtTexture : public UCustomizationFactory
{
	GENERATED_BODY()

public:
	UCustomizationFactory_UndershirtTexture() : UCustomizationFactory() { SupportedClass = UCustomizationAsset_UndershirtTexture::StaticClass(); }
};

UCLASS()
class NAUSEAEDITOR_API UCustomizationFactory_PantsTexture : public UCustomizationFactory
{
	GENERATED_BODY()

public:
	UCustomizationFactory_PantsTexture() : UCustomizationFactory() { SupportedClass = UCustomizationAsset_PantsTexture::StaticClass(); }
};

UCLASS()
class NAUSEAEDITOR_API UCustomizationFactory_GlovesTexture : public UCustomizationFactory
{
	GENERATED_BODY()

public:
	UCustomizationFactory_GlovesTexture() : UCustomizationFactory() { SupportedClass = UCustomizationAsset_GlovesTexture::StaticClass(); }
};

UCLASS()
class NAUSEAEDITOR_API UCustomizationFactory_BootsTexture : public UCustomizationFactory
{
	GENERATED_BODY()

public:
	UCustomizationFactory_BootsTexture() : UCustomizationFactory() { SupportedClass = UCustomizationAsset_BootsTexture::StaticClass(); }
};

UCLASS()
class NAUSEAEDITOR_API UCustomizationFactory_StrapsTexture : public UCustomizationFactory
{
	GENERATED_BODY()

public:
	UCustomizationFactory_StrapsTexture() : UCustomizationFactory() { SupportedClass = UCustomizationAsset_StrapsTexture::StaticClass(); }
};

UCLASS()
class NAUSEAEDITOR_API UCustomizationFactory_HairTexture : public UCustomizationFactory
{
	GENERATED_BODY()

public:
	UCustomizationFactory_HairTexture() : UCustomizationFactory() { SupportedClass = UCustomizationAsset_HairTexture::StaticClass(); }
};

UCLASS()
class NAUSEAEDITOR_API UCustomizationFactory_HelmetTexture : public UCustomizationFactory
{
	GENERATED_BODY()

public:
	UCustomizationFactory_HelmetTexture() : UCustomizationFactory() { SupportedClass = UCustomizationAsset_HelmetTexture::StaticClass(); }
};

UCLASS()
class NAUSEAEDITOR_API UCustomizationFactory_PlayerClassPrimaryTexture : public UCustomizationFactory
{
	GENERATED_BODY()

public:
	UCustomizationFactory_PlayerClassPrimaryTexture() : UCustomizationFactory() { SupportedClass = UCustomizationAsset_PlayerClassPrimaryTexture::StaticClass(); }
};

UCLASS()
class NAUSEAEDITOR_API UCustomizationFactory_PlayerClassSecondaryTexture : public UCustomizationFactory
{
	GENERATED_BODY()

public:
	UCustomizationFactory_PlayerClassSecondaryTexture() : UCustomizationFactory() { SupportedClass = UCustomizationAsset_PlayerClassSecondaryTexture::StaticClass(); }
};


UCLASS()
class NAUSEAEDITOR_API UCustomizationListFactory : public UFactory
{
	GENERATED_BODY()

public:
	UCustomizationListFactory()
		: UFactory()
	{
		bCreateNew = true;
		bEditAfterNew = true;
		SupportedClass = nullptr;
	}

//~ Begin UFactory Interface
public:
	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override
	{
		check(Class->IsChildOf(UCustomizationListAsset::StaticClass()));
		return NewObject<UCustomizationListAsset>(InParent, Class, Name, Flags);
	}

	virtual FText GetDisplayName() const
	{
		if (!SupportedClass)
		{
			return FText::FromString("Invalid");
		}

		const UCustomizationListAsset* CustomizationObject = Cast<UCustomizationListAsset>(SupportedClass.GetDefaultObject());

		if (!CustomizationObject)
		{
			return FText::FromString("Invalid");
		}

		return CustomizationObject->GetListEditorName();
	}
//~ End UFactory Interface
};

UCLASS()
class NAUSEAEDITOR_API UCustomizationListFactory_Accessory : public UCustomizationListFactory
{
	GENERATED_BODY()

public:
	UCustomizationListFactory_Accessory() : UCustomizationListFactory() { SupportedClass = UCustomizationList_Accessory::StaticClass(); }
};

UCLASS()
class NAUSEAEDITOR_API UCustomizationListFactory_TextureOverride : public UCustomizationListFactory
{
	GENERATED_BODY()

public:
	UCustomizationListFactory_TextureOverride() : UCustomizationListFactory() { SupportedClass = UCustomizationList_TextureOverride::StaticClass(); }
};

UCLASS()
class NAUSEAEDITOR_API UCustomizationListFactory_Head : public UCustomizationListFactory
{
	GENERATED_BODY()

public:
	UCustomizationListFactory_Head() : UCustomizationListFactory() { SupportedClass = UCustomizationList_Head::StaticClass(); }
};

UCLASS()
class NAUSEAEDITOR_API UCustomizationListFactory_Helmet : public UCustomizationListFactory
{
	GENERATED_BODY()

public:
	UCustomizationListFactory_Helmet() : UCustomizationListFactory() { SupportedClass = UCustomizationList_Helmet::StaticClass(); }
};

UCLASS()
class NAUSEAEDITOR_API UCustomizationListFactory_Backpack : public UCustomizationListFactory
{
	GENERATED_BODY()

public:
	UCustomizationListFactory_Backpack() : UCustomizationListFactory() { SupportedClass = UCustomizationList_Backpack::StaticClass(); }
};