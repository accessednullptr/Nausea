#include "NauseaEditor.h"
#include "Character/Customization/CustomizationObject.h"

IMPLEMENT_GAME_MODULE(FNauseaEditorModule, NauseaEditor);

FNauseaEditorStyleSheet::FNauseaEditorStyleSheet()
	: FSlateStyleSet("NauseaEditorStyleSheet")
{
	SetContentRoot(FPaths::ProjectContentDir() / TEXT("Editor/Icons"));

	const FVector2D Icon16x16(16.0f, 16.0f);
	const FVector2D Icon64x64(64.0f, 64.0f);

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
	Set("ClassIcon.PathFollowingComponent", new IMAGE_BRUSH("NavLinkProxy_16x", Icon16x16));
	Set("ClassThumbnail.PathFollowingComponent", new IMAGE_BRUSH("NavLinkProxy_64x", Icon64x64));

	Set("ClassIcon.ActionBrainComponent", new IMAGE_BRUSH("StringTable_16x", Icon16x16));
	Set("ClassThumbnail.ActionBrainComponent", new IMAGE_BRUSH("StringTable_64x", Icon64x64));

	Set("ClassIcon.RoutineManagerComponent", new IMAGE_BRUSH("ApplicationLifecycleComponent_16x", Icon16x16));
	Set("ClassThumbnail.RoutineManagerComponent", new IMAGE_BRUSH("ApplicationLifecycleComponent_64x", Icon64x64));

	Set("ClassIcon.EnemySelectionComponent", new IMAGE_BRUSH("DataTable_16x", Icon16x16));
	Set("ClassThumbnail.EnemySelectionComponent", new IMAGE_BRUSH("DataTable_64x", Icon64x64));

	Set("ClassIcon.AbilityDecalComponent", new IMAGE_BRUSH("DecalActor_16x", Icon16x16));
	Set("ClassThumbnail.AbilityDecalComponent", new IMAGE_BRUSH("DecalActor_64x", Icon64x64));

	Set("ClassIcon.Weapon", new IMAGE_BRUSH("DataAsset_16x", Icon16x16));
	Set("ClassThumbnail.Weapon", new IMAGE_BRUSH("DataAsset_64x", Icon64x64));
	Set("ClassIcon.FireMode", new IMAGE_BRUSH("DataAsset_16x", Icon16x16));
	Set("ClassThumbnail.FireMode", new IMAGE_BRUSH("DataAsset_64x", Icon64x64));
	Set("ClassIcon.Ammo", new IMAGE_BRUSH("DataAsset_16x", Icon16x16));
	Set("ClassThumbnail.Ammo", new IMAGE_BRUSH("DataAsset_64x", Icon64x64));

	Set("ClassIcon.CustomizationAsset_CoreMesh", new IMAGE_BRUSH("CustomizationCore_16x", Icon16x16));
	Set("ClassThumbnail.CustomizationAsset_CoreMesh", new IMAGE_BRUSH("CustomizationCore_64x", Icon64x64));
	Set("ClassIcon.CustomizationAsset_Body", new IMAGE_BRUSH("CustomizationBody_16x", Icon16x16));
	Set("ClassThumbnail.CustomizationAsset_Body", new IMAGE_BRUSH("CustomizationBody_64x", Icon64x64));
	Set("ClassIcon.CustomizationAsset_Accessory", new IMAGE_BRUSH("CustomizationAccessory_16x", Icon16x16));
	Set("ClassThumbnail.CustomizationAsset_Accessory", new IMAGE_BRUSH("CustomizationAccessory_64x", Icon64x64));
	Set("ClassIcon.CustomizationAsset_TextureOverride", new IMAGE_BRUSH("CustomizationTexture_16x", Icon16x16));
	Set("ClassThumbnail.CustomizationAsset_TextureOverride", new IMAGE_BRUSH("CustomizationTexture_64x", Icon64x64));

	Set("ClassIcon.CustomizationListAsset", new IMAGE_BRUSH("CustomizationList_16x", Icon16x16));
	Set("ClassThumbnail.CustomizationListAsset", new IMAGE_BRUSH("CustomizationList_64x", Icon64x64));

	FSlateStyleRegistry::RegisterSlateStyle(*this);
}

void FNauseaEditorModule::StartupModule()
{
	FNauseaEditorStyleSheet::Get();

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();

	EAssetTypeCategories::Type CustomizationAssetCategory = AssetTools.RegisterAdvancedAssetCategory(FName("Customization"), FText::FromString("Customization"));

	const UClass* CustomizationCoreClass = UCustomizationAsset_CoreMesh::StaticClass();
	const UClass* CustomizationAccessoryClass = UCustomizationAsset_Accessory::StaticClass();
	const UClass* CustomizationTextureOverrideClass = UCustomizationAsset_TextureOverride::StaticClass();
	const UClass* CustomizationListClass = UCustomizationListAsset::StaticClass();

	static const TArray<FText> CoreMeshSubMenus{ FText::FromString("Core Customization") };
	static const TArray<FText> AccessorySubMenus{ FText::FromString("Accessory Customization") };
	static const TArray<FText> TextureOverrideSubMenus{ FText::FromString("Texture Override Customization") };

	for (TObjectIterator<UClass> ClassItr; ClassItr; ++ClassItr)
	{
		if (ClassItr->IsChildOf(CustomizationCoreClass))
		{
			TSharedPtr<IAssetTypeActions> Action = MakeShareable(new FAssetTypeActions_CustomizationAsset(*ClassItr, CustomizationAssetCategory, CoreMeshSubMenus));
			AssetTools.RegisterAssetTypeActions(Action.ToSharedRef());
			CustomizationAssetTypeActions.Add(Action);
		}
		else if (ClassItr->IsChildOf(CustomizationAccessoryClass))
		{
			TSharedPtr<IAssetTypeActions> Action = MakeShareable(new FAssetTypeActions_CustomizationAsset(*ClassItr, CustomizationAssetCategory, AccessorySubMenus));
			AssetTools.RegisterAssetTypeActions(Action.ToSharedRef());
			CustomizationAssetTypeActions.Add(Action);
		}
		else if (ClassItr->IsChildOf(CustomizationTextureOverrideClass))
		{
			TSharedPtr<IAssetTypeActions> Action = MakeShareable(new FAssetTypeActions_CustomizationAsset(*ClassItr, CustomizationAssetCategory, TextureOverrideSubMenus));
			AssetTools.RegisterAssetTypeActions(Action.ToSharedRef());
			CustomizationAssetTypeActions.Add(Action);
		}
		else if (ClassItr->IsChildOf(CustomizationListClass))
		{
			TSharedPtr<IAssetTypeActions> Action = MakeShareable(new FAssetTypeActions_CustomizationListAsset(*ClassItr, CustomizationAssetCategory));
			AssetTools.RegisterAssetTypeActions(Action.ToSharedRef());
			CustomizationAssetTypeActions.Add(Action);
		}
	}
}

void FNauseaEditorModule::ShutdownModule()
{

}