#pragma once

#include "CoreMinimal.h"
#include "Async/AsyncWork.h"
#include "MeshMergeTypes.generated.h"

class UCustomizationAsset;

DECLARE_DELEGATE(FCustomizationMergeRequestDelegate);

USTRUCT()
struct FMeshList
{
	GENERATED_USTRUCT_BODY()

	FMeshList() {}

	FMeshList(const TArray<TSoftObjectPtr<USkeletalMesh>>& InMeshList)
	{
		MeshList = InMeshList;
	}
	
public:
	bool operator==(const FMeshList& Other) const;
	bool operator!=(const FMeshList& Other) const;

	const TArray<TSoftObjectPtr<USkeletalMesh>>& GetMeshList() const { return MeshList; }

protected:
	UPROPERTY(Transient)
	TArray<TSoftObjectPtr<USkeletalMesh>> MeshList = TArray<TSoftObjectPtr<USkeletalMesh>>();
};

USTRUCT()
struct FMeshMergeHandle
{
	GENERATED_USTRUCT_BODY()

public:
	FMeshMergeHandle()
	{
		Handle = MAX_uint64;
	}

	FORCEINLINE bool operator== (const FMeshMergeHandle& InData) const { return Handle == InData.Handle; }
	FORCEINLINE bool operator!= (const FMeshMergeHandle& InData) const { return Handle != InData.Handle; }
	FORCEINLINE bool operator== (uint64 InID) const { return this->Handle == InID; }
	FORCEINLINE bool operator!= (uint64 InID) const { return this->Handle != InID; }

	bool IsValid() const { return Handle != MAX_uint64; }

	static FMeshMergeHandle GenerateHandle()
	{
		if (++FMeshMergeHandle::HandleIDCounter == MAX_uint64)
		{
			FMeshMergeHandle::HandleIDCounter++;
		}

		return FMeshMergeHandle(FMeshMergeHandle::HandleIDCounter);
	}

	FORCEINLINE friend uint32 GetTypeHash(FMeshMergeHandle Other)
	{
		return GetTypeHash(Other.Handle);
	}

	friend FArchive& operator<<(FArchive& Ar, FMeshMergeHandle& MeshMergeHandle)
	{
		Ar << MeshMergeHandle.Handle;
		return Ar;
	}

protected:
	FMeshMergeHandle(uint64 InHandle) { Handle = InHandle; }

	UPROPERTY()
	uint64 Handle = MAX_uint64;

	static uint64 HandleIDCounter;
};

USTRUCT()
struct FMergeRequest : public FMeshList
{
	GENERATED_USTRUCT_BODY()

public:
	FMergeRequest() 
		: FMeshList() {}

	FMergeRequest(const FMeshList& InMeshList)
		: FMeshList(InMeshList)
	{
		RequestHandle = FMeshMergeHandle::GenerateHandle();
	}

	FORCEINLINE FCustomizationMergeRequestDelegate& AddDelegate(FCustomizationMergeRequestDelegate&& Delegate)
	{
		return DelegateList.Add_GetRef(MoveTempIfPossible(Delegate));
	}

	FORCEINLINE bool IsValid() const { return RequestHandle.IsValid() && MeshList.Num() != 0; }

	FORCEINLINE bool HasDelegates() const
	{
		for (const FCustomizationMergeRequestDelegate& Delegate : DelegateList)
		{
			if (Delegate.IsBound())
			{
				return true;
			}
		}

		return false;
	}

	void TransferDelegatesFrom(FMergeRequest& Target)
	{
		DelegateList = MoveTempIfPossible(Target.DelegateList);
	}

	void BroadcastComplete()
	{
		for (FCustomizationMergeRequestDelegate& Delegate : DelegateList)
		{
			Delegate.ExecuteIfBound();
		}
	}

	bool HasFailed() const { return bFailed; }
	bool HasCompleted() const { return bCompleted; }
	void MarkFailed() { bFailed = true; }
	void MarkCompleted() { bCompleted = true; }

	FMeshMergeHandle GetHandle() const { return RequestHandle; }

protected:
	TArray<FCustomizationMergeRequestDelegate> DelegateList = TArray<FCustomizationMergeRequestDelegate>();

	bool bCompleted = false;
	bool bFailed = false;

	UPROPERTY(Transient)
	FMeshMergeHandle RequestHandle = FMeshMergeHandle();
};

USTRUCT()
struct FPendingMergeRequest : public FMergeRequest
{
	GENERATED_USTRUCT_BODY()
		
	FPendingMergeRequest()
		: FMergeRequest() {}

	FPendingMergeRequest(FMergeRequest&& PendingMerge)
		: FMergeRequest(MoveTempIfPossible(PendingMerge))
	{

	}

};

DECLARE_STATS_GROUP(TEXT("Mesh Merger"), STATGROUP_MeshMerger, STATCAT_Advanced);
class FMeshMergerWorker : public FNonAbandonableTask
{
public:
	FMeshMergerWorker(FMeshMergeHandle InHandle, TSharedPtr<USkeletalMesh> InTargetMesh, const TArray<USkeletalMesh*>& InSourceMeshList)
		: Handle(InHandle),
		TargetMesh(InTargetMesh),
		SourceMeshList(InSourceMeshList)
	{

	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FMeshMergerWorker, STATGROUP_MeshMerger);
	}

	void DoWork();

	FMeshMergeHandle Handle = FMeshMergeHandle();
	TSharedPtr<USkeletalMesh> TargetMesh = nullptr;
	TArray<USkeletalMesh*> SourceMeshList = TArray<USkeletalMesh*>();
};
typedef FAsyncTask<FMeshMergerWorker> FMeshMergerTask;

USTRUCT()
struct FActiveMergeRequest : public FMergeRequest
{
	GENERATED_USTRUCT_BODY()

	FActiveMergeRequest() 
		: FMergeRequest() {}

	FActiveMergeRequest(FMergeRequest&& PendingMerge)
		: FMergeRequest(MoveTempIfPossible(PendingMerge))
	{
		TargetMesh = NewObject<USkeletalMesh>();
	}

	USkeletalMesh* GetTargetMesh() const { return TargetMesh; }

protected:
	UPROPERTY(Transient)
	USkeletalMesh* TargetMesh = nullptr;
};

USTRUCT()
struct FMergedMesh : public FMeshList
{
	GENERATED_USTRUCT_BODY()
		
	FMergedMesh()
		: FMeshList(){}

	FMergedMesh(const FActiveMergeRequest& ActiveMergeRequest)
	{
		MeshList = ActiveMergeRequest.GetMeshList();
		TargetMesh = ActiveMergeRequest.GetTargetMesh();
	}

public:
	bool IsValid() const { return TargetMesh != nullptr; }

	USkeletalMesh* GetSkeletalMesh() const { return TargetMesh; }

protected:
	UPROPERTY(Transient)
	USkeletalMesh* TargetMesh = nullptr;
};

UCLASS()
class NAUSEA_API UCustomizationMergeManager : public UObject
{
	GENERATED_BODY()

public:
	static FMeshMergeHandle RequestMeshMerge(const UObject* WorldContextObject, const FMeshList& MeshList, FCustomizationMergeRequestDelegate&& Delegate);

	static bool IsMeshMergeComplete(const UObject* WorldContextObject, FMeshMergeHandle Handle);

	static FMergedMesh GetMergedMesh(const UObject* WorldContextObject, FMeshMergeHandle Handle);

protected:
	FActiveMergeRequest& StartMeshMerge(const FMeshList& MeshList);
	void LoadMeshMergeAssets(FActiveMergeRequest& Request);
	void PerformMeshMerge(FActiveMergeRequest& Request, TArray<USkeletalMesh*> SkeletalMeshList);

	FPendingMergeRequest& AddPendingMeshMerge(const FMeshList& MeshList, FCustomizationMergeRequestDelegate&& Delegate);
	void StartNextPendingMeshMerge();

	void MeshMergeComplete(FActiveMergeRequest& Request);

	FActiveMergeRequest& GetActiveMergeRequest(FMeshMergeHandle Handle);

	FMeshMergeHandle GetMergedMeshFromCustomizationSet(const FMeshList& InMeshList) const;
	FMeshMergeHandle GetActiveMergeRequestFromCustomizationSet(const FMeshList& InMeshList) const;
	FMeshMergeHandle GetPendingMergeRequestFromCustomizationSet(const FMeshList& InMeshList) const;

protected:
	//Array of merges that are pending.
	UPROPERTY(Transient)
	TArray<FPendingMergeRequest> PendingMergeList = TArray<FPendingMergeRequest>();

	//Array of merges that are currently occurring.
	UPROPERTY(Transient)
	TArray<FActiveMergeRequest> ActiveMergeList = TArray<FActiveMergeRequest>();

	//Map of completed merges and keyed to their request handles.
	UPROPERTY(Transient)
	TMap<FMeshMergeHandle, FMergedMesh> MergedMeshMap = TMap<FMeshMergeHandle, FMergedMesh>();

	//Keeps reference counters to assets that we're merging to prevent them from being streamed out during merge.
	UPROPERTY(Transient)
	TSet<UObject*> LoadedAsssetList = TSet<UObject*>();
};