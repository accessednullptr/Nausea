// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "AITypes.h"
#include "GameFramework/Actor.h"
#include "Overlord/TrapTypes.h"
#include "Overlord/PlacementTypes.h"
#include "PlacementActor.generated.h"

class UArrowComponent;
class UBoxComponent;
class UStaticMeshComponent;

//Preview mesh in editor.
UCLASS(NotPlaceable, NotBlueprintType, Blueprintable)
class NAUSEA_API UPlacementMarkerComponent : public UStaticMeshComponent
{
	GENERATED_UCLASS_BODY()

public:
	void SetPlacementFlags(uint8 PlacementFlags);

protected:
	UPROPERTY(EditDefaultsOnly, Category = Placement)
	UMaterialInterface* InvalidMaterial = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = Placement)
	UMaterialInterface* WallGridMaterial = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = Placement)
	UMaterialInterface* FloorGridMaterial = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = Placement)
	UMaterialInterface* CeilingGridMaterial = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = Placement)
	UMaterialInterface* WallAndFloorGridMaterial = nullptr;
};

UCLASS(BlueprintType, Blueprintable)
class NAUSEA_API APlacementActor : public AActor
{
	GENERATED_UCLASS_BODY()

//~ Begin AActor Interface
public:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif //WITH_EDITOR
//~ End AActor Interface

public:
	void InitializePlacementData();

	const TArray<UPlacementMarkerComponent*>& GetPlacementComponentList() const { return PlacementMarkerList; }
	const FPlacementGrid& GetPlacementGrid() const { return PlacementGrid; }
	FPlacementGrid& GetPlacementGrid() { return PlacementGrid; }
	const FPlacementGrid& GetParentmostPlacementGrid() const;
	FPlacementGrid& GetParentmostPlacementGrid();

	APlacementActor* GetParentPlacementActor() const { return ParentPlacementActor.Get(); }
	APlacementActor* GetParentmostPlacementActor() const;
	bool HasParentPlacementActor() const { return ParentPlacementActor.Get() != nullptr; }

	void PushPlacementData(const FPlacementGrid& GridData);

	uint8 GetPlacementType() const { return PlacementType; }

	UFUNCTION(BlueprintCallable, Category = Placement, meta = (DisplayName = "Get Placement Type", ScriptName = "GetPlacementType"))
	void K2_GetPlacementType(UPARAM(meta = (Bitmask, BitmaskEnum = EPlacementType)) int32& Bitmask) const { Bitmask = PlacementType; }

	UFUNCTION(BlueprintCallable, Category = Placement)
	bool SupportsFloorTraps() const { return (PlacementType & static_cast<uint8>(EPlacementType::Floor)) != 0; }
	UFUNCTION(BlueprintCallable, Category = Placement)
	bool SupportsWallTraps() const { return (PlacementType & static_cast<uint8>(EPlacementType::Wall)) != 0; }
	UFUNCTION(BlueprintCallable, Category = Placement)
	bool SupportsCeilingTraps() const { return (PlacementType & static_cast<uint8>(EPlacementType::Ceiling)) != 0; }

#if WITH_EDITOR
	void OnPlacementPreviewUpdate();
#endif //WITH_EDITOR

	UFUNCTION(CallInEditor, Category = Generation)
	virtual void GeneratePlacementGrid();
	
	UFUNCTION(CallInEditor, Category = Generation)
	void ValidatePlacementSetup();

protected:
	UPROPERTY()
	TArray<UPlacementMarkerComponent*> PlacementMarkerList = TArray<UPlacementMarkerComponent*>();

	UPROPERTY()
	UBoxComponent* BoxComponent = nullptr;
	UPROPERTY()
	USceneComponent* RootSceneComponent = nullptr;
	UPROPERTY()
	UArrowComponent* ArrowComponent = nullptr;

	UPROPERTY()
	FPlacementGrid PlacementGrid = FPlacementGrid();

	UPROPERTY(EditInstanceOnly, Category = Generation)
	TSoftObjectPtr<APlacementActor> ParentPlacementActor = nullptr;


protected:
	UPROPERTY(EditDefaultsOnly, Category = Generation)
	TSubclassOf<UPlacementMarkerComponent> PlacementMarkerClass = nullptr;

	UPROPERTY(EditAnywhere, Category = Generation)
	uint8 Rows = 1;
	UPROPERTY(EditAnywhere, Category = Generation)
	uint8 Columns = 1;
	UPROPERTY(EditAnywhere, Category = Generation, meta = (Bitmask, BitmaskEnum = EPlacementType))
	uint8 PlacementType = 0;
};
