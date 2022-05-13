// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AITargetInterface.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTargetableStateChanged, AActor*, Actor, bool, bIsTargetable);

UINTERFACE(Blueprintable)
class UAITargetInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class NAUSEA_API IAITargetInterface
{
	GENERATED_IINTERFACE_BODY()

public:
	virtual bool IsTargetable(const AActor* Targeter = nullptr) const { return K2_IsTargetable(Targeter); }

	//Must broadcast when implementer can nolonger be targeted.
	virtual FTargetableStateChanged& GetTargetableStateChangedDelegate() = 0;

	//Called when this actor has become a target.
	virtual void OnBecomeTarget(AActor* Targeter) {}
	//Called when this actor is no longer a target.
	virtual void OnEndTarget(AActor* Targeter) {}

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = AITargetInterface, meta = (DisplayName="Is Targetable"))
	bool K2_IsTargetable(const AActor* Targeter = nullptr) const;

	UFUNCTION(BlueprintImplementableEvent, Category = AITargetInterface, meta = (DisplayName="On Become Target"))
	void K2_OnBecomeTarget(AActor* Targeter);
	UFUNCTION(BlueprintImplementableEvent, Category = AITargetInterface, meta = (DisplayName="On End Target"))
	void K2_OnEndTarget(AActor* Targeter);
};

UCLASS(Abstract, MinimalAPI)
class UAITargetStatics : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = AITargetInterface)
	static bool IsTargetable(TScriptInterface<IAITargetInterface> Target, const AActor* Targeter = nullptr);

	UFUNCTION(BlueprintCallable, Category = AITargetInterface)
	static void OnBecomeTarget(TScriptInterface<IAITargetInterface> Target, AActor* Targeter);
	UFUNCTION(BlueprintCallable, Category = AITargetInterface)
	static void OnEndTarget(TScriptInterface<IAITargetInterface> Target, AActor* Targeter);
};