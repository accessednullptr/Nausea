// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "AI/CoreAIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Damage.h"
#include "AIController.h"

FAISenseID UCoreAIPerceptionComponent::AISenseSightID = FAISenseID::InvalidID();
FAISenseID UCoreAIPerceptionComponent::AISenseHearingID = FAISenseID::InvalidID();
FAISenseID UCoreAIPerceptionComponent::AISenseDamageID = FAISenseID::InvalidID();

UCoreAIPerceptionComponent::UCoreAIPerceptionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	//Make sure sense IDs are ready before we cache them.
	AISenseSightID = GetMutableDefault<UAISense>(UAISense_Sight::StaticClass())->UpdateSenseID();
	AISenseHearingID = GetMutableDefault<UAISense>(UAISense_Hearing::StaticClass())->UpdateSenseID();
	AISenseDamageID = GetMutableDefault<UAISense>(UAISense_Damage::StaticClass())->UpdateSenseID();

	//Default UE4 implementation of MakeNoise uses weird legacy components. Directly forward MakeNoise requests to perception system.
	AActor::SetMakeNoiseDelegate(FMakeNoiseDelegate::CreateStatic(&UCoreAIPerceptionComponent::MakeNoise));
}

//void AActor::MakeNoiseImpl(AActor* NoiseMaker, float Loudness, APawn* NoiseInstigator, const FVector& NoiseLocation, float MaxRange, FName Tag)

void UCoreAIPerceptionComponent::BeginPlay()
{
	Super::BeginPlay();
	
	if (!OnTargetPerceptionUpdated.IsAlreadyBound(this, &UCoreAIPerceptionComponent::OnPerceptionUpdate))
	{
		OnTargetPerceptionUpdated.AddDynamic(this, &UCoreAIPerceptionComponent::OnPerceptionUpdate);
	}
}

float UCoreAIPerceptionComponent::GetMostRecentActiveStimulusAge(const FActorPerceptionInfo* PerceptionInfo) const
{
	if (!PerceptionInfo)
	{
		return FAIStimulus::NeverHappenedAge;
	}

	float BestAge = FAIStimulus::NeverHappenedAge;

	for (const FAIStimulus& Stimulus : PerceptionInfo->LastSensedStimuli)
	{
		if (!UCoreAIPerceptionComponent::IsUsableStimulus(Stimulus))
		{
			continue;
		}

		if (Stimulus.GetAge() >= BestAge)
		{
			continue;
		}
		
		BestAge = Stimulus.GetAge();
	}

	return BestAge;
}

bool UCoreAIPerceptionComponent::HasPerceivedActor(AActor* Actor, float MaxAge) const
{
	if (MaxAge < 0.f)
	{
		if (const FActorPerceptionInfo* PerceptionInfo = GetActorPerceptionInfo(Actor))
		{
			for (const FAIStimulus& Stimulus : PerceptionInfo->LastSensedStimuli)
			{
				if (!UCoreAIPerceptionComponent::IsUsableStimulus(Stimulus))
				{
					continue;
				}

				if (Stimulus.GetAge() != FAIStimulus::NeverHappenedAge)
				{
					return true;
				}
			}
		}
	}
	else
	{
		if (const FActorPerceptionInfo* PerceptionInfo = GetActorPerceptionInfo(Actor))
		{
			for (const FAIStimulus& Stimulus : PerceptionInfo->LastSensedStimuli)
			{
				if (!UCoreAIPerceptionComponent::IsUsableStimulus(Stimulus))
				{
					continue;
				}

				if (Stimulus.GetAge() > MaxAge)
				{
					continue;
				}

				return true;
			}
		}
	}

	return false;
}

FORCEINLINE float GetStimulusAge(const FActorPerceptionInfo* PerceptionInfo, FAISenseID Sense)
{
	if (!PerceptionInfo || !PerceptionInfo->LastSensedStimuli.IsValidIndex(Sense))
	{
		return FAIStimulus::NeverHappenedAge;
	}

	const FAIStimulus& Stimulus = PerceptionInfo->LastSensedStimuli[Sense];

	if (!Stimulus.IsValid())
	{
		return FAIStimulus::NeverHappenedAge;
	}

	if (!Stimulus.IsActive() || Stimulus.IsExpired())
	{
		return FAIStimulus::NeverHappenedAge;
	}

	return Stimulus.GetAge();
}

bool UCoreAIPerceptionComponent::HasSeenActor(AActor* Actor, float MaxAge) const
{
	if (MaxAge < 0.f)
	{
		return GetStimulusAge(GetActorPerceptionInfo(Actor), AISenseSightID) != FAIStimulus::NeverHappenedAge;
	}

	return GetStimulusAge(GetActorPerceptionInfo(Actor), AISenseSightID) <= MaxAge;
}

bool UCoreAIPerceptionComponent::HasRecentlyHeardActor(AActor* Actor, float MaxAge) const
{
	if (MaxAge < 0.f)
	{
		return GetStimulusAge(GetActorPerceptionInfo(Actor), AISenseHearingID) != FAIStimulus::NeverHappenedAge;
	}

	return GetStimulusAge(GetActorPerceptionInfo(Actor), AISenseHearingID) <= MaxAge;
}

bool UCoreAIPerceptionComponent::HasRecentlyReceivedDamageFromActor(AActor* Actor, float MaxAge) const
{
	if (MaxAge < 0.f)
	{
		return GetStimulusAge(GetActorPerceptionInfo(Actor), AISenseDamageID) != FAIStimulus::NeverHappenedAge;
	}

	return GetStimulusAge(GetActorPerceptionInfo(Actor), AISenseDamageID) <= MaxAge;
}

float UCoreAIPerceptionComponent::GetStimulusAgeForActor(AActor* Actor, FAISenseID AISenseID) const
{
	if (!Actor)
	{
		return FAIStimulus::NeverHappenedAge;
	}

	return GetStimulusAge(GetActorPerceptionInfo(Actor), AISenseID) != FAIStimulus::NeverHappenedAge;
}

void UCoreAIPerceptionComponent::OnPerceptionUpdate(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor || !AIOwner || !AIOwner->GetPawn())
	{
		return;
	}

	if (Stimulus.Type == AISenseSightID)
	{
		if (Stimulus.IsActive())
		{
			OnGainedSightOfActor.Broadcast(this, Actor);
		}
		else
		{
			OnLostSightOfActor.Broadcast(this, Actor);
		}
	}
	else if (Stimulus.Type == AISenseHearingID)
	{
		if (Stimulus.IsActive())
		{
			OnHeardNoiseFromActor.Broadcast(this, Actor);
		}
	}
	else if (Stimulus.Type == AISenseDamageID)
	{
		if (Stimulus.IsActive())
		{
			OnReceivedDamageFromActor.Broadcast(this, Actor, Stimulus.Strength);
		}
	}
}

void UCoreAIPerceptionComponent::MakeNoise(AActor* NoiseMaker, float Loudness, APawn* NoiseInstigator, const FVector& NoiseLocation, float MaxRange, FName Tag)
{
	UAISense_Hearing::ReportNoiseEvent(NoiseMaker, NoiseLocation, Loudness, NoiseMaker, MaxRange, Tag);
}