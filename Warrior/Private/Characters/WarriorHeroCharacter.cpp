// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/WarriorHeroCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DataAssets/Input/DataAsset_InputConfig.h"
#include "Components/Input/WarriorInputComponent.h"
#include "WarriorGameplayTags.h"
#include "AbilitySystem/WarriorAbilitySystemComponent.h"
#include "DataAssets/StartUpData/DataAsset_HeroStartUpData.h"
#include "Components/Combat/HeroCombatComponent.h"
#include "Components/UI/HeroUIComponent.h"
#include "AbilitySystemBlueprintLibrary.h"

#include "WarriorDebugHelper.h"
AWarriorHeroCharacter::AWarriorHeroCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->TargetArmLength = 200.f;
	CameraBoom->SocketOffset = FVector(0.f, 55.f, 65.f);
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 500.f, 0.f);
	GetCharacterMovement()->MaxWalkSpeed = 400.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	HeroCombatComponent = CreateDefaultSubobject<UHeroCombatComponent>(TEXT("HeroCombatComponent"));

	HeroUIComponent = CreateDefaultSubobject<UHeroUIComponent>(TEXT("HeroUIComponent"));

}

UPawnCombatComponent* AWarriorHeroCharacter::GetPawnCombatComponent() const
{
	return HeroCombatComponent;
}

UPawnUIComponent* AWarriorHeroCharacter::GetPawnUIComponent() const
{
	return HeroUIComponent;
}

UHeroUIComponent* AWarriorHeroCharacter::GetHeroUIComponent() const
{
	return HeroUIComponent;
}


void AWarriorHeroCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	// IsValid表示资源是否已加载，而我们要检查的是我们是否为这个软引用分配了有效的数据资产
	if (!CharacterStartUpData.IsNull()) {
		if (UDataAsset_StartUpDataBase* LoadedData = CharacterStartUpData.LoadSynchronous()) {
			LoadedData->GiveToAbilitySystemComponent(WarriorAbilitySystemComponent);
		}
	}

	/*if (WarriorAbilitySystemComponent && WarriorAttributeSet) {
		const FString ASCText = FString::Printf(TEXT("Owner Actor: %s, AvatarActor: %s"), *WarriorAbilitySystemComponent->GetOwnerActor()->GetActorLabel(), *WarriorAbilitySystemComponent->GetAvatarActor()->GetActorLabel());
		Debug::Print(TEXT("Ability system component valid. ") + ASCText, FColor::Green);
		Debug::Print(TEXT("AttributeSet valid. ") + ASCText, FColor::Green);
	}*/
}

void AWarriorHeroCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	checkf(InputConfigDataAsset, TEXT("Forgot to assign a valid data asset as input config"));

	ULocalPlayer* LocalPlayer = GetController<APlayerController>()->GetLocalPlayer();

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);

	check(Subsystem);
	
	Subsystem->AddMappingContext(InputConfigDataAsset->DefaultMappingContext, 0);

	UWarriorInputComponent* WarriorInputComponent = CastChecked<UWarriorInputComponent>(PlayerInputComponent);

	WarriorInputComponent->BindNativeInputAction(InputConfigDataAsset, WarriorGameplayTags::InputTag_Move, ETriggerEvent::Triggered, this, &ThisClass::Input_Move);
	WarriorInputComponent->BindNativeInputAction(InputConfigDataAsset, WarriorGameplayTags::InputTag_Look, ETriggerEvent::Triggered, this, &ThisClass::Input_Look);

	WarriorInputComponent->BindNativeInputAction(InputConfigDataAsset, WarriorGameplayTags::InputTag_SwitchTarget, ETriggerEvent::Triggered, this, &ThisClass::Input_SwitchTargetTriggered);
	WarriorInputComponent->BindNativeInputAction(InputConfigDataAsset, WarriorGameplayTags::InputTag_SwitchTarget, ETriggerEvent::Completed, this, &ThisClass::Input_SwitchTargetCompleted);

	WarriorInputComponent->BindAbilityInputAction(InputConfigDataAsset, this, &ThisClass::Input_AbilityInputPressed, &ThisClass::Input_AbilityInputReleased);

}

void AWarriorHeroCharacter::BeginPlay()
{
	// 游戏开始自动调用，UE的标准做法，确保父类的初始化逻辑能够执行
	// Super是UE提供的宏，指向当前类的父亲
	Super::BeginPlay(); 

	// Debug::Print(TEXT("Working"));
}

void AWarriorHeroCharacter::Input_Move(const FInputActionValue& InputActionValue)
{
	const FVector2D MovementVector = InputActionValue.Get<FVector2D>();

	const FRotator MovementRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);

	if (MovementVector.Y != 0.f) {
		const FVector ForwardDirection = MovementRotation.RotateVector(FVector::ForwardVector);

		AddMovementInput(ForwardDirection, MovementVector.Y);
	}
	if (MovementVector.X != 0.f) {
		const FVector RightDirection = MovementRotation.RotateVector(FVector::RightVector);

		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AWarriorHeroCharacter::Input_Look(const FInputActionValue& InputActionValue)
{
	const FVector2D LookAxisVector = InputActionValue.Get<FVector2D>();

	if (LookAxisVector.X != 0.f) {
		AddControllerYawInput(LookAxisVector.X);
	}

	if (LookAxisVector.Y != 0.f) {
		AddControllerPitchInput(LookAxisVector.Y);
	}

}

void AWarriorHeroCharacter::Input_SwitchTargetTriggered(const FInputActionValue& InputActionValue)
{
	SwitchDirection = InputActionValue.Get<FVector2D>();
}

void AWarriorHeroCharacter::Input_SwitchTargetCompleted(const FInputActionValue& InputActionValue)
{
	FGameplayEventData Data;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		this,
		SwitchDirection.X > 0.f ? WarriorGameplayTags::Player_Event_SwitchTarget_Right : WarriorGameplayTags::Player_Event_SwitchTarget_Left,
		Data
	);

	//Debug::Print(TEXT("Switch Direction: ") + SwitchDirection.ToString());
}

void AWarriorHeroCharacter::Input_AbilityInputPressed(FGameplayTag InInputTag)
{
	WarriorAbilitySystemComponent->OnAbilityInputPressed(InInputTag);
}

void AWarriorHeroCharacter::Input_AbilityInputReleased(FGameplayTag InInputTag)
{
	WarriorAbilitySystemComponent->OnAbilityInputReleased(InInputTag);
}
