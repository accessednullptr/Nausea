// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Gameplay/StatusType.h"
#include "DamageLogInterface.generated.h"

UINTERFACE(Blueprintable)
class UDamageLogInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

/**
 * 
 */
class  IDamageLogInterface
{
	GENERATED_IINTERFACE_BODY()

public:
	virtual FText GetDamageLogInstigatorName() const { return IDamageLogInterface::Execute_K2_GetDamageLogInstigatorName(Cast<UObject>(this)); }

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = DamageLogInterface, meta = (DisplayName="Get Damage Log Instigator Name"))
	FText K2_GetDamageLogInstigatorName() const;
};

UCLASS(Abstract, MinimalAPI)
class UDamageLogStatics : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = DamageLogInterface)
	static bool ImplementsDamageLogInterface(TSubclassOf<UObject> ObjectClass);

	UFUNCTION(BlueprintCallable, Category = DamageLogInterface)
	static TScriptInterface<IDamageLogInterface> GetDamageLogModifierInterface(TSubclassOf<UObject> ObjectClass);
	UFUNCTION(BlueprintCallable, Category = DamageLogInterface)
	static FText GetDamageLogInstigatorName(TScriptInterface<IDamageLogInterface> Target);

	UFUNCTION(BlueprintCallable, Category = DamageLogInterface)
	static FText GetDamageLogEventText(FDamageLogEvent DamageLogEvent);
	UFUNCTION(BlueprintCallable, Category = DamageLogInterface)
	static FText GetDamageLogModificationText(FDamageLogEventModifier DamageLogEventModifier);


	static FText GenerateDamageLogEventText(const FDamageLogEvent& DamageLogEvent);
	static FText GenerateDamageLogEventModifierText(const FDamageLogEventModifier& DamageLogEventModifier);
};