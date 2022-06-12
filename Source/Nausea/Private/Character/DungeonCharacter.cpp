// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/DungeonCharacter.h"
#include "Components/PrimitiveComponent.h"
#include "Components/CapsuleComponent.h"
#include "Camera/CameraComponent.h"
#include "SkeletalMeshComponentBudgeted.h"
#include "GameFramework/PlayerController.h"
#include "Overlord/DungeonGameState.h"
#include "Character/DungeonCharacterMovementComponent.h"

#if WITH_EDITORONLY_DATA
#include "Components/ArrowComponent.h"
#endif


ADungeonCharacter::ADungeonCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UDungeonCharacterMovementComponent>(ACharacter::CharacterMovementComponentName).SetDefaultSubobjectClass<USkeletalMeshComponentBudgeted>(ACharacter::MeshComponentName))
{
	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetGenerateOverlapEvents(false);

	}

	PrimaryActorTick.bStartWithTickEnabled = false;

	if (GetMesh())
	{
		GetMesh()->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		//GetMesh()->bEnableUpdateRateOptimizations = true;
		GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
		GetMesh()->bComponentUseFixedSkelBounds = true;
		GetMesh()->bUseAttachParentBound = true;

		if (!USkeletalMeshComponentBudgeted::OnCalculateSignificance().IsBound())
		{
			USkeletalMeshComponentBudgeted::OnCalculateSignificance().BindLambda([](USkeletalMeshComponentBudgeted* Component)
			{
				constexpr float LowestPriority = 0.f;
				if(!Component || Component->IsPendingKill())
				{
					return LowestPriority;
				}

				AActor* Owner = Component->GetOwner();

				if (!Owner || Owner->IsPendingKillPending())
				{
					return LowestPriority;
				}

				const UWorld* World = Owner->GetWorld();

				if (!World)
				{
					return LowestPriority;
				}

				FVector Location;
				FRotator Rotator;
				World->GetFirstPlayerController()->GetPlayerViewPoint(Location, Rotator);
				
				FBoxSphereBounds Bounds = Component->GetCachedLocalBounds();
				const float ComponentBoundSize = Bounds.SphereRadius;
				const FVector ComponentWorldLocation = Owner->GetActorTransform().TransformPosition(Bounds.Origin);
				return (FMath::Square(Bounds.SphereRadius) / FMath::Max(1.f, FVector::DistSquared(Location, ComponentWorldLocation))) * 100.f;
			});

		}

		if (USkeletalMeshComponentBudgeted* SkeletalMeshComponent = Cast<USkeletalMeshComponentBudgeted>(GetMesh()))
		{
			SkeletalMeshComponent->SetAutoCalculateSignificance(true);
		}
	}
}

void ADungeonCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (GetLocalRole() == ROLE_Authority)
	{
		SpawnLocation = GetActorLocation();
	}
}

void ADungeonCharacter::PreRegisterAllComponents()
{
	Super::PreRegisterAllComponents();

	TInlineComponentArray<USkeletalMeshComponent*> SkeletalMeshComponents(this);
	for (USkeletalMeshComponent* SkeletalMesh : SkeletalMeshComponents)
	{
		if (SkeletalMesh != GetMesh())
		{
			SkeletalMesh->DestroyComponent();
		}
	}

	TInlineComponentArray<UCameraComponent*> CameraComponents(this);
	for (UCameraComponent* Camera : CameraComponents)
	{
		Camera->DestroyComponent();
	}

#if WITH_EDITORONLY_DATA
	TInlineComponentArray<UArrowComponent*> ArrowComponents(this);
	for (UArrowComponent* Arrow : ArrowComponents)
	{
		Arrow->DestroyComponent();
	}
#endif

	TInlineComponentArray<UActorComponent*> Components(this);
	for (UActorComponent* Component : Components)
	{
		Component->SetCanEverAffectNavigation(false);
	}

	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(this);
	for (UPrimitiveComponent* Primitive : PrimitiveComponents)
	{
		Primitive->SetGenerateOverlapEvents(false);
		Primitive->SetCollisionEnabled(GetRootComponent() == Primitive ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::PhysicsOnly);
	}

	if (USkeletalMeshComponentBudgeted* BudgetedSkeletalMesh = Cast<USkeletalMeshComponentBudgeted>(GetMesh()))
	{
		if (!BudgetedSkeletalMesh->OnReduceWork().IsBound())
		{
			BudgetedSkeletalMesh->OnReduceWork().BindWeakLambda(BudgetedSkeletalMesh, [](USkeletalMeshComponentBudgeted* Component, bool bReduceWork)
			{
				if (!Component || Component->IsPendingKill())
				{
					return;
				}

				Component->SetCastShadow(!bReduceWork);
			});
		}
	}
}

void ADungeonCharacter::Died(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bHasDied)
	{
		return;
	}

	//Our animation blueprints use a lower res set of physics assets to do certain effects and we disable the mesh's physics asset until needed.
	if (!IsNetMode(NM_DedicatedServer) && RagdollPhysicsAsset)
	{
		if (USkeletalMeshComponentBudgeted* BudgetedSkeletalMesh = Cast<USkeletalMeshComponentBudgeted>(GetMesh()))
		{
			BudgetedSkeletalMesh->SetPhysicsAsset(RagdollPhysicsAsset);
		}
	}

	Super::Died(Damage, DamageEvent, EventInstigator, DamageCauser);
}

int32 ADungeonCharacter::GetCoinValue() const
{
	if (!BaseCoinValueDifficultyScale && !BaseCoinValueWaveScale)
	{
		return BaseCoinValue;
	}

	if (ADungeonGameState* DungeonGameState = GetWorld()->GetGameState<ADungeonGameState>())
	{
		const float DifficultyModifier = BaseCoinValueDifficultyScale ? BaseCoinValueDifficultyScale->GetFloatValue(DungeonGameState->GetGameDifficultyForScaling()) : 1.f;
		const float WaveModifier = BaseCoinValueWaveScale ? BaseCoinValueWaveScale->GetFloatValue(DungeonGameState->GetCurrentWaveNumber()) : 1.f;
		return FMath::CeilToInt(float(BaseCoinValue) * DifficultyModifier * WaveModifier);
	}

	return BaseCoinValue;
}

int32 ADungeonCharacter::GetGameDamageValue() const
{
	return BaseGameDamageValue;
}

void ADungeonCharacter::NotifyCharacterUnableToPath()
{
	TeleportTo(SpawnLocation, GetActorRotation());
}

TSubclassOf<UDungeonCharacterDescription> ADungeonCharacter::GetCharacterDescriptor(TSubclassOf<ADungeonCharacter> DungeonCharacterClass)
{
	if (!DungeonCharacterClass)
	{
		return nullptr;
	}

	return DungeonCharacterClass.GetDefaultObject()->CharacterDescription;
}