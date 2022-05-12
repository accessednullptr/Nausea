// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/FireMode/WeaponFireMode.h"
#include "TraceFireMode.generated.h"

class AActor;
class UPrimitiveComponent;
class UPhysicalMaterial;

USTRUCT()
struct NAUSEA_API FTraceHitResult
{
	GENERATED_USTRUCT_BODY()

	FTraceHitResult() {}

	FTraceHitResult(const FHitResult& HitResult);

	UPROPERTY()
	float Distance = 0.f; 
	
	UPROPERTY()
	FVector ImpactLocation = FVector(MAX_FLT);

	UPROPERTY()
	FVector_NetQuantizeNormal ImpactNormal = FVector(MAX_FLT);

	UPROPERTY()
	FVector_NetQuantize TraceStart = FVector(MAX_FLT);

	UPROPERTY()
	FVector_NetQuantize TraceEnd = FVector(MAX_FLT);

	UPROPERTY()
	TWeakObjectPtr<AActor> Actor = nullptr;

	UPROPERTY()
	TWeakObjectPtr<UPrimitiveComponent> Component = nullptr;

	UPROPERTY()
	int32 Item = 0;

	UPROPERTY()
	uint8 ElementIndex = 0;

	UPROPERTY()
	FName BoneName = NAME_None;

	UPROPERTY()
	TWeakObjectPtr<UPhysicalMaterial> PhysMaterial = nullptr;

	operator FHitResult() const;

	struct FSortByDistance
	{
		FSortByDistance() {}

		bool operator()(const FTraceHitResult& A, const FTraceHitResult& B) const
		{
			return A.Distance < B.Distance;
		}
	};
};

/**
 * 
 */
UCLASS()
class NAUSEA_API UTraceFireMode : public UWeaponFireMode
{
	GENERATED_UCLASS_BODY()
	
//~ Begin UFireMode Interface 
public:
	virtual bool Fire(float WorldTimeOverride = -1.f) override;
protected:
	virtual void PerformFire() override;
//~ End UFireMode Interface 

public:
	UFUNCTION(BlueprintCallable, Category = Trace)
	float GetTraceLength() const { return TraceLength; }
	UFUNCTION(BlueprintCallable, Category = Trace)
	uint8 GetMaxPenetrationCount() const { return MaxPenetrationCount; }

	UFUNCTION()
	float CalculateDamage(const FTraceHitResult& TraceHit, int32 PenetrationCount) const;

protected:
	//Payload containing trace data.
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Reliable_FireTrace(const FVector& Location, const FVector& Direction, const TArray<FTraceHitResult>& HitResultList);

	UFUNCTION()
	void ReceiveTraceData(const FVector& Location, const FVector& Direction, const TArray<FTraceHitResult>& HitResultList);

protected:
	UPROPERTY(EditDefaultsOnly, Category = Trace)
	TSubclassOf<class UCoreDamageType> DamageTypeClass = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = Trace)
	float TraceLength = 10000.f;
	UPROPERTY(EditDefaultsOnly, Category = Trace)
	class UCurveFloat* DistanceDamageFalloffCurve = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = Trace)
	uint8 MaxPenetrationCount = 0;
	UPROPERTY(EditDefaultsOnly, Category = Trace)
	class UCurveFloat* PenetrationDamageFalloffCurve = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = AI, meta = (PinHiddenByDefault, InlineEditConditionToggle))
	bool bMakeNoiseOnFire = true;
	UPROPERTY(EditDefaultsOnly, Category = AI, meta = (EditCondition = "bMakeNoiseOnFire", DisplayName = "Make Noise On Fire"))
	FCoreNoiseParams FireNoise = FCoreNoiseParams(CoreNoiseTag::WeaponFire, 2.f, 0.f);

	UPROPERTY(Transient)
	FVector LastReceivedLocation = FVector(MAX_FLT);
	UPROPERTY(Transient)
	FVector LastReceivedDirection = FVector(MAX_FLT);
	UPROPERTY(Transient)
	TArray<FTraceHitResult> LastReceivedHitList = TArray<FTraceHitResult>();
};