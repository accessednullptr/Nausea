// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#pragma once

#include "GenericTeamAgentInterface.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PlayerOwnershipInterfaceTypes.generated.h"

struct FGenericTeamId;

UENUM(BlueprintType)
enum class ETeam : uint8
{
	Alpha,
	Bravo,
	Charlie,
	Delta,
	Epsilon,

	IncursionTeam = 254,
	NoTeam = 255
};

UCLASS(Abstract, MinimalAPI)
class UPlayerOwnershipInterfaceTypes : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = PlayerOwnership)
	static const FGenericTeamId& GetGenericTeamIdFromTeamEnum(ETeam Team);
	UFUNCTION(BlueprintCallable, Category = PlayerOwnership)
	static ETeam GetTeamEnumFromGenericTeamId(const FGenericTeamId& TeamId);

	UFUNCTION(BlueprintCallable, Category = PlayerOwnership)
	static const FGenericTeamId& GetAlphaGenericTeamId() { return UPlayerOwnershipInterfaceTypes::Alpha; }
	UFUNCTION(BlueprintCallable, Category = PlayerOwnership)
	static const FGenericTeamId& GetBravoGenericTeamId() { return UPlayerOwnershipInterfaceTypes::Bravo; }
	UFUNCTION(BlueprintCallable, Category = PlayerOwnership)
	static const FGenericTeamId& GetCharlieGenericTeamId() { return UPlayerOwnershipInterfaceTypes::Charlie; }
	UFUNCTION(BlueprintCallable, Category = PlayerOwnership)
	static const FGenericTeamId& GetDeltaGenericTeamId() { return UPlayerOwnershipInterfaceTypes::Delta; }
	UFUNCTION(BlueprintCallable, Category = PlayerOwnership)
	static const FGenericTeamId& GetEpsilonGenericTeamId() { return UPlayerOwnershipInterfaceTypes::Epsilon; }
	
	UFUNCTION(BlueprintCallable, Category = PlayerOwnership)
	static const FGenericTeamId& GetIncursionTeamGenericTeamId() { return UPlayerOwnershipInterfaceTypes::IncursionTeam; }
	
	UFUNCTION(BlueprintCallable, Category = PlayerOwnership)
	static const FGenericTeamId& GetNoTeamGenericTeamId();

public:
	static const FGenericTeamId Alpha;
	static const FGenericTeamId Bravo;
	static const FGenericTeamId Charlie;
	static const FGenericTeamId Delta;
	static const FGenericTeamId Epsilon;

	static const FGenericTeamId IncursionTeam;
};