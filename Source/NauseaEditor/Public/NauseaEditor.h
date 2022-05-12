#pragma once

#include "Engine.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "UnrealEd.h"
#include "AssetToolsModule.h"
#include "IAssetRegistry.h"
#include "AssetTypeActions_Base.h"
#include "AssetTypeCategories.h"
#include "Internationalization/StringTableRegistry.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "EditorStyle.h"


class FNauseaEditorStyleSheet
	: public FSlateStyleSet
{
public:
	FNauseaEditorStyleSheet();

	static FNauseaEditorStyleSheet& Get()
	{
		static FNauseaEditorStyleSheet Inst;
		return Inst;
	}
	
	~FNauseaEditorStyleSheet()
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*this);
	}
};

class FAssetTypeActions_CustomizationAsset : public FAssetTypeActions_Base
{
public:
	FAssetTypeActions_CustomizationAsset(UClass* InCustomizationClass, EAssetTypeCategories::Type InCategory, const TArray<FText>& InSubMenus)
	{
		CustomizationClass = InCustomizationClass;
		CustomizationCategory = InCategory;
		CustomizationSubMenus = InSubMenus;
	}

//~ Begin IAssetTypeActions Interface
public:
	virtual FText GetName() const override { return NSLOCTEXT("Customization", "AssetTypeActions_Customization", "Customization"); }
	virtual FColor GetTypeColor() const override { return FColor(162, 63, 255); }
	virtual UClass* GetSupportedClass() const override { return CustomizationClass; }
	virtual uint32 GetCategories() override { return CustomizationCategory; }
	virtual const TArray<FText>& GetSubMenus() const override
	{
		return CustomizationSubMenus;
	}
//~ End IAssetTypeActions Interface

protected:
	UClass* CustomizationClass = nullptr;
	EAssetTypeCategories::Type CustomizationCategory;
	TArray<FText> CustomizationSubMenus;
};

class FAssetTypeActions_CustomizationListAsset : public FAssetTypeActions_Base
{
public:
	FAssetTypeActions_CustomizationListAsset(UClass* InCustomizationClass, EAssetTypeCategories::Type InCategory)
	{
		CustomizationListClass = InCustomizationClass;
		CustomizationCategory = InCategory;
	}

//~ Begin IAssetTypeActions Interface
public:
	virtual FText GetName() const override { return NSLOCTEXT("Customization", "AssetTypeActions_CustomizationList", "CustomizationList"); }
	virtual FColor GetTypeColor() const override { return FColor(92, 63, 255); }
	virtual UClass* GetSupportedClass() const override { return CustomizationListClass; }
	virtual uint32 GetCategories() override { return CustomizationCategory; }
	virtual const TArray<FText>& GetSubMenus() const override
	{
		static const TArray<FText> CustomizationListSubMenus{ FText::FromString("Customization List") };
		return CustomizationListSubMenus;
	}
//~ End IAssetTypeActions Interface

protected:
	UClass* CustomizationListClass = nullptr;
	EAssetTypeCategories::Type CustomizationCategory;
};

class FNauseaEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

protected:
	TArray<TSharedPtr<IAssetTypeActions>> CustomizationAssetTypeActions;
};