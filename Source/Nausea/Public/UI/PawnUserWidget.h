// Copyright 2021 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UI/CoreUserWidget.h"
#include "PawnUserWidget.generated.h"

class ACorePlayerController;
class ACoreCharacter;

/**
 * 
 */
UCLASS()
class NAUSEA_API UPawnUserWidget : public UCoreUserWidget
{
	GENERATED_UCLASS_BODY()
	
//~ Begin UUserWidget Interface
public:
	virtual bool Initialize() override;
//~ End UUserWidget Interface

protected:
	UFUNCTION()
	virtual void PossessedPawn(ACorePlayerController* PlayerController, ACoreCharacter* Pawn);
	UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic, Category = Widget, meta = (DisplayName = "On Possessed Pawn", ScriptName = "OnPossessedPawn"))
	void K2_OnPossessedPawn(ACoreCharacter* Pawn);
};
