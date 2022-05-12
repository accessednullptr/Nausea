// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.


#include "Nausea.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebugger.h"
#include "System/DebugCategory/GameplayDebuggerCategory_Mission.h"
#include "System/DebugCategory/GameplayDebuggerCategory_Routine.h"
#include "System/DebugCategory/GameplayDebuggerCategory_Vehicle.h"
#endif // WITH_GAMEPLAY_DEBUGGER

//LOG CATEGORIES
DEFINE_LOG_CATEGORY(LogInventoryManager);
DEFINE_LOG_CATEGORY(LogWeapon);
DEFINE_LOG_CATEGORY(LogFireMode);
DEFINE_LOG_CATEGORY(LogAmmo);
DEFINE_LOG_CATEGORY(LogUI);

IMPLEMENT_PRIMARY_GAME_MODULE(FNauseaModule, Nausea, "Nausea");

void FNauseaModule::StartupModule()
{

#if WITH_GAMEPLAY_DEBUGGER
	IGameplayDebugger& GameplayDebuggerModule = IGameplayDebugger::Get();
	GameplayDebuggerModule.RegisterCategory("Mission", IGameplayDebugger::FOnGetCategory::CreateStatic(&FGameplayDebuggerCategory_Mission::MakeInstance), EGameplayDebuggerCategoryState::EnabledInGameAndSimulate, 5);
	GameplayDebuggerModule.RegisterCategory("Routine", IGameplayDebugger::FOnGetCategory::CreateStatic(&FGameplayDebuggerCategory_Routine::MakeInstance), EGameplayDebuggerCategoryState::EnabledInGameAndSimulate, 6);
	GameplayDebuggerModule.RegisterCategory("Vehicle", IGameplayDebugger::FOnGetCategory::CreateStatic(&FGameplayDebuggerCategory_Vehicle::MakeInstance), EGameplayDebuggerCategoryState::EnabledInGameAndSimulate, 6);
	GameplayDebuggerModule.NotifyCategoriesChanged();
#endif
}

void FNauseaModule::ShutdownModule()
{
#if WITH_GAMEPLAY_DEBUGGER
	if (IGameplayDebugger::IsAvailable())
	{
		IGameplayDebugger& GameplayDebuggerModule = IGameplayDebugger::Get();
		GameplayDebuggerModule.UnregisterCategory("Mission");
		GameplayDebuggerModule.UnregisterCategory("Routine");
		GameplayDebuggerModule.UnregisterCategory("Vehicle");
		GameplayDebuggerModule.NotifyCategoriesChanged();
	}
#endif
}