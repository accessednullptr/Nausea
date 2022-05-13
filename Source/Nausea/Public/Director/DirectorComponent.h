// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DirectorComponent.generated.h"

//Region - a set of entities in a given area
//Entity - represents any entity that is within the director system.
//Persona - an entity that represents a character
//Agenda - a persona's list of things to do for a given day
//Task - a item to do in an agenda at a given time
//Action - a actionable within a task

/*
 * The Director Component manages gameplay events such as .
 * They are responsible for all physical interaction between the player or AI and the world, and also implement basic networking and input models.
 * They are designed for a vertically-oriented player representation that can walk, jump, fly, and swim through the world using CharacterMovementComponent.
*/
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class NAUSEA_API UDirectorComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = Time)
	virtual uint8 GetSeason() const { return 0; }
	UFUNCTION(BlueprintCallable, Category = Time)
	int64 GetCurrentDay() const { return 0; }
};
