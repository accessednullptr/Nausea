// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "AI/CoreAIController.h"
#include "NavigationSystem.h"
#include "NavMesh/NavMeshPath.h"
#include "AI/ActionBrainComponent.h"
#include "AI/EnemySelectionComponent.h"
#include "AI/RoutineManagerComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/CorePlayerState.h"
#include "Character/CoreCharacter.h"
#include "AI/CorePathFollowingComponent.h"

ACoreAIController::ACoreAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.DoNotCreateDefaultSubobject(TEXT("ActionsComp")).SetDefaultSubobjectClass<UCorePathFollowingComponent>(TEXT("PathFollowingComponent")))
{
#if WITH_EDITOR
	//UNavigationSystemV1::SetNavigationAutoUpdateEnabled(false, nullptr);
#endif // WITH_EDITOR
}

void ACoreAIController::BeginPlay()
{
	Super::BeginPlay();
}

void ACoreAIController::PostInitializeComponents()
{
	if (EnemySelectionComponent == NULL || EnemySelectionComponent->IsPendingKill() == true)
	{
		EnemySelectionComponent = FindComponentByClass<UEnemySelectionComponent>();
	}

	if (ActionBrainComponent == NULL || ActionBrainComponent->IsPendingKill() == true)
	{
		ActionBrainComponent = FindComponentByClass<UActionBrainComponent>();
		BrainComponent = ActionBrainComponent;

		if (ActionBrainComponent)
		{
			bStopAILogicOnUnposses = false;
		}
	}

	if (RoutineManagerComponent == NULL || RoutineManagerComponent->IsPendingKill() == true)
	{
		RoutineManagerComponent = FindComponentByClass<URoutineManagerComponent>();
	}

	Super::PostInitializeComponents();
}

void ACoreAIController::InitPlayerState()
{
	if (GetNetMode() == NM_Client)
	{
		return;
	}

	Super::InitPlayerState();

	if (ACorePlayerState* CorePlayerState = GetPlayerState<ACorePlayerState>())
	{
		if (bOverrideOwningTeam)
		{
			CachedTeamIdOverride = UPlayerOwnershipInterfaceTypes::GetGenericTeamIdFromTeamEnum(TeamOverride);
			CorePlayerState->SetGenericTeamId(CachedTeamIdOverride);
		}

		OnReceivePlayerState(CorePlayerState);
	}
}

void ACoreAIController::SetPawn(APawn* InPawn)
{
	const APawn* PreviousPawn = GetPawn();

	Super::SetPawn(InPawn);

	if (GetPawn() != PreviousPawn)
	{
		OnPawnUpdated.Broadcast(this, Cast<ACoreCharacter>(InPawn));
	}
}

bool ACoreAIController::RunBehaviorTree(UBehaviorTree* BTAsset)
{
	if (GetActionBrainComponent())
	{
		return false;
	}

	return Super::RunBehaviorTree(BTAsset);
}

void ACoreAIController::UpdateControlRotation(float DeltaTime, bool bUpdatePawn)
{
	APawn* const CurrentPawn = GetPawn();

	if (!CurrentPawn)
	{
		return;
	}

	FRotator NewControlRotation = GetControlRotation();

	// Look toward focus
	const FVector FocalPoint = GetFocalPoint();
	if (FAISystem::IsValidLocation(FocalPoint))
	{
		NewControlRotation = (FocalPoint - CurrentPawn->GetPawnViewLocation()).Rotation();
	}
	else if (bSetControlRotationFromPawnOrientation)
	{
		NewControlRotation = CurrentPawn->GetActorRotation();
	}

	// Don't pitch view unless looking at another pawn
	if (NewControlRotation.Pitch != 0 && Cast<APawn>(GetFocusActor()) == nullptr)
	{
		NewControlRotation.Pitch = 0.f;
	}

	SetControlRotation(NewControlRotation);

	if (bUpdatePawn)
	{
		const FRotator CurrentPawnRotation = CurrentPawn->GetActorRotation();
		const FRotator DesiredPawnRotation = FMath::RInterpConstantTo(CurrentPawnRotation, NewControlRotation, DeltaTime, GetMaxRotationRate());
		if (CurrentPawnRotation.Equals(DesiredPawnRotation, 1e-3f) == false)
		{
			CurrentPawn->FaceRotation(DesiredPawnRotation, DeltaTime);
		}
	}
}

ACorePlayerState* ACoreAIController::GetOwningPlayerState() const
{
	return GetPlayerState<ACorePlayerState>();
}

float ACoreAIController::GetMaxRotationRate() const
{
	if (!GetCharacter())
	{
		return 360.f;
	}

	const UCharacterMovementComponent* CharacterMovementComponent = GetCharacter()->GetCharacterMovement();

	if (!CharacterMovementComponent)
	{
		return 360.f;
	}

	return CharacterMovementComponent->RotationRate.Yaw;
}