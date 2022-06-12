// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#include "Character/CoreCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "GameFramework/GameModeBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/AudioComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Player/CorePlayerController.h"
#include "Player/CorePlayerState.h"
#include "Player/CorePlayerCameraManager.h"
#include "Player/NauseaCameraComponent.h"
#include "Character/CoreCharacterMovementComponent.h"
#include "Gameplay/PlayerOwnedStatusComponent.h"
#include "Gameplay/CoreDamageType.h"
#include "Weapon/InventoryManagerComponent.h"
#include "Gameplay/AbilityComponent.h"
#include "Gameplay/PawnInteractionComponent.h"
#include "Player/CoreWidgetInteractionComponent.h"
#include "Character/VoiceComponent.h"
#include "Character/CoreCharacterAnimInstance.h"
#include "System/ReplicatedObjectInterface.h"

inline void UpdatePlayerSkeletalMesh(USkeletalMeshComponent* Mesh)
{
	if (!Mesh)
	{
		return;
	}

	Mesh->AlwaysLoadOnClient = true;
	Mesh->PrimaryComponentTick.TickGroup = TG_PrePhysics;
	static FName MeshCollisionProfileName(TEXT("CharacterMesh"));
	Mesh->SetCollisionProfileName(MeshCollisionProfileName);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	Mesh->bEnablePhysicsOnDedicatedServer = false;
}

ACoreCharacter::ACoreCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCoreCharacterMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
	BaseEyeHeight = 64.0f;
	CrouchedEyeHeight = 40.f;

	CoreMovementComponent = Cast<UCoreCharacterMovementComponent>(GetCharacterMovement());

	StatusComponent = CreateDefaultSubobject<UPlayerOwnedStatusComponent>(TEXT("StatusComponent"));
	InventoryManager = CreateOptionalDefaultSubobject<UInventoryManagerComponent>(TEXT("InventoryManagerComponent"));
	AbilityComponent = CreateOptionalDefaultSubobject<UAbilityComponent>(TEXT("AbilityComponent"));

	PawnInteractionComponent = CreateOptionalDefaultSubobject<UPawnInteractionComponent>(TEXT("PawnInteractionComponent"));
	WidgetInteractionComponent = CreateOptionalDefaultSubobject<UCoreWidgetInteractionComponent>(TEXT("WidgetInteractionComponent"));

	VoiceComponent = CreateOptionalDefaultSubobject<UVoiceComponent>(TEXT("VoiceComponent"));

	FirstPersonCamera = CreateDefaultSubobject<UNauseaCameraComponent>(TEXT("FirstPersonCamera"));
	if (FirstPersonCamera)
	{
		FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
		FirstPersonCamera->SetRelativeLocation(FVector(0, 0, BaseEyeHeight));
		FirstPersonCamera->bUsePawnControlRotation = true;
	}

	CameraBoom = CreateOptionalDefaultSubobject<USpringArmComponent>(TEXT("FollowCameraBoom"));
	if (CameraBoom)
	{
		CameraBoom->TargetArmLength = 400.f;
		CameraBoom->bUsePawnControlRotation = true;
		CameraBoom->SetupAttachment(GetMesh());

		FollowCamera = CreateOptionalDefaultSubobject<UNauseaCameraComponent>(TEXT("FollowCamera"));
		if (FollowCamera)
		{
			FollowCamera->SetupAttachment(GetCameraBoom());
			FollowCamera->bUsePawnControlRotation = false;
		}
	}

	WeaponMesh = CreateOptionalDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	if (WeaponMesh)
	{
		WeaponMesh->SetupAttachment(GetMesh());
		UpdatePlayerSkeletalMesh(WeaponMesh);
		WeaponMesh->SetOnlyOwnerSee(false);
		WeaponMesh->SetOwnerNoSee(true);
	}

	Mesh1P = CreateOptionalDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh1P"));
	if (Mesh1P)
	{
		Mesh1P->SetupAttachment(GetFirstPersonCamera());
		UpdatePlayerSkeletalMesh(Mesh1P);
		Mesh1P->SetOnlyOwnerSee(true);
		Mesh1P->bAutoRegister = false;
		Mesh1P->bAlwaysCreatePhysicsState = false;
	}

	WeaponMesh1P = CreateOptionalDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh1P"));
	if (WeaponMesh1P)
	{
		WeaponMesh1P->SetupAttachment(GetMesh1P());
		UpdatePlayerSkeletalMesh(WeaponMesh1P);
		WeaponMesh1P->SetOnlyOwnerSee(true);
		WeaponMesh1P->bAutoRegister = false;
		WeaponMesh1P->bAlwaysCreatePhysicsState = false;
	}

	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickMontagesWhenNotRendered;
	GetMesh()->bEnablePhysicsOnDedicatedServer = false;
	
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->SetOnlyOwnerSee(false);

	if (GetWeaponMesh())
	{
		GetWeaponMesh()->SetOwnerNoSee(true);
		GetWeaponMesh()->SetOnlyOwnerSee(true);
	}

	if (GetMesh1P())
	{
		GetMesh1P()->SetOwnerNoSee(false);
		GetMesh1P()->SetOnlyOwnerSee(true);
		GetMesh1P()->SetCastShadow(false);
	}

	if (GetWeaponMesh1P())
	{
		GetWeaponMesh1P()->SetOwnerNoSee(false);
		GetWeaponMesh1P()->SetOnlyOwnerSee(true);
		GetWeaponMesh1P()->SetCastShadow(false);
	}

	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;
}

void ACoreCharacter::BeginPlay()
{
	Super::BeginPlay();

	CacheMeshList();

	if (GetController())
	{
		OnCharacterPossessed.Broadcast(this, GetController());
	}
}

bool ACoreCharacter::ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	check(Channel);
	check(Bunch);
	check(RepFlags);

	bool bWroteSomething = false;
	bool bCachedNetInitial = RepFlags->bNetInitial;

	for (UActorComponent* Component : ReplicatedComponents)
	{
		if (!Component || !Component->GetIsReplicated())
		{
			continue;
		}

		IReplicatedObjectInterface* ReplicatedObjectInterface = Cast<IReplicatedObjectInterface>(Component);
		if (ReplicatedObjectInterface && ReplicatedObjectInterface->ShouldSkipReplication(Channel, RepFlags))
		{
			continue;
		}

		//Prep bNetInitial in RepFlags for this object (adds support for objects added after owner's initial bunch to still use InitialOnly rep condition for its variables).
		RepFlags->bNetInitial = Channel->ReplicationMap.Find(Component) == nullptr;

		bWroteSomething = Component->ReplicateSubobjects(Channel, Bunch, RepFlags) || bWroteSomething;
		bWroteSomething = Channel->ReplicateSubobject(Component, *Bunch, *RepFlags) || bWroteSomething;
	}

	RepFlags->bNetInitial = bCachedNetInitial;

	return bWroteSomething;
}

void ACoreCharacter::PreRegisterAllComponents()
{
	if (IsNetMode(NM_DedicatedServer))
	{
		//TInlineComponentArray<UActorComponent*> Components;
		//GetComponents(Components);

		WeaponMesh1P->DestroyComponent();
		Mesh1P->DestroyComponent();
		FollowCamera->DestroyComponent();
		CameraBoom->DestroyComponent();
	}


	Super::PreRegisterAllComponents();
}

void ACoreCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	ensure(GetStatusComponent());
	ensure(GetInventoryManager());

}

void ACoreCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsCurrentlyFirstPersonViewedPawn())
	{
		TickCrouch(DeltaTime);
	}

	if (!IsNetMode(NM_DedicatedServer) && IsDead())
	{
		GetMesh()->SetLinearDamping(GetMesh()->GetLinearDamping() + DeltaTime * 5.f);
		GetMesh()->SetAngularDamping(GetMesh()->GetAngularDamping() + DeltaTime * 5.f);
	}
}

void ACoreCharacter::BecomeViewTarget(APlayerController* PC)
{
	Super::BecomeViewTarget(PC);

	if (PC && PC->IsLocalPlayerController())
	{
		ViewingPlayerController = Cast<ACorePlayerController>(PC);

		if (GetMesh1P())
		{
			GetMesh1P()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
			GetWeaponMesh1P()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
		}
	}

	if (ACorePlayerCameraManager* CorePlayerCameraManager = GetViewingPlayerController() ? GetViewingPlayerController()->GetPlayerCameraManager() : nullptr)
	{
		if (CorePlayerCameraManager->IsFirstPersonCamera())
		{
			SetMeshVisibility(true);
		}
		else
		{
			SetMeshVisibility(false);
		}
	}
}

void ACoreCharacter::EndViewTarget(APlayerController* PC)
{
	Super::EndViewTarget(PC);

	if (PC && PC->IsLocalController())
	{
		ViewingPlayerController = nullptr;

		if (GetMesh1P())
		{
			GetMesh1P()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
		}

		if (GetWeaponMesh1P())
		{
			GetWeaponMesh1P()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
		}
	}

	ResetMeshVisibility();
}

void ACoreCharacter::CalcCamera(float DeltaTime, struct FMinimalViewInfo& OutResult)
{
	if (!GetViewingPlayerController())
	{
		GetFollowCamera()->GetCameraView(DeltaTime, OutResult);
		return;
	}

	if (GetViewingPlayerController()->GetPlayerCameraManager() && GetViewingPlayerController()->GetPlayerCameraManager()->IsFirstPersonCamera())
	{
		GetFirstPersonCamera()->GetCameraView(DeltaTime, OutResult);
		return;
	}

	GetFollowCamera()->GetCameraView(DeltaTime, OutResult);
}

void ACoreCharacter::PossessedBy(AController* NewController)
{
	const AController* PreviousController = GetController();

	Super::PossessedBy(NewController);

	//Make sure we call AGameModeBase::SetPlayerDefaults (which then calls APawn::SetPlayerDefaults) on bNetStartup pawns (placed in map). 
	if (bNetStartup)
	{
		if (AGameModeBase* GameMode = GetWorld()->GetAuthGameMode<AGameModeBase>())
		{
			GameMode->SetPlayerDefaults(this);
		}
	}

	if (PreviousController != GetController())
	{
		OnCharacterPossessed.Broadcast(this, GetController());
	}

	//Get rid of WidgetInteractionComponent as soon as possible on the authority if we're not using it.
	if (WidgetInteractionComponent && (IsNetMode(NM_DedicatedServer) || !IsLocallyOwned()))
	{
		WidgetInteractionComponent->DestroyComponent();
	}
}

void ACoreCharacter::SetPlayerDefaults()
{
	Super::SetPlayerDefaults();

	if (GetInventoryManager())
	{
		GetInventoryManager()->SetPlayerDefaults();
	}

	if (GetStatusComponent())
	{
		GetStatusComponent()->SetPlayerDefaults();
	}

	if (GetCoreMovementComponent())
	{
		GetCoreMovementComponent()->SetPlayerDefaults();
	}
}

void ACoreCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ACoreCharacter::StartCrouch);
	PlayerInputComponent->BindAction("Crouch", IE_Released, this, &ACoreCharacter::StopCrouch);

	PlayerInputComponent->BindAxis("MoveForward", this, &ACoreCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ACoreCharacter::MoveRight);

	check(InventoryManager);
	InventoryManager->SetupInputComponent(PlayerInputComponent);

	check(PawnInteractionComponent);
	PawnInteractionComponent->SetupInputComponent(PlayerInputComponent);
	check(WidgetInteractionComponent);
	WidgetInteractionComponent->SetupInputComponent(PlayerInputComponent);
}

FRotator ACoreCharacter::GetViewRotation() const
{
	if (GetController() != nullptr)
	{
		return GetController()->GetControlRotation();
	}

	if (GetViewingPlayerController() && GetViewingPlayerController()->GetPlayerCameraManager() && !GetViewingPlayerController()->GetPlayerCameraManager()->IsFirstPersonCamera())
	{
		return GetViewingPlayerController()->GetControlRotation();
	}

	return GetBaseAimRotation();
}

void ACoreCharacter::RecalculateBaseEyeHeight()
{
	const ACharacter* DefaultCharacter = GetDefault<ACharacter>(GetClass());

	if (bIsCrouched)
	{
		BaseEyeHeight = CrouchedEyeHeight;
	}
	else
	{
		BaseEyeHeight = DefaultCharacter->BaseEyeHeight;
	}
}

bool ACoreCharacter::ShouldTakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) const
{
	if (!CanBeDamaged())
	{
		return false;
	}

	if (!GetTearOff() && GetLocalRole() != ROLE_Authority)
	{
		return false;
	}

	return true;
}

void ACoreCharacter::FaceRotation(FRotator NewControlRotation, float DeltaTime)
{
	FRotator DesiredRotation = NewControlRotation - GetActorRotation();
	DesiredRotation.Normalize();
	GetCoreMovementComponent()->ModifyRotationRate(DesiredRotation);

	Super::FaceRotation(GetActorRotation() + DesiredRotation, DeltaTime);
}

void ACoreCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	GetFirstPersonCamera()->AddWorldOffset(FVector(0.f, 0.f, HalfHeightAdjust));
}

void ACoreCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	GetFirstPersonCamera()->AddWorldOffset(FVector(0.f, 0.f, -HalfHeightAdjust));
}

FGenericTeamId ACoreCharacter::GetGenericTeamId() const
{
	if (GetOwningPlayerState())
	{
		return GetOwningPlayerState()->GetGenericTeamId();
	}

	return FGenericTeamId::NoTeam;
}

ACorePlayerState* ACoreCharacter::GetOwningPlayerState() const
{
	if (!GetPlayerState())
	{
		if (GetController())
		{
			GetController()->GetPlayerState<ACorePlayerState>();
		}

		return nullptr;
	}

	return Cast<ACorePlayerState>(GetPlayerState());
}

void ACoreCharacter::Died(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bHasDied)
	{
		return;
	}

	bHasDied = true;

	IStatusInterface::Execute_K2_Died(this, Damage, DamageEvent, EventInstigator, DamageCauser);

	if (GetLocalRole() == ROLE_Authority)
	{
		if (GetController())
		{
			GetController()->PawnPendingDestroy(this);
		}

		OnTargetableStateChanged.Broadcast(this, false);

		//Force final update with tear off.
		ForceNetUpdate();
		TearOff();
	}

	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->SetMovementMode(MOVE_None);
	}

	if (!IsNetMode(NM_DedicatedServer))
	{
		const FDeathEvent& DeathEvent = GetStatusComponent()->GetDeathEvent();
		const FVector& HitMomentum = DeathEvent.HitMomentum * (DeathEvent.DamageType ? DeathEvent.DamageType->GetDefaultObject<UDamageType>()->DamageImpulse : 1.f);

		for (TWeakObjectPtr<UPrimitiveComponent> MeshComponent : ThirdPersonMeshList)
		{
			if (!MeshComponent.IsValid())
			{
				continue;
			}

			MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			MeshComponent->SetSimulatePhysics(true);

			if (MeshComponent == GetMesh())
			{
				MeshComponent->AddImpulseAtLocation(HitMomentum, DeathEvent.HitLocation);
			}
		}
	}
	else
	{
		for (TWeakObjectPtr<UPrimitiveComponent> MeshComponent : ThirdPersonMeshList)
		{
			if (!MeshComponent.IsValid())
			{
				continue;
			}
			
			MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}


	if (IsLocallyControlled() && InputComponent)
	{
		DisableInput(GetPlayerController());
	}
}

bool ACoreCharacter::IsDead() const
{
	if (bHasDied)
	{
		return true;
	}

	return GetStatusComponent()->IsDead();
}

void ACoreCharacter::Kill(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	return GetStatusComponent()->Kill(Damage, DamageEvent, EventInstigator, DamageCauser);
}

bool ACoreCharacter::IsTargetable(const AActor* Targeter) const
{
	return !IsDead();
}

ACorePlayerController* ACoreCharacter::GetPlayerController() const
{
	return GetController<ACorePlayerController>();
}

ACorePlayerController* ACoreCharacter::GetViewingPlayerController() const
{
	if (!ViewingPlayerController || !ViewingPlayerController->IsLocalController())
	{
		return nullptr;
	}

	return ViewingPlayerController;
}

bool ACoreCharacter::IsCurrentlyViewedPawn() const
{
	if (!GetViewingPlayerController())
	{
		return false;
	}

	return GetViewingPlayerController()->IsLocalPlayerController();
}

bool ACoreCharacter::IsCurrentlyFirstPersonViewedPawn() const
{
	if (!GetViewingPlayerController() || !GetViewingPlayerController()->IsLocalPlayerController())
	{
		return false;
	}

	if (!GetViewingPlayerController()->GetPlayerCameraManager() || !GetViewingPlayerController()->GetPlayerCameraManager()->IsFirstPersonCamera())
	{
		return false;
	}

	return true;
}

bool ACoreCharacter::IsFalling() const
{
	if (GetCoreMovementComponent())
	{
		return GetCoreMovementComponent()->IsFalling();
	}

	return false;
}

void ACoreCharacter::UpdateAnimObject(const UAnimationObject* InFirstPersonAnimationObject, const UAnimationObject* InThirdPersonAnimationObject)
{
	if (InFirstPersonAnimationObject)
	{
		FirstPersonAnimationObject = InFirstPersonAnimationObject;
	}
	else
	{
		if (UCoreCharacterAnimInstance* AnimInstance1P = Cast<UCoreCharacterAnimInstance>(GetMesh1P()->GetAnimInstance()))
		{
			FirstPersonAnimationObject = AnimInstance1P->GetDefaultAnimationObject();
		}
	}
	
	if (InThirdPersonAnimationObject)
	{
		ThirdPersonAnimationObject = InThirdPersonAnimationObject;
	}
	else
	{
		if (UCoreCharacterAnimInstance* AnimInstance3P = Cast<UCoreCharacterAnimInstance>(GetMesh()->GetAnimInstance()))
		{
			ThirdPersonAnimationObject = AnimInstance3P->GetDefaultAnimationObject();
		}
	}
}

bool ACoreCharacter::IsHoveringWorldWidget() const
{
	if (GetWidgetInteractionComponent())
	{
		return GetWidgetInteractionComponent()->GetHoveredWidgetPath().IsValid();
	}

	return false;
}

void ACoreCharacter::MoveForward(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void ACoreCharacter::MoveRight(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(Direction, Value);
	}
}

void ACoreCharacter::TurnAtRate(float Rate)
{
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void ACoreCharacter::LookUpAtRate(float Rate)
{
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void ACoreCharacter::StartCrouch()
{
	Crouch();
}

void ACoreCharacter::StopCrouch()
{
	UnCrouch();
}

void ACoreCharacter::TickCrouch(float DeltaTime)
{
	check(GetCharacterMovement());

	RecalculateBaseEyeHeight();

	const float CurrentEyeHeight = GetFirstPersonCamera()->GetRelativeLocation().Z;

	const float DesiredEyeHeight = FMath::FInterpTo(CurrentEyeHeight, BaseEyeHeight, DeltaTime, GetCrouchRate());

	const FVector MeshLocationOffset = GetMesh()->GetRelativeLocation();

	if (FMath::IsNearlyEqual(DesiredEyeHeight, BaseEyeHeight, 0.01f))
	{
		GetCameraBoom()->SetRelativeLocation(FVector(MeshLocationOffset.X, MeshLocationOffset.Y, BaseEyeHeight - MeshLocationOffset.Z));
		GetFirstPersonCamera()->SetRelativeLocation(FVector(0.f, 0.f, BaseEyeHeight));
		return;
	}
	
	GetCameraBoom()->SetRelativeLocation(FVector(MeshLocationOffset.X, MeshLocationOffset.Y, DesiredEyeHeight - MeshLocationOffset.Z));
	GetFirstPersonCamera()->SetRelativeLocation(FVector(0.f, 0.f, DesiredEyeHeight));
}

void ACoreCharacter::CacheMeshList()
{
	TInlineComponentArray<UPrimitiveComponent*> Components(this);
	FirstPersonMeshList.Reserve(Components.Num());
	ThirdPersonMeshList.Reserve(Components.Num());

	for (UPrimitiveComponent* Component : Components)
	{
		if (!Component)
		{
			continue;
		}

		if (Component->bOwnerNoSee)
		{
			ThirdPersonMeshList.Add(Component);
			continue;
		}

		if (Component->bOnlyOwnerSee)
		{
			FirstPersonMeshList.Add(Component);
			Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			continue;
		}
	}

	FirstPersonMeshList.Shrink();
	ThirdPersonMeshList.Shrink();

	if (IsNetMode(NM_DedicatedServer))
	{
		return;
	}

	if (ACorePlayerCameraManager* CorePlayerCameraManager = GetViewingPlayerController() ? GetViewingPlayerController()->GetPlayerCameraManager() : nullptr)
	{
		if (CorePlayerCameraManager->IsFirstPersonCamera())
		{
			SetMeshVisibility(true);
		}
		else
		{
			SetMeshVisibility(false);
		}
	}
	else
	{
		SetMeshVisibility(false);
	}
}

void ACoreCharacter::SetMeshVisibility(bool bFirstPerson)
{
	if (!IsCurrentlyViewedPawn())
	{
		ResetMeshVisibility();
		return;
	}

	for (TWeakObjectPtr<UPrimitiveComponent> Component : FirstPersonMeshList)
	{
		if (!Component->IsRegistered())
		{
			Component->RegisterComponent();
		}

		Component->SetOnlyOwnerSee(bFirstPerson);
		Component->SetOwnerNoSee(!bFirstPerson);
	}

	for (TWeakObjectPtr<UPrimitiveComponent> Component : ThirdPersonMeshList)
	{
		Component->SetOnlyOwnerSee(!bFirstPerson);
		Component->SetOwnerNoSee(bFirstPerson);
	}
}

void ACoreCharacter::ResetMeshVisibility()
{
	for (TWeakObjectPtr<UPrimitiveComponent> Component : FirstPersonMeshList)
	{
		Component->SetOnlyOwnerSee(true);
		Component->SetOwnerNoSee(false);
	}

	for (TWeakObjectPtr<UPrimitiveComponent> Component : ThirdPersonMeshList)
	{
		Component->SetOnlyOwnerSee(false);
		Component->SetOwnerNoSee(true);
	}
}