// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UObject/Interface.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SpawnCharacterSystem.generated.h"

class ACoreCharacter;
DECLARE_DELEGATE_TwoParams(FCharacterSpawnRequestDelegate, const FSpawnRequest&, ACoreCharacter*);

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

	FSpawnRequest(TSubclassOf<ACoreCharacter> InCharacterClass, const FTransform& InCharacterTransform,
		const FActorSpawnParameters& InCharacterSpawnParameters)
		: CharacterClass(InCharacterClass), CharacterTransform(InCharacterTransform),
		CharacterSpawnParameters(InCharacterSpawnParameters), WeakSpawnParamOwner(), WeakSpawnParamInstigator()
	{}

public:
	bool IsValid() const;
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
			SpawnDelegate.Execute(*this, Character);
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
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = SpawnCharacterSystem)
	static bool SpawnCharacter(const UObject* WorldContextObject, TSubclassOf<ACoreCharacter> CoreCharacterClass, const FTransform& Transform, AActor* Owner, APawn* Instigator);

	static bool RequestSpawn(const UObject* WorldContextObject, TSubclassOf<ACoreCharacter> CoreCharacterClass, const FTransform& Transform, const FActorSpawnParameters& SpawnParameters, FCharacterSpawnRequestDelegate&& Delegate);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = SpawnCharacterSystem)
	static int32 CancelRequestsForObject(const UObject* WorldContextObject, const UObject* OwningObject);
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = SpawnCharacterSystem)
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

UINTERFACE()
class USpawnLocationInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class ISpawnLocationInterface
{
	GENERATED_IINTERFACE_BODY()
	
public:
	virtual bool GetSpawnTransform(TSubclassOf<ACoreCharacter> CoreCharacter, FTransform& SpawnTransform) PURE_VIRTUAL(ISpawnLocationInterface::GetSpawnTransform, return false;);
	virtual bool HasAvailableSpawnTransform(TSubclassOf<ACoreCharacter> CoreCharacter) const PURE_VIRTUAL(ISpawnLocationInterface::HasAvailableSpawnTransform, return false;);
};

UCLASS()
class NAUSEA_API USpawnLocationSystemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = SpawnLocation)
	static bool GetSpawnLocation(TScriptInterface<ISpawnLocationInterface> Target, TSubclassOf<ACoreCharacter> CoreCharacter, FTransform& SpawnTransform);
};