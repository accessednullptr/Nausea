// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Character/CoreCharacterComponent.h"
#include "Gameplay/InteractableTypes.h"
#include "PawnInteractionComponent.generated.h"

class UInputComponent;
class UInteractableComponent;
class UWidgetComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FInteractionEventSignature, UPawnInteractionComponent*, Instigator, UInteractableComponent*, Target, const FVector&, Location, const FVector&, Direction);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPendingInteractionEventSignature, UPawnInteractionComponent*, Instigator, UInteractableComponent*, Target);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FCurrentInteractionTargetChangedSignature, UPawnInteractionComponent*, Instigator, UInteractableComponent*, Target, UInteractableComponent*, PreviousTarget);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCurrentInteractionTargetEventSignature, UPawnInteractionComponent*, Instigator, UInteractableComponent*, Target);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCurrentLookAtActorUpdateSignature, UPawnInteractionComponent*, Instigator, AActor*, Target);

/*
* Interactable component for a given pawn to instigate reactions with.
*/
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class NAUSEA_API UPawnInteractionComponent : public UCoreCharacterComponent
{
	GENERATED_UCLASS_BODY()

//~ Begin UActorComponent Interface
public:
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
//~ End UActorComponent Interface

public:
	UFUNCTION()
	virtual void SetupInputComponent(UInputComponent* InputComponent);

	UFUNCTION(BlueprintCallable, Category = Interaction)
	EInteractionResponse CanInteract(UInteractableComponent* Target, const FVector& Location, const FVector& Direction) const;

	UFUNCTION(BlueprintCallable, Category = Interaction)
	EInteractionResponse Interact(UInteractableComponent* Target, const FVector& Location, const FVector& Direction);
	UFUNCTION()
	bool StopInteract(UInteractableComponent* Target = nullptr);

	UFUNCTION()
	void PendingInteractionStart(UInteractableComponent* Target);
	UFUNCTION()
	void PendingInteractionStop(UInteractableComponent* Target);
	UFUNCTION()
	void PendingInteractionComplete();
	
	UFUNCTION(BlueprintCallable, Category = Interaction)
	FVector GetInteractionLocation() const;
	UFUNCTION(BlueprintCallable, Category = Interaction)
	FVector GetInteractionDirection() const;
	UFUNCTION(BlueprintCallable, Category = Interaction)
	UInteractableComponent* GetCurrentInteractionTraget() const;

	UFUNCTION(BlueprintCallable, Category = Interaction)
	float GetPendingInteractionDuration() const;
	UFUNCTION(BlueprintCallable, Category = Interaction)
	float GetPendingInteractionDurationRemaining() const;
	UFUNCTION(BlueprintCallable, Category = Interaction)
	float GetPendingInteractionDurationPercent() const;

public:
	UPROPERTY(BlueprintAssignable, Category = Interaction)
	FInteractionEventSignature OnInteraction;

	//Pending interaction events.
	UPROPERTY(BlueprintAssignable, Category = Interaction)
	FPendingInteractionEventSignature OnInteractionStart;
	UPROPERTY(BlueprintAssignable, Category = Interaction)
	FPendingInteractionEventSignature OnInteractionStop;
	UPROPERTY(BlueprintAssignable, Category = Interaction)
	FPendingInteractionEventSignature OnInteractionComplete;
	
	UPROPERTY(BlueprintAssignable, Category = Interaction)
	FCurrentInteractionTargetChangedSignature OnCurrentInteractionTargetChanged;
	UPROPERTY(BlueprintAssignable, Category = Interaction)
	FCurrentInteractionTargetEventSignature OnSetCurrentInteractionTarget;
	UPROPERTY(BlueprintAssignable, Category = Interaction)
	FCurrentInteractionTargetEventSignature OnLostCurrentInteractionTarget;

	UPROPERTY(BlueprintAssignable, Category = Interaction)
	FCurrentLookAtActorUpdateSignature OnCurrentLookAtActorUpdate;

protected:
	UFUNCTION()
	void InteractPressed();
	UFUNCTION()
	void InteractReleased();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Reliable_Interact(UInteractableComponent* Target, const FVector& Location, const FVector& Direction);
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Reliable_StartPendingInteract(UInteractableComponent* Target, const FVector& Location, const FVector& Direction);
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Reliable_StopPendingInteract(UInteractableComponent* Target);
	UFUNCTION(Client, Reliable)
	void Client_Reliable_CompletePendingInteraction(UInteractableComponent* Target);
	UFUNCTION(Client, Reliable)
	void Client_Reliable_InterruptedPendingInteraction(UInteractableComponent* Target);

	UFUNCTION()
	void OnRep_InteractionCounter();

	UFUNCTION()
	void SetCurrentInteractionTarget(UInteractableComponent* InteractableComponent);
	UFUNCTION()
	void OnCurrentInteractionTargetSet(UInteractableComponent* InteractableComponent);
	UFUNCTION()
	void OnCurrentInteractionTargetLost(UInteractableComponent* InteractableComponent);

	UFUNCTION()
	void SetCurrentLookAtActor(AActor* Actor);
	
	UFUNCTION()
	void OnHoveredWidgetChanged(UWidgetComponent* WidgetComponent, UWidgetComponent* PreviousWidgetComponent);

protected:
	UPROPERTY(Transient, ReplicatedUsing = OnRep_InteractionCounter)
	int32 InteractionCounter = 0;

	UPROPERTY(Transient)
	TWeakObjectPtr<UInteractableComponent> PendingInteractableComponent = nullptr;
	UPROPERTY(Transient)
	FTimerHandle PendingInteractionTimer;

	UPROPERTY(Transient)
	TWeakObjectPtr<UWidgetComponent> HoveredWidgetComponent = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = Interaction)
	float InteractionDistance = 2000.f;
	UPROPERTY(EditDefaultsOnly, Category = Interaction)
	float InteractionExtent = 10.f;

	UPROPERTY()
	bool bIsAwaitingResponse = false;
	UPROPERTY()
	float AwaitingResponseHoldDuration = -1.f;

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<UInteractableComponent> CurrentInteractionTarget = nullptr;
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> CurrentLookAtActor = nullptr;
};