// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Gameplay/InteractableComponent.h"
#include "NauseaNetDefines.h"
#include "Gameplay/PawnInteractionComponent.h"
#include "Gameplay/InteractableInterface.h"

UInteractableComponent::UInteractableComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetAutoActivate(true);
}

void UInteractableComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_WITH_PARAMS_FAST(UInteractableComponent, PendingInteractionInstigatorList, PushReplicationParams::Default);
}

void UInteractableComponent::BeginPlay()
{
	InitializeInteractableComponent();

	Super::BeginPlay();
}

void UInteractableComponent::InitializeInteractableComponent()
{
	if (GetOwnerInterface())
	{
		return;
	}

	InteractableInterface = GetOwner();
}

EInteractionResponse UInteractableComponent::CanInteract(UPawnInteractionComponent* Instigator, const FVector& Location, const FVector& Direction) const
{
	if (!IsActive() || !Instigator || PendingInteractionInstigatorList.Num() >= MaxConcurrentInteractions)
	{
		return EInteractionResponse::Invalid;
	}

	//Default to true - it's not mandatory that our owning actor has implemented the interactable interface.
	if (!TSCRIPTINTERFACE_CALL_FUNC_RET(InteractableInterface, CanInteract, K2_CanInteract, true, Instigator, Location, Direction))
	{
		return EInteractionResponse::Invalid;
	}

	return EInteractionResponse::Success;
}

EInteractionResponse UInteractableComponent::Interact(UPawnInteractionComponent* InteractionInstigatorComponent, const FVector& Location, const FVector& Direction)
{
	EInteractionResponse Response = CanInteract(InteractionInstigatorComponent, Location, Direction);
	if (Response >= EInteractionResponse::Failed)
	{
		return Response;
	}

	if (IsHeldInteraction())
	{
		if (!PendingInteractionInstigatorList.Contains(InteractionInstigatorComponent))
		{
			StartPendingInteraction(InteractionInstigatorComponent);
		}

		return EInteractionResponse::Pending;
	}

	PerformInteract(InteractionInstigatorComponent, Location, Direction);
	return Response;
}

bool UInteractableComponent::StopInteract(UPawnInteractionComponent* InteractionInstigatorComponent)
{
	if (!PendingInteractionInstigatorList.Contains(InteractionInstigatorComponent))
	{
		return false;
	}

	StopPendingInteraction(InteractionInstigatorComponent);
	return true;
}

void UInteractableComponent::StartPendingInteraction(UPawnInteractionComponent* PendingInstigator)
{
	if (!PendingInstigator->IsSimulatedProxy())
	{
		PendingInteractionInstigatorList.Add(PendingInstigator);
		PendingInteractionInstigatorList.Remove(nullptr);
		MARK_PROPERTY_DIRTY_FROM_NAME(UInteractableComponent, PendingInteractionInstigatorList, this);
		PreviousPendingInteractionInstigatorSet.Add(PendingInstigator);
		PreviousPendingInteractionInstigatorSet.Remove(nullptr);
	}

	PendingInstigator->PendingInteractionStart(this);
	OnInteractStart.Broadcast(this, PendingInstigator);
}

void UInteractableComponent::StopPendingInteraction(UPawnInteractionComponent* PendingInstigator)
{
	if (!PendingInstigator->IsSimulatedProxy())
	{
		PendingInteractionInstigatorList.Remove(PendingInstigator);
		PendingInteractionInstigatorList.Remove(nullptr);
		MARK_PROPERTY_DIRTY_FROM_NAME(UInteractableComponent, PendingInteractionInstigatorList, this);
		PreviousPendingInteractionInstigatorSet.Remove(PendingInstigator);
		PreviousPendingInteractionInstigatorSet.Remove(nullptr);
	}

	PendingInstigator->PendingInteractionStop(this);
	OnInteractStop.Broadcast(this, PendingInstigator);
}

void UInteractableComponent::CompletePendingInteraction(UPawnInteractionComponent* PendingInstigator)
{
	if (!PendingInstigator)
	{
		return;
	}

	PendingInteractionInstigatorList.Remove(PendingInstigator);
	PendingInteractionInstigatorList.Remove(nullptr);
	MARK_PROPERTY_DIRTY_FROM_NAME(UInteractableComponent, PendingInteractionInstigatorList, this);
	PerformInteract(PendingInstigator, PendingInstigator->GetInteractionLocation(), PendingInstigator->GetInteractionDirection());
}

void UInteractableComponent::GetInteractableInfo(UPawnInteractionComponent* Instigator, FText& Text, TSoftObjectPtr<UTexture2D>& Icon) const
{
	Text = DefaultInteractionText;
	Icon = DefaultInteractionIcon;
}

void UInteractableComponent::PerformInteract(UPawnInteractionComponent* InteractionInstigatorComponent, const FVector& Location, const FVector& Direction)
{
	TSCRIPTINTERFACE_CALL_FUNC(InteractableInterface, OnInteraction, K2_OnInteraction, InteractionInstigatorComponent, Location, Direction);
	OnInteract.Broadcast(this, InteractionInstigatorComponent, Location, Direction);
}

void UInteractableComponent::OnRep_PendingInteractionInstigator()
{
	TSet<TWeakObjectPtr<UPawnInteractionComponent>> NewPendingInteractionInstigatorSet;
	NewPendingInteractionInstigatorSet.Reserve(PendingInteractionInstigatorList.Num());

	for (const TWeakObjectPtr<UPawnInteractionComponent>& NewPendingInteraction : PendingInteractionInstigatorList)
	{
		if (!NewPendingInteraction.IsValid())
		{
			continue;
		}

		if (!PreviousPendingInteractionInstigatorSet.Contains(NewPendingInteraction))
		{
			StartPendingInteraction(NewPendingInteraction.Get());
		}

		NewPendingInteractionInstigatorSet.Add(NewPendingInteraction);
	}

	TArray<TWeakObjectPtr<UPawnInteractionComponent>> PreviousInteractionInstigatorList = PreviousPendingInteractionInstigatorSet.Array();
	for (const TWeakObjectPtr<UPawnInteractionComponent>& PreviousPendingInteraction : PreviousInteractionInstigatorList)
	{
		if (!NewPendingInteractionInstigatorSet.Contains(PreviousPendingInteraction))
		{
			StopPendingInteraction(PreviousPendingInteraction.Get());
		}
	}

	PreviousPendingInteractionInstigatorSet = NewPendingInteractionInstigatorSet;
}