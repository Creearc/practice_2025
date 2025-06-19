// AAudioCaptureActor.h
#pragma once

#include "CoreMinimal.h"
#include "AudioCapture.h" 
#include "GameFramework/Actor.h"
#include "AAudioCaptureActor.generated.h"

// Forward-декларации:
// UAudioCapture используется как указатель
class UAudioCapture;
// FAudioGeneratorHandle возвращается из AddGeneratorDelegate
struct FAudioGeneratorHandle;

UCLASS()
class VOICE_CAPTURE_API AAudioCaptureActor : public AActor
{
    GENERATED_BODY()

public:
    AAudioCaptureActor();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    // ===== Поля для захвата аудио =====

    // Объект для захвата аудио; UPROPERTY, чтобы GC не удалил преждевременно
    UPROPERTY()
    UAudioCapture* AudioCapture = nullptr;

    // Хендл делегата захвата, чтобы можно было убрать привязку позже
    FAudioGeneratorHandle AudioGenHandle;

    // Буфер накопленных сэмплов во время записи
    TArray<float> AudioBuffer;

    // Флаги для записи с ID
    bool bRecordingWithID = false;
    int32 RecordingID = 0;

    // ===== Методы ввода =====
    void SetupInputBindings();

    // Начало и остановка захвата
    void StartRecording();
    void StartRecordingWithID();
    void StopRecording();

    // ===== Обработка и сохранение =====

    // Вынесено в отдельный метод: сохранить WAV и вызвать функцию заглушку
    void ProcessAndSaveRecording();

    // Заглушка распознавания; если id не нужно, можно передавать только путь
    void OnRecognitionComplete(const FString& FilePath = FString(), int32 InID = 0);

    // ===== UI / Выход =====

    // Показ виджета кнопки «Выйти из игры» (в реализации)
    void ShowExitWidget();

    // Обработчик клика кнопки выхода из игрового UI
    void OnExitButtonClicked();
};
