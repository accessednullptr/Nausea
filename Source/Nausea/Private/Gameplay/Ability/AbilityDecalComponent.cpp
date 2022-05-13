// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#include "Gameplay/Ability/AbilityDecalComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Gameplay/AbilityComponent.h"

const FName UAbilityDecalComponent::StartupBeginTimeScalarParameter(TEXT("StartupBeginTime"));
const FName UAbilityDecalComponent::StartupEndTimeScalarParameter(TEXT("StartupEndTime"));
const FName UAbilityDecalComponent::ActivationBeginTimeScalarParameter(TEXT("ActivationBeginTime"));
const FName UAbilityDecalComponent::ActivationEndTimeScalarParameter(TEXT("ActivationEndTime"));

UAbilityDecalComponent::UAbilityDecalComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UAbilityDecalComponent::InitializeForAbilityData(UAbilityComponent* AbilityComponent, const FAbilityInstanceData& AbilityInstance, const FAbilityTargetData& AbilityData)
{
	if (!DynamicDecalMaterial)
	{
		DynamicDecalMaterial = CreateDynamicMaterialInstance();
	}

	const FVector2D& TargetSize = UAbilityInfo::GetAbilityTargetDataTargetSize(AbilityInstance, AbilityData);

	if (TargetSize != FVector2D(-1.f))
	{
		DecalSize = FVector(DecalSize.X, TargetSize.X, TargetSize.Y);
	}

	const float ServerTime = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
	const float WorldTime = GetWorld()->GetTimeSeconds();
	const float TimeAdjustment = WorldTime - ServerTime;

	const FVector2D& StartupTime = UAbilityInfo::GetAbilityTargetDataStartupTime(AbilityInstance, AbilityData);
	DynamicDecalMaterial->SetScalarParameterValue(StartupBeginTimeScalarParameter, StartupTime.X + TimeAdjustment);
	DynamicDecalMaterial->SetScalarParameterValue(StartupEndTimeScalarParameter, StartupTime.Y + TimeAdjustment);
	const FVector2D& ActivationTime = UAbilityInfo::GetAbilityTargetDataActivationTime(AbilityInstance, AbilityData);
	DynamicDecalMaterial->SetScalarParameterValue(ActivationBeginTimeScalarParameter, ActivationTime.X + TimeAdjustment);
	DynamicDecalMaterial->SetScalarParameterValue(ActivationEndTimeScalarParameter, ActivationTime.Y + TimeAdjustment);

	Execute_K2_InitializeForAbilityData(this, AbilityComponent, AbilityInstance, AbilityData);
}

void UAbilityDecalComponent::OnAbilityActivate()
{
	Execute_K2_OnAbilityActivate(this);
}

void UAbilityDecalComponent::OnAbilityComplete()
{
	Execute_K2_OnAbilityComplete(this);
}

void UAbilityDecalComponent::OnAbilityCleanup()
{
	Execute_K2_OnAbilityCleanup(this);
}

UAbilityDecalComponent* CreateAbilityDecalComponent(UWorld* World, AActor* Actor, TSubclassOf<UAbilityDecalComponent> AbilityDecalClass)
{
	UAbilityDecalComponent* AbilityDecalComponent = NewObject<UAbilityDecalComponent>(Actor ? Actor : (UObject*)World, AbilityDecalClass);
	AbilityDecalComponent->bAllowAnyoneToDestroyMe = true;

	AbilityDecalComponent->SetUsingAbsoluteScale(true);
	AbilityDecalComponent->RegisterComponentWithWorld(World);
	
	return AbilityDecalComponent;
}

UAbilityDecalComponent* UAbilityDecalComponent::SpawnAbilityDecal(const UObject* WorldContextObject, TSubclassOf<UAbilityDecalComponent> AbilityDecalClass, const FAbilityTargetData& AbilityTargetData)
{
	if (!AbilityDecalClass)
	{
		return nullptr;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	if (!World)
	{
		return nullptr;
	}

	UAbilityDecalComponent* AbilityDecalComponent = CreateAbilityDecalComponent(World, AbilityTargetData.GetTargetActor(), AbilityDecalClass);

	if (!AbilityDecalComponent)
	{
		return nullptr;
	}

	switch (AbilityTargetData.GetTargetType())
	{
	case ETargetDataType::Actor:
	case ETargetDataType::ActorRelativeTransform:
		AbilityDecalComponent->AttachToComponent(AbilityTargetData.GetTargetActor()->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		AbilityDecalComponent->SetRelativeTransform(AbilityTargetData.GetTargetTransform());
		break;
	case ETargetDataType::Transform:
	case ETargetDataType::MovingTransform:
		AbilityDecalComponent->SetWorldTransform(AbilityTargetData.GetTransform());
		break;
	}

	FRotator Rotation = AbilityDecalComponent->GetComponentRotation();
	Rotation.Pitch = -90.f;
	Rotation.Roll = Rotation.Yaw;
	Rotation.Yaw = 0.f;
	AbilityDecalComponent->SetWorldRotation(Rotation);
	return AbilityDecalComponent;
}