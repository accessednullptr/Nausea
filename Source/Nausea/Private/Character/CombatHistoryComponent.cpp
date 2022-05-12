// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.


#include "Character/CombatHistoryComponent.h"
#include "NauseaHelpers.h"
#include "Gameplay/StatusInterface.h"
#include "Gameplay/StatusComponent.h"
#include "Gameplay/CoreDamageType.h"

UCombatHistoryComponent::UCombatHistoryComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	
	SetIsReplicatedByDefault(true);
}

void UCombatHistoryComponent::BeginPlay()
{
	Super::BeginPlay();

	OwningStatusInterface = GetOwner();
	BindToStatusComponent();
}

void UCombatHistoryComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (DamageLogBufferList.Num() == 0)
	{
		return;
	}

	const float WorldTimeSeconds = GetWorld()->GetTimeSeconds();
	for (int32 Index = DamageLogBufferList.Num() - 1; Index >= 0; Index--)
	{
		if (DamageLogBufferList[Index].IsExpired(WorldTimeSeconds))
		{
			AddDamageLogEvent(DamageLogBufferList[Index]);
			DamageLogBufferList.RemoveAt(Index, 1, false);
		}
	}

	SetComponentTickEnabled(DamageLogBufferList.Num() > 0);
}

UStatusComponent* UCombatHistoryComponent::GetStatusComponent() const
{
	return UStatusInterfaceStatics::GetStatusComponent(OwningStatusInterface);
}

void UCombatHistoryComponent::BindToStatusComponent()
{
	if (!GetStatusComponent())
	{
		return;
	}

	TWeakObjectPtr<UCombatHistoryComponent> WeakCombatHistoryComponent = this;
	GetStatusComponent()->OnDamageLogPopped.AddWeakLambda(this, [WeakCombatHistoryComponent](const UStatusComponent* StatusComponent, const FDamageLogEvent& DamageLogEvent)
	{
		if (!WeakCombatHistoryComponent.IsValid())
		{
			return;
		}

		WeakCombatHistoryComponent->ProcessDamageLogEvent(DamageLogEvent);
	});
}

inline bool IsBufferedEvent(const FDamageLogEvent& DamageLogEvent)
{
	if (const UCoreDamageType* DamageType = DamageLogEvent.InstigatorDamageType ? DamageLogEvent.InstigatorDamageType.GetDefaultObject() : nullptr)
	{
		return DamageType->ShouldBufferDamageLogEvent();
	}

	return false;
}

void UCombatHistoryComponent::ProcessDamageLogEvent(const FDamageLogEvent& DamageLogEvent)
{
	if (DamageLogEvent.IsDeathEvent())
	{
		ProcessDeathDamageLogEvent(DamageLogEvent);
		return;
	}

	if (IsBufferedEvent(DamageLogEvent))
	{
		PushDamageLogEventToBuffer(DamageLogEvent);
		SetComponentTickEnabled(true);
		return;
	}

	AddDamageLogEvent(DamageLogEvent);
}

inline bool CanCombine(const FDamageLogEvent& A, const FDamageLogEvent& B)
{
	if (A.Instigator == B.Instigator && A.InstigatorWeaponClass == B.InstigatorWeaponClass
		&& A.InstigatorFireModeClass == B.InstigatorFireModeClass && A.InstigatorDamageType == B.InstigatorDamageType)
	{
		return false;
	}

	return true;
}

void UCombatHistoryComponent::ProcessDeathDamageLogEvent(const FDamageLogEvent& DeathDamageLogEvent)
{
	//If the death damage log can be buffered, check if we have a suitable event in the buffer to merge with.
	if (IsBufferedEvent(DeathDamageLogEvent))
	{
		int32 CombinedEventIndex = INDEX_NONE;
		//Find if we have an event to merge with. While doing that, push through all events that were buffered (as we are about to die).
		for (int32 Index = 0; Index < DamageLogBufferList.Num(); Index++)
		{
			if (!CanCombine(DamageLogBufferList[Index], DeathDamageLogEvent))
			{
				AddDamageLogEvent(DamageLogBufferList[Index]);
				continue;
			}

			DamageLogBufferList[Index].CombineWithEvent(DeathDamageLogEvent);
			CombinedEventIndex = Index;
		}

		if (CombinedEventIndex == INDEX_NONE)
		{
			AddDamageLogEvent(DeathDamageLogEvent);
		}
		else
		{
			DamageLogBufferList[CombinedEventIndex];
		}
		return;
	}

	//Flush buffered damage log events, then push the death event.
	for (const FDamageLogEvent& BufferedDamageLogEvent : DamageLogBufferList)
	{
		AddDamageLogEvent(BufferedDamageLogEvent);
	}
	AddDamageLogEvent(DeathDamageLogEvent);
}

void UCombatHistoryComponent::AddDamageLogEvent(const FDamageLogEvent& DamageLogEvent)
{
	DamageLogList.Add(DamageLogEvent);

	OnReceivedDamageLogEvent.Broadcast(this, DamageLogEvent);

	if (DamageLogEvent.IsDeathEvent())
	{
		OnReceivedDamageLogDeathEvent.Broadcast(this, DamageLogEvent);
	}

	if (GetOwnerRole() == ROLE_Authority)
	{
		Client_Relaible_SendDamageLog(DamageLogEvent);
	}
}

int32 UCombatHistoryComponent::PushDamageLogEventToBuffer(const FDamageLogEvent& DamageLogEvent)
{
	for (int32 Index = DamageLogList.Num() - 1; Index >= 0; Index--)
	{
		if (!CanCombine(DamageLogBufferList[Index], DamageLogEvent))
		{
			continue;
		}

		DamageLogBufferList[Index].CombineWithEvent(DamageLogEvent);
		return Index;
	}

	DamageLogBufferList.Add(DamageLogEvent);
	return DamageLogBufferList.Num() - 1;
}

void UCombatHistoryComponent::Client_Relaible_SendDamageLog_Implementation(const FDamageLogEvent& DamageLog)
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		return;
	}

	AddDamageLogEvent(DamageLog);
}