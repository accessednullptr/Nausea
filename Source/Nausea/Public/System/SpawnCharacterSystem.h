// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "SpawnCharacterSystem.generated.h"

class ACoreCharacter;
DECLARE_DELEGATE_OneParam(FCharacterSpawnRequestDelegate, ACoreCharacter*);

USTRUCT(BlueprintType)
struct FSpawnRequest
{
	GENERATED_USTRUCT_BODY()

	FSpawnRequest() {}
	FSpawnRequest(TSubclassOf<ACoreCharacter> InCharacterClass, const FTransform& InCharacterTransform,
		const FActorSpawnParameters& InCharacterSpawnParameters, FCharacterSpawnRequestDelegate&& InSpawnDelegate)
		: CharacterClass(InCharacterClass), CharacterTransform(InCharacterTransform),
		CharacterSpawnParameters(InCharacterSpawnParameters), WeakSpawnParamOwner(), WeakSpawnParamInstigator(),
		SpawnDelegate(MoveTemp(InSpawnDelegate))
		{}

public:
	bool IsValid() const { return SpawnDelegate.IsBound(); }
	bool IsOwnedBy(const UObject* OwningObject) const { return SpawnDelegate.IsBoundToObject(OwningObject); }

	const TSubclassOf<ACoreCharacter>& GetCharacterClass() const { return CharacterClass; }
	const FTransform& GetTransform() const { return CharacterTransform; }
	const FActorSpawnParameters& GetActorSpawnParameters() const
	{
		FSpawnRequest& MutableThis = const_cast<FSpawnRequest&>(*this);
		MutableThis.CharacterSpawnParameters.Owner = WeakSpawnParamOwner.Get();
		MutableThis.CharacterSpawnParameters.Instigator = WeakSpawnParamInstigator.Get();
		return CharacterSpawnParameters;
	}

	bool BroadcastRequestResult(ACoreCharacter* Character)
	{
		if (SpawnDelegate.IsBound())
		{
			SpawnDelegate.Execute(Character);
			return true;
		}
		
		return false;
	}

protected:
	UPROPERTY(Transient)
	TSubclassOf<ACoreCharacter> CharacterClass = nullptr;
	UPROPERTY(Transient)
	FTransform CharacterTransform = FTransform::Identity;

	FActorSpawnParameters CharacterSpawnParameters = FActorSpawnParameters();
	//These are stored as raw pointers in FActorSpawnParameters so we're going to copy them in weak pointers for safety reasons.
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> WeakSpawnParamOwner;
	UPROPERTY(Transient)
	TWeakObjectPtr<APawn> WeakSpawnParamInstigator;

	FCharacterSpawnRequestDelegate SpawnDelegate;
};

/**
 * 
 */
UCLASS()
class NAUSEA_API USpawnCharacterSystem : public UObject
{
	GENERATED_UCLASS_BODY()
	
public:
	static bool RequestSpawn(const UObject* WorldContextObject, TSubclassOf<ACoreCharacter> CoreCharacterClass, const FTransform& Transform, const FActorSpawnParameters& SpawnParameters, FCharacterSpawnRequestDelegate&& Delegate);

	static int32 CancelRequestsForObject(const UObject* WorldContextObject, const UObject* OwningObject);
	static int32 CancelAllRequests(const UObject* WorldContextObject);

protected:
	bool AddRequest(FSpawnRequest&& SpawnRequest);
	int32 CancelRequestForObject(const UObject* OwningObject);
	int32 CancelAllRequests();

	UFUNCTION()
	void PerformNextSpawn();

protected:
	UPROPERTY(Transient)
	TArray<FSpawnRequest> CharacterSpawnRequestList;
	UPROPERTY(Transient)
	FTimerHandle SpawnTimerHandle = FTimerHandle();
};