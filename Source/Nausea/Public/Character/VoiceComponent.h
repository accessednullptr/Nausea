// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTags.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "NauseaHelpers.h"
#include "Character/CoreCharacterComponent.h"
#include "VoiceComponent.generated.h"

UENUM(BlueprintType)
enum class EVoiceRequestResponse : uint8
{
	Success,	//Voice request accepted.
	Ignored,	//Voice request has been ignored due to a higher priority currently playing.
	Muted,		//Voice request has been ignored due to being rate limited.
	Disabled,	//Voice request has been ignored due to the voice component being disabled.
	Invalid,	//Voice request has been ignored due to the relevant UVoiceDataObject not having a USoundWave for the specified GameplayTag.
	Cooldown	//Voice request has been ignored due to this voice command component being on cooldown.
};

UENUM(BlueprintType)
enum class EVoicePriority : uint8
{
	Invalid,				//For invalid voice tags.
	ReactionPassive,		//Very low priority idle chit chat that can be interrupted by anything.
	Reaction,				//Reactions to reloading, being out of ammo, healing or some other gameplay event that should interrupt idle chit chat.
	ReactionHighPriority,	//Reactions to taking damage or some affliction.
	VoiceCommand,			//Voice commands requested by the player.
	ForcedVoiceCommand		//Voice commands that gameplay is forcing.
};

class USoundWave;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FVoiceUpdateSignature, UVoiceComponent*, VoiceComponent, const FGameplayTag&, VoiceTag);

/**
 * Handles the audio for a player's voice. Receives requests from UVoiceCommandComponent for players. AI without player states can request this from the controller or other places.
 */
UCLASS(Blueprintable, BlueprintType, HideCategories = (ComponentTick, Collision, Tags, Variable, Activation, ComponentReplication, Cooking))
class NAUSEA_API UVoiceComponent : public UCoreCharacterComponent
{
	GENERATED_UCLASS_BODY()
	
//~ Begin UActorComponent Interface.
protected:
	virtual void BeginPlay() override;

private:
	//Remove ticking updates here.
	virtual void Activate(bool bReset = false) override { if (bReset || ShouldActivate()) { SetActiveFlag(true); OnComponentActivated.Broadcast(this, bReset); } };
	virtual void Deactivate() override { if (!ShouldActivate()) { SetActiveFlag(false); OnComponentDeactivated.Broadcast(this); } };
//~ End UActorComponent Interface.

//~ Begin IPlayerOwnershipInterface Interface.
public:
	virtual UVoiceComponent* GetVoiceComponent() const override { return const_cast<UVoiceComponent*>(this); }
//~ End IPlayerOwnershipInterface Interface.

public:
	//Intentionally not BlueprintCallable and should not be called directly externally. Check UVoiceCommandComponent::RequestVoiceCommand for the BlueprintCallable version.
	bool PlayVoice(const FGameplayTag& VoiceTag, bool bIsRadioPlayback = false);

public:
	UPROPERTY(BlueprintAssignable, Category = VoiceCommandComponent)
	FVoiceUpdateSignature OnVoiceStarted;
	UPROPERTY(BlueprintAssignable, Category = VoiceCommandComponent)
	FVoiceUpdateSignature OnVoiceInterrupted;
	UPROPERTY(BlueprintAssignable, Category = VoiceCommandComponent)
	FVoiceUpdateSignature OnVoiceCompleted;

protected:
	UPROPERTY(EditDefaultsOnly, Category = VoiceComponent)
	FName AudioComponentBoneAttachName = FName("head");

	//Will be replaced by UVoiceDataObject settings. This is just needed generate a valid UAudioComponent.
	UPROPERTY(EditDefaultsOnly, Category = VoiceComponent)
	class USoundBase* DefaultSoundCue = nullptr;

private:
	//Component used to actually play audio.
	UPROPERTY()
	UAudioComponent* VoiceAudioComponent = nullptr;
};

UENUM(BlueprintType)
enum class EVoiceCommandMenuInput : uint8
{
	ToggleMenu,
	CloseCurrentMenuLevel,
	CloseMenu
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRadioVoiceUpdateSignature, UVoiceCommandComponent*, VoiceCommandComponent, const FGameplayTag&, VoiceTag);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FVoiceCommandMenuInputReceived, UVoiceCommandComponent*, VoiceCommandComponent, EVoiceCommandMenuInput, Input);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FVoiceCommandMenuOptionInputReceived, UVoiceCommandComponent*, VoiceCommandComponent, int32, Index);

UCLASS(Blueprintable, BlueprintType, HideCategories = (ComponentTick, Collision, Tags, Variable, Activation, ComponentReplication, Cooking))
class NAUSEA_API UVoiceCommandComponent : public UActorComponent, public IPlayerOwnershipInterface
{
	GENERATED_UCLASS_BODY()
	
//~ Begin UActorComponent Interface.
public:
	virtual void BeginPlay() override;
//~ End UActorComponent Interface.

//~ Begin IPlayerOwnershipInterface Interface.
public:
	virtual ACorePlayerState* GetOwningPlayerState() const override;
	virtual AController* GetOwningController() const override;
	virtual APawn* GetOwningPawn() const override;
//~ End IPlayerOwnershipInterface Interface.

public:
	UFUNCTION(BlueprintCallable, Category = VoiceCommandComponent)
	EVoiceRequestResponse CanRequestVoiceCommand(const FGameplayTag& VoiceTag) const;

	//Intentionally not BlueprintCallable and should not be called directly externally. Check UVoiceCommandComponent::RequestVoiceCommand for the BlueprintCallable version.
	EVoiceRequestResponse PerformVoiceCommand(const FGameplayTag& VoiceTag);

	UFUNCTION(BlueprintCallable, Category = VoiceCommandComponent)
	bool HasValidVoiceDataObject() const;

	UVoiceDataObject* GetVoiceDataObject() const;

	UFUNCTION(BlueprintCallable, Category = VoiceCommandComponent)
	USoundWave* GetVoiceSoundWave(const FGameplayTag& VoiceTag) const;

	UFUNCTION(BlueprintCallable, Category = VoiceCommandComponent)
	bool IsVoiceCommandMenuBlockingInput() const;

	UFUNCTION(BlueprintCallable, Category = VoiceCommandComponent)
	bool IsVoiceCommandMenuSharingKeyBindingsWithActions(const TArray<FName>& ActionList);

	bool IsPerformingVoiceCommand() const;
	bool IsOnCooldown() const;

	bool IsMuted() const;

	//Intentionally not BlueprintCallable and should not be called directly externally. Check UVoiceCommandComponent::RequestVoiceCommand for the BlueprintCallable version.
	bool PlayRadioVoice(const FGameplayTag& VoiceTag);

	UFUNCTION()
	void OnVoicePlaybackCompleted();

	UFUNCTION()
	void SetVoiceDataObjectClass(TSubclassOf<UVoiceDataObject> InVoiceDataObjectClass);

public:
	UPROPERTY(BlueprintAssignable, Category = VoiceCommandComponent)
	FRadioVoiceUpdateSignature OnRadioVoiceStarted;
	UPROPERTY(BlueprintAssignable, Category = VoiceCommandComponent)
	FRadioVoiceUpdateSignature OnRadioVoiceInterrupted;
	UPROPERTY(BlueprintAssignable, Category = VoiceCommandComponent)
	FRadioVoiceUpdateSignature OnRadioVoiceCompleted;

	UPROPERTY(BlueprintAssignable, Category = VoiceCommandComponent)
	FVoiceCommandMenuInputReceived OnVoiceCommandMenuInputReceived;
	UPROPERTY(BlueprintAssignable, Category = VoiceCommandComponent)
	FVoiceCommandMenuOptionInputReceived OnVoiceCommandMenuOptionInputReceived;

protected:
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_Reliable_RequestVoiceCommand(const FGameplayTag& VoiceTag);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_Reliable_BroadcastVoiceCommand(const FGameplayTag& VoiceTag);

	void GenerateVoiceRandomFloat();
	float GetVoiceRandomFloat() const;

	template<EVoiceCommandMenuInput Input>
	void VoiceCommandMenuInput()
	{
		VoiceCommandMenuInput(Input);
	}

	template<int32 Index>
	void VoiceCommandMenuOptionInput()
	{
		VoiceCommandMenuOptionInput(Index);
	}

	UFUNCTION()
	void VoiceCommandMenuInput(EVoiceCommandMenuInput Input);
	UFUNCTION()
	void VoiceCommandMenuOptionInput(int32 Index);

	UFUNCTION()
	void OnRep_VoiceDataObjectClass();

protected:
	//TODO: Replace this with a system like UE2 Species driven by the playerstate/pawn.
	UPROPERTY(EditDefaultsOnly, ReplicatedUsing = OnRep_VoiceDataObjectClass)
	TSubclassOf<UVoiceDataObject> VoiceDataObjectClass = nullptr;
	UPROPERTY(Transient)
	UVoiceDataObject* VoiceDataObject = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = AI, meta = (EditCondition = "bMakeNoiseOnPickup", DisplayName = "Make Noise On Pickup"))
	FCoreNoiseParams VoiceCommandNoise = FCoreNoiseParams(CoreNoiseTag::InventoryPickup, 0.15f, 0.f);

	UPROPERTY(Transient)
	TMap<FGameplayTag, USoundWaveContainer*> LoadedVoiceSoundWaveContainerMap = TMap<FGameplayTag, USoundWaveContainer*>();

	UPROPERTY(Transient)
	ACorePlayerState* OwningPlayerState = nullptr;

	//Information pertaining to the current voice playback. Can be a regular voice, radio voice, or both.
	UPROPERTY(Transient)
	FTimerHandle VoiceTimerHandle;
	UPROPERTY(Transient)
	FGameplayTag CurrentVoiceGameplayTag;

	//Generic cooldown for voice requests. Ignored by requests for voice priority EVoicePriority::ForcedVoiceCommand.
	UPROPERTY(Transient)
	FTimerHandle CooldownTimerHandle;

	UPROPERTY(EditDefaultsOnly, Category = VoiceCommandComponent)
	float VoiceCommandCooldownDuration = 2.f;

	UPROPERTY(Transient)
	bool bVoiceCommandMenuOpen = false;
	UPROPERTY(Transient)
	FTimerHandle MenuFrameCloseDelayHandle;
	
private:
	//Component used to actually play audio.
	UPROPERTY(Transient)
	UAudioComponent* RadioAudioComponent = nullptr;

	//Base seed with which all voice variations are based on.
	UPROPERTY(Transient, Replicated)
	FRandomStream RandomVoiceSeed = 0;
	//Generated via UVoiceCommandComponent::GenerateVoiceRandomNumber and consumed later on.
	//Never directly interact with. Use UVoiceCommandComponent::GenerateVoiceRandomNumber and UVoiceCommandComponent::ConsumeVoiceRandomNumber to interact with.
	UPROPERTY(Transient)
	float VoiceRandomFloat = -1.f;

public:
	UFUNCTION(BlueprintCallable, Category = VoiceCommandComponent)
	static EVoiceRequestResponse RequestVoiceCommand(TScriptInterface<IPlayerOwnershipInterface> Target, const FGameplayTag& VoiceTag);

	UFUNCTION(BlueprintCallable, Category = VoiceCommandComponent)
	static void SetVoiceCommandState(TScriptInterface<IPlayerOwnershipInterface> Target, bool bOpen);

	UFUNCTION(BlueprintCallable, Category = VoiceCommandComponent)
	static TArray<FGameplayTag> GetVoiceCommandCategoryList();

	UFUNCTION(BlueprintCallable, Category = VoiceCommandComponent)
	static TArray<FGameplayTag> GetVoiceCommandsForCategory(const FGameplayTag& VoiceCommandCategoryTag);

	UFUNCTION(BlueprintCallable, Category = VoiceCommandComponent)
	static FText GetTitleForVoiceGameplayTag(const FGameplayTag& VoiceTag);
};

UCLASS(BlueprintType, Blueprintable, AutoExpandCategories = (VoiceDataObject))
class NAUSEA_API UVoiceDataObject : public UObject
{
	GENERATED_UCLASS_BODY()

//~ Begin UObject Interface
#if WITH_EDITOR
public:
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR
public:
	virtual void Serialize(FArchive& Ar) override;
//~ End UObject Interface


public:
	bool IsValidVoiceTag(const FGameplayTag& Request) const;
	USoundWave* GetSoundWave(const FGameplayTag& Request, float Random) const;

	bool CanIgnoreVoiceSoundWaveMap() const { return bIgnoresVoiceSoundWaveMap; }

	USoundBase* GetVoiceSoundAsset() const { return VoiceSoundAsset; }
	USoundBase* GetRadioSoundAsset() const { return RadioSoundAsset; }

	void LoadSoundWaveContainers();
	void UnloadSoundWaveContainers();


protected:
	//A map of all tags to USoundWaves that this voice can perform.
	UPROPERTY(EditDefaultsOnly, Category = VoiceDataObject)
	TMap<FGameplayTag, TSubclassOf<USoundWaveContainer>> VoiceCommandSoundWaveContainerClassMap = TMap<FGameplayTag, TSubclassOf<USoundWaveContainer>>();

	UPROPERTY(EditDefaultsOnly, Category = VoiceDataObject)
	TMap<FGameplayTag, TSubclassOf<USoundWaveContainer>> VoiceEventSoundWaveContainerClassMap = TMap<FGameplayTag, TSubclassOf<USoundWaveContainer>>();

//================
//Voice Commands

	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Voice Commands|Alert")
	USoundWaveContainer* Follow = nullptr;
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Voice Commands|Alert")
	USoundWaveContainer* Hold = nullptr;
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Voice Commands|Alert")
	USoundWaveContainer* LookOut = nullptr;
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Voice Commands|Alert")
	USoundWaveContainer* Run = nullptr;
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Voice Commands|Alert")
	USoundWaveContainer* Wait = nullptr;
	
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Voice Commands|Assist")
	USoundWaveContainer* Ammo = nullptr;
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Voice Commands|Assist")
	USoundWaveContainer* Heal = nullptr;
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Voice Commands|Assist")
	USoundWaveContainer* Help = nullptr;
	
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Voice Commands|Direction")
	USoundWaveContainer* MoveUp = nullptr;
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Voice Commands|Direction")
	USoundWaveContainer* MoveDown = nullptr;
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Voice Commands|Direction")
	USoundWaveContainer* MoveIn = nullptr;
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Voice Commands|Direction")
	USoundWaveContainer* MoveOut = nullptr;

	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Voice Commands|Insult")
	USoundWaveContainer* InsultAlly = nullptr;
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Voice Commands|Insult")
	USoundWaveContainer* InsultEnemy = nullptr;
	
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Voice Commands|Response")
	USoundWaveContainer* Affirmative = nullptr;
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Voice Commands|Response")
	USoundWaveContainer* Negative = nullptr;
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Voice Commands|Response")
	USoundWaveContainer* Sorry = nullptr;
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Voice Commands|Response")
	USoundWaveContainer* Thanks = nullptr;
	
	UPROPERTY()
	TMap<FGameplayTag, USoundWaveContainer*> VoiceCommandSoundWaveContainerMap = TMap<FGameplayTag, USoundWaveContainer*>();

//================
//Voice Events

	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Voice Events|Idle")
	USoundWaveContainer* Cold = nullptr;
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Voice Events|Idle")
	USoundWaveContainer* Hot = nullptr;

	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Voice Events|Pain")
	USoundWaveContainer* Pain = nullptr;
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Voice Events|Pain")
	USoundWaveContainer* PainHeavy = nullptr;
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Voice Events|Pain")
	USoundWaveContainer* Burn = nullptr;
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Voice Events|Pain")
	USoundWaveContainer* Poison = nullptr;
	
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Voice Events|Reaction")
	USoundWaveContainer* Good = nullptr;
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Voice Events|Reaction")
	USoundWaveContainer* Bad = nullptr;
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Voice Events|Reaction")
	USoundWaveContainer* Gross = nullptr;
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Voice Events|Reaction")
	USoundWaveContainer* Disgust = nullptr;
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Voice Events|Reaction")
	USoundWaveContainer* OutOfAmmo = nullptr;
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Voice Events|Reaction")
	USoundWaveContainer* Reload = nullptr;
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Voice Events|Reaction")
	USoundWaveContainer* Shock = nullptr;
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Voice Events|Reaction")
	USoundWaveContainer* Surprise = nullptr;

	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Voice Events|Status")
	USoundWaveContainer* Poisoned = nullptr;
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Voice Events|Status")
	USoundWaveContainer* Burned = nullptr;
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Voice Events|Status")
	USoundWaveContainer* LowHealth = nullptr;
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Voice Events|Status")
	USoundWaveContainer* NearDeath = nullptr;

	UPROPERTY()
	TMap<FGameplayTag, USoundWaveContainer*> VoiceEventSoundWaveContainerMap = TMap<FGameplayTag, USoundWaveContainer*>();
	
	UPROPERTY(Transient)
	bool bHasLoadedSoundContainers = false;

	//If true we ignore we if we can resolve a valid voice in UVoiceDataObject::VoiceSoundWaveMap when trying to perform a voice.
	//Used when UVoiceDataObject::VoiceSoundAsset, UVoiceDataObject::RadioSoundAsset , UVoiceDataObject::VoiceRadioSoundAsset do not use USoundWave parameters.
	UPROPERTY(EditDefaultsOnly, Category = VoiceDataObject)
	bool bIgnoresVoiceSoundWaveMap = false;

	//The asset used when a request to perform a voice-only voice.
	UPROPERTY(EditDefaultsOnly, Category = VoiceDataObject)
	USoundBase* VoiceSoundAsset = nullptr;
	
	//The asset used when a request to perform a radio-only voice.
	UPROPERTY(EditDefaultsOnly, Category = VoiceDataObject)
	USoundBase* RadioSoundAsset = nullptr;

public:
	UFUNCTION(BlueprintCallable, Category = VoiceCommandComponent)
	static bool IsVoiceCommand(const FGameplayTag& VoiceTag);
	UFUNCTION(BlueprintCallable, Category = VoiceCommandComponent)
	static bool IsVoiceEvent(const FGameplayTag& VoiceTag);

	UFUNCTION(BlueprintCallable, Category = VoiceCommandComponent)
	static EVoicePriority GetPriorityOfVoiceTag(const FGameplayTag& VoiceTag);

	//Master tag that is the parent-most tag of all voice tags. Used for validation of voice requests.
	static FGameplayTag VoiceParent;

	//Tags of each voice command category.
	static FGameplayTag VoiceCategoryAlert;
	static FGameplayTag VoiceCategoryAssist;
	static FGameplayTag VoiceCategoryDirection;
	static FGameplayTag VoiceCategoryInsult;
	static FGameplayTag VoiceCategoryResponse;

	//Tags of each voice event category.
	static FGameplayTag VoiceCategoryPain;
	static FGameplayTag VoiceCategoryReaction;
	static FGameplayTag VoiceCategoryIdle;
	static FGameplayTag VoiceCategoryStatus;
};

/*
* Container of USoundWave setup in UVoiceDataObject loaded once put on a UVoiceComponent.
*/
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class NAUSEA_API USoundWaveContainer : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	USoundWave* GetSoundWave(float Random) const;

	bool LoadSoundWaveContainer();
	void LoadComplete();
	bool UnloadSoundWaveContainer();

protected:
	UPROPERTY(EditDefaultsOnly, Category = VoiceEntry)
	TArray<TSoftObjectPtr<USoundWave>> SoundWaveList = TArray<TSoftObjectPtr<USoundWave>>();

	UPROPERTY(Transient)
	TArray<USoundWave*> LoadedSoundWaveList = TArray<USoundWave*>();

	TSharedPtr<FStreamableHandle> StreamableHandle;
};