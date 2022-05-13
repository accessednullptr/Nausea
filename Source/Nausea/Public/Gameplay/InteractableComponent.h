// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Internationalization/StringTableRegistry.h"
#include "Components/ActorComponent.h"
#include "Gameplay/InteractableTypes.h"
#include "InteractableComponent.generated.h"

class IInteractableInterface;
class UPawnInteractionComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FInteractableEventSignature, UInteractableComponent*, Target, UPawnInteractionComponent*, Instigator, const FVector&, Location, const FVector&, Direction);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPendingInteractableEventSignature, UInteractableComponent*, Target, UPawnInteractionComponent*, Instigator);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInteractableInfoChangedSignature, UInteractableComponent*, Target);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), HideCategories = (Variable, Sockets, Tags, ComponentTick, ComponentReplication, Activation, Cooking, Events, AssetUserData, UserAssetData, Collision))
class NAUSEA_API UInteractableComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()
		
friend UPawnInteractionComponent;

//~ Begin UActorComponent Interface 
protected:
	virtual void BeginPlay() override;
//~ End UActorComponent Interface

public:
	UFUNCTION(BlueprintCallable, Category = InteractableComponent)
	TScriptInterface<IInteractableInterface> GetOwnerInterface() const { return InteractableInterface; }

	UFUNCTION()
	void InitializeInteractableComponent();

	UFUNCTION()
	virtual EInteractionResponse CanInteract(UPawnInteractionComponent* Instigator, const FVector& Location, const FVector& Direction) const;

	UFUNCTION()
	EInteractionResponse Interact(UPawnInteractionComponent* InteractionInstigatorComponent, const FVector& Location, const FVector& Direction);
	UFUNCTION()
	bool StopInteract(UPawnInteractionComponent* InteractionInstigatorComponent);

	UFUNCTION()
	void StartPendingInteraction(UPawnInteractionComponent* PendingTarget);
	UFUNCTION()
	void StopPendingInteraction(UPawnInteractionComponent* PendingTarget);
	UFUNCTION()
	void CompletePendingInteraction(UPawnInteractionComponent* PendingTarget);

	UFUNCTION(BlueprintCallable, Category = Interaction)
	bool IsHeldInteraction() const { return bHoldInteraction; }
	UFUNCTION(BlueprintCallable, Category = Interaction)
	float GetHoldInteractionDuration() const { return HoldInteractionDuration; }

	UFUNCTION(BlueprintCallable, Category = Interaction)
	virtual void GetInteractableInfo(UPawnInteractionComponent* Instigator, FText& Text, TSoftObjectPtr<UTexture2D>& Icon) const;

public:
	UPROPERTY(BlueprintAssignable, Category = InteractableComponent)
	FInteractableEventSignature OnInteract;
	
	//Pending interaction events.
	UPROPERTY(BlueprintAssignable, Category = InteractableComponent)
	FPendingInteractableEventSignature OnInteractStart;
	UPROPERTY(BlueprintAssignable, Category = InteractableComponent)
	FPendingInteractableEventSignature OnInteractStop;
	UPROPERTY(BlueprintAssignable, Category = InteractableComponent)
	FPendingInteractableEventSignature OnInteractComplete;

	UPROPERTY(BlueprintAssignable, Category = InteractableComponent)
	FInteractableInfoChangedSignature OnInteractableInfoChanged;

protected:
	UFUNCTION()
	virtual void PerformInteract(UPawnInteractionComponent* InteractionInstigatorComponent, const FVector& Location, const FVector& Direction);
	
	UFUNCTION()
	void OnRep_PendingInteractionInstigator();

protected:
	UPROPERTY(EditDefaultsOnly, Category = Interaction)
	bool bHoldInteraction = false;
	
	UPROPERTY(EditDefaultsOnly, Category = Interaction)
	float HoldInteractionDuration = 3.f;

	UPROPERTY(EditDefaultsOnly, Category = Interaction)
	uint8 MaxConcurrentInteractions = 1;

	UPROPERTY(EditDefaultsOnly, Category = Interaction)
	FText DefaultInteractionText = LOCTABLE("/Game/Localization/InteractionStringTable.InteractionStringTable", "Interaction_DisplayText_Default");
	UPROPERTY(EditDefaultsOnly, Category = Interaction)
	TSoftObjectPtr<UTexture2D> DefaultInteractionIcon = nullptr;

private:
	UPROPERTY(Transient, ReplicatedUsing = OnRep_PendingInteractionInstigator)
	TArray<TWeakObjectPtr<UPawnInteractionComponent>> PendingInteractionInstigatorList;
	UPROPERTY(Transient)
	TSet<TWeakObjectPtr<UPawnInteractionComponent>> PreviousPendingInteractionInstigatorSet;

	UPROPERTY(Transient)
	TScriptInterface<IInteractableInterface> InteractableInterface = nullptr;
};
