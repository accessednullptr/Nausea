// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Overlord/PlacementTypes.h"
#include "DrawDebugHelpers.h"

static TAutoConsoleVariable<int32> CVarDebugDrawGridPlacement(
	TEXT("grid.DebugDrawGridPlacement"),
	0,
	TEXT("Enable drawing of grid placement calculation and adjustments."));

uint64 FPlacementHandle::HandleIDCounter = MAX_uint64;
FPlacementPoint FPlacementPoint::InvalidPoint = FPlacementPoint();

bool FPlacementGrid::IsDebugDrawPlacementCVarSet()
{
	return CVarDebugDrawGridPlacement.GetValueOnGameThread() != 0;
}

void FPlacementGrid::AddPointFromWorldPosition(const FVector& WorldPosition)
{
	const FVector RelativeLocation = RootTransform.InverseTransformPosition(WorldPosition);
	const int32 XCoordinate = FMath::RoundToInt(RelativeLocation.Y / TrapGridSize);
	const int32 YCoordinate = FMath::RoundToInt(RelativeLocation.Z / TrapGridSize);

	ensure(!Contains(FPlacementCoordinates(XCoordinate, YCoordinate)));

	if (YCoordinate < 0)
	{
		int32 InsertCount = -YCoordinate;
		while (InsertCount > 0)
		{
			RowList.Insert(FPlacementGridRow(), 0);
			InsertCount--;
		}

		const FVector NewRelativeRootLocation = FVector(0.f, 0.f, YCoordinate * TrapGridSize);
		RootTransform.SetLocation(RootTransform.TransformPosition(NewRelativeRootLocation));
	}

	if (XCoordinate < 0)
	{
		for (FPlacementGridRow& Row : RowList)
		{
			if (Row.Num() <= 0)
			{
				continue;
			}

			int32 InsertCount = -XCoordinate;
			while (InsertCount > 0)
			{
				Row.Insert(FPlacementPoint(), 0);
				InsertCount--;
			}
		}

		const FVector NewRelativeRootLocation = FVector(0.f, XCoordinate * TrapGridSize, 0.f);
		RootTransform.SetLocation(RootTransform.TransformPosition(NewRelativeRootLocation));
	}

	const int32 InsertX = FMath::Max(XCoordinate, 0);
	const int32 InsertY = FMath::Max(YCoordinate, 0);

	Set(InsertX, InsertY, FPlacementPoint(FPlacementHandle::GenerateHandle(), FPlacementCoordinates(InsertX, InsertY)));
}

bool FPlacementGrid::AdjustCoordinatesToValidPoint(UWorld* World, FPlacementCoordinates& Coordinates, const TArray<EPlacementDirection>& BiasOrder, bool bMustBeEmpty) const
{
	if (IsDebugDrawPlacementEnabled())
	{
		DrawDebugBox(World, GetCenteredWorldPosition(Coordinates), FVector(12.f), FColor::Green, false, 0.1f, 0, 2.f);
	}

	if (IsValidPoint(Coordinates, bMustBeEmpty))
	{
		return true;
	}

	FPlacementCoordinates OriginalCoordinates = Coordinates;

	for (EPlacementDirection BiasTest : BiasOrder)
	{
		switch (BiasTest)
		{
		case EPlacementDirection::Up:
			Coordinates.SetX(OriginalCoordinates.GetX());
			Coordinates.SetY(OriginalCoordinates.GetY() + 1);
			if (IsDebugDrawPlacementEnabled())
			{
				DrawDebugBox(World, GetCenteredWorldPosition(Coordinates), FVector(8.f), FColor::Blue, false, 0.1f, 0, 2.f);
			}
			if (IsValidPoint(Coordinates, bMustBeEmpty)) { return true; }
			break;
		case EPlacementDirection::Down:
			Coordinates.SetX(OriginalCoordinates.GetX());
			Coordinates.SetY(OriginalCoordinates.GetY() - 1);
			if (IsDebugDrawPlacementEnabled())
			{
				DrawDebugBox(World, GetCenteredWorldPosition(Coordinates), FVector(8.f), FColor::Blue, false, 0.1f, 0, 2.f);
			}
			if (IsValidPoint(Coordinates, bMustBeEmpty)) { return true; }
			break;
		case EPlacementDirection::Left:
			Coordinates.SetY(OriginalCoordinates.GetY());
			Coordinates.SetX(OriginalCoordinates.GetX() - 1);
			if (IsDebugDrawPlacementEnabled())
			{
				DrawDebugBox(World, GetCenteredWorldPosition(Coordinates), FVector(8.f), FColor::Blue, false, 0.1f, 0, 2.f);
			}
			if (IsValidPoint(Coordinates, bMustBeEmpty)) { return true; }
			break;
		case EPlacementDirection::Right:
			Coordinates.SetY(OriginalCoordinates.GetY());
			Coordinates.SetX(OriginalCoordinates.GetX() + 1);
			if (IsDebugDrawPlacementEnabled())
			{
				DrawDebugBox(World, GetCenteredWorldPosition(Coordinates), FVector(8.f), FColor::Blue, false, 0.1f, 0, 2.f);
			}
			if (IsValidPoint(Coordinates, bMustBeEmpty)) { return true; }
			break;
		}
	}

	return IsValidPoint(Coordinates, bMustBeEmpty);
}


FPlacementCoordinates FPlacementGrid::GetCoordinatesFromWorldPosition(UWorld* World, const FVector& WorldPosition, TArray<EPlacementDirection>& BiasOrder) const
{
	const FVector RelativeLocation = RootTransform.InverseTransformPosition(WorldPosition) - FVector(TrapGridSize * 0.5f);

	EPlacementDirection Bias = EPlacementDirection::MAX;
	const FVector RoundedRelativeLocation = { 0.f, FMath::RoundToFloat(RelativeLocation.Y / TrapGridSize), FMath::RoundToFloat(RelativeLocation.Z / TrapGridSize) };
	const FVector RelativeGridOffset = (RelativeLocation / TrapGridSize) - RoundedRelativeLocation;

	BiasOrder.Reset(4);

	FPlacementCoordinates Coordinates = FPlacementCoordinates(RoundedRelativeLocation.Y, RoundedRelativeLocation.Z);

	const bool bBiasYIsUp = RelativeGridOffset.Z > 0.f;
	const bool bBiasXIsRight = RelativeGridOffset.Y > 0.f;
	
	const bool bPrioritizeHorizontalBias = FMath::Abs(RelativeGridOffset.Y) > FMath::Abs(RelativeGridOffset.Z);
	if (bPrioritizeHorizontalBias)
	{
		BiasOrder.Add(bBiasXIsRight ? EPlacementDirection::Right : EPlacementDirection::Left);
		if (bBiasYIsUp)
		{
			BiasOrder.Append({ EPlacementDirection::Up, EPlacementDirection::Down });
		}
		else
		{
			BiasOrder.Append({ EPlacementDirection::Down, EPlacementDirection::Up });
		}
		BiasOrder.Add(bBiasXIsRight ? EPlacementDirection::Left : EPlacementDirection::Right);
	}
	else
	{
		BiasOrder.Add(bBiasYIsUp ? EPlacementDirection::Up : EPlacementDirection::Down);
		if (bBiasXIsRight)
		{
			BiasOrder.Append({ EPlacementDirection::Right, EPlacementDirection::Left });
		}
		else
		{
			BiasOrder.Append({ EPlacementDirection::Left, EPlacementDirection::Right });
		}
		BiasOrder.Add(bBiasYIsUp ? EPlacementDirection::Down : EPlacementDirection::Up);
	}

	//AdjustCoordinatesToValidPoint(World, Coordinates, BiasOrder, false);

	return Coordinates;
}

inline EPlacementDirection GetVerticalBias(const TArray<EPlacementDirection>& BiasOrder)
{
	for (EPlacementDirection Bias : BiasOrder)
	{
		if (Bias > EPlacementDirection::Down)
		{
			continue;
		}

		return Bias;
	}

	return EPlacementDirection::Up;
}

inline EPlacementDirection GetHorizontalBias(const TArray<EPlacementDirection>& BiasOrder)
{
	for (EPlacementDirection Bias : BiasOrder)
	{
		if (Bias < EPlacementDirection::Left)
		{
			continue;
		}

		return Bias;
	}

	return EPlacementDirection::Right;
}

FPlacementCoordinates FPlacementGrid::GetPlacementCoordinatesForSizeNeo(UWorld* World, const FVector& WorldPosition, int32 SizeX, int32 SizeY, bool bMustBeEmpty) const
{
	SetDebugDrawPlacementEnabled(IsDebugDrawPlacementCVarSet());

	TArray<EPlacementDirection> BiasOrder;
	const FPlacementCoordinates WorldLocationCoordinates = GetCoordinatesFromWorldPosition(World, WorldPosition, BiasOrder);

	if (GetSizeX() < SizeX || GetSizeY() < SizeY)
	{
		return FPlacementCoordinates();
	}

	const FVector Result = GetCenteredWorldPosition(WorldLocationCoordinates);

	if (IsDebugDrawPlacementEnabled())
	{
		DrawDebugString(World, Result, FString::Printf(TEXT("%i / %i"), WorldLocationCoordinates.GetX(), WorldLocationCoordinates.GetY()), nullptr, FColor::Cyan, 0.1f, false, 0.9f);
		DrawDebugDirectionalArrow(World, WorldPosition, Result, 8.f, FColor::Red, false, 0.1f, 0, 4.f);
		DrawDebugBox(World, Result, FVector(32.f), FColor::Red, false, 0.1f, 0, 4.f);

		const FVector XDirectionOffset = GetRootTransform().TransformPosition(FVector::YAxisVector * 100.f + FVector::XAxisVector * 100.f);
		const FVector YDirectionOffset = GetRootTransform().TransformPosition(FVector::ZAxisVector * 100.f + FVector::XAxisVector * 100.f);
		const FVector RootDrawPosition = GetRootTransform().TransformPosition(FVector::XAxisVector * 100.f);
		//DrawDebugDirectionalArrow(const UWorld* InWorld, FVector const& LineStart, FVector const& LineEnd, float ArrowSize, FColor const& Color, bool bPersistentLines = false, float LifeTime = -1.f, uint8 DepthPriority = 0, float Thickness = 0.f) {}
		DrawDebugDirectionalArrow(World, RootDrawPosition, XDirectionOffset, 32.f, FColor::Red, false, 0.1f, 0, 16.f);
		DrawDebugDirectionalArrow(World, RootDrawPosition, YDirectionOffset, 32.f, FColor::Green, false, 0.1f, 0, 16.f);
	}

	const int32 HalfSizeX = FMath::FloorToInt(float(SizeX) * 0.5f);
	const int32 HalfSizeY = FMath::FloorToInt(float(SizeY) * 0.5f);
	//Anchor refers to bottom left point of a placed item so offset now.

	FPlacementCoordinates AdjustedStartingLocation = WorldLocationCoordinates;

	if ((SizeX & 1) == 0 && GetHorizontalBias(BiasOrder) == EPlacementDirection::Right)
	{
		AdjustedStartingLocation.GetX()++;
	}

	if ((SizeY & 1) == 0 && GetVerticalBias(BiasOrder) == EPlacementDirection::Up)
	{
		AdjustedStartingLocation.GetY()++;
	}

	AdjustedStartingLocation -= FPlacementCoordinates(HalfSizeX, HalfSizeY);

	if (IsDebugDrawPlacementEnabled())
	{
		DrawDebugString(World, GetCenteredWorldPosition(AdjustedStartingLocation), FString::Printf(TEXT("%i / %i"), AdjustedStartingLocation.GetX(), AdjustedStartingLocation.GetY()), nullptr, FColor::Cyan, 0.1f, false, 1.1f);
		DrawDebugBox(World, GetCenteredWorldPosition(AdjustedStartingLocation), FVector(28.f), FColor::Silver, false, 0.1f, 0, 4.f);
	}

	bool bFoundValidPlacement = IsValidPlacement(AdjustedStartingLocation, SizeX, SizeY, bMustBeEmpty);
	FPlacementCoordinates Adjustment;
	int32 RemainingAttemptCount = 8;
	while (!bFoundValidPlacement && RemainingAttemptCount > 0)
	{
		Adjustment = GetAdjustmentDirection(World, AdjustedStartingLocation, SizeX, SizeY, bMustBeEmpty);

		if (!Adjustment.IsValid())
		{
			return FPlacementCoordinates();
		}

		AdjustedStartingLocation += Adjustment;

		bFoundValidPlacement = IsValidPlacement(AdjustedStartingLocation, SizeX, SizeY, bMustBeEmpty);
		RemainingAttemptCount--;
	}

	if (!bFoundValidPlacement)
	{
		return FPlacementCoordinates();
	}

	return AdjustedStartingLocation;
}

FPlacementCoordinates FPlacementGrid::GetAdjustmentDirection(UWorld* World, FPlacementCoordinates& Coordinates, int32 SizeX, int32 SizeY, bool bMustBeEmpty) const
{
	bool bAllPointsOccupied = true;
	int32 DesiredLeftShift = 0;
	int32 DesiredRightShift = 0;
	int32 DesiredUpShift = 0;
	int32 DesiredDownShift = 0;
	
	const int32 StartX = Coordinates.GetX();
	const int32 StartY = Coordinates.GetY();
	const int32 EndX = Coordinates.GetX() + SizeX;
	const int32 EndY = Coordinates.GetY() + SizeY;

	const int32 HalfSizeX = FMath::FloorToInt(float(SizeX) / 2.f);
	const int32 HalfSizeY = FMath::FloorToInt(float(SizeY) / 2.f);
	
	static auto IncrementShift = [](int32& ShiftValue, bool bIsEdge)
	{
		ShiftValue += bIsEdge ? 2 : 1;
	};

	const int32 GridSizeY = RowList.Num();
	int32 IndexX;
	int32 IndexY;
	for (IndexY = StartY; IndexY < EndY; IndexY++)
	{
		//If this Y is out of bounds, then score all of this row's values as failed.
		if(!RowList.IsValidIndex(IndexY))
		{
			for (IndexX = StartX; IndexX < EndX; IndexX++)
			{
				if (IndexX < StartX + HalfSizeX)
				{
					if (IsDebugDrawPlacementEnabled())
					{
						DrawDebugBox(World, GetCenteredWorldPosition({ IndexX, IndexY }), FVector(50.f), FColor::Yellow, false, 0.1f, 0, 2.f);
					}
					IncrementShift(DesiredRightShift, IndexX == StartX);
				}
				else if (IndexX > EndX - HalfSizeX - 1)
				{
					if (IsDebugDrawPlacementEnabled())
					{
						DrawDebugBox(World, GetCenteredWorldPosition({ IndexX, IndexY }), FVector(49.f), FColor::Orange, false, 0.1f, 0, 2.f);
					}
					IncrementShift(DesiredLeftShift, IndexX == EndX - 1);
				}

				if (IndexY < StartY + HalfSizeY)
				{
					if (IsDebugDrawPlacementEnabled())
					{
						DrawDebugBox(World, GetCenteredWorldPosition({ IndexX, IndexY }), FVector(51.f), FColor::Green, false, 0.1f, 0, 2.f);
					}
					IncrementShift(DesiredUpShift, IndexY == StartY);
				}
				else if (IndexY > EndY - HalfSizeY - 1)
				{
					if (IsDebugDrawPlacementEnabled())
					{
						DrawDebugBox(World, GetCenteredWorldPosition({ IndexX, IndexY }), FVector(47.f), FColor::Red, false, 0.1f, 0, 2.f);
					}
					IncrementShift(DesiredDownShift, IndexY == EndY - 1);
				}
			}
			continue;
		}


		const TArray<FPlacementPoint>& Row = RowList[IndexY].GetRow();
		const int32 RowSize = Row.Num();
		for (IndexX = StartX; IndexX < EndX; IndexX++)
		{
			//If this index can fit in the current row, check that point's validity. Otherwise, autofail.
			if (Row.IsValidIndex(IndexX))
			{
				const FPlacementPoint& Point = Row[IndexX];

				if (Point.IsValid() && (!bMustBeEmpty || !Point.IsOccupied()))
				{
					bAllPointsOccupied = false;
					continue;
				}
			}

			if (IndexX < StartX + HalfSizeX)
			{
				if (IsDebugDrawPlacementEnabled())
				{
					DrawDebugBox(World, GetCenteredWorldPosition({ IndexX, IndexY }), FVector(50.f), FColor::Yellow, false, 0.1f, 0, 2.f);
				}
				IncrementShift(DesiredRightShift, IndexX == StartX);
			}
			else if(IndexX > EndX - HalfSizeX - 1)
			{
				if (IsDebugDrawPlacementEnabled())
				{
					DrawDebugBox(World, GetCenteredWorldPosition({ IndexX, IndexY }), FVector(49.f), FColor::Orange, false, 0.1f, 0, 2.f);
				}
				IncrementShift(DesiredLeftShift, IndexX == EndX - 1);
			}

			if (IndexY < StartY + HalfSizeY)
			{
				if (IsDebugDrawPlacementEnabled())
				{
					DrawDebugBox(World, GetCenteredWorldPosition({ IndexX, IndexY }), FVector(51.f), FColor::Green, false, 0.1f, 0, 2.f);
				}
				IncrementShift(DesiredUpShift, IndexY == StartY);
			}
			else if (IndexY > EndY - HalfSizeY - 1)
			{
				if (IsDebugDrawPlacementEnabled())
				{
					DrawDebugBox(World, GetCenteredWorldPosition({ IndexX, IndexY }), FVector(47.f), FColor::Red, false, 0.1f, 0, 2.f);
				}
				IncrementShift(DesiredDownShift, IndexY == EndY - 1);
			}
		}
	}
	
	//Uh oh.
	if (bAllPointsOccupied || (DesiredLeftShift == DesiredRightShift && DesiredUpShift == DesiredDownShift && DesiredUpShift == DesiredLeftShift))
	{
		return FPlacementCoordinates();
	}

	struct FPlacementDirectionSort
	{
		FPlacementDirectionSort(EPlacementDirection InDirection, int32 InWeight)
			: Direction(InDirection), Weight(InWeight) {}

		EPlacementDirection Direction;
		int32 Weight;

		bool operator < (const FPlacementDirectionSort& Other) const { return Weight < Other.Weight; }
	};

	TArray<FPlacementDirectionSort> DirectionPriority;
	DirectionPriority.Reserve(4);
	DirectionPriority.Add({ EPlacementDirection::Up, DesiredUpShift });
	DirectionPriority.Add({ EPlacementDirection::Down, DesiredDownShift });
	DirectionPriority.Add({ EPlacementDirection::Left, DesiredLeftShift });
	DirectionPriority.Add({ EPlacementDirection::Right, DesiredRightShift });
	DirectionPriority.Sort();


	static FPlacementCoordinates OffsetForDirection[4] = {
		{0, 1},
		{0, -1},
		{-1, 0},
		{1, 0}
	};

	FPlacementCoordinates Offset = OffsetForDirection[static_cast<uint8>(DirectionPriority[3].Direction)];

	if (DirectionPriority[3].Weight == DirectionPriority[2].Weight)
	{
		Offset += OffsetForDirection[static_cast<uint8>(DirectionPriority[2].Direction)];
	}

	return Offset;
}

bool FPlacementGrid::IsValidPlacement(const FPlacementCoordinates& Coordinates, int32 SizeX, int32 SizeY, bool bMustBeEmpty) const
{
	if (!IsValid() || !Coordinates.IsValid())
	{
		return false;
	}

	const int32 StartX = Coordinates.GetX();
	const int32 StartY = Coordinates.GetY();
	if (StartX < 0 || StartY < 0)
	{
		return false;
	}

	const int32 EndX = Coordinates.GetX() + SizeX;
	const int32 EndY = Coordinates.GetY() + SizeY;
	if (EndX - 1 >= GetSizeX() || EndY - 1 >= GetSizeY())
	{
		return false;
	}

	int32 IndexX;
	int32 IndexY;
	for (IndexY = Coordinates.GetY(); IndexY < EndY; IndexY++)
	{
		const TArray<FPlacementPoint>& Row = RowList[IndexY].GetRow();
		
		//If this row can't even contain EndX, then this placement is not possible (don't bother checking the whole array).
		if (Row.Num() <= EndX - 1)
		{
			return false;
		}

		for (IndexX = StartX; IndexX < EndX; IndexX++)
		{
			const FPlacementPoint& Point = Row[IndexX];
			if (!Point.IsValid() || (bMustBeEmpty && Point.IsOccupied()))
			{
				return false;
			}
		}
	}

	return true;
}

bool FPlacementGrid::AdjustAnchorToValidPoint(UWorld* World, FPlacementCoordinates& Coordinates, EPlacementAnchor AnchorType, int32 HalfSizeX, int32 HalfSizeY, bool bMustBeEmpty) const
{
	FColor AnchorColorTest;
	switch(AnchorType)
	{
	case EPlacementAnchor::BottomLeft:
		AnchorColorTest = FColor::Cyan;
		break;
	case EPlacementAnchor::BottomRight:
		AnchorColorTest = FColor::Turquoise;
		break;
	case EPlacementAnchor::TopLeft:
		AnchorColorTest = FColor::Magenta;
		break;
	case EPlacementAnchor::TopRight:
		AnchorColorTest = FColor::Blue;
		break;
	}	

	if (IsDebugDrawPlacementEnabled())
	{
		DrawDebugBox(World, GetCenteredWorldPosition(Coordinates), FVector(32.f), AnchorColorTest, false, 0.1f, 0, 2.f);
	}
	
	const TArray<TArray<EPlacementDirection>> AnchorTestBiasOrderList = {
		{ EPlacementDirection::Up, EPlacementDirection::Right },		//EPlacementAnchor::BottomLeft
		{ EPlacementDirection::Down, EPlacementDirection::Right },	//EPlacementAnchor::TopLeft
		{ EPlacementDirection::Down, EPlacementDirection::Left },		//EPlacementAnchor::TopRight
		{ EPlacementDirection::Up, EPlacementDirection::Left }		//EPlacementAnchor::BottomRight
	};

	const EPlacementDirection XBias = AnchorTestBiasOrderList[static_cast<uint8>(AnchorType)][1];
	const EPlacementDirection YBias = AnchorTestBiasOrderList[static_cast<uint8>(AnchorType)][0];

	const TArray<FPlacementCoordinates> AdjustmentList = {
		FPlacementCoordinates(0, 1),		//EPlacementDirection::Up
		FPlacementCoordinates(0, -1),		//EPlacementDirection::Down
		FPlacementCoordinates(-1, 0),		//EPlacementDirection::Left
		FPlacementCoordinates(1, 0)			//EPlacementDirection::Right
	};

	const FPlacementCoordinates& XAdjustment = AdjustmentList[static_cast<uint8>(XBias)];
	const FPlacementCoordinates& YAdjustment = AdjustmentList[static_cast<uint8>(YBias)];
	
	int32 XWeight, YWeight;
	int32 XTestCount, YTestCount;

	int32 RemaningXAdjustmentCount = HalfSizeX;
	int32 RemaningYAdjustmentCount = HalfSizeY;
	const int32 FullTestSizeX = (HalfSizeX * 2) - 1;
	const int32 FullTestSizeY = (HalfSizeY * 2) - 1;
	HalfSizeX--;
	HalfSizeY--;
	int32 TestSizeX = HalfSizeX;
	int32 TestSizeY = HalfSizeY;


	int32 YAdjustmentCount = 0;

	FPlacementCoordinates TestCoordinates;

	while (RemaningXAdjustmentCount > 0 && RemaningYAdjustmentCount > 0)
	{
		TestSizeX = HalfSizeX;
		TestSizeY = HalfSizeY;
		XWeight = 0;
		YWeight = 0;

		if (!IsValidPoint(Coordinates, bMustBeEmpty))
		{
			XWeight++;
			YWeight++;
		}

		if (IsDebugDrawPlacementEnabled())
		{
			DrawDebugBox(World, GetCenteredWorldPosition(Coordinates), FVector(8.f), AnchorColorTest, false, 0.1f, 0, 2.f);
		}
		TestCoordinates = Coordinates;
		for (XTestCount = 0; XTestCount < TestSizeX; XTestCount++)
		{
			TestCoordinates += XAdjustment;
			if (!IsValidPoint(TestCoordinates, bMustBeEmpty))
			{
				if (IsDebugDrawPlacementEnabled())
				{
					DrawDebugBox(World, GetCenteredWorldPosition(TestCoordinates), FVector(8.f), AnchorColorTest, false, 0.1f, 0, 2.f);
				}
				XWeight++;

				if (XWeight >= TestSizeX && TestSizeY != FullTestSizeX)
				{
					TestSizeX = FullTestSizeX;
					TestSizeY = FullTestSizeY;
				}
			}
		}

		TestCoordinates = Coordinates;
		for (YTestCount = 0; YTestCount < TestSizeY; YTestCount++)
		{
			TestCoordinates += YAdjustment;
			if (!IsValidPoint(TestCoordinates, bMustBeEmpty))
			{
				if (IsDebugDrawPlacementEnabled())
				{
					DrawDebugBox(World, GetCenteredWorldPosition(TestCoordinates), FVector(8.f), AnchorColorTest, false, 0.1f, 0, 2.f);
				}
				YWeight++;

				if (YWeight >= TestSizeY && TestSizeY != FullTestSizeY)
				{
					TestSizeX = FullTestSizeX;
					TestSizeY = FullTestSizeY;
				}
			}
		}
		
		TestCoordinates = Coordinates;
		//If Y test caused an expanded test, do the rest of X now.
		for (; XTestCount < TestSizeX; XTestCount++)
		{
			TestCoordinates += XAdjustment;
			if (!IsValidPoint(TestCoordinates, bMustBeEmpty))
			{
				if (IsDebugDrawPlacementEnabled())
				{
					DrawDebugBox(World, GetCenteredWorldPosition(TestCoordinates), FVector(8.f), AnchorColorTest, false, 0.1f, 0, 2.f);
				}
				XWeight++;
			}
		}

		if (XWeight == 0 && YWeight == 0)
		{
			break;
		}

		if (XWeight > YWeight && RemaningYAdjustmentCount > 0)
		{
			Coordinates += YAdjustment;
			RemaningYAdjustmentCount--;
		}
		else if(YWeight > XWeight && RemaningXAdjustmentCount > 0)
		{
			Coordinates += XAdjustment;
			RemaningXAdjustmentCount--;
		}
		else
		{
			if (RemaningXAdjustmentCount > 0)
			{
				Coordinates += XAdjustment;
			}

			if (RemaningYAdjustmentCount > 0)
			{
				Coordinates += YAdjustment;
			}

			RemaningXAdjustmentCount--;
			RemaningYAdjustmentCount--;
		}
	}

	if (IsDebugDrawPlacementEnabled())
	{
		DrawDebugBox(World, GetCenteredWorldPosition(Coordinates), FVector(24.f), FColor::Green, false, 0.1f, 0, 2.f);
	}
	return IsValidPoint(Coordinates, bMustBeEmpty);
}

bool FPlacementGrid::CanMergeWith(const FPlacementGrid& InGrid) const
{
	const FVector InRelativeGridLocation = RootTransform.InverseTransformPosition(InGrid.RootTransform.GetLocation());
	
	if (!FMath::IsNearlyZero(InRelativeGridLocation.X, 0.5f))
	{
		return false;
	}

	const float GridXDistance = InRelativeGridLocation.Y / TrapGridSize;
	const float GridYDistance = InRelativeGridLocation.Z / TrapGridSize;
	const float XGridDiff = GridXDistance - float(FMath::RoundToFloat(GridXDistance));
	const float YGridDiff = GridYDistance - float(FMath::RoundToFloat(GridYDistance));

	if (!FMath::IsNearlyZero(XGridDiff, 0.01f))
	{
		return false;
	}

	if (!FMath::IsNearlyZero(YGridDiff, 0.01f))
	{
		return false;
	}

	return true;
}

bool FPlacementGrid::Append(const FPlacementGrid& InGrid)
{
	if (!CanMergeWith(InGrid))
	{
		return false;
	}

	const FVector InGridRelativeLocation = RootTransform.InverseTransformPosition(InGrid.RootTransform.GetLocation());

	const int32 InGridSizeY = InGrid.GetSizeY();
	const int32 InGridSizeX = InGrid.GetSizeX();

	int32 IndexX = 0;
	FPlacementCoordinates Coordinates;
	for (int32 IndexY = 0; IndexY < InGridSizeY; IndexY++)
	{
		for (IndexX = 0; IndexX < InGridSizeX; IndexX++)
		{
			Coordinates = FPlacementCoordinates(IndexX, IndexY);
			if (!InGrid.Contains(Coordinates))
			{
				continue;
			}

			AddPointFromWorldPosition(InGrid.GetWorldPosition(Coordinates));
		}
	}

	RecalculateHandleMap();
	return true;
}

void FPlacementGrid::RecalculateHandleMap()
{
	PlacementHandleMap.Reset();

	const int32 SizeY = GetSizeY();

	int32 IndexX = 0;
	for (int32 IndexY = 0; IndexY < SizeY; IndexY++)
	{
		const FPlacementGridRow& Row = RowList[IndexY];
		const int32 SizeX = Row.Num();
		CachedSizeX = SizeX > CachedSizeX ? SizeX : CachedSizeX;
		for (IndexX = 0; IndexX < SizeX; IndexX++)
		{
			const FPlacementPoint& Point = Get(IndexX, IndexY);
			if (!Point.IsValid())
			{
				continue;
			}

			PlacementHandleMap.Add(Point.GetHandle()) = FPlacementCoordinates(IndexX, IndexY);
		}
	}
}

void FPlacementGrid::Reset()
{
	RowList.Reset();
}
