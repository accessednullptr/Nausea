
#include "System/MeshMergeTypes.h"
#include "Async/Async.h"
#include "Engine/AssetManager.h"
#include "SkeletalMeshMerge.h"
#include "Engine/SkeletalMesh.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "Animation/Skeleton.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Public/SkeletalMeshMerge.h"
#include "System/NauseaGameInstance.h"
#include "Character/Customization/CustomizationObject.h"

#if WITH_EDITOR
#include "Editor.h"
extern UNREALED_API UEditorEngine* GEditor;
#endif

uint64 FMeshMergeHandle::HandleIDCounter = MAX_uint64;

bool FMeshList::operator==(const FMeshList& Other) const
{
	return MeshList == Other.MeshList;
}

bool FMeshList::operator!=(const FMeshList& Other) const
{
	return MeshList != Other.MeshList;
}

FMeshMergeHandle UCustomizationMergeManager::RequestMeshMerge(const UObject* WorldContextObject, const FMeshList& MeshList, FCustomizationMergeRequestDelegate&& Delegate)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	if (!World || World->IsNetMode(NM_DedicatedServer))
	{
		return FMeshMergeHandle();
	}

	UNauseaGameInstance* NauseaGameInstance = World->GetGameInstance<UNauseaGameInstance>();
	if (!NauseaGameInstance)
	{
		return FMeshMergeHandle();
	}

	UCustomizationMergeManager* Manager = NauseaGameInstance->GetCustomizationManager();
	if (!Manager)
	{
		return FMeshMergeHandle();
	}

	FMeshMergeHandle ExistingMergeHandle = Manager->GetMergedMeshFromCustomizationSet(MeshList);

	if (ExistingMergeHandle.IsValid())
	{
		return ExistingMergeHandle;
	}

	ExistingMergeHandle = Manager->GetActiveMergeRequestFromCustomizationSet(MeshList);

	if (ExistingMergeHandle.IsValid())
	{
		return ExistingMergeHandle;
	}

	ExistingMergeHandle = Manager->GetPendingMergeRequestFromCustomizationSet(MeshList);

	if (ExistingMergeHandle.IsValid())
	{
		return ExistingMergeHandle;
	}

	if (Manager->ActiveMergeList.Num() > 0)
	{
		FPendingMergeRequest& Request = Manager->AddPendingMeshMerge(MeshList, MoveTempIfPossible(Delegate));

		if (!Request.IsValid())
		{
			return FMeshMergeHandle();
		}

		return Request.GetHandle();
	}

	FActiveMergeRequest& Request = Manager->StartMeshMerge(MeshList);

	if (!Request.IsValid())
	{
		return FMeshMergeHandle();
	}

	Request.AddDelegate(MoveTempIfPossible(Delegate));

	return Request.GetHandle();
}

bool UCustomizationMergeManager::IsMeshMergeComplete(const UObject* WorldContextObject, FMeshMergeHandle Handle)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	if (!World || World->IsNetMode(NM_DedicatedServer))
	{
		return false;
	}

	UNauseaGameInstance* NauseaGameInstance = World->GetGameInstance<UNauseaGameInstance>();

	if (!NauseaGameInstance)
	{
		return false;
	}

	UCustomizationMergeManager* Manager = NauseaGameInstance->GetCustomizationManager();
	if (!Manager)
	{
		return false;
	}

	return Manager->MergedMeshMap.Contains(Handle);
}

FMergedMesh UCustomizationMergeManager::GetMergedMesh(const UObject* WorldContextObject, FMeshMergeHandle Handle)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);

	if (!World || World->IsNetMode(NM_DedicatedServer))
	{
		return FMergedMesh();
	}

	UNauseaGameInstance* NauseaGameInstance = World->GetGameInstance<UNauseaGameInstance>();

	if (!NauseaGameInstance)
	{
		return FMergedMesh();
	}

	UCustomizationMergeManager* Manager = NauseaGameInstance->GetCustomizationManager();
	if (!Manager)
	{
		return FMergedMesh();
	}

	if (!Manager->MergedMeshMap.Contains(Handle))
	{
		return FMergedMesh();
	}

	return Manager->MergedMeshMap[Handle];
}

FActiveMergeRequest& UCustomizationMergeManager::StartMeshMerge(const FMeshList& MeshList)
{
	static FActiveMergeRequest InvalidRequest = FActiveMergeRequest();

	FActiveMergeRequest& ActiveMergeRequest = ActiveMergeList.Add_GetRef(FActiveMergeRequest(MoveTempIfPossible(FMergeRequest(MeshList))));
	LoadMeshMergeAssets(ActiveMergeRequest);

	if (ActiveMergeRequest.HasFailed())
	{
		ActiveMergeList.RemoveAt(ActiveMergeList.Num() - 1, 1, false);
		return InvalidRequest;
	}

	return ActiveMergeRequest;
}

void UCustomizationMergeManager::LoadMeshMergeAssets(FActiveMergeRequest& Request)
{
	TWeakObjectPtr<UCustomizationMergeManager> WeakThis = this;
	FMeshMergeHandle RequestHandle = Request.GetHandle();

	TArray<TSoftObjectPtr<USkeletalMesh>> MeshesToLoad = Request.GetMeshList();

	if (MeshesToLoad.Num() == 0)
	{
		Request.MarkFailed();
		return;
	}

	if (!UAssetManager::IsValid())
	{
		Request.MarkFailed();
		return;
	}

	FStreamableManager& StreamableManager = UAssetManager::GetStreamableManager();

	auto OnMeshAsyncLoadComplete = [WeakThis, RequestHandle, MeshesToLoad]()
	{
		if (!WeakThis.IsValid())
		{
			return;
		}

		TArray<USkeletalMesh*> SkeletalMeshList;
		for (TSoftObjectPtr<USkeletalMesh> SoftSkeletalMesh : MeshesToLoad)
		{
			SkeletalMeshList.Add(SoftSkeletalMesh.Get());
		}

		for (USkeletalMesh* LoadedMesh : SkeletalMeshList)
		{
			WeakThis->LoadedAsssetList.Add(LoadedMesh);
		}
		
		WeakThis->PerformMeshMerge(WeakThis->GetActiveMergeRequest(RequestHandle), SkeletalMeshList);
	};

	TArray<FSoftObjectPath> TargetsToStream;
	for (const TSoftObjectPtr<USkeletalMesh>& Mesh : MeshesToLoad)
	{
		TargetsToStream.Add(Mesh.ToSoftObjectPath());
	}

	TSharedPtr<FStreamableHandle> Handle = StreamableManager.RequestAsyncLoad(TargetsToStream, FStreamableDelegate::CreateWeakLambda(this, OnMeshAsyncLoadComplete));

	if (Handle->HasLoadCompleted())
	{
		OnMeshAsyncLoadComplete();
	}
}

void UCustomizationMergeManager::PerformMeshMerge(FActiveMergeRequest& Request, TArray<USkeletalMesh*> SkeletalMeshList)
{
	if (!Request.IsValid())
	{
		return;
	}

	TWeakObjectPtr<UCustomizationMergeManager> WeakThis = this;
	FMeshMergeHandle Handle = Request.GetHandle();
	TWeakObjectPtr<USkeletalMesh> TargetMesh = Request.GetTargetMesh();
	TSharedRef<TArray<FSkelMeshMergeSectionMapping>, ESPMode::Fast> SectionMapping = MakeShareable(new TArray<FSkelMeshMergeSectionMapping>());

	TargetMesh->SetSkeleton(SkeletalMeshList[0]->GetSkeleton());
	TargetMesh->SetPhysicsAsset(SkeletalMeshList[0]->GetPhysicsAsset());
	
	AsyncTask(ENamedThreads::GameThread, [WeakThis, Handle, TargetMesh, SkeletalMeshList, SectionMapping]()
	{
		if (!WeakThis.IsValid() || !TargetMesh.IsValid())
		{
			return;
		}

		QUICK_SCOPE_CYCLE_COUNTER(STAT_Customization_PerformMerge);
		FSkeletalMeshMerge SkeletalMeshMerger(TargetMesh.Get(), SkeletalMeshList, SectionMapping.Get(), 0, EMeshBufferAccess::Default, nullptr);

		{
			QUICK_SCOPE_CYCLE_COUNTER(STAT_Customization_MergeSkeleton);
			SkeletalMeshMerger.MergeSkeleton();
		}
		
		{
			QUICK_SCOPE_CYCLE_COUNTER(STAT_Customization_FinalizeMesh);
			SkeletalMeshMerger.FinalizeMesh();
		}

		AsyncTask(ENamedThreads::GameThread, [WeakThis, Handle, TargetMesh]()
			{
				QUICK_SCOPE_CYCLE_COUNTER(STAT_Customization_ApplyMergeMesh);

				if (!WeakThis.IsValid())
				{
					return;
				}

				WeakThis->MeshMergeComplete(WeakThis->GetActiveMergeRequest(Handle));
			});
	});
}

FPendingMergeRequest& UCustomizationMergeManager::AddPendingMeshMerge(const FMeshList& MeshList, FCustomizationMergeRequestDelegate&& Delegate)
{
	FPendingMergeRequest& PendingMergeRequest = PendingMergeList.Add_GetRef(FPendingMergeRequest(MoveTempIfPossible(FMergeRequest(MeshList))));
	PendingMergeRequest.AddDelegate(MoveTempIfPossible(Delegate));
	return PendingMergeRequest;
}

void UCustomizationMergeManager::StartNextPendingMeshMerge()
{
	if (PendingMergeList.Num() == 0)
	{
		return;
	}

	FPendingMergeRequest& PendingMerge = PendingMergeList[0];

	if (!PendingMerge.HasDelegates())
	{
		PendingMergeList.RemoveAt(0, 1, false);
		StartNextPendingMeshMerge();
		return;
	}

	FActiveMergeRequest& ActiveMergeRequest = ActiveMergeList.Add_GetRef(FActiveMergeRequest(MoveTempIfPossible(PendingMerge)));
	LoadMeshMergeAssets(ActiveMergeRequest);

	if (ActiveMergeRequest.HasFailed())
	{
		ActiveMergeRequest.BroadcastComplete();
		ActiveMergeList.RemoveAt(ActiveMergeList.Num() - 1, 1, false);
	}

	PendingMergeList.RemoveAt(0, 1, false);
}

void UCustomizationMergeManager::MeshMergeComplete(FActiveMergeRequest& Request)
{
	if (Request.IsValid())
	{
		MergedMeshMap.Add(Request.GetHandle(), MoveTempIfPossible(FMergedMesh(Request)));

		Request.MarkCompleted();
		Request.BroadcastComplete();

		FMeshMergeHandle Handle = Request.GetHandle();
		ActiveMergeList.RemoveAll([Handle](const FActiveMergeRequest& ActiveRequest)
		{
			return ActiveRequest.GetHandle() == Handle;
		});
	}

	GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UCustomizationMergeManager::StartNextPendingMeshMerge));
}

FActiveMergeRequest& UCustomizationMergeManager::GetActiveMergeRequest(FMeshMergeHandle Handle)
{
	static FActiveMergeRequest InvalidRequest = FActiveMergeRequest();

	for (FActiveMergeRequest& ActiveRequest : ActiveMergeList)
	{
		if (ActiveRequest.GetHandle() == Handle)
		{
			return ActiveRequest;
		}
	}

	return InvalidRequest;
}

FMeshMergeHandle UCustomizationMergeManager::GetMergedMeshFromCustomizationSet(const FMeshList& InMeshList) const
{
	for (const TPair<FMeshMergeHandle, FMergedMesh>& Entry : MergedMeshMap)
	{
		if (Entry.Value == InMeshList)
		{
			return Entry.Key;
		}
	}

	return FMeshMergeHandle();
}

FMeshMergeHandle UCustomizationMergeManager::GetActiveMergeRequestFromCustomizationSet(const FMeshList& InMeshList) const
{
	for (const FPendingMergeRequest& PendingMerge : PendingMergeList)
	{
		if (PendingMerge == InMeshList)
		{
			return PendingMerge.GetHandle();
		}
	}

	return FMeshMergeHandle();
}

FMeshMergeHandle UCustomizationMergeManager::GetPendingMergeRequestFromCustomizationSet(const FMeshList& InMeshList) const
{
	for (const FActiveMergeRequest& ActiveMerge : ActiveMergeList)
	{
		if (ActiveMerge == InMeshList)
		{
			return ActiveMerge.GetHandle();
		}
	}

	return FMeshMergeHandle();
}