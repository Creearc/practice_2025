// AAudioCaptureActor.cpp

#include "AAudioCaptureActor.h"
#include "AudioCapture.h"            // для UAudioCapture и FAudioGeneratorHandle
#include "Sound/SampleBufferIO.h"    // для FSampleBuffer, FSoundWavePCMWriter
#include "Kismet/GameplayStatics.h"  // для GetPlayerController
#include "Kismet/KismetSystemLibrary.h" // для QuitGame, если нужен
#include "Engine/Engine.h"           // для GEngine->AddOnScreenDebugMessage, UE_LOG
#include "Components/InputComponent.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "UObject/ConstructorHelpers.h"


AAudioCaptureActor::AAudioCaptureActor()
{
    PrimaryActorTick.bCanEverTick = false;
    AutoReceiveInput = EAutoReceiveInput::Player0;
}

void AAudioCaptureActor::BeginPlay()
{
    Super::BeginPlay();
    UE_LOG(LogTemp, Log, TEXT("%s: BeginPlay called"), *GetName());

    if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
    {
        EnableInput(PC);
        UE_LOG(LogTemp, Log, TEXT("%s: EnableInput on PC"), *GetName());
    }

    if (InputComponent)
    {
        UE_LOG(LogTemp, Log, TEXT("%s: InputComponent valid, binding actions"), *GetName());
        SetupInputBindings();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: InputComponent is NULL"), *GetName());
    }

    // Enable input для этого актора
    if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
    {
        EnableInput(PC);
    }

    // Привязка действий ввода
    if (InputComponent)
    {
        InputComponent->BindAction("RecordVoice", IE_Pressed, this, &AAudioCaptureActor::StartRecording);
        InputComponent->BindAction("RecordVoice", IE_Released, this, &AAudioCaptureActor::StopRecording);
        InputComponent->BindAction("RecordVoiceWithID", IE_Pressed, this, &AAudioCaptureActor::StartRecordingWithID);
        InputComponent->BindAction("RecordVoiceWithID", IE_Released, this, &AAudioCaptureActor::StopRecording);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: InputComponent is null in BeginPlay"), *GetName());
    }

    // Инициализируем AudioCapture один раз и открываем поток
    if (!AudioCapture)
    {
        AudioCapture = NewObject<UAudioCapture>(this);
        if (AudioCapture)
        {
            // Предотвращаем GC; т.к. UPROPERTY хранит, часто AddToRoot не обязателен, но можно:
            AudioCapture->AddToRoot();

            bool bOpened = AudioCapture->OpenDefaultAudioStream();
            if (bOpened)
            {
                UE_LOG(LogTemp, Log, TEXT("%s: Audio stream opened successfully"), *GetName());
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("%s: Failed to open default audio stream"), *GetName());
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("%s: Failed to create UAudioCapture object"), *GetName());
        }
    }
}

void AAudioCaptureActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Безопасно остановим захват, удалим делегат и очистим AudioCapture
    if (AudioCapture)
    {
        if (AudioCapture->IsCapturingAudio())
        {
            // Удаляем привязанный делегат
            AudioCapture->RemoveGeneratorDelegate(AudioGenHandle);
            AudioCapture->StopCapturingAudio();
            UE_LOG(LogTemp, Log, TEXT("%s: Audio capturing stopped in EndPlay"), *GetName());
        }
        // Очищаем объект
        AudioCapture->RemoveFromRoot();
        AudioCapture = nullptr;
    }

    Super::EndPlay(EndPlayReason);
}

void AAudioCaptureActor::StartRecording()
{
    UE_LOG(LogTemp, Log, TEXT("%s: StartRecording"), *GetName());

    // Сброс буфера
    AudioBuffer.Reset();

    // Если ранее AudioCapture был создан, но в неактивном состоянии, можно переиспользовать
    if (!AudioCapture)
    {
        AudioCapture = NewObject<UAudioCapture>(this);
        AudioCapture->AddToRoot(); // чтобы GC не удалил раньше времени
    }
    else
    {
        // Если он был занят предыдущим делегатом или состоянием, остановим его
        if (AudioCapture->IsCapturingAudio())
        {
            AudioCapture->StopCapturingAudio();
            AudioCapture->RemoveGeneratorDelegate(AudioGenHandle);
        }
    }

    // Привязываем делегат и сохраняем хендл
    AudioGenHandle = AudioCapture->AddGeneratorDelegate([this](const float* InAudio, int32 NumSamples) {
        if (NumSamples > 0 && InAudio)
        {
            float MaxAmp = 0.f;
            for (int32 i = 0; i < NumSamples; ++i)
            {
                MaxAmp = FMath::Max(MaxAmp, FMath::Abs(InAudio[i]));
            }
            UE_LOG(LogTemp, Log, TEXT("%s: Captured %d samples, MaxAmp=%f"), *GetName(), NumSamples, MaxAmp);
            AudioBuffer.Append(InAudio, NumSamples);
        }
        });

    // Если поток ещё не открыт, откроем
    // AudioCapture->OpenDefaultAudioStream() лучше вызывать один раз в BeginPlay. 
    // Но если нужно, можно повторно открывать:
    bool bOpened = AudioCapture->OpenDefaultAudioStream();
    if (!bOpened)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Failed to open audio stream in StartRecording"), *GetName());
    }

    AudioCapture->StartCapturingAudio();
    UE_LOG(LogTemp, Log, TEXT("%s: AudioCapture->StartCapturingAudio() called"), *GetName());
}

void AAudioCaptureActor::StartRecordingWithID()
{
    UE_LOG(LogTemp, Log, TEXT("%s: StartRecordingWithID"), *GetName());
    bRecordingWithID = true;
    RecordingID = 123; // пример

    StartRecording(); // можно переиспользовать логику,
    // или скопировать код StartRecording, но с учётом флага bRecordingWithID
}

void AAudioCaptureActor::StopRecording()
{
    UE_LOG(LogTemp, Log, TEXT("%s: StopRecording"), *GetName());
    if (!AudioCapture || !AudioCapture->IsCapturingAudio())
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: No active capture to stop"), *GetName());
        return;
    }

    // Удаляем делегат перед остановкой
    AudioCapture->RemoveGeneratorDelegate(AudioGenHandle);

    AudioCapture->StopCapturingAudio();

    // Сохраняем WAV; имя может быть фиксированным, чтобы перезаписывать:
    ProcessAndSaveRecording();

    // Если хочешь пересоздавать AudioCapture каждый раз, очисти:
    AudioCapture->RemoveFromRoot();
    AudioCapture = nullptr;
}

void AAudioCaptureActor::ProcessAndSaveRecording()
{
    if (AudioBuffer.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: AudioBuffer is empty, nothing to save"), *GetName());
        OnRecognitionComplete(); // всё равно вызвать заглушку, если нужно
        return;
    }

    if (!AudioCapture)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: AudioCapture null in ProcessAndSaveRecording"), *GetName());
        OnRecognitionComplete();
        return;
    }

    int32 NumChannels = AudioCapture->GetNumChannels();
    int32 SampleRate = AudioCapture->GetSampleRate();

    Audio::FSampleBuffer SampleBuffer(AudioBuffer.GetData(), AudioBuffer.Num(), NumChannels, SampleRate);
    Audio::FSoundWavePCMWriter Writer;

    FString FileName = TEXT("RecordedAudio.wav");
    FString Directory = FPaths::ProjectSavedDir(); // например Saved/
    FString OutFilePath;
    bool bSuccess = Writer.SynchronouslyWriteToWavFile(SampleBuffer, FileName, Directory, &OutFilePath);

    if (bSuccess)
    {
        UE_LOG(LogTemp, Log, TEXT("%s: WAV file saved to: %s"), *GetName(), *OutFilePath);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Failed to save WAV file"), *GetName());
    }

    // Вызов заглушки распознавания;
    OnRecognitionComplete();
}

void AAudioCaptureActor::OnRecognitionComplete(const FString& FilePath, int32 InID)
{
    if (GEngine)
    {
        FString Msg = FString::Printf(TEXT("Здесь мог быть обработанный текст"), *FilePath, InID);
        GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, Msg);
    }
}


void AAudioCaptureActor::OnExitButtonClicked()
{
    // Если у вас есть логика выхода из игры через этот метод:
    if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
    {
        UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, true);
    }
}

void AAudioCaptureActor::SetupInputBindings()
{
    // Пока можно оставить пустым
}

