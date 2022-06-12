// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Player/PlayerOwnershipInterfaceTypes.h"
#include "PlayerOwnershipInterface.generated.h"

class ACorePlayerState;

UINTERFACE()
class NAUSEA_API UPlayerOwnershipInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class NAUSEA_API IPlayerOwnershipInterface
{
	GENERATED_IINTERFACE_BODY()

public:
	virtual ACorePlayerState* GetOwningPlayerState() const PURE_VIRTUAL(IPlayerOwnershipInterface::GetOwningPlayerState, return nullptr;);

	virtual AController* GetOwningController() const;

	virtual APawn* GetOwningPawn() const;

	virtual FGenericTeamId GetOwningTeamId() const;

	ETeam GetOwningTeam() const;

	virtual class UPlayerClassComponent* GetPlayerClassComponent() const;
	virtual class UPlayerStatisticsComponent* GetPlayerStatisticsComponent() const;
	virtual class UVoiceComponent* GetVoiceComponent() const;
	virtual class UVoiceCommandComponent* GetVoiceCommandComponent() const;
	virtual class UAbilityComponent* GetAbilityComponent() const;

	template <class T>
	T* GetOwningPlayerState() const
	{
		return Cast<T>(GetOwningPlayerState());
	}

	template <class T>
	T* GetOwningController() const
	{
		return Cast<T>(GetOwningController());
	}

	template <class T>
	T* GetOwningPawn() const
	{
		return Cast<T>(GetOwningPawn());
	}

	bool IsLocallyOwned() const;
};

UCLASS()
class NAUSEA_API UPlayerOwnershipSystemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = PlayerOwnership)
	static ACorePlayerState* GetActorPlayerState(TScriptInterface<IPlayerOwnershipInterface> Target);

	UFUNCTION(BlueprintCallable, Category = PlayerOwnership)
	static bool IsLocallyOwnedActor(TScriptInterface<IPlayerOwnershipInterface> Target);

	UFUNCTION(BlueprintCallable, Category = PlayerOwnership)
	static ETeam GetActorTeam(TScriptInterface<IPlayerOwnershipInterface> Target);

	UFUNCTION(BlueprintCallable, Category = PlayerOwnership)
	static UPlayerClassComponent* GetActorPlayerClass(TScriptInterface<IPlayerOwnershipInterface> Target);

	UFUNCTION(BlueprintCallable, Category = PlayerOwnership)
	static UPlayerStatisticsComponent* GetActorPlayerStatistics(TScriptInterface<IPlayerOwnershipInterface> Target);

	UFUNCTION(BlueprintCallable, Category = PlayerOwnership)
	static UVoiceCommandComponent* GetActorVoiceCommandComponent(TScriptInterface<IPlayerOwnershipInterface> Target);
};