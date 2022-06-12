// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Player/CorePlayerController.h"
#include "DungeonPlayerController.generated.h"

class ATrapBase;
class ATrapPreview;
class APlacementActor;
class ACoreGameState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSelectedTrapClassUpdateSignature, ADungeonPlayerController*, PlayerController, TSubclassOf<ATrapBase>, TrapClass);

UCLASS()
class NAUSEA_API ADungeonPlayerController : public ACorePlayerController
{
	GENERATED_UCLASS_BODY()
	
//~ Begin AActor Interface
protected:
	virtual void BeginPlay() override;
//~ End AActor Interface

//~ Begin APlayerController Interface
public:
	virtual void PlayerTick(float DeltaTime) override;
	virtual void SetupInputComponent() override;
//~ End APlayerController Interface

public:
	UFUNCTION(BlueprintCallable, Category = Placement)
	void SetPlacementTrapClass(TSubclassOf<ATrapBase> TrapClass);
	UFUNCTION(BlueprintCallable, Category = Placement)
	void ClearPlacementTrapClass();

	UFUNCTION(BlueprintCallable, Category = Placement)
	TSubclassOf<ATrapBase> GetPlacementTrapClass() const { return PlacementTrapClass; }

	const ATrapBase* GetPlacementTrapCDO() const;

	void SetPlacementEnabled(bool bEnabled);

protected:
	UFUNCTION()
	void OnMatchStateChanged(ACoreGameState* GameState, FName MatchState);

	bool UpdatePlacement(const FHitResult& HitResult, FTransform& PlacementTransform);
	void UpdatePreviewActor(bool bActive, const FTransform& PlacementTransform, APlacementActor* PlacementActor);

	void OnBuildPressed();
	void OnCancelBuildPressed();

	void OnRotatePlacement();
	void OnUnrotatePlacement();

	uint8 GetRotationIndex() const { return Rotation; }

public:
	UPROPERTY(BlueprintAssignable, Category = Placement)
	FSelectedTrapClassUpdateSignature OnSelectedTrapClassUpdate;

protected:
	UPROPERTY()
	uint8 Rotation = 0;

	UPROPERTY(Transient)
	TSubclassOf<ATrapBase> PlacementTrapClass = nullptr;
	UPROPERTY(Transient)
	TSubclassOf<ATrapPreview> PlacementTrapPreviewClass = nullptr;
	UPROPERTY(Transient)
	ATrapPreview* TrapPreviewActor = nullptr;
	UPROPERTY(Transient)
	bool bPlacementEnabled = false;

public:
	UFUNCTION(BlueprintCallable, Category = Input)
	static bool IsMouseEventMatchingActionEvent(const FPointerEvent& MouseEvent, FName InActionName);
};