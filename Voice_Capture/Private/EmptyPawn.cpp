// EmptyPawn.cpp
#include "EmptyPawn.h"
#include "GameFramework/PlayerController.h"

AEmptyPawn::AEmptyPawn()
{
    PrimaryActorTick.bCanEverTick = false;

    // „тобы Pawn автоматически получал ввод от Player0:
    AutoPossessPlayer = EAutoReceiveInput::Player0;
}

void AEmptyPawn::BeginPlay()
{
    Super::BeginPlay();
}

void AEmptyPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);
    // Ќ≈ прив€зываем никаких движений или других Action Mapping здесь,
    // чтобы автоматически не прив€зывались клавиши управлени€
}
