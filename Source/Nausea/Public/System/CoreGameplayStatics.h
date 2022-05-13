// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CoreGameplayStatics.generated.h"

class UWeapon;
class UFireMode;

/**
 * 
 */
UCLASS()
class NAUSEA_API UCoreGameplayStatics : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = GameplayStatics)
	static FString GetNetRoleNameForActor(AActor* Actor);

	UFUNCTION(BlueprintCallable, Category = GameplayStatics)
	static FString GetNetRoleName(ENetRole Role);

	UFUNCTION(BlueprintCallable, Category = GameplayStatics)
	static FString GetProjectVersion();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Game|Damage", meta=(WorldContext="WorldContextObject", AutoCreateRefTerm="IgnoreActors"))
	static bool ApplyWeaponRadialDamage(const UObject* WorldContextObject, float BaseDamage, TSubclassOf<UWeapon> WeaponClass, TSubclassOf<UFireMode> FireModeClass, const FVector& Origin, float DamageRadius, TSubclassOf<class UDamageType> DamageTypeClass, const TArray<AActor*>& IgnoreActors, AActor* DamageCauser = NULL, AController* InstigatedByController = NULL, bool bDoFullDamage = false, ECollisionChannel DamagePreventionChannel = ECC_Visibility);
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Game|Damage", meta=(WorldContext="WorldContextObject", AutoCreateRefTerm="IgnoreActors"))
	static bool ApplyWeaponRadialDamageWithFalloff(const UObject* WorldContextObject, float BaseDamage, TSubclassOf<UWeapon> WeaponClass, TSubclassOf<UFireMode> FireModeClass, float MinimumDamage, const FVector& Origin, float DamageInnerRadius, float DamageOuterRadius, float DamageFalloff, TSubclassOf<class UDamageType> DamageTypeClass, const TArray<AActor*>& IgnoreActors, AActor* DamageCauser = NULL, AController* InstigatedByController = NULL, ECollisionChannel DamagePreventionChannel = ECC_Visibility);
	
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Game|Damage")
	static float ApplyWeaponPointDamage(AActor* DamagedActor, float BaseDamage, TSubclassOf<UWeapon> WeaponClass, TSubclassOf<UFireMode> FireModeClass, const FVector& HitFromDirection, const FHitResult& HitInfo, AController* EventInstigator, AActor* DamageCauser, TSubclassOf<class UDamageType> DamageTypeClass);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Game|Damage")
	static float ApplyWeaponDamage(AActor* DamagedActor, float BaseDamage, TSubclassOf<UWeapon> WeaponClass, TSubclassOf<UFireMode> FireModeClass, AController* EventInstigator, AActor* DamageCauser, TSubclassOf<class UDamageType> DamageTypeClass);
};