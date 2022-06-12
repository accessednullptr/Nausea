// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "GameFramework/Actor.h"
#include "Overlord/PlacementTypes.h"
#include "TrapBase.generated.h"

class UTexture2D;
class ADungeonPlayerController;
class APlacementActor;

UCLASS()
class NAUSEA_API ATrapBase : public AActor
{
	GENERATED_UCLASS_BODY()

//~ Begin AActor Interface
public:
	virtual void PostInitializeComponents() override;
//~ End AActor Interface

public:
	UFUNCTION(BlueprintCallable, Category = Trap)
	int32 GetCost() const { return Cost; }
	UFUNCTION(BlueprintCallable, Category = Trap)
	int32 GetRefundCost() const { return RefundCost; }

	UFUNCTION(BlueprintCallable, Category = Trap)
	int32 GetSizeX() const { return SizeX; }
	UFUNCTION(BlueprintCallable, Category = Trap)
	int32 GetSizeY() const { return SizeY; }
	UFUNCTION(BlueprintCallable, Category = Trap)
	TSoftClassPtr<ATrapPreview> GetPlacementPreviewActor() const { return PlacementPreviewActor; }

	UFUNCTION(BlueprintCallable, Category = Trap)
	TSoftObjectPtr<UTexture2D> GetTrapIcon() const { return TrapIcon; }

	uint8 GetPlacementType() const { return PlacementType; }

	UFUNCTION(BlueprintCallable, Category = Trap, meta = (DisplayName = "Get Placement Type", ScriptName = "GetPlacementType"))
	void K2_GetPlacementType(UPARAM(meta = (Bitmask, BitmaskEnum = EPlacementType)) int32& Bitmask) const { Bitmask = PlacementType; }

	UFUNCTION(BlueprintCallable, Category = Placement)
	bool CanBeFloorTrap() const { return (PlacementType & static_cast<uint8>(EPlacementType::Floor)) != 0; }
	UFUNCTION(BlueprintCallable, Category = Placement)
	bool CanBeWallTrap() const { return (PlacementType & static_cast<uint8>(EPlacementType::Wall)) != 0; }
	UFUNCTION(BlueprintCallable, Category = Placement)
	bool CanBeCeilingTrap() const { return (PlacementType & static_cast<uint8>(EPlacementType::Ceiling)) != 0; }

	UFUNCTION(BlueprintCallable, Category = Trap)
	EPlacementResult CanPlaceTrapOnTarget(ADungeonPlayerController* PlacementInstigator, APlacementActor* TargetPlacementActor) const;

	void SetOccupancy(APlacementActor* TargetPlacementActor, const FPlacementCoordinates& BottomLeftCorner, const FPlacementCoordinates& TopRightCorner);
	void RevokeOccupancy();

protected:
	UFUNCTION(BlueprintCallable, Category = Trap, meta = (AutoCreateRefTerm = "ActorsToIgnore"))
	TArray<AActor*> PerformOverlapTestWithPrimitive(UPrimitiveComponent* Component, TSubclassOf<AActor> ActorClassFilter, TArray<AActor*> ActorsToIgnore);
	UFUNCTION(BlueprintCallable, Category = Trap, meta = (AutoCreateRefTerm = "ActorsToIgnore"))
	TArray<AActor*> PerformOverlapTestWithPrimitiveAndApplyDamage(UPrimitiveComponent* Component, TSubclassOf<AActor> ActorClassFilter, TArray<AActor*> ActorsToIgnore);

protected:
	UPROPERTY(EditDefaultsOnly, Category = Trap)
	int32 Cost = 100;
	UPROPERTY(EditDefaultsOnly, Category = Trap)
	int32 RefundCost = 50;

	UPROPERTY(EditDefaultsOnly, Category = Placement)
	int32 SizeX = 4;
	UPROPERTY(EditDefaultsOnly, Category = Placement)
	int32 SizeY = 4;
	UPROPERTY(EditAnywhere, Category = Placement, meta = (Bitmask, BitmaskEnum = EPlacementType))
	uint8 PlacementType = 0;
	UPROPERTY(EditDefaultsOnly, Category = Placement)
	TSoftClassPtr<ATrapPreview> PlacementPreviewActor = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = UI)
	TSoftObjectPtr<UTexture2D> TrapIcon = nullptr;


	UPROPERTY(Transient)
	TWeakObjectPtr<APlacementActor> OccupiedPlacementActor = nullptr;
	UPROPERTY(Transient)
	TArray<FPlacementHandle> OccupiedHandleList;

public:
	UFUNCTION(BlueprintCallable, Category = Trap)
	static TSoftObjectPtr<UTexture2D> GetTrapIconForClass(TSubclassOf<ATrapBase> TrapClass);
	UFUNCTION(BlueprintCallable, Category = Trap)
	static int32 GetCostForClass(TSubclassOf<ATrapBase> TrapClass);
	UFUNCTION(BlueprintCallable, Category = Trap)
	static int32 GetRefundCostIconForClass(TSubclassOf<ATrapBase> TrapClass);
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPlacementResultUpdateSignature, ATrapPreview*, PreviewActor, EPlacementResult, Result);

UCLASS()
class NAUSEA_API ATrapPreview : public AActor
{
	GENERATED_UCLASS_BODY()

//~ Begin AActor Interface
public:
	virtual void PostInitializeComponents() override;
//~ End AActor Interface

public:
	UFUNCTION()
	void SetPlacementFlags(uint8 InPlacementFlags);

	UFUNCTION()
	void UpdatePlacementState(EPlacementResult PlacementResult, const FTransform& NewPlacementTransform);

	UFUNCTION(BlueprintCallable, Category = TrapPreview)
	const FText& GetPlacementText() const;
	UFUNCTION(BlueprintCallable, Category = TrapPreview)
	const FLinearColor& GetPlacementTextColor() const;

	UFUNCTION(BlueprintCallable, Category = Placement)
	bool CanBeFloorTrap() const { return (CurrentPlacementFlags & static_cast<uint8>(EPlacementType::Floor)) != 0; }
	UFUNCTION(BlueprintCallable, Category = Placement)
	bool CanBeWallTrap() const { return (CurrentPlacementFlags & static_cast<uint8>(EPlacementType::Wall)) != 0; }
	UFUNCTION(BlueprintCallable, Category = Placement)
	bool CanBeCeilingTrap() const { return (CurrentPlacementFlags & static_cast<uint8>(EPlacementType::Ceiling)) != 0; }

public:
	UPROPERTY(BlueprintAssignable, Category = TrapPreview)
	FPlacementResultUpdateSignature OnPlacementResultUpdate;

protected:
	UPROPERTY(EditDefaultsOnly, Category = TrapPreview)
	TSoftObjectPtr<UMaterialInterface> ValidPlacementMaterial = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = TrapPreview)
	FText ValidPlacementText;
	UPROPERTY(EditDefaultsOnly, Category = TrapPreview)
	FLinearColor ValidPlacementColor;

	UPROPERTY(EditDefaultsOnly, Category = TrapPreview)
	TSoftObjectPtr<UMaterialInterface> InvalidPlacementTypeMaterial = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = TrapPreview)
	FText InvalidPlacementTypeText;
	UPROPERTY(EditDefaultsOnly, Category = TrapPreview)
	FLinearColor InvalidPlacementTypeColor;

	UPROPERTY(EditDefaultsOnly, Category = TrapPreview)
	TSoftObjectPtr<UMaterialInterface> NotEnoughFundsPlacementMaterial = nullptr;
	UPROPERTY(EditDefaultsOnly, Category = TrapPreview)
	FText NotEnoughFundsPlacementText;
	UPROPERTY(EditDefaultsOnly, Category = TrapPreview)
	FLinearColor NotEnoughFundsPlacementColor;

	UPROPERTY(Transient)
	uint8 CurrentPlacementFlags = 0;

private:
	UPROPERTY(EditDefaultsOnly, Category = UI)
	class UCoreWidgetComponent* ErrorWidgetComponent = nullptr;

	UPROPERTY(Transient)
	EPlacementResult LastKnownPlacementResult = EPlacementResult::MAX;
};
