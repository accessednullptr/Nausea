#include "NauseaNetDefines.h"

namespace PushReplicationParams
{
	const FDoRepLifetimeParams Default{ COND_None, REPNOTIFY_OnChanged, true };
	const FDoRepLifetimeParams InitialOnly{ COND_InitialOnly, REPNOTIFY_OnChanged, true };
	const FDoRepLifetimeParams OwnerOnly{ COND_OwnerOnly, REPNOTIFY_OnChanged, true };
	const FDoRepLifetimeParams SkipOwner{ COND_SkipOwner, REPNOTIFY_OnChanged, true };
	const FDoRepLifetimeParams SimulatedOnly{ COND_SimulatedOnly, REPNOTIFY_OnChanged, true };
	const FDoRepLifetimeParams AutonomousOnly{ COND_AutonomousOnly, REPNOTIFY_OnChanged, true };
	const FDoRepLifetimeParams SimulatedOrPhysics{ COND_SimulatedOrPhysics, REPNOTIFY_OnChanged, true };
	const FDoRepLifetimeParams InitialOrOwner{ COND_InitialOrOwner, REPNOTIFY_OnChanged, true };
	const FDoRepLifetimeParams Custom{ COND_Custom, REPNOTIFY_OnChanged, true };
	const FDoRepLifetimeParams ReplayOrOwner{ COND_ReplayOrOwner, REPNOTIFY_OnChanged, true };
	const FDoRepLifetimeParams ReplayOnly{ COND_ReplayOnly, REPNOTIFY_OnChanged, true };
	const FDoRepLifetimeParams SimulatedOnlyNoReplay{ COND_SimulatedOnlyNoReplay, REPNOTIFY_OnChanged, true };
	const FDoRepLifetimeParams SimulatedOrPhysicsNoReplay{ COND_SimulatedOrPhysicsNoReplay, REPNOTIFY_OnChanged, true };
	const FDoRepLifetimeParams SkipReplay{ COND_SkipReplay, REPNOTIFY_OnChanged, true };
	const FDoRepLifetimeParams Never{ COND_Never, REPNOTIFY_OnChanged, true };
}