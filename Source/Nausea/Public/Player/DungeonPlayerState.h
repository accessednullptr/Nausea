// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Player/CorePlayerState.h"
#include "DungeonPlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTrapCoinsChangedDelegate, ADungeonPlayerState*, PlayerState, int32, Coins);

/**
 * 
 */
UCLASS()
class NAUSEA_API ADungeonPlayerState : public ACorePlayerState
{
	GENERATED_UCLASS_BODY()

//~ Begin AActor Interface
protected:
	virtual void BeginPlay() override;
//~ End AActor Interface

public:
	UFUNCTION(BlueprintCallable, Category = Dungeon)
	int32 GetTrapCoins() const;
	UFUNCTION(BlueprintCallable, Category = Dungeon)
	int32 AddTrapCoins(int32 Amount);
	UFUNCTION(BlueprintCallable, Category = Dungeon)
	int32 RemoveTrapCoins(int32 Amount);
	UFUNCTION(BlueprintCallable, Category = Dungeon)
	int32 SetTrapCoins(int32 Amount);
	
	UFUNCTION(BlueprintCallable, Category = Dungeon)
	bool HasEnoughTrapCoins(int32 InCost) const;

public:
	UPROPERTY(BlueprintAssignable, Category = Dungeon)
	FTrapCoinsChangedDelegate OnTrapCoinsChanged;

protected:
	UFUNCTION()
	void OnRep_TrapCoins();

protected:
	UPROPERTY(ReplicatedUsing = OnRep_TrapCoins, Transient)
	int32 TrapCoins = 0;
};
