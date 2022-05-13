// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "WeaponAnimInstance.generated.h"

class UWeapon;
class UMeleeFireMode;


USTRUCT()
struct FMeleeNotifyDataHandle
{
	GENERATED_USTRUCT_BODY()

public:
	FMeleeNotifyDataHandle()
	{
		Handle = MAX_uint64;
	}

	FORCEINLINE bool operator== (const FMeleeNotifyDataHandle& InData) const { return Handle == InData.Handle; }
	FORCEINLINE bool operator!= (const FMeleeNotifyDataHandle& InData) const { return Handle != InData.Handle; }
	FORCEINLINE bool operator== (uint64 InID) const { return this->Handle == InID; }
	FORCEINLINE bool operator!= (uint64 InID) const { return this->Handle != InID; }

	bool IsValid() const { return Handle != MAX_uint64; }

	static FMeleeNotifyDataHandle GenerateHandle()
	{
		if (++FMeleeNotifyDataHandle::HandleIDCounter == MAX_uint64)
		{
			FMeleeNotifyDataHandle::HandleIDCounter++;
		}

		return FMeleeNotifyDataHandle(FMeleeNotifyDataHandle::HandleIDCounter);
	}

	FORCEINLINE friend uint32 GetTypeHash(FMeleeNotifyDataHandle Other)
	{
		return GetTypeHash(Other.Handle);
	}

	friend FArchive& operator<<(FArchive& Ar, FMeleeNotifyDataHandle& MeleeNotifyDataHandle)
	{
		Ar << MeleeNotifyDataHandle.Handle;
		return Ar;
	}

protected:
	FMeleeNotifyDataHandle(uint64 InHandle) { Handle = InHandle; }

	UPROPERTY()
	uint64 Handle = MAX_uint64;

	static uint64 HandleIDCounter;
};

USTRUCT(BlueprintType)
struct FMeleeNotifyData
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY()
	FMeleeNotifyDataHandle Handle;
};

/**
 * 
 */
UCLASS()
class NAUSEA_API UWeaponAnimInstance : public UAnimInstance
{
	GENERATED_UCLASS_BODY()
	
public:
	UFUNCTION()
	void RegisterOwningWeapon(UWeapon* Weapon);

	void ProcessMeleeNotifyHitBox(const FMeleeNotifyData& Data);

	UFUNCTION()
	void OnFireStart(UMeleeFireMode* FireMode);
	UFUNCTION()
	void OnFireStop(UMeleeFireMode* FireMode);

	//Weapons should dynamically bind and unbind to this delegate when firing.
	DECLARE_EVENT_TwoParams(UWeaponAnimInstance, FMeleeHitboxEventSignature, UWeaponAnimInstance*, const FMeleeNotifyData&)
	FMeleeHitboxEventSignature OnProcessMeleeHitboxEvent;

protected:
	UPROPERTY(Transient)
	TWeakObjectPtr<UWeapon> OwningWeapon = nullptr;

	FDelegateHandle FireModeBroadcastHandle = FDelegateHandle();
};
