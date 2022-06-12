// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Overlord/PlacementActor.h"
#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "DrawDebugHelpers.h"

UPlacementMarkerComponent::UPlacementMarkerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	SetMobility(EComponentMobility::Static);
	SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	SetCastShadow(false);
	SetCanEverAffectNavigation(false);
	bVisibleInReflectionCaptures = false;
	bVisibleInRealTimeSkyCaptures = false;
	bVisibleInRayTracing = false;
	bRenderInMainPass = true;
	bRenderInDepthPass = true;
	bUseAsOccluder = false;
}

void UPlacementMarkerComponent::SetPlacementFlags(uint8 PlacementFlags)
{
	const bool bFloor = (PlacementFlags & static_cast<uint8>(EPlacementType::Floor)) != 0;
	const bool bWall = (PlacementFlags & static_cast<uint8>(EPlacementType::Wall)) != 0;
	const bool bCeiling = (PlacementFlags & static_cast<uint8>(EPlacementType::Ceiling)) != 0;

	if (bFloor && !bWall && !bCeiling)
	{
		SetMaterial(0, FloorGridMaterial);
	}
	else if (bWall && !bFloor && !bCeiling)
	{
		SetMaterial(0, WallGridMaterial);
	}
	else if (bCeiling && !bFloor && !bWall)
	{
		SetMaterial(0, CeilingGridMaterial);
	}
	else if (bFloor && bWall && !bCeiling)
	{
		SetMaterial(0, WallAndFloorGridMaterial);
	}
	else
	{
		SetMaterial(0, InvalidMaterial);
	}
}

APlacementActor::APlacementActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootSceneComponent->SetMobility(EComponentMobility::Static);
	SetRootComponent(RootSceneComponent);

	BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("PlacementCollider"));
	BoxComponent->SetMobility(EComponentMobility::Static);
	BoxComponent->SetCollisionProfileName("PlacementMarker");
	BoxComponent->SetCanEverAffectNavigation(false);
	BoxComponent->SetupAttachment(GetRootComponent());

#if WITH_EDITOR
	ArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowComponent"));
	ArrowComponent->SetMobility(EComponentMobility::Static);
	ArrowComponent->SetCanEverAffectNavigation(false);
	ArrowComponent->ArrowSize = 8.f;
	ArrowComponent->ArrowLength = 40.f;
	ArrowComponent->SetupAttachment(GetRootComponent());
#endif //WITH_EDITOR

	SetActorHiddenInGame(true);

	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void APlacementActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void APlacementActor::BeginPlay()
{
	Super::BeginPlay();

	if (GetParentPlacementActor())
	{
		return;
	}

	return;
	const FVector RightVector = GetActorRightVector();
	const FVector UpVector = GetActorUpVector();

	const FTransform& RootTransform = PlacementGrid.GetRootTransform();

	const FVector AnchorPoint = RootTransform.GetLocation();
	const FVector XOffset = RightVector * (TrapGridSize * float(GetPlacementGrid().GetSizeX()));
	const FVector YOffset = UpVector * (TrapGridSize * float(GetPlacementGrid().GetSizeY()));
	const FVector DrawOffset = GetActorForwardVector() * 32.f;

	DrawDebugLine(GetWorld(), AnchorPoint + DrawOffset, AnchorPoint + DrawOffset + XOffset, FColor::Green, false, 32.f, 0, 4.f);
	DrawDebugLine(GetWorld(), AnchorPoint + DrawOffset + XOffset, AnchorPoint + DrawOffset + YOffset + XOffset, FColor::Blue, false, 32.f, 0, 4.f);
	DrawDebugLine(GetWorld(), AnchorPoint + DrawOffset + YOffset, AnchorPoint + DrawOffset + YOffset + XOffset, FColor::Red, false, 32.f, 0, 4.f);
	DrawDebugLine(GetWorld(), AnchorPoint + DrawOffset, AnchorPoint + DrawOffset + YOffset, FColor::Yellow, false, 32.f, 0, 4.f);

	FColor WallColor;
	if (SupportsWallTraps())
	{
		WallColor = FColor::Red;
	}
	else if(SupportsFloorTraps())
	{
		WallColor = FColor::Green;
	}
	else if (SupportsCeilingTraps())
	{
		WallColor = FColor::Yellow;
	}
	else
	{
		WallColor = FColor::Magenta;
	}

	const float DrawExtent = 0.25f * TrapGridSize;
	const int32 GridSizeY = PlacementGrid.GetSizeY();
	const int32 GridSizeX = PlacementGrid.GetSizeX();
	int32 IndexX;
	FPlacementCoordinates Coordinates;
	for (int32 IndexY = 0; IndexY < GridSizeY; IndexY++)
	{
		for (IndexX = 0; IndexX < GridSizeX; IndexX++)
		{
			Coordinates = FPlacementCoordinates(IndexX, IndexY);
			if (!PlacementGrid.Contains(Coordinates))
			{
				continue;
			}

			const FVector Location = PlacementGrid.GetCenteredWorldPosition(Coordinates);
		
			//void DrawDebugSphere(const UWorld* InWorld, FVector const& Center, float Radius, int32 Segments, FColor const& Color, bool bPersistentLines, float LifeTime, uint8 DepthPriority, float Thickness)
			DrawDebugSphere(GetWorld(), Location, DrawExtent, 4, WallColor, true, 32.f, 0, 2.f);
		}
	}
}

#if WITH_EDITOR
void APlacementActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FProperty* MemberPropertyThatChanged = PropertyChangedEvent.MemberProperty;
	const FName MemberPropertyName = MemberPropertyThatChanged != nullptr ? MemberPropertyThatChanged->GetFName() : NAME_None;

	const bool bIsUndo = MemberPropertyName == NAME_None && PropertyChangedEvent.ChangeType == EPropertyChangeType::Unspecified;

	const bool bChangedPlacementType = MemberPropertyName == GET_MEMBER_NAME_CHECKED(APlacementActor, PlacementType);

	const bool bRecalculatePlacement = (PropertyChangedEvent.ChangeType != EPropertyChangeType::Interactive || GFrameCounter % 5 == 0)
		&& (MemberPropertyName == GET_MEMBER_NAME_CHECKED(APlacementActor, Rows) || MemberPropertyName == GET_MEMBER_NAME_CHECKED(APlacementActor, Columns) || bChangedPlacementType);

	if (bIsUndo || bRecalculatePlacement)
	{
		GeneratePlacementGrid();
	}

	if (bIsUndo || bChangedPlacementType)
	{
		OnPlacementPreviewUpdate();
	}

	if (bIsUndo || bChangedPlacementType || MemberPropertyName == GET_MEMBER_NAME_CHECKED(APlacementActor, ParentPlacementActor))
	{
		ValidatePlacementSetup();
	}
}
#endif //WITH_EDITOR

void APlacementActor::InitializePlacementData()
{
	FTransform Transform = GetActorTransform();
	Transform.SetScale3D(FVector(1.f));
	PlacementGrid.SetRootTransform(Transform);
}

const FPlacementGrid& APlacementActor::GetParentmostPlacementGrid() const
{
	APlacementActor* Parentmost = GetParentmostPlacementActor();
	if(ensure(Parentmost))
	{
		return Parentmost->GetPlacementGrid();
	}

	return GetPlacementGrid();
}

FPlacementGrid& APlacementActor::GetParentmostPlacementGrid()
{
	APlacementActor* Parentmost = GetParentmostPlacementActor();
	if (ensure(Parentmost))
	{
		return Parentmost->GetPlacementGrid();
	}

	return GetPlacementGrid();
}

APlacementActor* APlacementActor::GetParentmostPlacementActor() const
{
	const APlacementActor* CurrentParent = this;
	while (CurrentParent && CurrentParent->GetParentPlacementActor())
	{
		CurrentParent = CurrentParent->GetParentPlacementActor();
	}

	return const_cast<APlacementActor*>(CurrentParent);
}

void APlacementActor::PushPlacementData(const FPlacementGrid& InGridData)
{
	if (GetParentPlacementActor())
	{
		GetParentPlacementActor()->PushPlacementData(InGridData);
		return;
	}
	
	ensure(PlacementGrid.Append(InGridData));
}

#if WITH_EDITOR
void APlacementActor::OnPlacementPreviewUpdate()
{
	if (ArrowComponent)
	{
		FColor Color = FColor::White;
		const bool bFloor = (PlacementType & static_cast<uint8>(EPlacementType::Floor)) != 0;
		const bool bWall = (PlacementType & static_cast<uint8>(EPlacementType::Wall)) != 0;
		const bool bCeiling = (PlacementType & static_cast<uint8>(EPlacementType::Ceiling)) != 0;

		if (bFloor && !bWall && !bCeiling)
		{
			Color = FColor::Green;
		}
		else if (bWall && !bFloor && !bCeiling)
		{
			Color = FColor::Red;
		}
		else if (bCeiling && !bFloor && !bWall)
		{
			Color = FColor::Yellow;
		}
		else if (bFloor && bWall && !bCeiling)
		{
			Color = FColor::Magenta;
		}

		ArrowComponent->SetArrowColor(Color);
	}
}
#endif //WITH_EDITOR

void APlacementActor::GeneratePlacementGrid()
{
	const int32 GridSize = Rows * Columns;

	for (UPlacementMarkerComponent* PlacementMarker : PlacementMarkerList)
	{
		if (!PlacementMarker || PlacementMarker->IsPendingKill())
		{
			continue;
		}

		PlacementMarker->DestroyComponent();
	}

	TInlineComponentArray<UPlacementMarkerComponent*> PlacementMarkers(this);
	for (UPlacementMarkerComponent* PlacementMarker : PlacementMarkers)
	{
		if (!PlacementMarker || PlacementMarker->IsPendingKill())
		{
			continue;
		}

		PlacementMarker->DestroyComponent();
	}

	PlacementMarkerList.Reset(GridSize);

	const FVector Anchor = GetActorLocation();
	const FVector ForwardVector = GetActorForwardVector();
	const FVector UpVector = GetActorUpVector();
	const FVector RightVector = GetActorRightVector();
	PlacementGrid.Reset();
	PlacementGrid.SetSize(Columns, Rows);
	
	FTransform Transform = GetActorTransform();
	Transform.SetScale3D(FVector(1.f));
	PlacementGrid.SetRootTransform(Transform);

	const FVector CenteredOffset = (UpVector * TrapGridSize * 0.5f) + (RightVector * TrapGridSize * 0.5f);

	if (PlacementMarkerClass)
	{
		int32 ColumnIndex;
		FVector CurrentLocation = GetActorLocation();
		for (int32 RowIndex = 0; RowIndex < Rows; RowIndex++)
		{
			CurrentLocation = (Anchor + (UpVector * (float(RowIndex) * TrapGridSize))) + (UpVector * TrapGridSize * 0.5f) + (RightVector * TrapGridSize * 0.5f) + (ForwardVector * TrapGridSize * 0.1f);

			for (ColumnIndex = 0; ColumnIndex < Columns; ColumnIndex++)
			{
				UPlacementMarkerComponent* PlacementComponentMarker = NewObject<UPlacementMarkerComponent>(this, PlacementMarkerClass);
				PlacementComponentMarker->SetupAttachment(GetRootComponent());
				PlacementComponentMarker->RegisterComponent();
				PlacementComponentMarker->SetWorldLocation(CurrentLocation);
				PlacementComponentMarker->SetWorldScale3D(FVector(1.f));
				PlacementComponentMarker->AddRelativeRotation(FRotator(90.f, 0.f, 0.f));
				PlacementComponentMarker->SetPlacementFlags(PlacementType);

				PlacementGrid.Set(ColumnIndex, RowIndex, FPlacementPoint(FPlacementHandle::GenerateHandle(), FPlacementCoordinates(ColumnIndex, RowIndex)));
				PlacementMarkerList.Add(PlacementComponentMarker);

				CurrentLocation += RightVector * TrapGridSize;
			}
		}
	}

	ArrowComponent->SetRelativeLocation(FVector(0.f, float(Columns) * TrapGridSize * 0.5f, float(Rows) * TrapGridSize * 0.5f));
	BoxComponent->SetBoxExtent(FVector(16.f, float(Columns + 1) * TrapGridSize * 0.5f, float(Rows + 1) * TrapGridSize * 0.5f));
	BoxComponent->SetRelativeLocation(FVector(0.f, float(Columns) * TrapGridSize * 0.5f, float(Rows) * TrapGridSize * 0.5f));
	Modify(true);
}

void APlacementActor::ValidatePlacementSetup()
{
	const FTransform& ActorTransform = GetActorTransform();

	bool bModified = false;
	APlacementActor* CurrentParent = this;
	while (CurrentParent && CurrentParent->GetParentPlacementActor())
	{
		CurrentParent = CurrentParent->GetParentPlacementActor();

		if (CurrentParent == this)
		{
			ParentPlacementActor = nullptr;
			bModified = true;
			break;
		}

		if (!CurrentParent->GetActorTransform().GetRotation().Equals(ActorTransform.GetRotation(), 0.1f))
		{
			ParentPlacementActor = nullptr;
			bModified = true;
			break;
		}

		if (GetPlacementType() != CurrentParent->GetPlacementType())
		{
			ParentPlacementActor = nullptr;
			bModified = true;
			break;
		}

		if (ActorTransform.InverseTransformPosition(CurrentParent->GetActorLocation()).X > 2.f)
		{
			ParentPlacementActor = nullptr;
			bModified = true;
			break;
		}
	}

	if (!bModified)
	{
		return;
	}

	Modify(true);
}