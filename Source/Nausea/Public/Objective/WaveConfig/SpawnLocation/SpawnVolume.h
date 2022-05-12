// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Volume.h"
#include "Objective/WaveConfig/WaveSpawnLocationInterface.h"
#include "DebugRenderSceneProxy.h"
#include "Components/PrimitiveComponent.h"
#include "SpawnVolume.generated.h"

USTRUCT(BlueprintType)
struct FSpawnLocationData
{
	GENERATED_USTRUCT_BODY()

	FSpawnLocationData() {}

	FSpawnLocationData(const FVector& InLocation, const FVector2D& InExtent)
	{
		Location = InLocation;
		CylinderExtent = InExtent;
	}

	FSpawnLocationData GetTransformedCopy(const FTransform& Transform) const
	{
		return FSpawnLocationData(Transform.GetLocation() + Location, CylinderExtent);
	}

public:
	UPROPERTY()
	FVector Location = FVector(-MAX_FLT);
	UPROPERTY()
	FVector2D CylinderExtent = FVector2D(0.f);
};

/**
 * 
 */
UCLASS()
class NAUSEA_API ASpawnVolume : public AVolume, public IWaveSpawnLocationInterface
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITOR
//~ Begin UObject Interface
public:
	virtual void PostEditImport() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
//~ End UObject Interface

//~ Begin AActor Interface
public:
	virtual void PostEditMove(bool bFinished) override;
	virtual void OnConstruction(const FTransform& Transform) override;
//~ End AActor Interface
#endif //WITH_EDITOR

//~ Begin IWaveSpawnLocationInterface Interface
public:
	virtual FTransform GetSpawnLocation(TSubclassOf<ACoreCharacter> SpawnClass, int32& RequestID) const override;
	virtual void ProcessSpawn(ACoreCharacter* SpawnedCharacter, int32& RequestID) override;
	virtual float GetSpawnScore() const override;
//~ End IWaveSpawnLocationInterface Interface

protected:
	//If larger than 0, will limit the amount of spawn locations this volume will generate.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = SpawnVolume)
	int32 MaxSpawnLocations = -1;

	//Radius of a given spawn location.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = SpawnVolume)
	float SpawnLocationRadius = 48.f;

	//Height of a given spawn location.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = SpawnVolume)
	float SpawnLocationHeight = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = SpawnVolume)
	float PercentSpacing = 0.1f;

	UPROPERTY(Transient)
	TArray<FSpawnLocationData> SpawnLocationList;
	UPROPERTY()
	TArray<FSpawnLocationData> WorldSpawnLocationList;

#if WITH_EDITOR
protected:
	UFUNCTION(CallInEditor, Category = SpawnVolume)
	void UpdateDebugCylinderList();
private:
	void OnEditorSelectionChanged(UObject* NewSelection);
private:
	FDelegateHandle OnEditorSelectionChangedHandle;

	USpawnVolumeRenderingComponent* SpawnLocationRenderComponent = nullptr;
#endif //WITH_EDITOR
};


//==================================
//EDITOR DRAWING

#if WITH_EDITOR
class NAUSEA_API FSpawnVolumeSceneProxy : public FDebugRenderSceneProxy
{
	friend class FSpawnVolumeRenderingDebugDrawDelegateHelper;

public:
	explicit FSpawnVolumeSceneProxy(const UPrimitiveComponent* InComponent, const TArray<FDebugRenderSceneProxy::FWireCylinder>& CylinderList = TArray<FDebugRenderSceneProxy::FWireCylinder>());

	//~ Begin FDebugRenderSceneProxy Interface
public:
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;
	//~ End FDebugRenderSceneProxy Interface

public:
	void SetCylinderData(const TArray<FWireCylinder>& CylinderList);
	bool SafeIsActorSelected() const;

private:
	AActor* ActorOwner = nullptr;
};

class FSpawnVolumeRenderingDebugDrawDelegateHelper : public FDebugDrawDelegateHelper
{
public:
	FSpawnVolumeRenderingDebugDrawDelegateHelper() {}

//~ Begin FDebugDrawDelegateHelper Interface
public:
	virtual void InitDelegateHelper(const FDebugRenderSceneProxy* InSceneProxy) override { check(0); } //Don't use this one.
//~ End FDebugDrawDelegateHelper Interface

public:
	void InitDelegateHelper(const FSpawnVolumeSceneProxy* InSceneProxy) { FDebugDrawDelegateHelper::InitDelegateHelper(InSceneProxy); ActorOwner = InSceneProxy->ActorOwner; }

private:
	AActor* ActorOwner = nullptr;
};
#endif

UCLASS(hidecategories = Object)
class NAUSEA_API USpawnVolumeRenderingComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITOR
//~ Begin UPrimitiveComponent Interface
public:
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
//~ End UPrimitiveComponent Interface

//~ Begin UActorComponent Interface
public:
	virtual void CreateRenderState_Concurrent(FRegisterComponentContext* Context) override;
	virtual void DestroyRenderState_Concurrent() override;
//~ End UActorComponent Interface

	void SetSpawnLocations(const TArray<FSpawnLocationData>& InSpawnLocationList);

private:
	TArray<FSpawnLocationData> CachedSpawnLocationList;
	TArray<FDebugRenderSceneProxy::FWireCylinder> DebugSpawnLocationList;

	FSpawnVolumeSceneProxy* SpawnVolumeSceneProxy;

	FSpawnVolumeRenderingDebugDrawDelegateHelper DebugDrawDelegateHelper;
#endif
};