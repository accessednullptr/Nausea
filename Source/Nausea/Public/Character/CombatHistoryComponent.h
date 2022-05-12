// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Gameplay/StatusType.h"
#include "CombatHistoryComponent.generated.h"

class IStatusInterface;
class UStatusComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FReceivedDamageLogEventSignature, UCombatHistoryComponent*, Component, const FDamageLogEvent&, DamageLogEvent);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), HideCategories = (ComponentTick, Collision, Tags, Variable, Activation, ComponentReplication, Cooking, Sockets, UserAssetData))
class NAUSEA_API UCombatHistoryComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

//~ Begin UActorComponent Interface
protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
//~ Begin UActorComponent Interface

public:
	UFUNCTION(BlueprintCallable, Category = CombatHistoryComponent)
	UStatusComponent* GetStatusComponent() const;

	UFUNCTION(BlueprintCallable, Category = CombatHistoryComponent)
	const TArray<FDamageLogEvent>& GetDamageLogList() const { return DamageLogList; }

	UFUNCTION(BlueprintCallable, Category = CombatHistoryComponent)
	const FDamageLogEvent& GetDamageLogDeathEvent() const { return DamageLogList.Last(); }

protected:
	UFUNCTION()
	virtual void BindToStatusComponent();

	UFUNCTION()
	void ProcessDamageLogEvent(const FDamageLogEvent& DamageLogEvent);

	UFUNCTION()
	void ProcessDeathDamageLogEvent(const FDamageLogEvent& DeathDamageLogEvent);

	UFUNCTION()
	void AddDamageLogEvent(const FDamageLogEvent& DamageLogEvent);

	//Returns true if damage log that was pushed to the buffer needs to be sent to the log list.
	UFUNCTION()
	int32 PushDamageLogEventToBuffer(const FDamageLogEvent& DamageLogEvent);

	UFUNCTION(Client, Reliable)
	void Client_Relaible_SendDamageLog(const FDamageLogEvent& DamageLogEvent);

public:
	UPROPERTY(BlueprintAssignable, Category = CombatHistoryComponent)
	FReceivedDamageLogEventSignature OnReceivedDamageLogEvent;

	UPROPERTY(BlueprintAssignable, Category = CombatHistoryComponent)
	FReceivedDamageLogEventSignature OnReceivedDamageLogDeathEvent;

private:
	UPROPERTY(Transient)
	TScriptInterface<IStatusInterface> OwningStatusInterface = nullptr;

	UPROPERTY(Transient)
	TArray<FDamageLogEvent> DamageLogBufferList = TArray<FDamageLogEvent>();

	UPROPERTY(Transient)
	TArray<FDamageLogEvent> DamageLogList = TArray<FDamageLogEvent>();
};
