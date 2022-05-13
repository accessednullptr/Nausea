// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "NauseaHelpers.h"
#include "Character/CoreCharacterComponent.h"
#include "Inventory.generated.h"

UCLASS(Blueprintable, BlueprintType, HideCategories = (ComponentTick, Collision, Tags, Variable, Activation, ComponentReplication, Cooking))
class NAUSEA_API UInventory : public UCoreCharacterComponent
{
	GENERATED_UCLASS_BODY()

//~ Begin UObject Interface
public:
	virtual void PostInitProperties() override;
//~ End UObject Interface

public:
	UFUNCTION(BlueprintCallable, Category = Inventory)
	FORCEINLINE UInventoryManagerComponent* GetOwningInventoryManager() const { return OwningInventoryManager; }
	
	//Is used as a multiplier for the player's speed. NOTE: UInventoryManagerComponent::RequestMovementSpeedModifierUpdate MUST BE CALLED IF THE RETURN CHANGES.
	UFUNCTION(BlueprintCallable, Category = Inventory)
	virtual float GetMovementSpeedModifier() const { return MovementSpeedModifier; }

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Inventory)
	void SetMovementSpeedModifier(float InMovementSpeedModifier);

protected:
	UFUNCTION()
	virtual void InventoryPickedUp();
	UFUNCTION()
	virtual void InventoryDropped();

protected:
	UPROPERTY()
	UInventoryManagerComponent* OwningInventoryManager = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = AI, meta = (PinHiddenByDefault, InlineEditConditionToggle))
	bool bMakeNoiseOnPickup = false;
	UPROPERTY(EditDefaultsOnly, Category = AI, meta = (EditCondition = "bMakeNoiseOnPickup", DisplayName = "Make Noise On Pickup"))
	FCoreNoiseParams PickupNoise = FCoreNoiseParams(CoreNoiseTag::InventoryPickup, 0.15f, 0.f);

	UPROPERTY(EditDefaultsOnly, Category = AI, meta = (PinHiddenByDefault, InlineEditConditionToggle))
	bool bMakeNoiseOnDrop = false;
	UPROPERTY(EditDefaultsOnly, Category = AI, meta = (EditCondition = "bMakeNoiseOnDrop", DisplayName = "Make Noise On Drop"))
	FCoreNoiseParams DropNoise = FCoreNoiseParams(CoreNoiseTag::InventoryDrop, 0.15f, 0.f);

private:
	UPROPERTY(EditDefaultsOnly, Category = Inventory)
	float MovementSpeedModifier = 1.f;
};
