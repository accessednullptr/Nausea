// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Gameplay/PawnInteractionComponent.h"
#include "Components/InputComponent.h"
#include "Components/WidgetComponent.h"
#include "NauseaHelpers.h"
#include "System/NetHelper.h"
#include "Character/CoreCharacter.h"
#include "Player/CorePlayerController.h"
#include "Player/CoreWidgetInteractionComponent.h"
#include "Gameplay/InteractableComponent.h"
#include "Gameplay/InteractableInterface.h"
#include "Player/CameraModifier/InteractionCameraModifier.h"
#include "Gameplay/StatusComponent.h"

UPawnInteractionComponent::UPawnInteractionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UPawnInteractionComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_WITH_PARAMS_FAST(UPawnInteractionComponent, InteractionCounter, PushReplicationParams::Default);
}

static FCollisionQueryParams InteractionQueryParams = FCollisionQueryParams("InteractionTrace", QUICK_USE_CYCLE_STAT(InteractionTrace, STATGROUP_Collision), true);
void UPawnInteractionComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (TickType == ELevelTick::LEVELTICK_PauseTick)
	{
		return;
	}

	if (PendingInteractableComponent.IsValid())
	{
		return;
	}

	if (HoveredWidgetComponent.IsValid())
	{
		return;
	}

	FCollisionQueryParams CQP = InteractionQueryParams;
	CQP.AddIgnoredActor(GetOwner());

	TArray<FHitResult> HitResultList;
	const FVector InteractionLocation = GetInteractionLocation();
	const FVector InteractionDirection = GetInteractionDirection();
	const FVector InteractionEndLocation = InteractionLocation + (InteractionDirection * InteractionDistance);
	GetWorld()->SweepMultiByChannel(HitResultList, InteractionLocation, InteractionEndLocation, FQuat::Identity, ECC_InteractionTrace, FCollisionShape::MakeSphere(InteractionExtent), CQP);

	for (const FHitResult& HitResult : HitResultList)
	{
		TScriptInterface<IInteractableInterface> HitInterface(HitResult.GetActor());
		
		if (!UInteractableInterfaceStatics::CanInteract(HitInterface, this, InteractionLocation, InteractionDirection))
		{
			continue;
		}

		UInteractableComponent* InteractableComponent = UInteractableInterfaceStatics::GetInteractableComponent(HitInterface);

		if (InteractableComponent)
		{
			SetCurrentLookAtActor(nullptr);
			SetCurrentInteractionTarget(InteractableComponent);
			return;
		}
	}

	SetCurrentInteractionTarget(nullptr);

	for (const FHitResult& HitResult : HitResultList)
	{
		TScriptInterface<IStatusInterface> HitInterface(HitResult.GetActor());

		if (UStatusComponent* HitStatusComponent = UStatusInterfaceStatics::GetStatusComponent(HitInterface))
		{
			if (!HitStatusComponent->CanDisplayStatus() || HitStatusComponent->IsDead())
			{
				continue;
			}

			SetCurrentLookAtActor(HitResult.GetActor());
			return;
		}
	}

	SetCurrentLookAtActor(nullptr);
}

void UPawnInteractionComponent::SetupInputComponent(UInputComponent* InputComponent)
{
	InputComponent->BindAction("Interact", EInputEvent::IE_Pressed, this, &UPawnInteractionComponent::InteractPressed);
	InputComponent->BindAction("Interact", EInputEvent::IE_Released, this, &UPawnInteractionComponent::InteractReleased);
	SetComponentTickEnabled(true);

	if (GetOwningCharacter())
	{
		GetOwningCharacter()->GetWidgetInteractionComponent()->OnHoveredWidgetChanged.AddDynamic(this, &UPawnInteractionComponent::OnHoveredWidgetChanged);
	}
}

EInteractionResponse UPawnInteractionComponent::CanInteract(UInteractableComponent* Target, const FVector& Location, const FVector& Direction) const
{
	if (!Target)
	{
		return EInteractionResponse::Failed;
	}

	return EInteractionResponse::Success;
}

EInteractionResponse UPawnInteractionComponent::Interact(UInteractableComponent* Target, const FVector& Location, const FVector& Direction)
{
	EInteractionResponse Response = CanInteract(Target, Location, Direction);
	if (Response >= EInteractionResponse::Failed)
	{
		return Response;
	}

	Response = Target->Interact(this, Location, Direction);
	if (Response >= EInteractionResponse::Failed)
	{
		return Response;
	}

	InteractionCounter++;
	MARK_PROPERTY_DIRTY_FROM_NAME(UPawnInteractionComponent, InteractionCounter, this);

	if (IsLocallyOwnedRemote())
	{
		switch (Response)
		{
		case EInteractionResponse::Success:
			Server_Reliable_Interact(Target, Location, Direction);
			OnInteractionComplete.Broadcast(this, Target);
			break;
		case EInteractionResponse::Pending:
			Server_Reliable_StartPendingInteract(Target, Location, Direction);
			break;
		}
		
		return Response;
	}
	

	switch (Response)
	{
	case EInteractionResponse::Success:
		OnInteractionComplete.Broadcast(this, Target);
		break;
	}

	return EInteractionResponse::Success;
}

bool UPawnInteractionComponent::StopInteract(UInteractableComponent* Target)
{
	if (!PendingInteractableComponent.IsValid() || (Target && Target != PendingInteractableComponent.Get()))
	{
		return false;
	}

	Target = Target ? Target : PendingInteractableComponent.Get();

	ensure(PendingInteractableComponent->StopInteract(this));
	
	if (IsLocallyOwnedRemote())
	{
		Server_Reliable_StopPendingInteract(Target);
	}

	return true;
}

void UPawnInteractionComponent::PendingInteractionStart(UInteractableComponent* Target)
{
	ensure(!PendingInteractableComponent.IsValid());
	PendingInteractableComponent = Target;
	
	if (IsLocallyOwned())
	{
		UInteractionCameraModifier::CreateInteractionRequest(Cast<ACorePlayerController>(GetOwningController()), this);
	}

	if (IsLocallyOwnedRemote())
	{
		GetWorld()->GetTimerManager().SetTimer(PendingInteractionTimer, Target->GetHoldInteractionDuration(), false);
		bIsAwaitingResponse = true;
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(PendingInteractionTimer, this, &UPawnInteractionComponent::PendingInteractionComplete, Target->GetHoldInteractionDuration(), false);
	}

	OnInteractionStart.Broadcast(this, Target);
}

void UPawnInteractionComponent::PendingInteractionStop(UInteractableComponent* Target)
{
	ensure(PendingInteractableComponent.Get() == Target);
	
	if (IsLocallyOwned())
	{
		UInteractionCameraModifier::ClearInteractionRequest(Cast<ACorePlayerController>(GetOwningController()), this);
	}

	GetWorld()->GetTimerManager().ClearTimer(PendingInteractionTimer);
	bIsAwaitingResponse = false;

	PendingInteractableComponent = nullptr;
	OnInteractionStop.Broadcast(this, Target);
}

void UPawnInteractionComponent::PendingInteractionComplete()
{
	UInteractableComponent* InteractableComponent = PendingInteractableComponent.Get();
	ensure(InteractableComponent);

	if (IsLocallyOwned())
	{
		UInteractionCameraModifier::ClearInteractionRequest(Cast<ACorePlayerController>(GetOwningController()), this);
	}

	GetWorld()->GetTimerManager().ClearTimer(PendingInteractionTimer);
	bIsAwaitingResponse = false;

	InteractableComponent->CompletePendingInteraction(this);

	PendingInteractableComponent = nullptr;
	OnInteractionComplete.Broadcast(this, InteractableComponent);

	if (IsNonOwningAuthority())
	{
		Client_Reliable_CompletePendingInteraction(InteractableComponent);
	}
}

FVector UPawnInteractionComponent::GetInteractionLocation() const
{
	return GetOwningCharacter()->GetPawnViewLocation();
}

FVector UPawnInteractionComponent::GetInteractionDirection() const
{
	return GetOwningCharacter()->GetBaseAimRotation().Vector();
}

UInteractableComponent* UPawnInteractionComponent::GetCurrentInteractionTraget() const
{
	return CurrentInteractionTarget.Get();
}

float UPawnInteractionComponent::GetPendingInteractionDuration() const
{
	if (GetWorld()->GetTimerManager().IsTimerActive(PendingInteractionTimer))
	{
		return GetWorld()->GetTimerManager().GetTimerRate(PendingInteractionTimer);
	}

	if (bIsAwaitingResponse)
	{
		return AwaitingResponseHoldDuration;
	}

	return GetWorld()->GetTimerManager().IsTimerActive(PendingInteractionTimer) ? GetWorld()->GetTimerManager().GetTimerRate(PendingInteractionTimer) : -1.f;
}

float UPawnInteractionComponent::GetPendingInteractionDurationRemaining() const
{
	if (GetWorld()->GetTimerManager().IsTimerActive(PendingInteractionTimer))
	{
		return GetWorld()->GetTimerManager().GetTimerRemaining(PendingInteractionTimer);
	}

	if (bIsAwaitingResponse)
	{
		return 0.f;
	}

	return -1.f;
}

float UPawnInteractionComponent::GetPendingInteractionDurationPercent() const
{
	if (GetWorld()->GetTimerManager().IsTimerActive(PendingInteractionTimer))
	{
		return (GetPendingInteractionDuration() - GetPendingInteractionDurationRemaining()) / GetPendingInteractionDuration();
	}

	if (bIsAwaitingResponse)
	{
		return 1.f;
	}

	return -1.f;
}

void UPawnInteractionComponent::InteractPressed()
{
	if (!IsLocallyOwned())
	{
		return;
	}

	if (GetOwningPilotCharacter() && GetOwningPilotCharacter()->GetCurrentVehicleSeat())
	{
		GetOwningPilotCharacter()->ExitVehicleSeat(nullptr, EVehicleExitType::Request);
		return;
	}

	if (!GetCurrentInteractionTraget())
	{
		return;
	}

	Interact(GetCurrentInteractionTraget(), GetInteractionLocation(), GetInteractionDirection());
}

void UPawnInteractionComponent::InteractReleased()
{
	if (!IsLocallyOwned())
	{
		return;
	}

	StopInteract();
}

bool UPawnInteractionComponent::Server_Reliable_Interact_Validate(UInteractableComponent* Target, const FVector& Location, const FVector& Direction)
{
	return true;
}

void UPawnInteractionComponent::Server_Reliable_Interact_Implementation(UInteractableComponent* Target, const FVector& Location, const FVector& Direction)
{
	Interact(Target, Location, Direction);
}

bool UPawnInteractionComponent::Server_Reliable_StartPendingInteract_Validate(UInteractableComponent* Target, const FVector& Location, const FVector& Direction)
{
	return true;
}

void UPawnInteractionComponent::Server_Reliable_StartPendingInteract_Implementation(UInteractableComponent* Target, const FVector& Location, const FVector& Direction)
{
	Interact(Target, Location, Direction);
}

bool UPawnInteractionComponent::Server_Reliable_StopPendingInteract_Validate(UInteractableComponent* Target)
{
	return true;
}

void UPawnInteractionComponent::Server_Reliable_StopPendingInteract_Implementation(UInteractableComponent* Target)
{
	if (PendingInteractableComponent.Get() != Target)
	{
		return;
	}

	StopInteract(Target);
}

void UPawnInteractionComponent::Client_Reliable_CompletePendingInteraction_Implementation(UInteractableComponent* Target)
{
	if (Target && PendingInteractableComponent.IsValid())
	{
		PendingInteractionComplete();
	}
}

void UPawnInteractionComponent::Client_Reliable_InterruptedPendingInteraction_Implementation(UInteractableComponent* Target)
{
	StopInteract(Target);
}

void UPawnInteractionComponent::OnRep_InteractionCounter()
{
	OnInteraction.Broadcast(this, nullptr, GetInteractionLocation(), GetInteractionDirection());
}

void UPawnInteractionComponent::SetCurrentInteractionTarget(UInteractableComponent* InteractableComponent)
{
	if (InteractableComponent == CurrentInteractionTarget)
	{
		return;
	}

	UInteractableComponent* PreviousInteractionTarget = CurrentInteractionTarget.Get();

	if (PreviousInteractionTarget)
	{
		OnCurrentInteractionTargetLost(PreviousInteractionTarget);
		CurrentInteractionTarget = nullptr;
	}

	if (!InteractableComponent)
	{
		OnCurrentInteractionTargetChanged.Broadcast(this, nullptr, PreviousInteractionTarget);
		return;
	}

	OnCurrentInteractionTargetSet(InteractableComponent);
	CurrentInteractionTarget = InteractableComponent;

	OnCurrentInteractionTargetChanged.Broadcast(this, InteractableComponent, PreviousInteractionTarget);
}

void UPawnInteractionComponent::OnCurrentInteractionTargetSet(UInteractableComponent* InteractableComponent)
{
	OnSetCurrentInteractionTarget.Broadcast(this, InteractableComponent);
}

void UPawnInteractionComponent::OnCurrentInteractionTargetLost(UInteractableComponent* InteractableComponent)
{
	if (PendingInteractableComponent.IsValid() && IsLocallyOwned())
	{
		StopInteract(InteractableComponent);
	}

	OnLostCurrentInteractionTarget.Broadcast(this, InteractableComponent);
}

void UPawnInteractionComponent::SetCurrentLookAtActor(AActor* Actor)
{
	if (Actor == CurrentLookAtActor)
	{
		return;
	}

	CurrentLookAtActor = Actor;
	OnCurrentLookAtActorUpdate.Broadcast(this, CurrentLookAtActor.Get());
}

void UPawnInteractionComponent::OnHoveredWidgetChanged(UWidgetComponent* WidgetComponent, UWidgetComponent* PreviousWidgetComponent)
{
	if (WidgetComponent)
	{
		SetCurrentInteractionTarget(nullptr);
		HoveredWidgetComponent = WidgetComponent;
	}
	else
	{
		HoveredWidgetComponent = nullptr;
	}
}