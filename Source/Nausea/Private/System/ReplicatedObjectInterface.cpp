// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "System/ReplicatedObjectInterface.h"
#include "Engine/ActorChannel.h"

UReplicatedObjectInterface::UReplicatedObjectInterface(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

bool IReplicatedObjectInterface::ShouldSkipReplication(UActorChannel* OwnerActorChannel, FReplicationFlags* OwnerRepFlags) const
{
	if (OwnerRepFlags)
	{
		switch (ReplicatedObjectInterfaceSkipReplicationLogic)
		{
		case ESkipReplicationLogic::SkipOwnerInitialOnNonNetOwner:
			if (OwnerRepFlags->bNetOwner) { break; }
		case ESkipReplicationLogic::SkipOwnerInitial:
			if (OwnerRepFlags->bNetInitial) { return true; }
		default:
			break;
		}
	}

	return false;
}

void IReplicatedObjectInterface::SetSkipReplicationLogic(ESkipReplicationLogic Logic)
{
	ReplicatedObjectInterfaceSkipReplicationLogic = Logic;
}

bool IReplicatedObjectInterface::HasPendingReplicatedSubobjects(UActorChannel* OwnerActorChannel) const
{
	if (ReplicatedObjectInterfaceSubobjectList.Num() == 0)
	{
		return false;
	}

	for (TWeakObjectPtr<UObject> WeakObject : ReplicatedObjectInterfaceSubobjectList)
	{
		if (WeakObject.IsValid() && !OwnerActorChannel->ReplicationMap.Contains(WeakObject.Get()))
		{
			return true;
		}

		IReplicatedObjectInterface* SubobjectReplicatedObjectInterface = Cast<IReplicatedObjectInterface>(WeakObject.Get());

		if (SubobjectReplicatedObjectInterface && SubobjectReplicatedObjectInterface->HasPendingReplicatedSubobjects(OwnerActorChannel))
		{
			return true;
		}
	}

	return false;
}

bool IReplicatedObjectInterface::ReplicateSubobjectList(UActorChannel* OwnerActorChannel, FOutBunch* Bunch, FReplicationFlags* OwnerRepFlags)
{
	bool bWroteSomething = false;
	const bool bOwnerCachedNetInitial = OwnerRepFlags->bNetInitial;
	
	OwnerRepFlags->bNetInitial = OwnerActorChannel->ReplicationMap.Find(CastChecked<UObject>(this)) == nullptr;

	for (TWeakObjectPtr<UObject> WeakSubobject : ReplicatedObjectInterfaceSubobjectList)
	{
		UObject* Subobject = WeakSubobject.Get();

		if (!Subobject)
		{
			continue;
		}

		IReplicatedObjectInterface* ReplicatedObjectInterface = Cast<IReplicatedObjectInterface>(Subobject);

		if (ReplicatedObjectInterface && ReplicatedObjectInterface->ShouldSkipReplication(OwnerActorChannel, OwnerRepFlags))
		{
			continue;
		}
		
		if (ReplicatedObjectInterface)
		{
			bWroteSomething = ReplicatedObjectInterface->ReplicateSubobjectList(OwnerActorChannel, Bunch, OwnerRepFlags) || bWroteSomething;
		}

		bWroteSomething = OwnerActorChannel->ReplicateSubobject(Subobject, *Bunch, *OwnerRepFlags) || bWroteSomething;
	}

	OwnerRepFlags->bNetInitial = bOwnerCachedNetInitial;

	return bWroteSomething;
}

bool IReplicatedObjectInterface::RegisterReplicatedSubobject(UObject* Subobject)
{
	if (!Subobject)
	{
		return false;
	}

	if (ReplicatedObjectInterfaceSubobjectList.Contains(Subobject))
	{
		return true;
	}

	if (!Subobject->IsInOuter(CastChecked<UObject>(this)->GetTypedOuter<AActor>()))
	{
		return false;
	}

	ReplicatedObjectInterfaceSubobjectList.Add(Subobject);
	return true;
}

bool IReplicatedObjectInterface::UnregisterReplicatedSubobject(UObject* Subobject)
{
	ReplicatedObjectInterfaceSubobjectList.Remove(Subobject);
	ReplicatedObjectInterfaceSubobjectList.Remove(nullptr);
	return true;
}

bool IReplicatedObjectInterface::ClearReplicatedSubobjectList()
{
	ReplicatedObjectInterfaceSubobjectList.Reset();
	return true;
}