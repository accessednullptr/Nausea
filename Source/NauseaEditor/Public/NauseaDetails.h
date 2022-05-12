// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/Attribute.h"
#include "UObject/WeakObjectPtr.h"
#include "Layout/Visibility.h"
#include "IDetailCustomization.h"

/**
 * 
 */
class NAUSEAEDITOR_API FNauseaDetails : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance(FCustomDetailLayoutNameMap CustomDetailLayoutNameMap);

	void SetReplacedCustomDetailLayoutNameMap(const FCustomDetailLayoutNameMap& InCustomDetailLayoutNameMap);

//~ Begin IDetailCustomization Interface 
public:
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
//~ End IDetailCustomization Interface 

private:
	FCustomDetailLayoutNameMap ReplacedCustomDetailLayoutNameMap;
};