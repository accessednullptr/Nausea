// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Overlord/OverlordWorldSettings.h"
#include "EngineUtils.h"
#include "Overlord/PlacementActor.h"

AOverlordWorldSettings::AOverlordWorldSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void AOverlordWorldSettings::BuildAllGridData()
{
	for (TActorIterator<APlacementActor> Itr(GetWorld()); Itr; ++Itr)
	{
		Itr->ValidatePlacementSetup();
	}

	for (TActorIterator<APlacementActor> Itr(GetWorld()); Itr; ++Itr)
	{
		if (Itr->GetParentPlacementActor())
		{
			continue;
		}

		Itr->GeneratePlacementGrid();
	}

	for (TActorIterator<APlacementActor> Itr(GetWorld()); Itr; ++Itr)
	{
		if (!Itr->GetParentPlacementActor())
		{
			continue;
		}


		Itr->GeneratePlacementGrid();
		if (Itr->GetParentPlacementActor())
		{
			Itr->GetParentPlacementActor()->PushPlacementData(Itr->GetPlacementGrid());
		}
	}
}