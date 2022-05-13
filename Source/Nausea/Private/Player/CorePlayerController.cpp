// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Player/CorePlayerController.h"
#include "GameFramework/GameMode.h"
#include "GameplayTagContainer.h"
#include "NauseaHelpers.h"
#include "System/CoreGameState.h"
#include "Player/CoreLocalMessage.h"
#include "Player/CorePlayerState.h"
#include "Player/PlayerStatistics/PlayerStatisticsComponent.h"
#include "Player/PlayerPromptComponent.h"
#include "Character/CoreCharacter.h"
#include "Character/VoiceComponent.h"
#include "Gameplay/StatusComponent.h"
#include "Player/PlayerClassComponent.h"
#include "Player/CorePlayerCameraManager.h"

const FName ACorePlayerController::MessageTypeSay = FName(TEXT("Say"));
const FName ACorePlayerController::MessageTypeTeamSay = FName(TEXT("TeamSay"));

ACorePlayerController::ACorePlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PlayerCameraManagerClass = ACorePlayerCameraManager::StaticClass();
	PlayerStatisticsComponent = CreateDefaultSubobject<UPlayerStatisticsComponent>(TEXT("PlayerStatisticsComponent"));
	PlayerPromptComponent = CreateDefaultSubobject<UPlayerPromptComponent>(TEXT("PlayerPromptComponent"));

	ClientMessageClass = UCoreLocalMessage::StaticClass();
}

void ACorePlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalPlayerController())
	{
		return;
	}

	ensure(GetPlayerStatisticsComponent());

	GetPlayerStatisticsComponent()->OnPlayerDataReady.AddDynamic(this, &ACorePlayerController::OnPlayerDataReady);
	GetPlayerStatisticsComponent()->LoadPlayerData();
}

void ACorePlayerController::CalcCamera(float DeltaTime, FMinimalViewInfo& OutResult)
{
	if (IsLocalController() && GetPawnOrSpectator())
	{
		GetPawnOrSpectator()->CalcCamera(DeltaTime, OutResult);
		return;
	}

	Super::CalcCamera(DeltaTime, OutResult);
}

void ACorePlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();

	//ACorePlayerController::AcknowledgePossession handles when pawn is received.
	if (GetLocalRole() != ROLE_Authority && !GetPawn())
	{
		OnPawnUpdated.Broadcast(this, Cast<ACoreCharacter>(GetPawn()));
	}
}

void ACorePlayerController::SetPawn(APawn* InPawn)
{
	if (GetLocalRole() != ROLE_Authority)
	{
		Super::SetPawn(InPawn);
		return;
	}

	APawn* CachedPreviousPawn = GetPawn();

	Super::SetPawn(InPawn);

	//If no change, no need for updating.
	if (CachedPreviousPawn == GetPawn())
	{
		return;
	}

	if (UStatusComponent* StatusComponent = UStatusInterfaceStatics::GetStatusComponent(CachedPreviousPawn))
	{
		if (StatusComponent->OnDied.IsAlreadyBound(this, &ACorePlayerController::PlayerPawnKilled))
		{
			StatusComponent->OnDied.RemoveDynamic(this, &ACorePlayerController::PlayerPawnKilled);
		}
	}
	if (UStatusComponent* StatusComponent = UStatusInterfaceStatics::GetStatusComponent(GetPawn()))
	{
		if (!StatusComponent->OnDied.IsAlreadyBound(this, &ACorePlayerController::PlayerPawnKilled))
		{
			StatusComponent->OnDied.AddDynamic(this, &ACorePlayerController::PlayerPawnKilled);
		}
	}

	OnPawnUpdated.Broadcast(this, Cast<ACoreCharacter>(InPawn));
}

void ACorePlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	OnReceivedPlayerState.Broadcast(this, Cast<ACorePlayerState>(PlayerState));
}

void ACorePlayerController::AcknowledgePossession(APawn* InPawn)
{
	if (Cast<ULocalPlayer>(Player) == nullptr)
	{
		return;
	}

	Super::AcknowledgePossession(InPawn);

	if (GetLocalRole() != ROLE_Authority)
	{
		OnPawnUpdated.Broadcast(this, Cast<ACoreCharacter>(InPawn));
	}

	//Temporarily allow for auto managed camera on possession.
	if (AcknowledgedPawn && IsLocalPlayerController())
	{
		const bool bCachedAutoManageActiveCameraTarget = bAutoManageActiveCameraTarget;
		bAutoManageActiveCameraTarget = true;
		AutoManageActiveCameraTarget(this);
		bAutoManageActiveCameraTarget = bCachedAutoManageActiveCameraTarget;
	}
}

void ACorePlayerController::SpawnPlayerCameraManager()
{
	Super::SpawnPlayerCameraManager();

	CorePlayerCameraManager = Cast<ACorePlayerCameraManager>(PlayerCameraManager);
}

void ACorePlayerController::SetCameraMode(FName NewCamMode)
{
	//Avoid redundant updates
	if (PlayerCameraManager && PlayerCameraManager->CameraStyle == NewCamMode)
	{
		return;
	}

	if (GetPlayerCameraManager())
	{
		GetPlayerCameraManager()->SetCameraStyle(NewCamMode);
	}

	if (!IsLocalPlayerController())
	{
		ClientSetCameraMode(NewCamMode);
	}
}

bool ACorePlayerController::CanRestartPlayer()
{
	if (!Super::CanRestartPlayer())
	{
		return false;
	}

	if (!GetPlayerClassComponent())
	{
		return false;
	}

	return true;
}

void ACorePlayerController::AutoManageActiveCameraTarget(AActor* SuggestedTarget)
{
	Super::AutoManageActiveCameraTarget(SuggestedTarget);
}

void ACorePlayerController::ClientTeamMessage_Implementation(APlayerState* SenderPlayerState, const FString& S, FName Type, float MsgLifeTime)
{
	Super::ClientTeamMessage_Implementation(SenderPlayerState, S, Type, MsgLifeTime);

	if (const UCoreLocalMessage* Message = ClientMessageClass.GetDefaultObject())
	{
		FClientReceiveData ClientData;
		ClientData.LocalPC = this;
		ClientData.MessageType = Type;
		ClientData.MessageIndex = 0;
		ClientData.MessageString = S;
		ClientData.RelatedPlayerState_1 = SenderPlayerState;
		ClientData.RelatedPlayerState_2 = nullptr;
		ClientData.OptionalObject = nullptr;
		Message->ClientReceive(ClientData);
	}
}

#define BIND_NONBLOCKING_AXIS_INPUT(AxisName, Func)\
FInputAxisBinding& AxisBinding##AxisName = InputComponent->BindAxis(#AxisName, this, &Func);\
AxisBinding##AxisName.bConsumeInput = false;\

void ACorePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	check(InputComponent);

	BIND_NONBLOCKING_AXIS_INPUT(Turn, ACorePlayerController::AddYawInput);
	BIND_NONBLOCKING_AXIS_INPUT(TurnRate, ACorePlayerController::AddYawInput);
	BIND_NONBLOCKING_AXIS_INPUT(LookUp, ACorePlayerController::AddPitchInput);
	BIND_NONBLOCKING_AXIS_INPUT(LookUpRate, ACorePlayerController::AddPitchInput);
}

FGenericTeamId ACorePlayerController::GetGenericTeamId() const
{
	return GetOwningPlayerState() ? GetOwningPlayerState()->GetGenericTeamId() : FGenericTeamId::NoTeam;
}

ACorePlayerState* ACorePlayerController::GetOwningPlayerState() const
{
	return Cast<ACorePlayerState>(PlayerState);
}

EPlayerClassSelectionResponse ACorePlayerController::SelectPlayerClass(TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant)
{
	uint8 RequestID;
	return SelectPlayerClass(PlayerClass, Variant, RequestID);
}

static uint8 SelectPlayerClassRequestID = 0;
EPlayerClassSelectionResponse ACorePlayerController::SelectPlayerClass(TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant, uint8& RequestID)
{
	GetPlayerStatisticsComponent()->SetPlayerClassSelection(PlayerClass, Variant);

	EPlayerClassSelectionResponse SelectionResponse = CanSelectPlayerClass(PlayerClass, Variant);
	if (SelectionResponse != EPlayerClassSelectionResponse::Success)
	{
		return SelectionResponse;
	}

	if (GetLocalRole() != ROLE_Authority)
	{
		SelectPlayerClassRequestID++;
		Server_Reliable_SelectPlayerClass(PlayerClass, Variant, SelectPlayerClassRequestID);
		RequestID = SelectPlayerClassRequestID;
		return EPlayerClassSelectionResponse::Pending;
	}

	ACorePlayerState* CorePlayerState = GetPlayerState<ACorePlayerState>();
	CorePlayerState->SetPlayerClassComponent(PlayerClass, Variant);
	return SelectionResponse;
}

EInventorySelectionResponse ACorePlayerController::SetInventorySelection(TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant, const TArray<TSubclassOf<UInventory>>& InInventorySelection)
{
	if (!GetPlayerStatisticsComponent())
	{
		return EInventorySelectionResponse::Invalid;
	}

	return GetPlayerStatisticsComponent()->SetInventorySelection(PlayerClass, Variant, InInventorySelection);
}

EInventorySelectionResponse ACorePlayerController::SetInventorySelection(TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant, const TArray<TSubclassOf<UInventory>>& InInventorySelection, uint8& RequestID)
{
	if (!GetPlayerStatisticsComponent())
	{
		return EInventorySelectionResponse::Invalid;
	}

	return GetPlayerStatisticsComponent()->SetInventorySelection(PlayerClass, Variant, InInventorySelection);
}

EPlayerClassSelectionResponse ACorePlayerController::CanSelectPlayerClass(TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant) const
{
	if (GetPlayerClassComponent() && GetPlayerClassComponent()->GetClass() == PlayerClass && GetPlayerClassComponent()->GetVariant() == Variant)
	{
		return EPlayerClassSelectionResponse::AlreadySelected;
	}

	ACoreGameState* CoreGameState = GetWorld()->GetGameState<ACoreGameState>();

	if (!CoreGameState || !CoreGameState->CanPlayerSelectPlayerClass(this, PlayerClass))
	{
		return EPlayerClassSelectionResponse::Invalid;
	}

	return EPlayerClassSelectionResponse::Success;
}

void ACorePlayerController::OnPlayerClassSelectionChange(TSubclassOf<UPlayerClassComponent> PlayerClass)
{
	for (const FSelectedPlayerClassUpdatedSignature& Delegate : SelectedPlayerClassDelegateList)
	{
		Delegate.ExecuteIfBound(PlayerClass);
	}
}

void ACorePlayerController::OnPlayerClassVariantSelectionChange(TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant)
{
	for (const FSelectedPlayerClassVariantUpdatedSignature& Delegate : SelectedPlayerClassVariantDelegateList)
	{
		Delegate.ExecuteIfBound(PlayerClass, Variant);
	}
}

void ACorePlayerController::BindPlayerClassSelectionChangeDelegate(const FSelectedPlayerClassUpdatedSignature& Delegate)
{
	if (!GetPlayerStatisticsComponent())
	{
		return;
	}

	SelectedPlayerClassDelegateList.Add(Delegate);
}

void ACorePlayerController::BindPlayerClassVariantSelectionChangeDelegate(const FSelectedPlayerClassVariantUpdatedSignature& Delegate)
{
	if (!GetPlayerStatisticsComponent())
	{
		return;
	}

	SelectedPlayerClassVariantDelegateList.Add(Delegate);
}

TSoftClassPtr<UPlayerClassComponent> ACorePlayerController::GetSelectedPlayerClass() const
{
	if (!GetPlayerStatisticsComponent())
	{
		return nullptr;
	}

	return GetPlayerStatisticsComponent()->GetSelectedPlayerClass();
}

EPlayerClassVariant ACorePlayerController::GetSelectedPlayerClassVariant(TSubclassOf<UPlayerClassComponent> PlayerClass) const
{
	if (!GetPlayerStatisticsComponent())
	{
		return EPlayerClassVariant::Invalid;
	}

	return GetPlayerStatisticsComponent()->GetSelectedPlayerClassVariant(TSoftClassPtr<UPlayerClassComponent>(PlayerClass));
}

bool ACorePlayerController::SetSelectedPlayerClass(TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant)
{
	if (!GetPlayerStatisticsComponent())
	{
		return false;
	}

	return GetPlayerStatisticsComponent()->SetPlayerClassSelection(PlayerClass, Variant);
}

bool ACorePlayerController::SetSelectedPlayerClassVariant(TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant)
{
	if (!GetPlayerStatisticsComponent())
	{
		return false;
	}

	return GetPlayerStatisticsComponent()->SetPlayerClassVariantSelection(PlayerClass, Variant);
	return false;
}

TArray<TSubclassOf<UInventory>> ACorePlayerController::GetInitialInventorySelection(TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant) const
{
	if (!PlayerClass || Variant == EPlayerClassVariant::Invalid || Variant == EPlayerClassVariant::Special)
	{
		return TArray<TSubclassOf<UInventory>>();
	}

	const TArray<TSoftClassPtr<UInventory>>& PlayerSelection = GetPlayerStatisticsComponent()->GetInventorySelection(PlayerClass, Variant);

	if (PlayerSelection.Num() == 0)
	{
		return FNauseaHelpers::ConvertFromSoftClass(UPlayerClassComponent::GetDefaultInventoryListFromClass(PlayerClass, Variant));
	}

	return FNauseaHelpers::ConvertFromSoftClass(PlayerSelection);
}

void ACorePlayerController::ForceCameraMode(FName NewCameraMode)
{
	Super::SetCameraMode(NewCameraMode);
}

void ACorePlayerController::ExecSetCameraMode(const FString& NewCameraMode)
{
	SetCameraMode(FName(*NewCameraMode));
}

void ACorePlayerController::ResetPlayerStats()
{

}

void ACorePlayerController::SendPlayerSay(const FString& String)
{
	Server_Reliable_Say(String);
}

bool ACorePlayerController::Server_Reliable_Say_Validate(const FString& String)
{
	return true;
}

void ACorePlayerController::Server_Reliable_Say_Implementation(const FString& String)
{
	if (String.IsEmpty())
	{
		return;
	}

	if (AGameMode* GameMode = (GetWorld() ? GetWorld()->GetAuthGameMode<AGameMode>() : nullptr))
	{
		GameMode->Broadcast(this, String, NAME_Say);
	}
}

bool ACorePlayerController::Server_Reliable_TeamSay_Validate(const FString& String)
{
	return true;
}

void ACorePlayerController::Server_Reliable_TeamSay_Implementation(const FString& String)
{

}

bool ACorePlayerController::Server_Reliable_AdminSay_Validate(const FString& String)
{
	return true;
}

void ACorePlayerController::Server_Reliable_AdminSay_Implementation(const FString& String)
{

}

void ACorePlayerController::ReceivedLocalMessage(const FClientReceiveData& ClientData)
{
	OnReceivedLocalMessage.Broadcast(ClientData.MessageType, ClientData.MessageIndex, ClientData.MessageString, Cast<ACorePlayerState>(ClientData.RelatedPlayerState_1), Cast<ACorePlayerState>(ClientData.RelatedPlayerState_2), ClientData.OptionalObject);
}

void ACorePlayerController::OnPlayerDataReady()
{
	TSubclassOf<UPlayerClassComponent> SelectedSublcass(GetSelectedPlayerClass().IsValid() ? GetSelectedPlayerClass().Get() : nullptr);
	if (SelectedSublcass)
	{
		SelectPlayerClass(SelectedSublcass, GetSelectedPlayerClassVariant(SelectedSublcass));
	}
	else if (ACoreGameState* CoreGameState = GetWorld()->GetGameState<ACoreGameState>())
	{
		const TArray<TSubclassOf<UPlayerClassComponent>>& PlayerClassList = CoreGameState->GetPlayerClassList();
		if (PlayerClassList.IsValidIndex(0))
		{
			SelectPlayerClass(PlayerClassList[0], GetSelectedPlayerClassVariant(PlayerClassList[0]));
		}
	}
}

bool ACorePlayerController::Server_Reliable_SelectPlayerClass_Validate(TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant, uint8 RequestID)
{
	return true;
}

void ACorePlayerController::Server_Reliable_SelectPlayerClass_Implementation(TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant, uint8 RequestID)
{
	EPlayerClassSelectionResponse Response = SelectPlayerClass(PlayerClass, Variant, RequestID);
	Client_Reliable_SelectPlayerClassResponse(PlayerClass, Variant, RequestID, Response);
}

void ACorePlayerController::Client_Reliable_SelectPlayerClassResponse_Implementation(TSubclassOf<UPlayerClassComponent> PlayerClass, EPlayerClassVariant Variant, uint8 ReqID, EPlayerClassSelectionResponse Response)
{
	OnPlayerClassSelectionResponse.Broadcast(this, PlayerClass, Variant, Response, ReqID);
}

void ACorePlayerController::PlayerPawnKilled(UStatusComponent* Component, float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	ACoreCharacter* CoreCharacter = Cast<ACoreCharacter>(Component->GetOwner());

	Client_Reliable_PlayerPawnKilled(CoreCharacter, DamageEvent.DamageTypeClass, Damage, EventInstigator ? EventInstigator->GetPlayerState<APlayerState>() : nullptr);
}

void ACorePlayerController::Client_Reliable_PlayerPawnKilled_Implementation(ACoreCharacter* KilledCharacter, TSubclassOf<UDamageType> DamageType, float Damage, APlayerState* KillerPlayerState)
{
	OnPlayerPawnKilled.Broadcast(KilledCharacter, DamageType, Damage, KillerPlayerState);
}