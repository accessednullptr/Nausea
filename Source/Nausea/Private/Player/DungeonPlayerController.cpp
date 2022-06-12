// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/DungeonPlayerController.h"
#include "GameFramework/InputSettings.h"
#include "Runtime/Engine/Classes/Components/DecalComponent.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Engine/World.h"
#include "NauseaGlobalDefines.h"
#include "Overlord/DungeonGameState.h"
#include "Player/DungeonPlayerState.h"
#include "Character/DungeonCharacter.h"
#include "Overlord/PlacementActor.h"
#include "Overlord/TrapBase.h"
#include "DrawDebugHelpers.h"

static TAutoConsoleVariable<int32> CVarPlacementTestMode(
	TEXT("grid.PlacementTestMode"),
	0,
	TEXT("Enable changing build button to update hit test location instead of building."));

ADungeonPlayerController::ADungeonPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Crosshairs;
}

void ADungeonPlayerController::BeginPlay()
{
	Super::BeginPlay();

	ADungeonGameState* DungeonGameState = GetWorld()->GetGameState<ADungeonGameState>();
	ensure(DungeonGameState);

	if (DungeonGameState)
	{
		DungeonGameState->OnMatchStateChanged.AddDynamic(this, &ADungeonPlayerController::OnMatchStateChanged);
		OnMatchStateChanged(DungeonGameState, DungeonGameState->GetMatchState());
	}
}

static bool bTestPlacement = false;
static FHitResult TestHitResult;
void ADungeonPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (!bPlacementEnabled)
	{
		return;
	}

	bTestPlacement = CVarPlacementTestMode.GetValueOnGameThread() != 0;

	FHitResult HitResult;
	if (!bTestPlacement)
	{
		GetHitResultUnderCursor(ECC_PlacementTrace, false, HitResult);
	}
	else
	{
		HitResult = TestHitResult;
	}

	FTransform PlacementTransform = FTransform::Identity;
	if (UpdatePlacement(HitResult, PlacementTransform))
	{
		UpdatePreviewActor(true, PlacementTransform, Cast<APlacementActor>(HitResult.GetActor()));
	}
	else
	{
		UpdatePreviewActor(false, FTransform::Identity, nullptr);
	}
}

void ADungeonPlayerController::SetupInputComponent()
{
	APlayerController::SetupInputComponent();

	InputComponent->BindAction("Build", IE_Pressed, this, &ADungeonPlayerController::OnBuildPressed);
	InputComponent->BindAction("RotatePlacement", IE_Pressed, this, &ADungeonPlayerController::OnRotatePlacement);
	InputComponent->BindAction("CancelBuild", IE_Pressed, this, &ADungeonPlayerController::OnCancelBuildPressed);
}

void ADungeonPlayerController::SetPlacementTrapClass(TSubclassOf<ATrapBase> TrapClass)
{
	if (!bPlacementEnabled)
	{
		return;
	}

	PlacementTrapClass = TrapClass;

	if (const ATrapBase* TrapCDO = GetPlacementTrapCDO())
	{
		PlacementTrapPreviewClass = TrapCDO->GetPlacementPreviewActor().LoadSynchronous();
	}
	else
	{
		PlacementTrapPreviewClass = nullptr;
	}
	
	if (TrapPreviewActor && !TrapPreviewActor->IsPendingKillPending() && (TrapPreviewActor->GetClass() != PlacementTrapPreviewClass))
	{
		TrapPreviewActor->Destroy();
		TrapPreviewActor = nullptr;
	}

	OnSelectedTrapClassUpdate.Broadcast(this, PlacementTrapClass);
}

void ADungeonPlayerController::ClearPlacementTrapClass()
{
	SetPlacementTrapClass(nullptr);
}

const ATrapBase* ADungeonPlayerController::GetPlacementTrapCDO() const
{
	return PlacementTrapClass.GetDefaultObject();
}

void ADungeonPlayerController::SetPlacementEnabled(bool bEnabled)
{
	if (bPlacementEnabled == bEnabled)
	{
		return;
	}

	bPlacementEnabled = bEnabled;

	if (!bPlacementEnabled)
	{
		ClearPlacementTrapClass();
	}
}

void ADungeonPlayerController::OnMatchStateChanged(ACoreGameState* GameState, FName MatchState)
{
	SetPlacementEnabled(GameState->IsInProgress());
}

bool ADungeonPlayerController::UpdatePlacement(const FHitResult& HitResult, FTransform& PlacementTransform)
{
	if (!HitResult.bBlockingHit)
	{
		return false;
	}

	APlacementActor* PlacementActor = Cast<APlacementActor>(HitResult.GetActor());

	if (!PlacementActor)
	{
		return false;
	}

	const ATrapBase* PlacementTrapCDO = GetPlacementTrapCDO();

	if (!PlacementTrapCDO)
	{
		return false;
	}

	const FPlacementGrid& Grid = PlacementActor->GetParentmostPlacementGrid();

	const int32 SizeX = PlacementTrapCDO->GetSizeX();
	const int32 SizeY = PlacementTrapCDO->GetSizeY();

	FPlacementCoordinates Coordinates = Grid.GetPlacementCoordinatesForSizeNeo(GetWorld(), HitResult.Location, SizeX, SizeY, true);
	
	if (!Grid.IsValidPoint(Coordinates, true))
	{
		return false;
	}

	PlacementTransform = Grid.GetRootTransform();
	PlacementTransform.SetScale3D(FVector(1.f));
	PlacementTransform.SetLocation(Grid.GetWorldPosition(Coordinates));

	if(Grid.IsDebugDrawPlacementEnabled())
	{
		DrawDebugBox(GetWorld(), HitResult.Location, FVector(4.f), FColor::Red, false, 0.1f, 0, 1.f);
		const FVector Result = Grid.GetCenteredWorldPosition(Coordinates);
		DrawDebugDirectionalArrow(GetWorld(), HitResult.Location, Result, 8.f, FColor::Yellow, false, 0.1f, 0, 4.f);
		DrawDebugBox(GetWorld(), Grid.GetCenteredWorldPosition(Coordinates), FVector(32.f), FColor::Orange, false, 0.1f, 0, 4.f);
	}

	return true;
}

void ADungeonPlayerController::UpdatePreviewActor(bool bActive, const FTransform& PlacementTransform, APlacementActor* PlacementActor)
{
	const ATrapBase* PlacementTrapCDO = GetPlacementTrapCDO();

	if (!PlacementTrapCDO)
	{
		if (TrapPreviewActor && !TrapPreviewActor->IsPendingKillPending())
		{
			TrapPreviewActor->Destroy();
			TrapPreviewActor = nullptr;
		}

		return;
	}

	if (!TrapPreviewActor)
	{
		TSubclassOf<ATrapPreview> PreviewActorClass = PlacementTrapCDO->GetPlacementPreviewActor().Get();

		if(!PreviewActorClass)
		{
			return;
		}

		FActorSpawnParameters ASP = FActorSpawnParameters();
		ASP.bDeferConstruction = true;
		TrapPreviewActor = GetWorld()->SpawnActor<ATrapPreview>(PreviewActorClass, PlacementTransform, ASP);

		if (!TrapPreviewActor)
		{
			return;
		}

		TrapPreviewActor->SetPlacementFlags(PlacementTrapCDO->GetPlacementType());
		TrapPreviewActor->FinishSpawning(PlacementTransform);
	}
	
	EPlacementResult Result = bActive ? PlacementTrapCDO->CanPlaceTrapOnTarget(this, PlacementActor) : EPlacementResult::InvalidLocation;
	TrapPreviewActor->UpdatePlacementState(Result, PlacementTransform);
}

void ADungeonPlayerController::OnBuildPressed()
{
	FHitResult HitResult;
	GetHitResultUnderCursor(ECC_PlacementTrace, false, HitResult);

	bTestPlacement = CVarPlacementTestMode.GetValueOnGameThread() != 0;

	if (bTestPlacement)
	{
		TestHitResult = HitResult;
		return;
	}

	if (!HitResult.bBlockingHit)
	{
		return;
	}

	APlacementActor* PlacementActor = Cast<APlacementActor>(HitResult.GetActor());

	if (!PlacementActor)
	{
		return;
	}

	FPlacementGrid& Grid = PlacementActor->GetParentmostPlacementGrid();

	if (!Grid.IsValid())
	{
		return;
	}

	const ATrapBase* PlacementTrapCDO = GetPlacementTrapCDO();

	if (!PlacementTrapCDO)
	{
		return;
	}

	if (PlacementTrapCDO->CanPlaceTrapOnTarget(this, PlacementActor) != EPlacementResult::Success)
	{
		return;
	}

	const int32 SizeX = PlacementTrapCDO->GetSizeX();
	const int32 SizeY = PlacementTrapCDO->GetSizeY();

	FPlacementCoordinates Coordinates = Grid.GetPlacementCoordinatesForSizeNeo(GetWorld(), HitResult.Location, SizeX, SizeY, true);

	if (!Grid.IsValidPoint(Coordinates, true))
	{
		return;
	}

	FTransform ResolvedTransform = Grid.GetRootTransform();
	ResolvedTransform.SetScale3D(FVector(1.f));
	ResolvedTransform.SetLocation(Grid.GetWorldPosition(Coordinates));

	ADungeonPlayerState* DungeonPlayerState = GetPlayerState<ADungeonPlayerState>();

	FActorSpawnParameters ActorSpawnParams = FActorSpawnParameters();
	ActorSpawnParams.Owner = DungeonPlayerState ? DungeonPlayerState : nullptr;

	ATrapBase* TrapBase = GetWorld()->SpawnActor<ATrapBase>(PlacementTrapClass, ResolvedTransform, ActorSpawnParams);
	TrapBase->SetOccupancy(PlacementActor, Coordinates, FPlacementCoordinates(Coordinates, SizeX - 1, SizeY - 1));

	if (DungeonPlayerState)
	{
		DungeonPlayerState->RemoveTrapCoins(PlacementTrapCDO->GetCost());
	}
}

void ADungeonPlayerController::OnCancelBuildPressed()
{
	ClearPlacementTrapClass();
}

void ADungeonPlayerController::OnRotatePlacement()
{
	Rotation = (Rotation++) % 4;
}

void ADungeonPlayerController::OnUnrotatePlacement()
{
	if (Rotation == 0)
	{
		Rotation = 3;
	}
	else
	{
		Rotation--;
	}
}

bool ADungeonPlayerController::IsMouseEventMatchingActionEvent(const FPointerEvent& MouseEvent, FName InActionName)
{
	UInputSettings* InputSettings = UInputSettings::GetInputSettings();

	if (!InputSettings)
	{
		return false;
	}

	const TSet<FKey>& MouseEventKey = MouseEvent.GetPressedButtons();
	const bool bMouseShiftClick = MouseEvent.IsShiftDown();
	const bool bMouseControlClick = MouseEvent.IsControlDown();
	const bool bMouseAltClick = MouseEvent.IsAltDown();

	TArray<FInputActionKeyMapping> ActionMappingList;
	InputSettings->GetActionMappingByName(InActionName, ActionMappingList);
	for (const FInputActionKeyMapping& Binding : ActionMappingList)
	{
		if (bMouseShiftClick != Binding.bShift)
		{
			continue;
		}
		
		if (bMouseControlClick != Binding.bCtrl)
		{
			continue;
		}

		if (bMouseAltClick != Binding.bAlt)
		{
			continue;
		}

		if (MouseEventKey.Contains(Binding.Key))
		{
			return true;
		}
	}

	return false;
}