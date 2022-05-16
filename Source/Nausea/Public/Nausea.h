// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

//LOG CATEGORIES
DECLARE_LOG_CATEGORY_EXTERN(LogUI, Warning, All);

class FNauseaModule : public FDefaultGameModuleImpl
{
	// Begin IModuleInterface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	// End IModuleInterface
};