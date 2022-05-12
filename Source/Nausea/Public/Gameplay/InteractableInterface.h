// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "InteractableInterface.generated.h"

class UInteractableComponent;

UINTERFACE(Blueprintable)
class UInteractableInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class NAUSEA_API IInteractableInterface
{
	GENERATED_IINTERFACE_BODY()

public:
	virtual UInteractableComponent* GetInteractableComponent() const { return IInteractableInterface::Execute_K2_GetInteractableComponent(Cast<UObject>(this)); }
	virtual bool CanInteract(UPawnInteractionComponent* InstigatorComponent, const FVector& Location, const FVector& Direction) const { return IInteractableInterface::Execute_K2_CanInteract(Cast<UObject>(this), InstigatorComponent, Location, Direction); }
	virtual void OnInteraction(UPawnInteractionComponent* InstigatorComponent, const FVector& Location, const FVector& Direction) { IInteractableInterface::Execute_K2_OnInteraction(Cast<UObject>(this), InstigatorComponent, Location, Direction); }

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = InteractableInterface, meta = (DisplayName="Get Interactable Component"))
	UInteractableComponent* K2_GetInteractableComponent() const;
	UFUNCTION(BlueprintImplementableEvent, Category = InteractableInterface, meta = (DisplayName="Can Interactable"))
	bool K2_CanInteract(UPawnInteractionComponent* InstigatorComponent, const FVector& Location, const FVector& Direction) const;
	UFUNCTION(BlueprintImplementableEvent, Category = InteractableInterface, meta = (DisplayName="On Interact"))
	void K2_OnInteraction(UPawnInteractionComponent* InstigatorComponent, const FVector& Location, const FVector& Direction);
};

UCLASS(Abstract, MinimalAPI)
class UInteractableInterfaceStatics : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = StatusInterface)
	static UInteractableComponent* GetInteractableComponent(TScriptInterface<IInteractableInterface> Target);

	UFUNCTION(BlueprintCallable, Category = StatusInterface)
	static bool CanInteract(TScriptInterface<IInteractableInterface> Target, UPawnInteractionComponent* InstigatorComponent, const FVector& Location, const FVector& Direction);

protected:
	UFUNCTION()
	void RemoveDeadData();

protected:
	TMap<TObjectKey<UObject>, TWeakObjectPtr<UInteractableComponent>> CachedInteractableComponentMap = TMap<TObjectKey<UObject>, TWeakObjectPtr<UInteractableComponent>>();
};