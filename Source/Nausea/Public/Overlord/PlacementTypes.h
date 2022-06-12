// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Overlord/TrapTypes.h"
#include "PlacementTypes.generated.h"

UENUM(BlueprintType)
enum class EPlacementResult : uint8
{
	//Success
	Success,
	//Failure Types:
	InvalidLocation, //Could not resolve trap placement.
	InvalidPlacementType, //Trap and Placement Actor PlacementType do not match.
	NotEnoughFunds, //Instigator does not have the funds to place this trap.
	MAX = 255
};

UENUM()
enum class EPlacementDirection : uint8
{
	Up,
	Down,
	Left,
	Right,
	MAX = 255
};

UENUM()
enum class EPlacementAnchor : uint8
{
	BottomLeft,
	TopLeft,
	TopRight,
	BottomRight,
	MAX = 255
};

FORCEINLINE EPlacementAnchor& operator++(EPlacementAnchor& Value, int) {
	Value = static_cast<EPlacementAnchor>((static_cast<uint8>(Value) + 1) % static_cast<uint8>(EPlacementAnchor::MAX));
	return Value;
}

USTRUCT()
struct FPlacementCoordinates
{
	GENERATED_USTRUCT_BODY()

	FPlacementCoordinates() {}

	FPlacementCoordinates(int32 InX, int32 InY)
		: X(InX), Y(InY) {}

	FPlacementCoordinates(const FPlacementCoordinates& InBaseCoordinates, int32 InX, int32 InY)
		: X(InBaseCoordinates.X + InX), Y(InBaseCoordinates.Y + InY) {}

	FORCEINLINE bool operator== (const FPlacementCoordinates& InData) const { return X == InData.X && Y == InData.Y; }
	FORCEINLINE bool operator!= (const FPlacementCoordinates& InData) const { return X != InData.X || Y != InData.Y; }

	FORCEINLINE FPlacementCoordinates operator+ (const FPlacementCoordinates& Other) const
	{
		if (!IsValid()) { return Other; }
		if (!Other.IsValid()) { return *this; }
		return FPlacementCoordinates(X + Other.X, Y + Other.Y);
	}
	FORCEINLINE FPlacementCoordinates operator-() const 
	{
		if (!IsValid()) { return *this; }
		return FPlacementCoordinates(-X, -Y);
	}
	FORCEINLINE FPlacementCoordinates operator- (const FPlacementCoordinates& Other) const
	{
		if (!IsValid()) { return Other; }
		if (!Other.IsValid()) { return *this; }
		return FPlacementCoordinates(X - Other.X, Y - Other.Y);
	}
	FORCEINLINE FPlacementCoordinates operator* (const FPlacementCoordinates& Other) const
	{
		if (!IsValid()) { return Other; }
		if (!Other.IsValid()) { return *this; }
		return FPlacementCoordinates(X * Other.X, Y * Other.Y);
	}
	FORCEINLINE FPlacementCoordinates operator/ (const FPlacementCoordinates& Other) const
	{
		if (!IsValid()) { return Other; }
		if (!Other.IsValid()) { return *this; }
		return FPlacementCoordinates(X / Other.X, Y / Other.Y);
	}

	FORCEINLINE FPlacementCoordinates& operator+= (const FPlacementCoordinates& Other)
	{
		if (!IsValid() || !Other.IsValid()) { return *this; }
		X += Other.X;
		Y += Other.Y;
		return *this;
	}
	FORCEINLINE FPlacementCoordinates& operator-= (const FPlacementCoordinates& Other)
	{
		if (!IsValid() || !Other.IsValid()) { return *this; }
		X -= Other.X;
		Y -= Other.Y;
		return *this;
	}

	FORCEINLINE FPlacementCoordinates operator* (int32 Other) const
	{
		if (!IsValid()) { return FPlacementCoordinates(); }
		return FPlacementCoordinates(X * Other, Y * Other);
	}
	FORCEINLINE FPlacementCoordinates operator/ (int32 Other) const
	{
		if (!IsValid()) { return FPlacementCoordinates(); }
		return FPlacementCoordinates(X / Other, Y / Other);
	}


	FORCEINLINE bool IsValid() const { return X != MAX_int32 && Y != MAX_int32; }
	FORCEINLINE bool IsEqual(int32 InX, int32 InY) const { return X == InX && Y == InY; }

	FORCEINLINE const int32& GetX() const { return X; }
	FORCEINLINE const int32& GetY() const { return Y; }
	FORCEINLINE int32& GetX() { return X; }
	FORCEINLINE int32& GetY() { return Y; }

	FORCEINLINE void SetX(int32 NewX) { X = NewX; }
	FORCEINLINE void SetY(int32 NewY) { Y = NewY; }

	FORCEINLINE friend uint64 GetTypeHash(FPlacementCoordinates Other)
	{
		return GetTypeHash(Other.X + Other.Y);
	}

	FORCEINLINE bool Clamp()
	{
		if (!IsValid())
		{
			return false;
		}

		X = FMath::Max(X, 0);
		Y = FMath::Max(Y, 0);
		return true;
	}

protected:
	UPROPERTY()
	int32 X = MAX_int32;
	UPROPERTY()
	int32 Y = MAX_int32;
};

USTRUCT()
struct FPlacementHandle
{
	GENERATED_USTRUCT_BODY()

public:
	FPlacementHandle() {}

	FORCEINLINE bool operator== (FPlacementHandle InData) const { return Handle == InData.Handle; }
	FORCEINLINE bool operator== (uint64 InID) const { return this->Handle == InID; }

	bool IsValid() const { return Handle != MAX_uint64; }

	static FPlacementHandle GenerateHandle()
	{
		if (++FPlacementHandle::HandleIDCounter == MAX_uint64)
		{
			FPlacementHandle::HandleIDCounter++;
		}

		return FPlacementHandle(FPlacementHandle::HandleIDCounter);
	}

	FORCEINLINE friend uint64 GetTypeHash(FPlacementHandle Other)
	{
		return GetTypeHash(Other.Handle);
	}

protected:
	FPlacementHandle(int64 InHandle) { Handle = InHandle; }

	UPROPERTY()
	uint64 Handle = MAX_uint64;

	static uint64 HandleIDCounter;
};

USTRUCT()
struct FPlacementPoint
{
	GENERATED_USTRUCT_BODY()

public:
	FPlacementPoint() {}

	FPlacementPoint(FPlacementHandle&& InHandle, const FPlacementCoordinates& InPoint)
		: Handle(MoveTemp(InHandle)) {}

	bool IsValid() const { return Handle.IsValid(); }
	bool IsOccupied() const { return Occupant.IsValid(); }
	void SetOccupant(UObject* InOccupant) { Occupant = InOccupant; }

	FPlacementHandle GetHandle() const { return Handle; }

	static FPlacementPoint InvalidPoint;

private:
	UPROPERTY()
	FPlacementHandle Handle = FPlacementHandle();
	UPROPERTY(Transient)
	TWeakObjectPtr<UObject> Occupant = nullptr;
};

USTRUCT()
struct FPlacementGridRow
{
	GENERATED_USTRUCT_BODY()

public:
	FPlacementGridRow() {}

	const TArray<FPlacementPoint>& GetRow() const { return Row; }

	FORCEINLINE bool Contains(int32 X) const
	{
		return Row.IsValidIndex(X) && Row[X].IsValid();
	}

	FORCEINLINE const FPlacementPoint& Get(int32 X) const
	{
		return Row.IsValidIndex(X) ? Row[X] : FPlacementPoint::InvalidPoint;
	}

	FORCEINLINE int32 Num() const
	{
		return Row.Num();
	}

	FORCEINLINE void SetNum(int32 SizeX)
	{
		return Row.SetNum(SizeX);
	}

	FORCEINLINE void Reserve(int32 Count)
	{
		Row.Reserve(Count);
	}

	FORCEINLINE bool SetOccupant(int32 X, UObject* Occupant)
	{
		if (X < 0)
		{
			return false;
		}

		if (Row.Num() <= X)
		{
			return false;
		}

		Row[X].SetOccupant(Occupant);
		return true;
	}

	FORCEINLINE bool Set(int32 X, const FPlacementPoint& InPoint)
	{
		if (X < 0)
		{
			return false;
		}

		if (Row.Num() <= X)
		{
			Row.SetNum(X + 1);
		}

		Row[X] = InPoint;
		return true;
	}

	FORCEINLINE void Insert(FPlacementPoint&& Item, int32 Index)
	{
		Row.Insert(Item, Index);
	}

protected:
	UPROPERTY()
	TArray<FPlacementPoint> Row = TArray<FPlacementPoint>();
};

USTRUCT()
struct FPlacementGrid
{
	GENERATED_USTRUCT_BODY()

public:
	FPlacementGrid() {}

	static bool IsDebugDrawPlacementCVarSet();

	FORCEINLINE bool IsValid() const { return !RootTransform.Equals(FTransform::Identity); }

	FORCEINLINE bool IsDebugDrawPlacementEnabled() const { return bDebugDrawPlacementEnabled; }
	FORCEINLINE void SetDebugDrawPlacementEnabled(bool bInDebugDrawPlacementEnabled) const { bDebugDrawPlacementEnabled = bInDebugDrawPlacementEnabled; }

	//Returns true if the given coordinates refer to a valid (that is to say valid coordinates that contain a valid placement point).
	FORCEINLINE bool Contains(const FPlacementCoordinates& Coordinates) const
	{
		const int32 Y = Coordinates.GetY();
		return RowList.IsValidIndex(Y) && RowList[Y].Contains(Coordinates.GetX());
	}

	FORCEINLINE bool Contains(int32 X, int32 Y) const
	{
		return RowList.IsValidIndex(Y) && RowList[Y].Contains(X);
	}

	//Get our x extent (largest row size).
	FORCEINLINE int32 GetSizeX() const
	{
		if (CachedSizeX > 0)
		{
			return CachedSizeX;
		}

		int32 LargestSize = 0;
		for (const FPlacementGridRow& Row : RowList)
		{
			const int32 RowSize = Row.Num();
			LargestSize = LargestSize < RowSize ? RowSize : LargestSize;
		}

		return LargestSize;
	}

	FORCEINLINE int32 GetSizeY() const
	{
		return RowList.Num();
	}

	FORCEINLINE const FPlacementPoint& Get(const FPlacementCoordinates& Coordinates) const
	{
		const int32 Y = Coordinates.GetY();
		return RowList.IsValidIndex(Y) ? RowList[Y].Get(Coordinates.GetX()) : FPlacementPoint::InvalidPoint;
	}

	FORCEINLINE void SetSize(int32 SizeX, int32 SizeY)
	{
		RowList.SetNum(SizeY);
		for (FPlacementGridRow& Row : RowList)
		{
			Row.SetNum(SizeX);
		}
	}

	FORCEINLINE void SetRootTransform(const FTransform& InRootTransform)
	{
		RootTransform = InRootTransform;
	}

	FORCEINLINE const FTransform& GetRootTransform() const { return RootTransform; }


	FORCEINLINE bool Set(int32 X, int32 Y, const FPlacementPoint& InPoint)
	{
		if (Y >= RowList.Num())
		{
			RowList.SetNum(Y + 1);
		}

		return RowList.IsValidIndex(Y) ? RowList[Y].Set(X, InPoint) : false;
	}

	FORCEINLINE FPlacementPoint Get(int32 X, int32 Y) const
	{
		return RowList.IsValidIndex(Y) ? RowList[Y].Get(X) : FPlacementPoint::InvalidPoint;
	}

	FORCEINLINE bool SetOccupant(int32 X, int32 Y, UObject* InOccupant)
	{
		if (!RowList.IsValidIndex(Y))
		{
			return false;
		}

		return RowList[Y].SetOccupant(X, InOccupant);
	}

	FORCEINLINE bool ClearPlacementOccupantByHandle(FPlacementHandle InHandle)
	{
		if (!InHandle.IsValid() || !PlacementHandleMap.Contains(InHandle))
		{
			return false;
		}

		const FPlacementCoordinates& Coordinates = PlacementHandleMap[InHandle];
		
		if (!RowList.IsValidIndex(Coordinates.GetY()))
		{
			return false;
		}

		return RowList[Coordinates.GetY()].SetOccupant(Coordinates.GetX(), nullptr);
	}

	FORCEINLINE FVector GetCenteredWorldPosition(const FPlacementCoordinates& Coords) const
	{
		const FVector RelativeLocation = FVector(0.f, (Coords.GetX() * TrapGridSize) + (TrapGridSize * 0.5f), (Coords.GetY() * TrapGridSize) + (TrapGridSize * 0.5f));
		return RootTransform.TransformPosition(RelativeLocation);
	}

	FORCEINLINE FVector GetWorldPosition(const FPlacementCoordinates& Coords) const
	{
		const FVector RelativeLocation = FVector(0.f, (Coords.GetX() * TrapGridSize), (Coords.GetY() * TrapGridSize));
		return RootTransform.TransformPosition(RelativeLocation);
	}

	void AddPointFromWorldPosition(const FVector& WorldPosition);

	FORCEINLINE bool IsValidPoint(const FPlacementCoordinates& Coordinates, bool bMustBeEmpty) const
	{
		return Contains(Coordinates) && (!bMustBeEmpty || !Get(Coordinates).IsOccupied());
	}

	FORCEINLINE bool IsValidPoint(const int32 X, const int32 Y, bool bMustBeEmpty) const
	{
		return Contains(X, Y) && (!bMustBeEmpty || !Get(X, Y).IsOccupied());
	}

	bool AdjustCoordinatesToValidPoint(UWorld* World, FPlacementCoordinates& Coordinates, const TArray<EPlacementDirection>& BiasOrder, bool bMustBeEmpty) const;

	FPlacementCoordinates GetCoordinatesFromWorldPosition(UWorld* World, const FVector& WorldPosition, TArray<EPlacementDirection>& BiasOrder) const;

	FPlacementCoordinates GetPlacementCoordinatesForSizeNeo(UWorld* World, const FVector& WorldPosition, int32 SizeX, int32 SizeY, bool bMustBeEmpty) const;

	FPlacementCoordinates GetAdjustmentDirection(UWorld* World, FPlacementCoordinates& Coordinates, int32 SizeX, int32 SizeY, bool bMustBeEmpty) const;
	bool IsValidPlacement(const FPlacementCoordinates& Coordinates, int32 SizeX, int32 SizeY, bool bMustBeEmpty) const;

	bool AdjustAnchorToValidPoint(UWorld* World, FPlacementCoordinates& Coordinates, EPlacementAnchor AnchorType, int32 SizeX, int32 SizeY, bool bMustBeEmpty) const;

	bool CanMergeWith(const FPlacementGrid& InGrid) const;
	bool Append(const FPlacementGrid& InGrid);
	void RecalculateHandleMap();
	void Reset();

protected:
	UPROPERTY()
	TArray<FPlacementGridRow> RowList = TArray<FPlacementGridRow>();

	UPROPERTY()
	TMap<FPlacementHandle, FPlacementCoordinates> PlacementHandleMap = TMap<FPlacementHandle, FPlacementCoordinates>();

	//World-space transform of this grid. Scale should always be one.
	UPROPERTY()
	FTransform RootTransform = FTransform::Identity;

	UPROPERTY()
	int32 CachedSizeX = 0;

	UPROPERTY(Transient)
	mutable bool bDebugDrawPlacementEnabled = false;
};
