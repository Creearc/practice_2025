// EmptyPawn.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "EmptyPawn.generated.h"

UCLASS()
class VOICE_CAPTURE_API AEmptyPawn : public APawn
{
    GENERATED_BODY()

public:
    AEmptyPawn();

protected:

    virtual void BeginPlay() override;

    // Переопределим SetupPlayerInputComponent, но НЕ будем привязывать движение.
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};
