// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Async/AsyncWork.h"
#include "UObject/NoExportTypes.h"
#include "Engine/StreamableManager.h"
#include "System/MeshMergeTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Character/CoreCharacterComponent.h"
#include "CoreCustomizationComponent.generated.h"

class UCustomizationAsset;

UCLASS(BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class NAUSEA_API UCoreCustomizationComponent : public UCoreCharacterComponent
{
	GENERATED_UCLASS_BODY()

//~ Begin UActorComponent Interface
protected:
	virtual void BeginPlay() override;
public:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
//~ End UActorComponent Interface

public:
    UFUNCTION()
    void SetCustomization(const TArray<UCustomizationAsset*>& InCustomizationList);

    //Any UCustomizationAsset::Apply implementation that needs to load something should do so and let us know here so we can cancel it if we change our customization while loading.
    void AddCustomizationStreamHandle(TSharedPtr<FStreamableHandle> StreamHandle);

    //Any UCustomizationAsset::Apply implementation that adds a new component to the character should do so and let us know here so we can destroy it if we change our customization.
    void AddCustomizationComponent(UActorComponent* Component);

protected:
    UFUNCTION()
    void ValidateCustomization(TArray<UCustomizationAsset*>& InCustomizationList);
    UFUNCTION()
    virtual void UpdateCustomization();
    
    UFUNCTION()
    void OnMeshMergeComplete();

protected:
    UPROPERTY(EditAnywhere, Category = Customization)
    TArray<UCustomizationAsset*> DefaultCustomizationList = TArray<UCustomizationAsset*>();

    UPROPERTY()
    TArray<UCustomizationAsset*> CustomizationList = TArray<UCustomizationAsset*>();

    UPROPERTY(Transient)
    FMeshMergeHandle MeshMergeHandle = FMeshMergeHandle();

    UPROPERTY(EditDefaultsOnly)
    bool bPerformMergeInEditor = false;

	TArray<TSharedPtr<FStreamableHandle>> CustomizationStreamableHandleList;
	TArray<TWeakObjectPtr<UActorComponent>> CustomizationComponentList;
};