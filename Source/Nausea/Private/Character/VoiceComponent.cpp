// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Character/VoiceComponent.h"
#include "Audio.h"
#include "AudioDevice.h"
#include "Kismet/GameplayStatics.h"
#include "Internationalization/StringTableRegistry.h"
#include "GameFramework/PlayerInput.h"
#include "GameFramework/GameMode.h"
#include "Components/AudioComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "NauseaNetDefines.h"
#include "Player/CorePlayerController.h"
#include "Player/CorePlayerState.h"
#include "Character/CoreCharacter.h"

UVoiceComponent::UVoiceComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	SetAutoActivate(true);

	SetIsReplicatedByDefault(true);
}

void UVoiceComponent::BeginPlay()
{
	Super::BeginPlay();

	if (IsNetMode(NM_DedicatedServer))
	{
		return;
	}

	FAudioDevice::FCreateComponentParams Params(GetOwningCharacter());
	Params.bAutoDestroy = false;
	Params.bPlay = false;
	VoiceAudioComponent = FAudioDevice::CreateComponent(DefaultSoundCue, Params);

	if (!VoiceAudioComponent)
	{
		return;
	}

	VoiceAudioComponent->bAutoManageAttachment = true;

	if (IsLocallyOwned())
	{
		VoiceAudioComponent->AutoAttachParent = GetOwningCharacter()->GetFirstPersonCamera();
		VoiceAudioComponent->AutoAttachSocketName = NAME_None;
	}
	else
	{
		VoiceAudioComponent->AutoAttachParent = GetOwningCharacter()->GetMesh();
		VoiceAudioComponent->AutoAttachSocketName = AudioComponentBoneAttachName;
	}

	VoiceAudioComponent->AutoAttachLocationRule = EAttachmentRule::SnapToTarget;
	VoiceAudioComponent->AutoAttachRotationRule = EAttachmentRule::SnapToTarget;
	VoiceAudioComponent->AutoAttachScaleRule = EAttachmentRule::SnapToTarget;
}

bool UVoiceComponent::PlayVoice(const FGameplayTag& VoiceTag, bool bIsRadioPlayback)
{
	if (IsNetMode(NM_DedicatedServer))
	{
		return true;
	}

	if (!VoiceAudioComponent)
	{
		return false;
	}
	
	const UVoiceCommandComponent* VoiceCommandComponent = GetVoiceCommandComponent();

	if (!VoiceCommandComponent)
	{
		return false;
	}

	const UVoiceDataObject* VoiceDataObject = VoiceCommandComponent->GetVoiceDataObject();

	if (!VoiceDataObject)
	{
		return false;
	}

	VoiceAudioComponent->SetSound(VoiceDataObject->GetVoiceSoundAsset());
	VoiceAudioComponent->SetWaveParameter("Voice", VoiceCommandComponent->GetVoiceSoundWave(VoiceTag));
	VoiceAudioComponent->SetVolumeMultiplier(bIsRadioPlayback ? 0.5f : 1.f);

	VoiceAudioComponent->Play();
	return true;
}

static TArray<FName> VoiceCommandOptionActionList;
UVoiceCommandComponent::UVoiceCommandComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	SetIsReplicatedByDefault(true);

	VoiceCommandOptionActionList.Add("VoiceCommandMenuOptionOne");
	VoiceCommandOptionActionList.Add("VoiceCommandMenuOptionTwo");
	VoiceCommandOptionActionList.Add("VoiceCommandMenuOptionThree");
	VoiceCommandOptionActionList.Add("VoiceCommandMenuOptionFour");
	VoiceCommandOptionActionList.Add("VoiceCommandMenuOptionFive");
}

void UVoiceCommandComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_WITH_PARAMS_FAST(UVoiceCommandComponent, VoiceDataObjectClass, PushReplicationParams::Default);
	DOREPLIFETIME_WITH_PARAMS_FAST(UVoiceCommandComponent, RandomVoiceSeed, PushReplicationParams::InitialOnly);
}

#define BIND_VOICE_COMMAND_OPTION(Name, Index)\
FInputActionBinding& CommandOption##Name = InputComponent->BindAction("VoiceCommandMenuOption" #Name, EInputEvent::IE_Pressed, this, &UVoiceCommandComponent::VoiceCommandMenuOptionInput<Index>);\
CommandOption##Name.bConsumeInput = false;\

void UVoiceCommandComponent::BeginPlay()
{
	if (GetOwnerRole() == ROLE_Authority)
	{
		RandomVoiceSeed.GenerateNewSeed();
		MARK_PROPERTY_DIRTY_FROM_NAME(UVoiceCommandComponent, RandomVoiceSeed, this);
	}

	OwningPlayerState = GetTypedOuter<ACorePlayerState>();

	if (IsLocallyOwned())
	{
		if (APlayerController* PlayerController = Cast<APlayerController>(GetOwningController()))
		{
			if (UInputComponent* InputComponent = PlayerController->InputComponent)
			{
				FInputActionBinding& EscapeMenuBinding = InputComponent->BindAction("EscapeMenu", EInputEvent::IE_Pressed, this, &UVoiceCommandComponent::VoiceCommandMenuInput<EVoiceCommandMenuInput::CloseMenu>);
				EscapeMenuBinding.bConsumeInput = false;

				InputComponent->BindAction("ToggleVoiceCommandMenu", EInputEvent::IE_Pressed, this, &UVoiceCommandComponent::VoiceCommandMenuInput<EVoiceCommandMenuInput::ToggleMenu>);

				BIND_VOICE_COMMAND_OPTION(One, 1);
				BIND_VOICE_COMMAND_OPTION(Two, 2);
				BIND_VOICE_COMMAND_OPTION(Three, 3);
				BIND_VOICE_COMMAND_OPTION(Four, 4);
				BIND_VOICE_COMMAND_OPTION(Five, 5);
			}
		}
	}

	Super::BeginPlay();

	OnRep_VoiceDataObjectClass();
}

ACorePlayerState* UVoiceCommandComponent::GetOwningPlayerState() const
{
	if (!OwningPlayerState)
	{
		return GetTypedOuter<ACorePlayerState>();
	}

	return OwningPlayerState;
}

AController* UVoiceCommandComponent::GetOwningController() const
{
	return Cast<AController>(GetOwningPlayerState()->GetOwner());
}

APawn* UVoiceCommandComponent::GetOwningPawn() const
{
	return GetOwningPlayerState()->GetPawn();
}

EVoiceRequestResponse UVoiceCommandComponent::CanRequestVoiceCommand(const FGameplayTag& VoiceTag) const
{
	if (!HasValidVoiceDataObject())
	{
		return EVoiceRequestResponse::Invalid;
	}

	if (IsMuted())
	{
		return EVoiceRequestResponse::Muted;
	}

	if (VoiceTag != FGameplayTag::EmptyTag && !VoiceTag.GetGameplayTagParents().HasTag(UVoiceDataObject::VoiceParent))
	{
		//VoiceTag specified is not in the Voice hierarchy and so this is an invalid request.
		return EVoiceRequestResponse::Invalid;
	}

	if (!IsNetMode(NM_DedicatedServer) && GetVoiceDataObject()->IsValidVoiceTag(VoiceTag))
	{
		return EVoiceRequestResponse::Invalid;
	}

	EVoicePriority VoiceTagPriority = UVoiceDataObject::GetPriorityOfVoiceTag(VoiceTag);

	if (IsOnCooldown() && VoiceTagPriority < EVoicePriority::ForcedVoiceCommand)
	{
		return EVoiceRequestResponse::Cooldown;
	}

	if (IsPerformingVoiceCommand() && VoiceTagPriority < UVoiceDataObject::GetPriorityOfVoiceTag(CurrentVoiceGameplayTag))
	{
		return EVoiceRequestResponse::Ignored;
	}

	return EVoiceRequestResponse::Success;
}

EVoiceRequestResponse UVoiceCommandComponent::PerformVoiceCommand(const FGameplayTag& VoiceTag)
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		GetWorld()->GetTimerManager().ClearTimer(VoiceTimerHandle);
	}

	EVoiceRequestResponse Response = CanRequestVoiceCommand(VoiceTag);
	if (Response != EVoiceRequestResponse::Success)
	{
		return Response;
	}

	//Push the voice command out to all clients.
	if (GetOwnerRole() == ROLE_Authority)
	{
		Multicast_Reliable_BroadcastVoiceCommand(VoiceTag);
	}

	if (IsNetMode(NM_DedicatedServer))
	{
		return EVoiceRequestResponse::Success;
	}

	USoundWave* VoiceSoundWave = GetVoiceSoundWave(VoiceTag);
	const bool bIsVoiceCommand = UVoiceDataObject::IsVoiceCommand(VoiceTag);

	if (bIsVoiceCommand)
	{
		PlayRadioVoice(VoiceTag);
		const FString VoiceTagTitle = GetTitleForVoiceGameplayTag(VoiceTag).ToString();

		//Find and locally send a client message with this radio call.
		for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			APlayerController* PC = Iterator->Get();

			if (PC && PC->IsLocalPlayerController())
			{
				PC->ClientTeamMessage(GetOwningPlayerState(), VoiceTagTitle, NAME_VoiceCommand, 0.f);
			}
		}
	}
	
	if (bIsVoiceCommand || UVoiceDataObject::IsVoiceEvent(VoiceTag))
	{
		if (UVoiceComponent* VoiceComponent = GetVoiceComponent())
		{
			VoiceComponent->PlayVoice(VoiceTag, bIsVoiceCommand);
		}
	}

	CurrentVoiceGameplayTag = VoiceTag;

	GetWorld()->GetTimerManager().SetTimer(VoiceTimerHandle, this, &UVoiceCommandComponent::OnVoicePlaybackCompleted, VoiceSoundWave ? VoiceSoundWave->GetDuration() : 1.f, false);

	if (bIsVoiceCommand)
	{
		GetWorld()->GetTimerManager().SetTimer(CooldownTimerHandle, VoiceCommandCooldownDuration, false);
	}

	return EVoiceRequestResponse::Success;
}

bool UVoiceCommandComponent::HasValidVoiceDataObject() const
{
	if (!VoiceDataObjectClass)
	{
		return false;
	}

	return VoiceDataObjectClass.GetDefaultObject() != nullptr;
}

UVoiceDataObject* UVoiceCommandComponent::GetVoiceDataObject() const
{
	return VoiceDataObject;
}

USoundWave* UVoiceCommandComponent::GetVoiceSoundWave(const FGameplayTag& VoiceTag) const
{
	if (!LoadedVoiceSoundWaveContainerMap.Contains(VoiceTag))
	{
		return nullptr;
	}

	return LoadedVoiceSoundWaveContainerMap[VoiceTag]->GetSoundWave(GetVoiceRandomFloat());
}

bool UVoiceCommandComponent::IsVoiceCommandMenuBlockingInput() const
{
	return bVoiceCommandMenuOpen || GetWorld()->GetTimerManager().IsTimerActive(MenuFrameCloseDelayHandle);
}

bool UVoiceCommandComponent::IsVoiceCommandMenuSharingKeyBindingsWithActions(const TArray<FName>& ActionList)
{
	if (!IsLocallyOwned())
	{
		return false;
	}

	const UPlayerInput* PlayerInput = Cast<APlayerController>(GetOwningController()) ? Cast<APlayerController>(GetOwningController())->PlayerInput : nullptr;

	if (!PlayerInput)
	{
		return false;
	}

	//Set of all keys used by the voice command option actions.
	TSet<FKey> CommandOptionKeySet;

	for (const FName& OptionAction : VoiceCommandOptionActionList)
	{
		const TArray<FInputActionKeyMapping>& CommandOptionKeyList = PlayerInput->GetKeysForAction(OptionAction);
		CommandOptionKeySet.Reserve(CommandOptionKeySet.Num() + CommandOptionKeyList.Num());

		for (const FInputActionKeyMapping& UsedKey : CommandOptionKeyList)
		{
			CommandOptionKeySet.Add(UsedKey.Key);
		}
	}

	//Check if any of the requested actions share keys. It would have been nice to know if it's our specific keypress that is related but input actions do not provide that context.
	for (const FName& RequestedAction : ActionList)
	{
		const TArray<FInputActionKeyMapping>& RequestedActionKeyList = PlayerInput->GetKeysForAction(RequestedAction);
		for (const FInputActionKeyMapping& RequestedActionKey : RequestedActionKeyList)
		{
			if (CommandOptionKeySet.Contains(RequestedActionKey.Key))
			{
				return true;
			}
		}
	}

	return false;
}

bool UVoiceCommandComponent::IsPerformingVoiceCommand() const
{
	return GetWorld()->GetTimerManager().IsTimerActive(VoiceTimerHandle);
}

bool UVoiceCommandComponent::IsOnCooldown() const
{
	return GetWorld()->GetTimerManager().IsTimerActive(CooldownTimerHandle);
}

bool UVoiceCommandComponent::IsMuted() const
{
	return false;
}

bool UVoiceCommandComponent::PlayRadioVoice(const FGameplayTag& VoiceTag)
{
	if (IsNetMode(NM_DedicatedServer))
	{
		return true;
	}

	if (!GetVoiceDataObject())
	{
		return false;
	}

	if (!RadioAudioComponent)
	{
		FAudioDevice::FCreateComponentParams Params(GetWorld());
		Params.bAutoDestroy = false;
		Params.bPlay = false;
		RadioAudioComponent = FAudioDevice::CreateComponent(GetVoiceDataObject()->GetRadioSoundAsset(), Params);
	}
	else
	{
		RadioAudioComponent->Stop();
	}

	if (!RadioAudioComponent)
	{
		return false;
	}

	RadioAudioComponent->SetWaveParameter("Voice", GetVoiceSoundWave(VoiceTag));
	RadioAudioComponent->Play();
	return true;
}

void UVoiceCommandComponent::OnVoicePlaybackCompleted()
{
	CurrentVoiceGameplayTag = FGameplayTag::EmptyTag;
}

void UVoiceCommandComponent::SetVoiceDataObjectClass(TSubclassOf<UVoiceDataObject> InVoiceDataObjectClass)
{
	VoiceDataObjectClass = InVoiceDataObjectClass;
	MARK_PROPERTY_DIRTY_FROM_NAME(UVoiceCommandComponent, VoiceDataObjectClass, this);
}

bool UVoiceCommandComponent::Server_Reliable_RequestVoiceCommand_Validate(const FGameplayTag& VoiceTag)
{
	return true;
}

void UVoiceCommandComponent::Server_Reliable_RequestVoiceCommand_Implementation(const FGameplayTag& VoiceTag)
{
	PerformVoiceCommand(VoiceTag);
}

void UVoiceCommandComponent::Multicast_Reliable_BroadcastVoiceCommand_Implementation(const FGameplayTag& VoiceTag)
{
	//Server has made a request to play a voice command.
	//Regardless of who we are or whether we think we can perform this voice command, we need to move the seed forward to keep up with the FRandomStream.
	GenerateVoiceRandomFloat();

	if (GetOwnerRole() == ROLE_Authority)
	{
		return;
	}

	if (!VoiceTag.IsValid())
	{
		return;
	}

	PerformVoiceCommand(VoiceTag);
}

void UVoiceCommandComponent::GenerateVoiceRandomFloat()
{
	VoiceRandomFloat = RandomVoiceSeed.FRand();
	MARK_PROPERTY_DIRTY_FROM_NAME(UVoiceCommandComponent, RandomVoiceSeed, this);
}

float UVoiceCommandComponent::GetVoiceRandomFloat() const
{
	return VoiceRandomFloat;
}

void UVoiceCommandComponent::VoiceCommandMenuInput(EVoiceCommandMenuInput Input)
{
	OnVoiceCommandMenuInputReceived.Broadcast(this, Input);
}

void UVoiceCommandComponent::VoiceCommandMenuOptionInput(int32 Index)
{
	if (!bVoiceCommandMenuOpen)
	{
		return;
	}

	OnVoiceCommandMenuOptionInputReceived.Broadcast(this, Index - 1);
}

void UVoiceCommandComponent::OnRep_VoiceDataObjectClass()
{
	if (VoiceDataObject)
	{
		if (!VoiceDataObject->IsPendingKill())
		{
			VoiceDataObject->UnloadSoundWaveContainers();
			VoiceDataObject->MarkPendingKill();
		}

		VoiceDataObject = nullptr;
	}

	if (VoiceDataObjectClass)
	{
		VoiceDataObject = NewObject<UVoiceDataObject>(this, VoiceDataObjectClass);
	}

	if (!VoiceDataObject)
	{
		return;
	}

	if (IsNetMode(NM_DedicatedServer))
	{
		return;
	}

	VoiceDataObject->LoadSoundWaveContainers();
}

EVoiceRequestResponse UVoiceCommandComponent::RequestVoiceCommand(TScriptInterface<IPlayerOwnershipInterface> Target, const FGameplayTag& VoiceTag)
{
	if (!Target)
	{
		return EVoiceRequestResponse::Invalid;
	}

	UVoiceCommandComponent* VoiceCommandComponent = Target->GetVoiceCommandComponent();

	if (!VoiceCommandComponent)
	{
		return EVoiceRequestResponse::Invalid;
	}

	if (VoiceCommandComponent->GetOwnerRole() < ROLE_Authority)
	{
		EVoiceRequestResponse Response = VoiceCommandComponent->CanRequestVoiceCommand(VoiceTag);

		if (Response == EVoiceRequestResponse::Success)
		{
			VoiceCommandComponent->Server_Reliable_RequestVoiceCommand(VoiceTag);
		}

		return Response;
	}

	return VoiceCommandComponent->PerformVoiceCommand(VoiceTag);
}

void UVoiceCommandComponent::SetVoiceCommandState(TScriptInterface<IPlayerOwnershipInterface> Target, bool bOpen)
{
	if (!Target)
	{
		return;
	}

	UVoiceCommandComponent* VoiceCommandComponent = Target->GetVoiceCommandComponent();

	if (!VoiceCommandComponent)
	{
		return;
	}

	VoiceCommandComponent->bVoiceCommandMenuOpen = bOpen;

	VoiceCommandComponent->GetWorld()->GetTimerManager().SetTimer(VoiceCommandComponent->MenuFrameCloseDelayHandle, 0.01f, false);
}

//Main two types of voice categories.
static TSet<FGameplayTag> VoiceCommandCategoryTagSet;
static TSet<FGameplayTag> VoiceEventCategoryTagSet;

//Sets of voice categories used by priority checks.
static TSet<FGameplayTag> ReactionHighPriorityCategoryTagSet;
static TSet<FGameplayTag> ReactionCategoryTagSet;
static TSet<FGameplayTag> ReactionPassiveCategoryTagSet;

//Map of all voice command tags and voice command category tags to localized text.
static TMap<FGameplayTag, FText> VoiceCommandTextMap;

//All individual voice tags (not categories) used for initialization of UVoiceDataObject maps.
static TSet<FGameplayTag> VoiceCommandTagSet;
static TSet<FGameplayTag> VoiceEventTagSet;

TArray<FGameplayTag> UVoiceCommandComponent::GetVoiceCommandCategoryList()
{
	return VoiceCommandCategoryTagSet.Array();
}

TArray<FGameplayTag> UVoiceCommandComponent::GetVoiceCommandsForCategory(const FGameplayTag& VoiceCommandCategoryTag)
{
	if (!VoiceCommandCategoryTagSet.Contains(VoiceCommandCategoryTag))
	{
		return TArray<FGameplayTag>();
	}

	TArray<FGameplayTag> GameplayTagList;
	UGameplayTagsManager::Get().RequestGameplayTagChildren(VoiceCommandCategoryTag).GetGameplayTagArray(GameplayTagList);
	return GameplayTagList;
}

FText UVoiceCommandComponent::GetTitleForVoiceGameplayTag(const FGameplayTag& VoiceTag)
{
	if (VoiceCommandTextMap.Contains(VoiceTag))
	{
		return VoiceCommandTextMap[VoiceTag];
	}

	return FText::GetEmpty();
}

FGameplayTag UVoiceDataObject::VoiceParent = FGameplayTag();
FGameplayTag UVoiceDataObject::VoiceCategoryAlert = FGameplayTag();
FGameplayTag UVoiceDataObject::VoiceCategoryAssist = FGameplayTag();
FGameplayTag UVoiceDataObject::VoiceCategoryDirection = FGameplayTag();
FGameplayTag UVoiceDataObject::VoiceCategoryInsult = FGameplayTag();
FGameplayTag UVoiceDataObject::VoiceCategoryResponse = FGameplayTag();
FGameplayTag UVoiceDataObject::VoiceCategoryPain = FGameplayTag();
FGameplayTag UVoiceDataObject::VoiceCategoryReaction = FGameplayTag();
FGameplayTag UVoiceDataObject::VoiceCategoryIdle = FGameplayTag();
FGameplayTag UVoiceDataObject::VoiceCategoryStatus = FGameplayTag();

UVoiceDataObject::UVoiceDataObject(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	VoiceParent = FGameplayTag::RequestGameplayTag("Voice", true);

	VoiceCategoryAlert = FGameplayTag::RequestGameplayTag("Voice.Alert", true);
	VoiceCategoryAssist = FGameplayTag::RequestGameplayTag("Voice.Assist", true);
	VoiceCategoryDirection = FGameplayTag::RequestGameplayTag("Voice.Direction", true);
	VoiceCategoryInsult = FGameplayTag::RequestGameplayTag("Voice.Insult", true);
	VoiceCategoryResponse = FGameplayTag::RequestGameplayTag("Voice.Response", true);

	VoiceCategoryPain = FGameplayTag::RequestGameplayTag("Voice.Pain", true);
	VoiceCategoryReaction = FGameplayTag::RequestGameplayTag("Voice.Reaction", true);
	VoiceCategoryIdle = FGameplayTag::RequestGameplayTag("Voice.Idle", true);
	VoiceCategoryStatus = FGameplayTag::RequestGameplayTag("Voice.Status", true);

	VoiceCommandCategoryTagSet.Empty(VoiceCommandCategoryTagSet.Num());
	VoiceCommandCategoryTagSet.Add(VoiceCategoryAlert);
	VoiceCommandCategoryTagSet.Add(VoiceCategoryAssist);
	VoiceCommandCategoryTagSet.Add(VoiceCategoryDirection);
	VoiceCommandCategoryTagSet.Add(VoiceCategoryInsult);
	VoiceCommandCategoryTagSet.Add(VoiceCategoryResponse);

	VoiceEventCategoryTagSet.Empty(VoiceEventCategoryTagSet.Num());
	VoiceEventCategoryTagSet.Add(VoiceCategoryIdle);
	VoiceEventCategoryTagSet.Add(VoiceCategoryPain);
	VoiceEventCategoryTagSet.Add(VoiceCategoryReaction);
	VoiceEventCategoryTagSet.Add(VoiceCategoryStatus);

	ReactionHighPriorityCategoryTagSet.Empty(ReactionHighPriorityCategoryTagSet.Num());
	ReactionHighPriorityCategoryTagSet.Add(VoiceCategoryStatus);
	ReactionHighPriorityCategoryTagSet.Add(VoiceCategoryPain);

	ReactionCategoryTagSet.Empty(ReactionCategoryTagSet.Num());
	ReactionCategoryTagSet.Add(VoiceCategoryReaction);

	ReactionPassiveCategoryTagSet.Empty(ReactionPassiveCategoryTagSet.Num());
	ReactionPassiveCategoryTagSet.Add(VoiceCategoryIdle);
	
	FString CategoryTagString;
	FString ChildTagString;
	for (const FGameplayTag& CategoryTag : VoiceCommandCategoryTagSet)
	{
		CategoryTagString = CategoryTag.ToString();
		CategoryTagString.Split(".", nullptr, &CategoryTagString, ESearchCase::IgnoreCase, ESearchDir::FromEnd);

		VoiceCommandTextMap.Add(CategoryTag) = FStringTableRegistry::Get().Internal_FindLocTableEntry(TEXT("/Game/Localization/VoiceCommandStringTable.VoiceCommandStringTable"),
			FString::Printf(TEXT("Category_%s"), *CategoryTagString), EStringTableLoadingPolicy::FindOrLoad);

		FGameplayTagContainer CategoryChildrenTag = UGameplayTagsManager::Get().RequestGameplayTagChildren(CategoryTag);
		for (const FGameplayTag& ChildTag : CategoryChildrenTag)
		{
			VoiceCommandTagSet.Add(ChildTag);
			VoiceCommandSoundWaveContainerClassMap.Add(ChildTag);

			ChildTagString = ChildTag.ToString();
			ChildTagString.Split(".", nullptr, &ChildTagString, ESearchCase::IgnoreCase, ESearchDir::FromEnd);

			VoiceCommandTextMap.Add(ChildTag) = FStringTableRegistry::Get().Internal_FindLocTableEntry(TEXT("/Game/Localization/VoiceCommandStringTable.VoiceCommandStringTable"),
				FString::Printf(TEXT("%s_%s"), *CategoryTagString, *ChildTagString), EStringTableLoadingPolicy::FindOrLoad);
		}
	}

	for (const FGameplayTag& CategoryTag : VoiceEventCategoryTagSet)
	{
		FGameplayTagContainer CategoryChildrenTag = UGameplayTagsManager::Get().RequestGameplayTagChildren(CategoryTag);
		for (const FGameplayTag& ChildTag : CategoryChildrenTag)
		{
			VoiceEventTagSet.Add(ChildTag);
			VoiceEventSoundWaveContainerClassMap.Add(ChildTag);
		}
	}

}

#if WITH_EDITOR
void UVoiceDataObject::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UVoiceDataObject, VoiceCommandSoundWaveContainerClassMap))
	{
		TSet<FGameplayTag> KeySet;
		VoiceCommandSoundWaveContainerClassMap.GetKeys(KeySet);
		for (const FGameplayTag& Key : KeySet)
		{
			if (!VoiceCommandTagSet.Contains(Key))
			{
				VoiceCommandSoundWaveContainerClassMap.Remove(Key);
			}
		}

		for (const FGameplayTag& Key : VoiceCommandTagSet)
		{
			if (!KeySet.Contains(Key))
			{
				VoiceCommandSoundWaveContainerClassMap.Add(Key);
			}
		}
	}

	if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UVoiceDataObject, VoiceEventSoundWaveContainerClassMap))
	{
		TSet<FGameplayTag> KeySet;
		VoiceEventSoundWaveContainerClassMap.GetKeys(KeySet);
		for (const FGameplayTag& Key : KeySet)
		{
			if (!VoiceEventTagSet.Contains(Key))
			{
				VoiceEventSoundWaveContainerClassMap.Remove(Key);
			}
		}

		for (const FGameplayTag& Key : VoiceEventTagSet)
		{
			if (!KeySet.Contains(Key))
			{
				VoiceEventSoundWaveContainerClassMap.Add(Key);
			}
		}
	}
}
#endif // WITH_EDITOR

void UVoiceDataObject::Serialize(FArchive& Ar)
{
	if (!Ar.IsSaving() || !HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		Super::Serialize(Ar);
		return;
	}

	if (GetNameSafe(GetClass()).StartsWith("REINST"))
	{
		Super::Serialize(Ar);
		return;
	}

	VoiceCommandSoundWaveContainerMap.Reset();
	auto AssignToVoiceCommandMap = [this](const FGameplayTag& Tag, USoundWaveContainer* VoiceContainer)
	{
		VoiceCommandSoundWaveContainerMap.Add(Tag) = VoiceContainer;
	};

	VoiceEventSoundWaveContainerMap.Reset();
	auto AssignToVoiceEventMap = [this](const FGameplayTag& Tag, USoundWaveContainer* VoiceContainer)
	{
		VoiceEventSoundWaveContainerMap.Add(Tag) = VoiceContainer;
	};

	AssignToVoiceCommandMap(FGameplayTag::RequestGameplayTag("Voice.Alert.Follow", true), Follow);
	AssignToVoiceCommandMap(FGameplayTag::RequestGameplayTag("Voice.Alert.Hold", true), Hold);
	AssignToVoiceCommandMap(FGameplayTag::RequestGameplayTag("Voice.Alert.LookOut", true), LookOut);
	AssignToVoiceCommandMap(FGameplayTag::RequestGameplayTag("Voice.Alert.Run", true), Run);
	AssignToVoiceCommandMap(FGameplayTag::RequestGameplayTag("Voice.Alert.Wait", true), Wait);

	AssignToVoiceCommandMap(FGameplayTag::RequestGameplayTag("Voice.Assist.Ammo", true), Ammo);
	AssignToVoiceCommandMap(FGameplayTag::RequestGameplayTag("Voice.Assist.Heal", true), Heal);
	AssignToVoiceCommandMap(FGameplayTag::RequestGameplayTag("Voice.Assist.Help", true), Help);

	AssignToVoiceCommandMap(FGameplayTag::RequestGameplayTag("Voice.Direction.MoveUp", true), MoveUp);
	AssignToVoiceCommandMap(FGameplayTag::RequestGameplayTag("Voice.Direction.MoveDown", true), MoveDown);
	AssignToVoiceCommandMap(FGameplayTag::RequestGameplayTag("Voice.Direction.MoveIn", true), MoveIn);
	AssignToVoiceCommandMap(FGameplayTag::RequestGameplayTag("Voice.Direction.MoveDown", true), MoveOut);

	AssignToVoiceCommandMap(FGameplayTag::RequestGameplayTag("Voice.Insult.InsultAlly", true), InsultAlly);
	AssignToVoiceCommandMap(FGameplayTag::RequestGameplayTag("Voice.Insult.InsultEnemy", true), InsultEnemy);

	AssignToVoiceCommandMap(FGameplayTag::RequestGameplayTag("Voice.Response.Yes", true), Affirmative);
	AssignToVoiceCommandMap(FGameplayTag::RequestGameplayTag("Voice.Response.No", true), Negative);
	AssignToVoiceCommandMap(FGameplayTag::RequestGameplayTag("Voice.Response.Sorry", true), Sorry);
	AssignToVoiceCommandMap(FGameplayTag::RequestGameplayTag("Voice.Response.Thanks", true), Thanks);
	
	AssignToVoiceCommandMap(FGameplayTag::RequestGameplayTag("Voice.Idle.Cold", true), Cold);
	AssignToVoiceCommandMap(FGameplayTag::RequestGameplayTag("Voice.Idle.Hot", true), Hot);

	AssignToVoiceEventMap(FGameplayTag::RequestGameplayTag("Voice.Pain.Pain", true), Pain);
	AssignToVoiceEventMap(FGameplayTag::RequestGameplayTag("Voice.Pain.PainHeavy", true), PainHeavy);
	AssignToVoiceEventMap(FGameplayTag::RequestGameplayTag("Voice.Pain.Burn", true), Burn);
	AssignToVoiceEventMap(FGameplayTag::RequestGameplayTag("Voice.Pain.Poison", true), Poison);

	AssignToVoiceEventMap(FGameplayTag::RequestGameplayTag("Voice.Reaction.Good", true), Good);
	AssignToVoiceEventMap(FGameplayTag::RequestGameplayTag("Voice.Reaction.Bad", true), Bad);
	AssignToVoiceEventMap(FGameplayTag::RequestGameplayTag("Voice.Reaction.Gross", true), Gross);
	AssignToVoiceEventMap(FGameplayTag::RequestGameplayTag("Voice.Reaction.Disgust", true), Disgust);
	AssignToVoiceEventMap(FGameplayTag::RequestGameplayTag("Voice.Reaction.OutOfAmmo", true), OutOfAmmo);
	AssignToVoiceEventMap(FGameplayTag::RequestGameplayTag("Voice.Reaction.Reload", true), Reload);
	AssignToVoiceEventMap(FGameplayTag::RequestGameplayTag("Voice.Reaction.Shock", true), Shock);
	AssignToVoiceEventMap(FGameplayTag::RequestGameplayTag("Voice.Reaction.Surprise", true), Surprise);
	
	AssignToVoiceEventMap(FGameplayTag::RequestGameplayTag("Voice.Status.Poisoned", true), Poisoned);
	AssignToVoiceEventMap(FGameplayTag::RequestGameplayTag("Voice.Status.Burned", true), Burned);
	AssignToVoiceEventMap(FGameplayTag::RequestGameplayTag("Voice.Status.LowHealth", true), LowHealth);
	AssignToVoiceEventMap(FGameplayTag::RequestGameplayTag("Voice.Status.NearDeath", true), NearDeath);

	Super::Serialize(Ar);
}

bool UVoiceDataObject::IsValidVoiceTag(const FGameplayTag& Request) const
{
	if (CanIgnoreVoiceSoundWaveMap())
	{
		return true;
	}

	if (!VoiceCommandSoundWaveContainerMap.Contains(Request) && !VoiceEventSoundWaveContainerMap.Contains(Request))
	{
		return false;
	}

	return true;
}


USoundWave* UVoiceDataObject::GetSoundWave(const FGameplayTag& Request, float Random) const
{
	if (VoiceCommandSoundWaveContainerMap.Contains(Request))
	{
		if (!VoiceCommandSoundWaveContainerMap[Request])
		{
			return nullptr;
		}

		return VoiceCommandSoundWaveContainerMap[Request]->GetSoundWave(Random);
	}

	if (VoiceEventSoundWaveContainerMap.Contains(Request))
	{
		if (!VoiceEventSoundWaveContainerMap[Request])
		{
			return nullptr;
		}

		return VoiceEventSoundWaveContainerMap[Request]->GetSoundWave(Random);
	}

	return nullptr;
}

void UVoiceDataObject::LoadSoundWaveContainers()
{
	if (bHasLoadedSoundContainers)
	{
		return;
	}

	bHasLoadedSoundContainers = true;

	TArray<USoundWaveContainer*> Containers;
	VoiceCommandSoundWaveContainerMap.GenerateValueArray(Containers);
	for (USoundWaveContainer* Container : Containers)
	{
		if (!Container)
		{
			continue;
		}

		Container->LoadSoundWaveContainer();
	}

	Containers.Reset();
	VoiceEventSoundWaveContainerMap.GenerateValueArray(Containers);
	for (USoundWaveContainer* Container : Containers)
	{
		if (!Container)
		{
			continue;
		}

		Container->LoadSoundWaveContainer();
	}
}

void UVoiceDataObject::UnloadSoundWaveContainers()
{
	if (!bHasLoadedSoundContainers)
	{
		return;
	}

	TArray<USoundWaveContainer*> Containers;
	VoiceCommandSoundWaveContainerMap.GenerateValueArray(Containers);
	for (USoundWaveContainer* Container : Containers)
	{
		if (!Container)
		{
			continue;
		}

		Container->UnloadSoundWaveContainer();
	}

	Containers.Reset();
	VoiceEventSoundWaveContainerMap.GenerateValueArray(Containers);
	for (USoundWaveContainer* Container : Containers)
	{
		if (!Container)
		{
			continue;
		}

		Container->UnloadSoundWaveContainer();
	}

	bHasLoadedSoundContainers = false;
}


bool UVoiceDataObject::IsVoiceCommand(const FGameplayTag& VoiceTag)
{
	return VoiceCommandCategoryTagSet.Contains(VoiceTag.RequestDirectParent());
}

bool UVoiceDataObject::IsVoiceEvent(const FGameplayTag& VoiceTag)
{
	return VoiceEventCategoryTagSet.Contains(VoiceTag.RequestDirectParent());
}

EVoicePriority UVoiceDataObject::GetPriorityOfVoiceTag(const FGameplayTag& VoiceTag)
{
	FGameplayTag RequestParentTag = VoiceTag.RequestDirectParent();

	if (VoiceCommandCategoryTagSet.Contains(RequestParentTag))
	{
		return EVoicePriority::VoiceCommand;
	}

	if (ReactionHighPriorityCategoryTagSet.Contains(RequestParentTag))
	{
		return EVoicePriority::ReactionHighPriority;
	}

	if (ReactionCategoryTagSet.Contains(RequestParentTag))
	{
		return EVoicePriority::Reaction;
	}
	
	if (ReactionPassiveCategoryTagSet.Contains(RequestParentTag))
	{
		return EVoicePriority::ReactionPassive;
	}

	return EVoicePriority::Invalid;
}

USoundWaveContainer::USoundWaveContainer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

USoundWave* USoundWaveContainer::GetSoundWave(float Random) const
{
	if (LoadedSoundWaveList.Num() == 0)
	{
		return nullptr;
	}

	return LoadedSoundWaveList[FMath::RoundToInt(float(LoadedSoundWaveList.Num() - 1) * Random)];
}

bool USoundWaveContainer::LoadSoundWaveContainer()
{
	UAssetManager* AssetManager = UAssetManager::GetIfValid();

	if (!AssetManager)
	{
		UE_LOG(LogTemp, Error, TEXT("USoundWaveContainer::LoadSoundWaveList failed to load map sound wave list due to missing asset manager."));
		return false;
	}

	TArray<FSoftObjectPath> SoundSoftObjectPathList;
	for (const TSoftObjectPtr<USoundWave>& SoundWaveSoftObject : SoundWaveList)
	{
		SoundSoftObjectPathList.Add(SoundWaveSoftObject.ToSoftObjectPath());
	}

	FStreamableManager& StreamableManager = AssetManager->GetStreamableManager();
	StreamableHandle = StreamableManager.RequestAsyncLoad(SoundSoftObjectPathList);

	if (!StreamableHandle.IsValid())
	{
		return false;
	}

	TWeakObjectPtr<USoundWaveContainer> WeakThis = TWeakObjectPtr<USoundWaveContainer>(this);

	if (StreamableHandle->HasLoadCompleted())
	{
		LoadComplete();
		return true;
	}

	auto StreamClassCompleteDelegate = [WeakThis] {
		if (!WeakThis.IsValid())
		{
			return;
		}

		WeakThis->LoadComplete();
	};

	StreamableHandle->BindCompleteDelegate(FStreamableDelegate::CreateWeakLambda(this, StreamClassCompleteDelegate));
	return true;
}

void USoundWaveContainer::LoadComplete()
{
	LoadedSoundWaveList.Reserve(SoundWaveList.Num());
	for (const TSoftObjectPtr<USoundWave>& SoundWaveSoftObject : SoundWaveList)
	{
		LoadedSoundWaveList.Add(SoundWaveSoftObject.Get());
	}
}

bool USoundWaveContainer::UnloadSoundWaveContainer()
{
	LoadedSoundWaveList.Empty();

	if (StreamableHandle.IsValid())
	{
		if (StreamableHandle->IsLoadingInProgress())
		{
			StreamableHandle->CancelHandle();
		}

		StreamableHandle.Reset();
	}

	return true;
}

/*
Category Alert:
	Follow (alerting players to follow me)
	Hold (alerting players to hold by me)
	LookOut (alerting players to look out for some immediate threat)
	Run (alerting players to run away)
	Wait (alerting players to wait for me)

Category Assist:
	Ammo (requesting for ammo)
	Heal (requesting for health)
	Help (requesting for some immediate help)

Category Direction:
	MoveDown (move downward - specifically elevation)
	MoveUp (move upward - specifically elevation)
	MoveIn (move into a place)
	MoveOut (move out of a place)

Category Insult:
	Insult Ally (insult an ally)
	Insult Enemy (insult an enemy)

Category Response:
	Yes (accepting some request or agreeing with a statement)
	No (declining some request or disagreeing with a statement)
	Thanks (expressing thanks for assistance or for a compliment)
	Sorry (expressing guilt for a mistake)

Category Pain:
	Burn (has caught on fire)
	Pain (has taken a bit of damage)
	PainHeavy (has taken a large amount of damage)
	Poison (has been poisoned)

Category Reaction:
	Reload (has started a reload of some weapon)
	OutOfAmmo (has run out of ammo for some weapon)
	Surprise (reaction to a sudden positive sight)
	Shock (reaction to a sudden negative sight)
	Disgust (reaction to a sudden disgusting sight)

Category Idle:
	Cold (expressing discomfort due to the low temperature, can be a joke or just a whine)
	Hot (expressing discomfort due to the high temperature, can be a joke or just a whine)
*/