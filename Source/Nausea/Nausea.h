// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

//LOG CATEGORIES
DECLARE_LOG_CATEGORY_EXTERN(LogInventoryManager, Warning, All);
DECLARE_LOG_CATEGORY_EXTERN(LogWeapon, Warning, All);
DECLARE_LOG_CATEGORY_EXTERN(LogFireMode, Warning, All);
DECLARE_LOG_CATEGORY_EXTERN(LogAmmo, Warning, All);
DECLARE_LOG_CATEGORY_EXTERN(LogUI, Warning, All);

#define ECC_WeaponTrace ECollisionChannel::ECC_GameTraceChannel1
#define ECC_AbilityTrace ECollisionChannel::ECC_GameTraceChannel5

#define ECC_InteractionTrace ECollisionChannel::ECC_GameTraceChannel6
#define ECC_WidgetInteractionTrace ECollisionChannel::ECC_GameTraceChannel3

#define TSCRIPTINTERFACE_CALL_FUNC(ScriptInterface, NativeFunctionName, BlueprintFunctionName, ...)\
if(ScriptInterface) { ScriptInterface->NativeFunctionName(__VA_ARGS__); } else if(ScriptInterface.GetObject() && ScriptInterface.GetObject()->Implements<decltype(ScriptInterface)::InterfaceClass::UClassType>()) { decltype(ScriptInterface)::InterfaceClass::Execute_##BlueprintFunctionName(ScriptInterface.GetObject(), ##__VA_ARGS__); }\

#define TSCRIPTINTERFACE_CALL_FUNC_RET(ScriptInterface, NativeFunctionName, BlueprintFunctionName, DefaultValue, ...)\
(ScriptInterface ? ScriptInterface->NativeFunctionName(__VA_ARGS__) : ((ScriptInterface.GetObject() && ScriptInterface.GetObject()->Implements<decltype(ScriptInterface)::InterfaceClass::UClassType>()) ? decltype(ScriptInterface)::InterfaceClass::Execute_##BlueprintFunctionName(ScriptInterface.GetObject(), ##__VA_ARGS__) : DefaultValue))\

#define TSCRIPTINTERFACE_IS_VALID(ScriptInterface)\
(ScriptInterface || (ScriptInterface.GetObject() && ScriptInterface.GetObject()->Implements<decltype(ScriptInterface)::InterfaceClass::UClassType>()))\

class FNauseaModule : public FDefaultGameModuleImpl
{
	// Begin IModuleInterface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	// End IModuleInterface
};