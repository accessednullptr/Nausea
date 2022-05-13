// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.

#pragma once

#include "CoreMinimal.h"
#include "Character/PilotCharacter.h"
#include "GenericTeamAgentInterface.h"
#include "Player/PlayerOwnershipInterface.h"
#include "Gameplay/StatusInterface.h"
#include "AI/EnemySelection/AITargetInterface.h"
#include "Gameplay/DamageLogInterface.h"
#include "CoreCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UPrimitiveComponent;
class USkeletalMeshComponent;
class ACorePlayerController;
class ACorePlayerState;
class UCoreCharacterMovementComponent;
class UCoreCharacterComponent;
class UInventoryManagerComponent;
class UAnimationObject;
class UPawnInteractionComponent;
class UCoreWidgetInteractionComponent;
class UVoiceComponent;
class UAbilityComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCharacterPossessedSignature, ACoreCharacter*, Character, AController*, Controller);

UCLASS(HideFunctions = (K2_GetStatusComponent, K2_Died, K2_GetDamageLogInstigatorName))
class NAUSEA_API ACoreCharacter : public APilotCharacter,
	public IGenericTeamAgentInterface,
	public IPlayerOwnershipInterface,
	public IStatusInterface,
	public IDamageLogInterface,
	public IAITargetInterface
{
	GENERATED_UCLASS_BODY()

//~ Begin AActor Interface
protected:
	virtual void BeginPlay() override;
public:
	virtual bool ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags) override;
	virtual void PreRegisterAllComponents() override;
	virtual void PostInitializeComponents() override;
	virtual void Tick(float DeltaTime) override;
	virtual void BecomeViewTarget(APlayerController* PC) override;
	virtual void EndViewTarget(APlayerController* PC) override;
	virtual void CalcCamera(float DeltaTime, struct FMinimalViewInfo& OutResult) override;
//~ End AActor Interface

//~ Begin APawn Interface
public:
	virtual void PossessedBy(AController* NewController) override;
	virtual void SetPlayerDefaults() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual FRotator GetViewRotation() const override;
	virtual void RecalculateBaseEyeHeight() override;
	virtual bool ShouldTakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) const override;
//~ End APawn Interface

//~ Begin ACharacter Interface
public:
	virtual void FaceRotation(FRotator NewControlRotation, float DeltaTime) override;
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
//~ End ACharacter Interface

//~ Begin IGenericTeamAgentInterface Interface
public:
	virtual FGenericTeamId GetGenericTeamId() const override;
//~ End IGenericTeamAgentInterface Interface

//~ Begin IPlayerOwnershipInterface Interface
public:
	virtual ACorePlayerState* GetOwningPlayerState() const override;
	virtual UVoiceComponent* GetVoiceComponent() const override { return VoiceComponent; }
	virtual UAbilityComponent* GetAbilityComponent() const override { return AbilityComponent; }
//~ End IPlayerOwnershipInterface Interface

//~ Begin IStatusInterface Interface
public:
	virtual UStatusComponent* GetStatusComponent() const override { return StatusComponent; }
	virtual void Died(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
protected:
	UPROPERTY(VisibleDefaultsOnly, Category = Character)
	UStatusComponent* StatusComponent = nullptr;
//~ End IStatusInterface Interface

	//IStatusInterface convenience functions.
public:
	bool IsDead() const;
	void Kill(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser);

//~ Begin IAITargetInterface Interface
public:
	virtual bool IsTargetable(const AActor* Targeter = nullptr) const override;
	virtual FTargetableStateChanged& GetTargetableStateChangedDelegate() override { return OnTargetableStateChanged; }
protected:
	UPROPERTY()
	FTargetableStateChanged OnTargetableStateChanged;
//~ End IAITargetInterface Interface

public:
	UFUNCTION(BlueprintCallable, Category = Character)
	ACorePlayerController* GetPlayerController() const;
	//Returns the player controller locally viewing this pawn (if they are viewing it).
	UFUNCTION(BlueprintCallable, Category = Character)
	ACorePlayerController* GetViewingPlayerController() const;
	UFUNCTION(BlueprintCallable, Category = Character)
	bool IsCurrentlyViewedPawn() const;
	UFUNCTION(BlueprintCallable, Category = Character)
	bool IsCurrentlyFirstPersonViewedPawn() const;
	
	UFUNCTION(BlueprintCallable, Category = Character)
	bool IsCurrentlyReplayingMoves() const { return bClientUpdating; }

	UFUNCTION(BlueprintCallable, Category = Character)
	bool IsFalling() const;
	
	UFUNCTION(BlueprintCallable, Category = Character)
	UCoreCharacterMovementComponent* GetCoreMovementComponent() const { return CoreMovementComponent; }
	UFUNCTION(BlueprintCallable, Category = Character)
	UInventoryManagerComponent* GetInventoryManager() const { return InventoryManager; }

	UFUNCTION(BlueprintCallable, Category = Character)
	UPawnInteractionComponent* GetPawnInteractionComponent() const { return PawnInteractionComponent; }
	UFUNCTION(BlueprintCallable, Category = Character)
	UCoreWidgetInteractionComponent* GetWidgetInteractionComponent() const { return WidgetInteractionComponent; }

	UFUNCTION(BlueprintCallable, Category = Character)
	USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	UFUNCTION(BlueprintCallable, Category = Character)
	USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	UFUNCTION(BlueprintCallable, Category = Character)
	USkeletalMeshComponent* GetWeaponMesh1P() const { return WeaponMesh1P; }

	void UpdateAnimObject(const UAnimationObject* InFirstPersonAnimationObject, const UAnimationObject* InThirdPersonAnimationObject);
	const UAnimationObject* GetFirstPersonAnimObject() const { return FirstPersonAnimationObject; }
	const UAnimationObject* GetThirdPersonAnimObject() const { return ThirdPersonAnimationObject; }

	UFUNCTION(BlueprintCallable, Category = Character)
	UCameraComponent* GetFirstPersonCamera() const { return FirstPersonCamera; }
	UFUNCTION(BlueprintCallable, Category = Character)
	USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	UFUNCTION(BlueprintCallable, Category = Character)
	UCameraComponent* GetFollowCamera() const { return FollowCamera; }
	
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category = Character)
	bool IsHoveringWorldWidget() const;

	void MoveForward(float Value);
	void MoveRight(float Value);

	void TurnAtRate(float Rate);
	void LookUpAtRate(float Rate);

	virtual void StartCrouch();
	virtual void StopCrouch();

	UFUNCTION()
	virtual float GetCrouchRate() const { return CrouchRate; }
	UFUNCTION()
	virtual void TickCrouch(float DeltaTime);

protected:
	//Used to cache which meshes are third person meshes and which ones are first person.
	UFUNCTION()
	virtual void CacheMeshList();
	UFUNCTION()
	virtual void SetMeshVisibility(bool bFirstPerson);
	UFUNCTION()
	virtual void ResetMeshVisibility();

public:
	UPROPERTY(BlueprintAssignable, Category = Character)
	FCharacterPossessedSignature OnCharacterPossessed;

protected:
	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Character)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Character)
	float BaseLookUpRate;

	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<UPrimitiveComponent>> ThirdPersonMeshList;
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<UPrimitiveComponent>> FirstPersonMeshList;

	UPROPERTY(Transient)
	bool bHasDied = false;

	UPROPERTY(EditDefaultsOnly, Category=Crouch)
	float CrouchRate = 8.f;
	UPROPERTY(Transient)
	float CrouchAmount = 0.f;

	UPROPERTY(EditDefaultsOnly, Category=Crouch)
	FText PawnName = FText::FromString("Pawn");

private:
	UPROPERTY()
	UCoreCharacterMovementComponent* CoreMovementComponent = nullptr;

	UPROPERTY(VisibleDefaultsOnly, Category = Character, meta = (AllowPrivateAccess = "true"))
	UInventoryManagerComponent* InventoryManager = nullptr;

	UPROPERTY(VisibleDefaultsOnly, Category = Character, meta = (AllowPrivateAccess = "true"))
	UPawnInteractionComponent* PawnInteractionComponent = nullptr;

	UPROPERTY(VisibleDefaultsOnly, Category = Character, meta = (AllowPrivateAccess = "true"))
	UCoreWidgetInteractionComponent* WidgetInteractionComponent = nullptr;

	UPROPERTY(VisibleDefaultsOnly, Category = Character, meta = (AllowPrivateAccess = "true"))
	UVoiceComponent* VoiceComponent = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	UAudioComponent* VoiceAudioComponent = nullptr;

	UPROPERTY(VisibleDefaultsOnly, Category = Character, meta = (AllowPrivateAccess = "true"))
	UAbilityComponent* AbilityComponent = nullptr;

	UPROPERTY(VisibleDefaultsOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* WeaponMesh = nullptr;
	UPROPERTY(VisibleDefaultsOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* Mesh1P = nullptr;
	UPROPERTY(VisibleDefaultsOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* WeaponMesh1P = nullptr;

	UPROPERTY(VisibleDefaultsOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCamera = nullptr;
	UPROPERTY(VisibleDefaultsOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom = nullptr;
	UPROPERTY(VisibleDefaultsOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera = nullptr;
	
	UPROPERTY(Transient)
	const UAnimationObject* FirstPersonAnimationObject = nullptr;
	UPROPERTY(Transient)
	const UAnimationObject* ThirdPersonAnimationObject = nullptr;

	//Local client only. Used to track the controller currently viewing this actor.
	UPROPERTY()
	ACorePlayerController* ViewingPlayerController = nullptr;
};
