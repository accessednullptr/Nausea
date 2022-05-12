// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Player/PlayerClass/PlayerClassObject.h"
#include "Player/PlayerStatistics/PlayerStatisticsTypes.h"
#include "PlayerClassExperienceSource.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FExperienceSourceGrantSignature, UPlayerClassExperienceSource*, ExperienceSource, int32, ExperienceAmount);

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, AutoExpandCategories = (Default))
class NAUSEA_API UPlayerClassExperienceSource : public UPlayerClassObject
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = ExperienceSource)
	FExperienceSourceGrantSignature OnGrantedExperience;

protected:
	UFUNCTION()
	void GrantExperience(uint64 Amount);
};

UCLASS()
class NAUSEA_API UPlayerClassSimpleExperienceSource : public UPlayerClassExperienceSource
{
	GENERATED_UCLASS_BODY()

public:
	virtual void Initialize(UPlayerClassComponent* OwningComponent) override;

protected:
	UFUNCTION()
	void OnStatIncreased(UPlayerStatisticsComponent* PlayerStatistics, EPlayerStatisticType StatisticType, uint64 Delta);

protected:
	UPROPERTY(EditDefaultsOnly, Category = ExperienceSource)
	EPlayerStatisticType StatSource = EPlayerStatisticType::Invalid;
	
	UPROPERTY(EditDefaultsOnly, Category = ExperienceSource)
	uint64 StatRequiredPerGrant = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = ExperienceSource)
	uint32 ExperienceToGrant = 100;

	UPROPERTY()
	uint32 CurrentStatProgress = 0;
};
