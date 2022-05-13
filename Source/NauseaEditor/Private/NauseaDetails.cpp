// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#include "NauseaDetails.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "IPropertyUtilities.h"

class FEditConditionContext;

TSharedRef<IDetailCustomization> FNauseaDetails::MakeInstance(FCustomDetailLayoutNameMap CustomDetailLayoutNameMap)
{
	TSharedRef<FNauseaDetails> NauseaDetails = MakeShareable(new FNauseaDetails);
	NauseaDetails->SetReplacedCustomDetailLayoutNameMap(CustomDetailLayoutNameMap);
	return NauseaDetails;
}

void FNauseaDetails::SetReplacedCustomDetailLayoutNameMap(const FCustomDetailLayoutNameMap& InCustomDetailLayoutNameMap)
{
	ReplacedCustomDetailLayoutNameMap = InCustomDetailLayoutNameMap;
}

void FNauseaDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	const FDetailLayoutCallback* ReplacedCallback = ReplacedCustomDetailLayoutNameMap.Find(DetailLayout.GetBaseClass()->GetFName());

	if (ReplacedCallback && ReplacedCallback->DetailLayoutDelegate.IsBound())
	{
		TSharedRef<IDetailCustomization> CustomizationInstance = ReplacedCallback->DetailLayoutDelegate.Execute();
		CustomizationInstance->CustomizeDetails(DetailLayout);
	}

	return;

	TArray<FName> CategoryNameList;
	DetailLayout.GetCategoryNames(CategoryNameList);

	TArray<TSharedRef<IPropertyHandle>> DetailLayoutPropertyList;

	static const FName HideConditionName = TEXT("HideCondition");

	for (const FName& Name : CategoryNameList)
	{
		IDetailCategoryBuilder& CategoryBuilder = DetailLayout.EditCategory(Name);

		TArray<TSharedRef<IPropertyHandle>> CategoryPropertyList;
		CategoryBuilder.GetDefaultProperties(CategoryPropertyList);
		DetailLayoutPropertyList.Append(CategoryPropertyList);
	}

	for (TSharedRef<IPropertyHandle> PropertyHandle : DetailLayoutPropertyList)
	{
		const FString& HideConditionPropertyName = PropertyHandle->GetMetaData(HideConditionName);

		if (HideConditionPropertyName == "")
		{
			continue;
		}

		TArray<UObject*> ObjectList;
		PropertyHandle->GetOuterObjects(ObjectList);

		if (ObjectList.Num() == 0)
		{
			continue;
		}

		FBoolProperty* HideConditionProperty = FindFProperty<FBoolProperty>(PropertyHandle->GetProperty()->GetOwnerStruct(), *HideConditionPropertyName);

		bool bHideConditionValue = HideConditionProperty->GetPropertyValue_InContainer(ObjectList[0]);

		if (bHideConditionValue)
		{
			DetailLayout.HideProperty(PropertyHandle);
		}
	}
}