// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Weapon/FireMode/Ammo.h"
#include "LoadedAmmo.generated.h"

/**
 * 
 */
UCLASS()
class NAUSEA_API ULoadedAmmo : public UAmmo
{
	GENERATED_UCLASS_BODY()

//~ Begin UAmmo Interface
public:
	virtual void Initialize(UFireMode* FireMode) override;
	virtual bool ConsumeAmmo(float Amount) override;
	virtual bool CanConsumeAmmo(float Amount) const override;
	virtual void ApplyAmmoCorrection(float Amount) override;
	virtual bool CanReload() const override;
	virtual bool Reload(float WorldTimeOverride) override;
	virtual bool StopReload(float WorldTimeOverride) override;
	virtual bool IsReloading() const override;
	virtual bool IsReloadNearlyComplete() const override;
	virtual bool CanPutDown() const override;
	virtual bool BlockAction(const UFireMode* InstigatorFireMode = nullptr) const override;
protected:
	virtual void OnReloadCosmetic() override;
	virtual void UpdateAmmoCapacity(bool bFirstInitialization) override;
	virtual void Client_Reliable_ReloadFailed_Implementation(float WorldTimeOverride) override;
//~ End UAmmo Interface

public:
	UFUNCTION(BlueprintCallable, Category = Ammo)
	FORCEINLINE float GetLoadedAmmoAmount() const { return LoadedAmmoAmount; }

	UFUNCTION(BlueprintCallable, Category = Ammo)
	FORCEINLINE float GetMaxLoadedAmmoAmount() const { return MaxLoadedAmmoAmount; }

	UFUNCTION(BlueprintCallable, Category = Ammo)
	FORCEINLINE float GetLoadedAmmoPercent() const { return LoadedAmmoAmount / MaxLoadedAmmoAmount; }

	UFUNCTION(BlueprintCallable, Category = Ammo)
	float GetReloadRate() const;

	UFUNCTION(BlueprintCallable, Category = Ammo)
	float GetReloadAmount() const;

	virtual bool IsReloading() { return false; }
	
	UFUNCTION(BlueprintCallable, Category = Ammo)
	FORCEINLINE bool IsIndividualReload() const { return bIndividualReload; }
	UFUNCTION(BlueprintCallable, Category = Ammo)
	FORCEINLINE float GetIndividualReloadAmount() const { return IndividualReloadAmount; }
	UFUNCTION(BlueprintCallable, Category = Ammo)
	FORCEINLINE bool IsReloadCancelable() const { return bIndividualReload && bIndividualReloadIsCancelable; }
	UFUNCTION(BlueprintCallable, Category = Ammo)
	FORCEINLINE bool ShouldAutoRepeatReload() const { return bIndividualReload && bAutoRepeatInidividualReload; }

public:
	UPROPERTY(BlueprintAssignable)
	FAmmoChangedSignature OnLoadedAmmoChanged;

protected:
	UFUNCTION()
	void OnRep_LoadedAmmoAmount(float PreviousAmount);

	UFUNCTION()
	void OnRep_ReloadCounter();

	virtual void ReloadComplete(float ReloadStartTime) override;

protected:
	UPROPERTY(EditDefaultsOnly, Category = Ammo)
	float MaxLoadedAmmoAmount = 10.f;

	UPROPERTY(Transient, ReplicatedUsing = OnRep_LoadedAmmoAmount)
	float LoadedAmmoAmount = -1.f;

	UPROPERTY(EditDefaultsOnly, Category = Ammo)
	float ReloadRate = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = Ammo)
	bool bIndividualReload = false;
	UPROPERTY(EditDefaultsOnly, Category = Ammo, meta = (EditCondition = "bIndividualReload", EditConditionHides))
	float IndividualReloadAmount = 1.f;
	UPROPERTY(EditDefaultsOnly, Category = Ammo, meta = (EditCondition = "bIndividualReload", EditConditionHides))
	bool bAutoRepeatInidividualReload = true;
	UPROPERTY(EditDefaultsOnly, Category = Ammo, meta = (EditCondition = "bIndividualReload", EditConditionHides))
	bool bIndividualReloadIsCancelable = false;

	UPROPERTY(EditDefaultsOnly, Category = AI, meta = (PinHiddenByDefault, InlineEditConditionToggle))
	bool bMakeNoiseOnReload = false;
	UPROPERTY(EditDefaultsOnly, Category = AI, meta = (EditCondition = "bMakeNoiseOnReload", DisplayName = "Make Noise On Reload"))
	FCoreNoiseParams ReloadNoise = FCoreNoiseParams(CoreNoiseTag::WeaponReload, 0.3f, 0.f);

	UPROPERTY()
	FTimerHandle ReloadTimer;
	
	UPROPERTY(Transient, ReplicatedUsing = OnRep_ReloadCounter)
	int32 ReloadCounter = 0;
};