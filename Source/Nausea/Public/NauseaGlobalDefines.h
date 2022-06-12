#pragma once

#define NAUSEA_DEBUG_DRAW !(UE_BUILD_SHIPPING || UE_BUILD_TEST)

#define IS_K2_FUNCTION_IMPLEMENTED(Object, FunctionName)\
(Object ? (Object->FindFunction(GET_FUNCTION_NAME_CHECKED(std::remove_pointer<decltype(Object)>::type, FunctionName)) ? Object->FindFunction(GET_FUNCTION_NAME_CHECKED(std::remove_pointer<decltype(Object)>::type, FunctionName))->IsInBlueprint() : false) : false)


#define ECC_WeaponTrace ECollisionChannel::ECC_GameTraceChannel1
#define ECC_ProjectileTrace ECollisionChannel::ECC_GameTraceChannel2

#define ECC_WidgetInteractionTrace ECollisionChannel::ECC_GameTraceChannel3
#define ECC_Widget ECollisionChannel::ECC_GameTraceChannel4

#define ECC_AbilityTrace ECollisionChannel::ECC_GameTraceChannel5

#define ECC_InteractionTrace ECollisionChannel::ECC_GameTraceChannel6

#define ECC_DungeonPawn ECollisionChannel::ECC_GameTraceChannel7

#define ECC_PlacementTrace ECollisionChannel::ECC_GameTraceChannel8
#define ECC_PlacementObject ECollisionChannel::ECC_GameTraceChannel9