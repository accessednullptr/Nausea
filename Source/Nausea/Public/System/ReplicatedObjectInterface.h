// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ReplicatedObjectInterface.generated.h"

class UActorChannel;

UENUM(BlueprintType)
enum class ESkipReplicationLogic : uint8
{
	None, //Never skip replicating this subobject.
	SkipOwnerInitial, //Skip replicating this subobject if we're in the owner's initial bunch.
	SkipOwnerInitialOnNonNetOwner //Skip replicating this subobject if we're in the owner's initial bunch ONLY if we're not the NetOwner (bNetOwner in RepFlags) of this actor.
};

UINTERFACE(MinimalAPI)
class UReplicatedObjectInterface : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

/**
 * 
 */
class NAUSEA_API IReplicatedObjectInterface
{
	GENERATED_IINTERFACE_BODY()

public:
	FORCEINLINE bool ShouldSkipReplication(UActorChannel* OwnerActorChannel, FReplicationFlags* OwnerRepFlags) const;
	FORCEINLINE void SetSkipReplicationLogic(ESkipReplicationLogic Logic);
	
	FORCEINLINE bool HasPendingReplicatedSubobjects(UActorChannel* OwnerActorChannel) const;
	bool ReplicateSubobjectList(UActorChannel* OwnerActorChannel, class FOutBunch* Bunch, FReplicationFlags* RepFlags);

	FORCEINLINE bool RegisterReplicatedSubobject(UObject* Subobject);

	template<class T>
	bool RegisterReplicatedSubobjects(TArray<T*> SubobjectList)
	{
		static_assert(TPointerIsConvertibleFromTo<T, const UObject>::Value, "'T' template parameter to RegisterReplicatedSubobjects must be derived from UObject");
		bool bRegisteredAll = SubobjectList.Num() > 0;
		for (T* Object : SubobjectList)
		{
			bRegisteredAll = RegisterReplicatedSubobject(Object) && bRegisteredAll;
		}

		return bRegisteredAll;
	}

	FORCEINLINE bool UnregisterReplicatedSubobject(UObject* Subobject);
	FORCEINLINE bool ClearReplicatedSubobjectList();
	

private:
	ESkipReplicationLogic ReplicatedObjectInterfaceSkipReplicationLogic = ESkipReplicationLogic::None;
	TArray<TWeakObjectPtr<UObject>> ReplicatedObjectInterfaceSubobjectList = TArray<TWeakObjectPtr<UObject>>();
};
