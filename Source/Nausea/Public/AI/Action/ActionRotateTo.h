// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AI/ActionBrainComponentAction.h"
#include "ActionRotateTo.generated.h"

/**
 * 
 */
UCLASS()
class NAUSEA_API UActionRotateTo : public UActionBrainComponentAction
{
	GENERATED_UCLASS_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category = "Action Move To", meta = (WorldContext = "WorldContextObject", CallableWithoutWorldContext))
	static UActionRotateTo* CreateRotateToAction(const UObject* WorldContextObject, UObject* DesiredViewTarget, FVector DesiredViewLocation, float RotationRateModifier = 1.f, bool bUseLocationAsDirection = false);

//~ Begin UActionBrainComponentAction Interface
protected:
	virtual bool Start() override;
	virtual bool Pause(const UActionBrainComponentAction* PausedBy) override;
	virtual bool Resume() override;
	virtual EPawnActionAbortState::Type PerformAbort(EAIForceParam::Type ShouldForce) override;
	virtual void Finish(TEnumAsByte<EPawnActionResult::Type> WithResult) override;
	virtual FString GetDebugInfoString(int32 Depth) const;
//~ End UActionBrainComponentAction Interface

protected:
	UFUNCTION(BlueprintCallable, Category = RotateTo)
	void SetViewTarget(UObject* InViewTarget, FVector InViewTargetLocation, bool bUseLocationAsDirection = false);
	UFUNCTION(BlueprintCallable, Category = RotateTo)
	void SetRotationRate(float InRotationRate);

	void UpdateFocus();

	void ClearFocus();

protected:
	UPROPERTY()
	TWeakObjectPtr<UObject> ViewTarget = nullptr;
	
	UPROPERTY()
	FVector ViewTargetLocation = FAISystem::InvalidLocation;
	
	UPROPERTY(EditDefaultsOnly, Category = RotateTo)
	float RotationRate = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = RotateTo)
	bool bLocationAsDirection = false;
};
