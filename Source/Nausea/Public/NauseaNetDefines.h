#pragma once

#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

#define FAST_ARRAY_SERIALIZER_OPERATORS(ElementType, ArrayVariable)\
public:\
	FORCEINLINE TArray<ElementType>& operator*() { return ArrayVariable; }\
	FORCEINLINE TArray<ElementType>* operator->() { return &ArrayVariable; }\
	FORCEINLINE const TArray<ElementType>& operator*() const { return ArrayVariable; }\
	FORCEINLINE const TArray<ElementType>* operator->() const { return &ArrayVariable; }\
	FORCEINLINE ElementType& operator[](int32 Index) { return ArrayVariable[Index]; }\
	FORCEINLINE const ElementType& operator[](int32 Index) const { return ArrayVariable[Index]; }\
\

namespace PushReplicationParams
{
	extern NAUSEA_API const FDoRepLifetimeParams Default;
	extern NAUSEA_API const FDoRepLifetimeParams InitialOnly;
	extern NAUSEA_API const FDoRepLifetimeParams OwnerOnly;
	extern NAUSEA_API const FDoRepLifetimeParams SkipOwner;
	extern NAUSEA_API const FDoRepLifetimeParams SimulatedOnly;
	extern NAUSEA_API const FDoRepLifetimeParams AutonomousOnly;
	extern NAUSEA_API const FDoRepLifetimeParams SimulatedOrPhysics;
	extern NAUSEA_API const FDoRepLifetimeParams InitialOrOwner;
	extern NAUSEA_API const FDoRepLifetimeParams Custom;
	extern NAUSEA_API const FDoRepLifetimeParams ReplayOrOwner;
	extern NAUSEA_API const FDoRepLifetimeParams ReplayOnly;
	extern NAUSEA_API const FDoRepLifetimeParams SimulatedOnlyNoReplay;
	extern NAUSEA_API const FDoRepLifetimeParams SimulatedOrPhysicsNoReplay;
	extern NAUSEA_API const FDoRepLifetimeParams SkipReplay;
	extern NAUSEA_API const FDoRepLifetimeParams Never;
}