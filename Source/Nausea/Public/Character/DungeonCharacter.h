// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/CoreCharacter.h"
#include "DungeonCharacter.generated.h"

class USkeletalMeshComponentBudgeted;
class UCurveFloat;

class UDungeonCharacterDescription;

UCLASS(Blueprintable)
class NAUSEA_API ADungeonCharacter : public ACoreCharacter
{
	GENERATED_UCLASS_BODY()

//~ Begin AActor Interface
protected:
	virtual void BeginPlay() override;
public:
	virtual void PreRegisterAllComponents() override;
//~ End AActor Interface

//~ Begin IStatusInterface Interface
public:
	virtual void Died(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
//~ End IStatusInterface Interface

public:
	int32 GetCoinValue() const;
	int32 GetGameDamageValue() const;

	void NotifyCharacterUnableToPath();

protected:
	UPROPERTY(EditDefaultsOnly, Category = Mesh)
	UPhysicsAsset* RagdollPhysicsAsset = nullptr;

	//Base amount of coins to grant per kill of this character.
	UPROPERTY(EditDefaultsOnly, Category = Dungeon)
	int32 BaseCoinValue = 5;

	//Curve applied specially to this character based on difficulty.
	UPROPERTY(EditDefaultsOnly, Category = Dungeon)
	UCurveFloat* BaseCoinValueDifficultyScale = nullptr;

	//Curve applied specially to this character based on wave.
	UPROPERTY(EditDefaultsOnly, Category = Dungeon)
	UCurveFloat* BaseCoinValueWaveScale = nullptr;

	//Amount of damage this character applies once it arrives at an end point.
	UPROPERTY(EditDefaultsOnly, Category = Dungeon)
	int32 BaseGameDamageValue = 5;

	UPROPERTY(EditDefaultsOnly, Category = Dungeon)
	TSubclassOf<UDungeonCharacterDescription> CharacterDescription = nullptr;

public:
	UFUNCTION(BlueprintCallable, Category = Dungeon)
	static TSubclassOf<UDungeonCharacterDescription> GetCharacterDescriptor(TSubclassOf<ADungeonCharacter> DungeonCharacterClass);

protected:
	UPROPERTY(Transient)
	FVector SpawnLocation = FAISystem::InvalidLocation;
};