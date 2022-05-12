// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Persona.generated.h"

class ACoreCharacter;
class AAIController;
class UAgenda;

/**
 * 
 */
UCLASS()
class NAUSEA_API UPersona : public UObject
{
	GENERATED_UCLASS_BODY()
	
public:
	UFUNCTION()
	virtual void InitializePersona();

	UFUNCTION(BlueprintCallable, Category = Persona)
	virtual TSoftClassPtr<ACoreCharacter> GetPawnClass() const { return DefaultPawnClass; }

	UFUNCTION(BlueprintCallable, Category = Persona)
	virtual TSoftClassPtr<AAIController> GetAIClass() const { return DefaultAIClass; }

	UFUNCTION(BlueprintCallable, Category = Persona)
	virtual TSoftClassPtr<UAgenda> GetAgendaClass() const { return DefaultAgendaClass; }

	UFUNCTION()
	virtual void Killed(ACoreCharacter* Character) { if (CanRespawn()) { RequestRespawn(); } }

	UFUNCTION(BlueprintCallable, Category = Persona)
	virtual bool CanRespawn() const { return false; }

	UFUNCTION()
	virtual void RequestRespawn() {}

protected:
	UPROPERTY(EditDefaultsOnly, Category = Persona)
	TSoftClassPtr<ACoreCharacter> DefaultPawnClass = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = Persona)
	TSoftClassPtr<AAIController> DefaultAIClass = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = Persona)
	TSoftClassPtr<UAgenda> DefaultAgendaClass = nullptr;

	UPROPERTY()
	TMap<TSubclassOf<UAgenda>,UAgenda*> LoadedAgendaMap;
};
