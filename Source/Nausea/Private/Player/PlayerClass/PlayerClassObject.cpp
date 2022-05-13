// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Player/PlayerClass/PlayerClassObject.h"
#include "Net/UnrealNetwork.h"
#include "Engine/NetDriver.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerState.h"
#include "Player/PlayerClassComponent.h"

UPlayerClassObject::UPlayerClassObject(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UPlayerClassObject::PostInitProperties()
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		return;
	}
}

void UPlayerClassObject::BeginDestroy()
{
	Super::BeginDestroy();
	OwningPlayerClassComponent = nullptr;
	WorldPrivate = nullptr;
}

int32 UPlayerClassObject::GetFunctionCallspace(UFunction* Function, FFrame* Stack)
{
	check(GetOuter());
	return GetOuter()->GetFunctionCallspace(Function, Stack);
}

bool UPlayerClassObject::CallRemoteFunction(UFunction* Function, void* Parameters, FOutParmRec* OutParms, FFrame* Stack)
{
	bool bProcessed = false;

	if (AActor* MyOwner = GetPlayerClassComponent()->GetOwningPlayerState())
	{
		FWorldContext* const Context = GEngine->GetWorldContextFromWorld(GetWorld());
		if (Context != nullptr)
		{
			for (FNamedNetDriver& Driver : Context->ActiveNetDrivers)
			{
				if (Driver.NetDriver != nullptr && Driver.NetDriver->ShouldReplicateFunction(MyOwner, Function))
				{
					Driver.NetDriver->ProcessRemoteFunction(MyOwner, Function, Parameters, OutParms, Stack, this);
					bProcessed = true;
				}
			}
		}
	}
	return bProcessed;
}

void UPlayerClassObject::Initialize(UPlayerClassComponent* PlayerClassComponent)
{
	if (IsInitialized())
	{
		return;
	}

	OwningPlayerClassComponent = PlayerClassComponent;
	WorldPrivate = OwningPlayerClassComponent->GetWorld();
}

void UPlayerClassObject::EndPlay(EEndPlayReason::Type EndPlayReason)
{

}

UWorld* UPlayerClassObject::GetWorld_Uncached() const
{
	UWorld* ObjectWorld = nullptr;

	const UObject* Owner = GetPlayerClassComponent();

	if (Owner && !Owner->HasAnyFlags(RF_ClassDefaultObject))
	{
		ObjectWorld = Owner->GetWorld();
	}

	if (ObjectWorld == nullptr)
	{
		if (AActor* Actor = GetTypedOuter<AActor>())
		{
			ObjectWorld = Actor->GetWorld();
		}
		else
		{
			ObjectWorld = Cast<UWorld>(GetOuter());
		}
	}

	return ObjectWorld;
}