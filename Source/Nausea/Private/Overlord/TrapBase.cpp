// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#include "Overlord/TrapBase.h"
#include "Components/SkinnedMeshComponent.h"
#include "NauseaGlobalDefines.h"
#include "Player/DungeonPlayerController.h"
#include "Player/DungeonPlayerState.h"
#include "Overlord/PlacementActor.h"
#include "Components/PrimitiveComponent.h"
#include "UI/CoreWidgetComponent.h"

ATrapBase::ATrapBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
}

void ATrapBase::PostInitializeComponents()
{
	//Shut down all component ticks on primitives. Trying to cut down tick groups.
	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(this);
	for (UPrimitiveComponent* Component : PrimitiveComponents)
	{
		if (!Component)
		{
			continue;
		}

		Component->SetComponentTickEnabled(false);
	}

	Super::PostInitializeComponents();
}

EPlacementResult ATrapBase::CanPlaceTrapOnTarget(ADungeonPlayerController* PlacementInstigator, APlacementActor* TargetPlacementActor) const
{
	if (!TargetPlacementActor || (GetPlacementType() & TargetPlacementActor->GetPlacementType()) == 0)
	{
		return EPlacementResult::InvalidPlacementType;
	}

	if (const ADungeonPlayerState* InstigatorPlayerState = PlacementInstigator ? PlacementInstigator->GetPlayerState<ADungeonPlayerState>() : nullptr)
	{
		if (!InstigatorPlayerState->HasEnoughTrapCoins(GetCost()))
		{
			return EPlacementResult::NotEnoughFunds;
		}
	}

	return EPlacementResult::Success;
}

void ATrapBase::SetOccupancy(APlacementActor* TargetPlacementActor, const FPlacementCoordinates& BottomLeftCorner, const FPlacementCoordinates& TopRightCorner)
{
	if (!ensure(TargetPlacementActor))
	{
		return;
	}

	if (!ensure(!OccupiedPlacementActor.IsValid()))
	{
		return;
	}

	OccupiedPlacementActor = TargetPlacementActor;
	OccupiedHandleList.Reset();

	FPlacementGrid& TargetGrid = TargetPlacementActor->GetParentmostPlacementGrid();

	if (!ensure(TargetGrid.IsValid()))
	{
		return;
	}

	const int32 StartX = BottomLeftCorner.GetX();
	const int32 StartY = BottomLeftCorner.GetY();
	const int32 EndX = TopRightCorner.GetX();
	const int32 EndY = TopRightCorner.GetY();

	int32 IndexX, IndexY;

	for (IndexY = StartY; IndexY <= EndY; IndexY++)
	{
		for (IndexX = StartX; IndexX <= EndX; IndexX++)
		{
			if (!TargetGrid.IsValidPoint(IndexX, IndexY, false))
			{
				continue;
			}

			ensure(TargetGrid.IsValidPoint(IndexX, IndexY, true));

			TargetGrid.SetOccupant(IndexX, IndexY, this);
			OccupiedHandleList.Add(TargetGrid.Get(IndexX, IndexY).GetHandle());
		}
	}
}

void ATrapBase::RevokeOccupancy()
{
	if (!ensure(OccupiedPlacementActor.IsValid()))
	{
		OccupiedPlacementActor = nullptr;
		OccupiedHandleList.Reset();
		return;
	}
	
	FPlacementGrid& TargetGrid = OccupiedPlacementActor->GetParentmostPlacementGrid();

	if (!ensure(TargetGrid.IsValid()))
	{
		OccupiedPlacementActor = nullptr;
		OccupiedHandleList.Reset();
		return;
	}

	for (const FPlacementHandle& OccupiedHandle : OccupiedHandleList)
	{
		TargetGrid.ClearPlacementOccupantByHandle(OccupiedHandle);
	}

	OccupiedPlacementActor = nullptr;
	OccupiedHandleList.Reset();
}

TArray<AActor*> ATrapBase::PerformOverlapTestWithPrimitive(UPrimitiveComponent* Component, TSubclassOf<AActor> ActorClassFilter, TArray<AActor*> ActorsToIgnore)
{
	if (!GetWorld() || !Component)
	{
		return TArray<AActor*>();
	}

	static TArray<FOverlapResult> OverlapList;
	OverlapList.Reset();

	FTransform OverlapTransform = Component->GetComponentTransform();
	FCollisionShape OverlapShape = Component->GetCollisionShape();
	GetWorld()->OverlapMultiByObjectType(OverlapList, OverlapTransform.GetLocation(), OverlapTransform.GetRotation(), FCollisionObjectQueryParams(ECC_DungeonPawn), OverlapShape);

	if (OverlapList.Num() == 0)
	{
		return TArray<AActor*>();
	}

	static TSet<AActor*> OverlapActorSet;
	OverlapActorSet.Reset();
	for (const FOverlapResult& Result : OverlapList)
	{
		OverlapActorSet.Add(Result.GetActor());
	}

	return OverlapActorSet.Array();
}

TArray<AActor*> ATrapBase::PerformOverlapTestWithPrimitiveAndApplyDamage(UPrimitiveComponent* Component, TSubclassOf<AActor> ActorClassFilter, TArray<AActor*> ActorsToIgnore)
{
	static TArray<AActor*> OverlapActorList;
	OverlapActorList = PerformOverlapTestWithPrimitive(Component, ActorClassFilter, ActorsToIgnore);

	

	return OverlapActorList;
}

TSoftObjectPtr<UTexture2D> ATrapBase::GetTrapIconForClass(TSubclassOf<ATrapBase> TrapClass)
{
	if (const ATrapBase* TrapCDO = TrapClass.GetDefaultObject())
	{
		return TrapCDO->GetTrapIcon();
	}

	return nullptr;
}

int32 ATrapBase::GetCostForClass(TSubclassOf<ATrapBase> TrapClass)
{
	if (const ATrapBase* TrapCDO = TrapClass.GetDefaultObject())
	{
		return TrapCDO->GetCost();
	}

	return INDEX_NONE;
}

int32 ATrapBase::GetRefundCostIconForClass(TSubclassOf<ATrapBase> TrapClass)
{
	if (const ATrapBase* TrapCDO = TrapClass.GetDefaultObject())
	{
		return TrapCDO->GetRefundCost();
	}

	return INDEX_NONE;
}

ATrapPreview::ATrapPreview(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	ErrorWidgetComponent = CreateDefaultSubobject<UCoreWidgetComponent>(TEXT("ErrorWidgetComponent"));
}

void ATrapPreview::PostInitializeComponents()
{
	TInlineComponentArray<UPrimitiveComponent*> PrimitiveComponents(this);
	for (UPrimitiveComponent* Component : PrimitiveComponents)
	{
		if (!Component)
		{
			continue;
		}

		Component->SetComponentTickEnabled(false);
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	Super::PostInitializeComponents();
}

void ATrapPreview::SetPlacementFlags(uint8 InPlacementFlags)
{
	CurrentPlacementFlags = InPlacementFlags;
}

void ATrapPreview::UpdatePlacementState(EPlacementResult PlacementResult, const FTransform& NewPlacementTransform)
{
	if (LastKnownPlacementResult == PlacementResult)
	{
		SetActorTransform(NewPlacementTransform);
		return;
	}

	LastKnownPlacementResult = PlacementResult;

	UMaterialInterface* Material = nullptr;
	switch (PlacementResult)
	{
		case EPlacementResult::InvalidLocation:
			break;
		case EPlacementResult::InvalidPlacementType:
			Material = InvalidPlacementTypeMaterial.LoadSynchronous();
			break;
		case EPlacementResult::NotEnoughFunds:
			Material = NotEnoughFundsPlacementMaterial.LoadSynchronous();
			break;
		case EPlacementResult::Success:
			Material = ValidPlacementMaterial.LoadSynchronous();
			break;
	}

	if (!Material)
	{
		SetActorHiddenInGame(true);
		OnPlacementResultUpdate.Broadcast(this, PlacementResult);
		return;
	}

	const bool bWasPreviouslyHidden = IsHidden();

	if (bWasPreviouslyHidden)
	{
		SetActorHiddenInGame(false);
	}

	SetActorTransform(NewPlacementTransform);

	TInlineComponentArray<UMeshComponent*> MeshComponentList(this);
	for (UMeshComponent* Component : MeshComponentList)
	{
		const int32 MaterialCount = Component->GetNumMaterials();

		if (MaterialCount <= 0)
		{
			continue;
		}

		for (int32 Index = MaterialCount - 1; Index >= 0; Index--)
		{
			Component->SetMaterial(Index, Material);
		}
	}

	OnPlacementResultUpdate.Broadcast(this, PlacementResult);
}

const FText& ATrapPreview::GetPlacementText() const
{
	static FText EmptyString = FText::GetEmpty();

	switch (LastKnownPlacementResult)
	{
		case EPlacementResult::InvalidLocation:
			return EmptyString;
		case EPlacementResult::InvalidPlacementType:
			return InvalidPlacementTypeText;
		case EPlacementResult::NotEnoughFunds:
			return NotEnoughFundsPlacementText;
		case EPlacementResult::Success:
			return ValidPlacementText;
	}

	return EmptyString;
}

const FLinearColor& ATrapPreview::GetPlacementTextColor() const
{
	static FLinearColor InvalidColor = FLinearColor();

	switch (LastKnownPlacementResult)
	{
	case EPlacementResult::InvalidLocation:
		return InvalidColor;
	case EPlacementResult::InvalidPlacementType:
		return InvalidPlacementTypeColor;
	case EPlacementResult::NotEnoughFunds:
		return NotEnoughFundsPlacementColor;
	case EPlacementResult::Success:
		return ValidPlacementColor;
	}

	return InvalidColor;
}