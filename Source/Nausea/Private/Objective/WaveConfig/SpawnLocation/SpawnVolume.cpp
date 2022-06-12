// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Objective/WaveConfig/SpawnLocation/SpawnVolume.h"
#include "Character/CoreCharacter.h"

#if WITH_EDITOR
#include "LevelEditor.h"
#include "Editor.h"
#include "Engine/Selection.h"
extern UNREALED_API UEditorEngine* GEditor;
#endif

ASpawnVolume::ASpawnVolume(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	SpawnLocationRenderComponent = CreateDefaultSubobject<USpawnVolumeRenderingComponent>(TEXT("SpawnLocationRenderComponent"));
#endif
}

void ASpawnVolume::BeginPlay()
{
	SpawnLocationStatusList.SetNum(WorldSpawnLocationList.Num());

	Super::BeginPlay();
}

bool ASpawnVolume::GetSpawnTransform(TSubclassOf<ACoreCharacter> CoreCharacter, FTransform& SpawnTransform)
{
	if (!CoreCharacter || WorldSpawnLocationList.Num() == 0)
	{
		return false;
	}

	const ACoreCharacter* CharacterCDO = CoreCharacter.GetDefaultObject();
	const float WorldTime = GetWorld()->GetTimeSeconds();

	int32 Index;

	for (Index = WorldSpawnLocationList.Num() - 1; Index >= 0; Index--)
	{
		if (!SpawnLocationStatusList[Index].IsAvailable(WorldTime))
		{
			continue;
		}

		if (!GetWorld()->FindTeleportSpot(CharacterCDO, WorldSpawnLocationList[Index].Location, FRotator::ZeroRotator))
		{
			SpawnLocationStatusList[Index].MarkFailureTime(WorldTime);
			continue;
		}

		break;
	}

	if (!SpawnLocationStatusList.IsValidIndex(Index))
	{
		return false;
	}

	FSpawnLocationData& LocationData = WorldSpawnLocationList[Index];

	FSpawnLocationStatusData& StatusData = SpawnLocationStatusList[Index];
	StatusData.MarkUseTime(WorldTime);

	SpawnTransform.SetLocation(LocationData.Location);
	FRotator Forward = GetActorRotation();
	Forward.Yaw = 0.f;
	Forward.Yaw += FMath::RandRange(-60.f, 60.f);
	Forward.Roll = 0.f;
	SpawnTransform.SetRotation(Forward.Quaternion());
	return true;
}

bool ASpawnVolume::HasAvailableSpawnTransform(TSubclassOf<ACoreCharacter> CoreCharacter) const
{
	const float WorldTime = GetWorld()->GetTimeSeconds();

	for (const FSpawnLocationStatusData& Status : SpawnLocationStatusList)
	{
		if (Status.IsAvailable(WorldTime))
		{
			return true;
		}
	}

	return false;
}

#if WITH_EDITOR
void ASpawnVolume::PostEditImport()
{
	Super::PostEditImport();

	UpdateDebugCylinderList();
}

void ASpawnVolume::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	UpdateDebugCylinderList();
}

static int32 MoveUpdateCounter = 0;
void ASpawnVolume::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);

	if (!bFinished && MoveUpdateCounter++ % 3 != 0)
	{
		return;
	}

	UpdateDebugCylinderList();
}

void ASpawnVolume::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (OnEditorSelectionChangedHandle.IsValid())
	{
		return;
	}

	OnEditorSelectionChangedHandle = USelection::SelectionChangedEvent.AddUObject(this, &ASpawnVolume::OnEditorSelectionChanged);
}

void ASpawnVolume::UpdateDebugCylinderList()
{
	FBoxSphereBounds VolumeBounds = GetBounds();

	int32 RowCount = FMath::FloorToInt(VolumeBounds.BoxExtent.X / (SpawnLocationRadius * (1.f + PercentSpacing)));
	int32 ColumnCount = FMath::FloorToInt(VolumeBounds.BoxExtent.Y / (SpawnLocationRadius * (1.f + PercentSpacing)));

	if (MaxSpawnLocations > 0)
	{
		//Shrink down the column size until we hit a good number.
		while ((RowCount * ColumnCount) > MaxSpawnLocations)
		{
			RowCount = FMath::Max(RowCount - 1, 1);

			if ((RowCount * ColumnCount) <= MaxSpawnLocations)
			{
				break;
			}

			ColumnCount = FMath::Max(ColumnCount - 1, 1);
		}
	}

	const FVector2D Extent = FVector2D(SpawnLocationRadius, FMath::Min(SpawnLocationHeight, VolumeBounds.BoxExtent.Z));

	const float Diameter = SpawnLocationRadius * 2.f;
	FVector CurrentRelativeLocation = FVector(Diameter * float(RowCount - 1) * 0.5f, Diameter * float(ColumnCount - 1) * 0.5f, 0);
	CurrentRelativeLocation.X += PercentSpacing * CurrentRelativeLocation.X;
	CurrentRelativeLocation.Y += PercentSpacing * CurrentRelativeLocation.Y;

	SpawnLocationList.Empty(RowCount * ColumnCount);
	int32 RemainingPlacements = (RowCount * ColumnCount) - 1;
	while (RemainingPlacements >= 0)
	{
		SpawnLocationList.Add(FSpawnLocationData(CurrentRelativeLocation, Extent));

		if (RemainingPlacements % RowCount == 0)
		{
			CurrentRelativeLocation.X = Diameter * float(RowCount - 1) * 0.5f * (1.f + PercentSpacing);
			CurrentRelativeLocation.Y -= Diameter * (1.f + PercentSpacing);
		}
		else
		{
			CurrentRelativeLocation.X -= Diameter * (1.f + PercentSpacing);
		}

		RemainingPlacements--;
	}

	FTransform ActorTransform = GetActorTransform();

	WorldSpawnLocationList.Empty(SpawnLocationList.Num());
	for (const FSpawnLocationData& RelativeSpawnLocation : SpawnLocationList)
	{
		WorldSpawnLocationList.Add(RelativeSpawnLocation.GetTransformedCopy(ActorTransform));
	}

	SpawnLocationRenderComponent->SetSpawnLocations(WorldSpawnLocationList);
}

void ASpawnVolume::OnEditorSelectionChanged(UObject* NewSelection)
{
	if (!SpawnLocationRenderComponent)
	{
		return;
	}

	if (NewSelection == this)
	{
		SpawnLocationRenderComponent->SetSpawnLocations(WorldSpawnLocationList);
		SpawnLocationRenderComponent->SetVisibility(true);
	}

	const USelection* Selection = Cast<USelection>(NewSelection);

	if (!Selection)
	{
		SpawnLocationRenderComponent->SetVisibility(false);
		return;
	}

	SpawnLocationRenderComponent->SetSpawnLocations(WorldSpawnLocationList);
	SpawnLocationRenderComponent->SetVisibility(Selection->IsSelected(this));
}

//==================================
//EDITOR DRAWING

FSpawnVolumeSceneProxy::FSpawnVolumeSceneProxy(const UPrimitiveComponent* InComponent, const TArray<FDebugRenderSceneProxy::FWireCylinder>& CylinderList)
	: FDebugRenderSceneProxy(InComponent)
{
	bWantsSelectionOutline = false;
	DrawType = WireMesh;

	ActorOwner = InComponent->GetOwner();

	Cylinders = CylinderList;
}

FPrimitiveViewRelevance FSpawnVolumeSceneProxy::GetViewRelevance(const FSceneView* View) const
{
	FPrimitiveViewRelevance Result;
	Result.bDrawRelevance = !IsInGameThread() && IsShown(View) && (SafeIsActorSelected());
	Result.bDynamicRelevance = true;
	// ideally the TranslucencyRelevance should be filled out by the material, here we do it conservative
	Result.bSeparateTranslucency = Result.bNormalTranslucency = IsShown(View);
	return Result;
}

void FSpawnVolumeSceneProxy::SetCylinderData(const TArray<FDebugRenderSceneProxy::FWireCylinder>& CylinderList)
{
	Cylinders = CylinderList;
}

bool FSpawnVolumeSceneProxy::SafeIsActorSelected() const
{
	if (ActorOwner)
	{
		return ActorOwner->IsSelected();
	}

	return false;
}
#endif

USpawnVolumeRenderingComponent::USpawnVolumeRenderingComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bHiddenInGame = true;
}

#if WITH_EDITOR
FPrimitiveSceneProxy* USpawnVolumeRenderingComponent::CreateSceneProxy()
{
	SpawnVolumeSceneProxy = new FSpawnVolumeSceneProxy(this, DebugSpawnLocationList);

	DebugDrawDelegateHelper.InitDelegateHelper(SpawnVolumeSceneProxy);
	DebugDrawDelegateHelper.ReregisterDebugDrawDelgate();

	return SpawnVolumeSceneProxy;
}

FBoxSphereBounds USpawnVolumeRenderingComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	if (AVolume* Volume = Cast<AVolume>(GetOwner()))
	{
		return Volume->GetBounds();
	}

	return FBoxSphereBounds(ForceInit).TransformBy(LocalToWorld);
}

void USpawnVolumeRenderingComponent::CreateRenderState_Concurrent(FRegisterComponentContext* Context)
{
	Super::CreateRenderState_Concurrent(Context);

	DebugDrawDelegateHelper.RegisterDebugDrawDelgate();
}

void USpawnVolumeRenderingComponent::DestroyRenderState_Concurrent()
{
	DebugDrawDelegateHelper.UnregisterDebugDrawDelgate();

	Super::DestroyRenderState_Concurrent();
}

void USpawnVolumeRenderingComponent::SetSpawnLocations(const TArray<FSpawnLocationData>& InSpawnLocationList)
{
	CachedSpawnLocationList = InSpawnLocationList;
	DebugSpawnLocationList.Empty(CachedSpawnLocationList.Num());

	for (const FSpawnLocationData& SpawnLocation : CachedSpawnLocationList)
	{
		DebugSpawnLocationList.Add(FDebugRenderSceneProxy::FWireCylinder(SpawnLocation.Location, SpawnLocation.CylinderExtent.X, SpawnLocation.CylinderExtent.Y, FColor::Green));
	}

	UpdateBounds();
	MarkRenderStateDirty();

	if (GEditor != nullptr)
	{
		GEditor->RedrawLevelEditingViewports();
	}
}
#endif