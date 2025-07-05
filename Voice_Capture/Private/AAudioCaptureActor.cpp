// AAudioCaptureActor.cpp

#include "AAudioCaptureActor.h"
#include "AudioCapture.h"
#include "Sound/SampleBufferIO.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/Engine.h"
#include "Components/InputComponent.h"
#include "Misc/Paths.h"

AAudioCaptureActor::AAudioCaptureActor()
{
    PrimaryActorTick.bCanEverTick = false;
    AutoReceiveInput = EAutoReceiveInput::Player0;
}

void AAudioCaptureActor::BeginPlay()
{
    FString ModelDir = FPaths::ProjectDir() / TEXT("ThirdParty/Vosk/models/vosk-model-small-ru-0.22");
    std::string ModelPathAnsi = TCHAR_TO_UTF8(*ModelDir);
    VoiceRecognizer = MakeUnique<voice>(2, ModelPathAnsi.c_str());

    Super::BeginPlay();
    UE_LOG(LogTemp, Log, TEXT("BeginPlay called"));

    if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
    {
        EnableInput(PC);
        UE_LOG(LogTemp, Log, TEXT("Input enabled"));
    }

    if (InputComponent)
    {
        SetupInputBindings();
        UE_LOG(LogTemp, Log, TEXT("Input bindings set"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("InputComponent is null!"));
    }

    if (!AudioCapture)
    {
        AudioCapture = NewObject<UAudioCapture>(this);
        if (AudioCapture)
        {
            AudioCapture->AddToRoot();
            UE_LOG(LogTemp, Log, TEXT("AudioCapture created"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to create AudioCapture"));
        }
    }
}

void AAudioCaptureActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (AudioCapture)
    {
        if (AudioCapture->IsCapturingAudio())
        {
            AudioCapture->RemoveGeneratorDelegate(AudioGenHandle);
            AudioCapture->StopCapturingAudio();
        }

        AudioCapture->RemoveFromRoot();
        AudioCapture = nullptr;
    }

    Super::EndPlay(EndPlayReason);
}

void AAudioCaptureActor::SetupInputBindings()
{
    InputComponent->BindAction("RecordVoice", IE_Pressed, this, &AAudioCaptureActor::StartRecording);
    InputComponent->BindAction("RecordVoice", IE_Released, this, &AAudioCaptureActor::StopRecording);
    InputComponent->BindAction("RecordVoiceWithID", IE_Pressed, this, &AAudioCaptureActor::StartRecordingWithID);
    InputComponent->BindAction("RecordVoiceWithID", IE_Released, this, &AAudioCaptureActor::StopRecordingWithID);
}

void AAudioCaptureActor::StartRecording()
{
    UE_LOG(LogTemp, Log, TEXT("%s: StartRecording"), *GetName());
    AudioBuffer.Reset();

    if (AudioCapture)
    {
        if (AudioCapture->IsCapturingAudio())
        {
            AudioCapture->StopCapturingAudio();
        }
        AudioCapture->RemoveGeneratorDelegate(AudioGenHandle);
        AudioCapture->RemoveFromRoot();
        AudioCapture = nullptr;
    }

    AudioCapture = NewObject<UAudioCapture>(this);
    AudioCapture->AddToRoot();

    // Изменение: захватываем данные в сыром виде (как есть с устройства)
    AudioGenHandle = AudioCapture->AddGeneratorDelegate([this](const float* InAudio, int32 NumSamples) {
        if (NumSamples > 0 && InAudio)
        {
            AudioBuffer.Append(InAudio, NumSamples);
        }
        });

    bool bOpened = AudioCapture->OpenDefaultAudioStream();
    if (!bOpened)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Failed to open audio stream"), *GetName());
        AudioCapture->RemoveFromRoot();
        AudioCapture = nullptr;
        return;
    }

    AudioCapture->StartCapturingAudio();
    UE_LOG(LogTemp, Log, TEXT("%s: Audio capturing started"), *GetName());
}


void AAudioCaptureActor::StartRecordingWithID()
{
    RecordingID = 123;
    StartRecording();
}

void AAudioCaptureActor::StopRecordingWithID()
{
    StopRecording();
}

void AAudioCaptureActor::StopRecording()
{
    UE_LOG(LogTemp, Log, TEXT("%s: StopRecording"), *GetName());

    // 7. Проверка валидности объекта перед операциями
    if (!AudioCapture)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: AudioCapture is null"), *GetName());
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("%s: before stop"), *GetName());

    if (AudioCapture->IsCapturingAudio())
    {
        AudioCapture->RemoveGeneratorDelegate(AudioGenHandle);
        AudioCapture->StopCapturingAudio();
    }

    UE_LOG(LogTemp, Log, TEXT("%s: after stop"), *GetName());


    ProcessAndSaveRecording();

    // Очистить буфер, (необязательно)
    AudioBuffer.Reset();
}

void AAudioCaptureActor::ProcessAndSaveRecording()
{
    if (AudioBuffer.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: AudioBuffer is empty"), *GetName());
        return;
    }

    if (!AudioCapture)
    {
        UE_LOG(LogTemp, Error, TEXT("%s: AudioCapture null"), *GetName());
        return;
    }

    const int32 NumChannels = AudioCapture->GetNumChannels();
    const int32 SampleRate = AudioCapture->GetSampleRate();

    // Критическое изменение: преобразование в моно
    TArray<float> MonoBuffer;
    if (NumChannels > 1)
    {
        const int32 NumFrames = AudioBuffer.Num() / NumChannels;
        MonoBuffer.Reserve(NumFrames);

        for (int32 i = 0; i < NumFrames; ++i)
        {
            float Sum = 0.0f;
            for (int32 Channel = 0; Channel < NumChannels; ++Channel)
            {
                Sum += AudioBuffer[i * NumChannels + Channel];
            }
            MonoBuffer.Add(Sum / NumChannels);
        }
    }
    else
    {
        MonoBuffer = AudioBuffer;
    }

    // Создаем буфер с 1 каналом (моно)
    Audio::FSampleBuffer SampleBuffer(
        MonoBuffer.GetData(),
        MonoBuffer.Num(),
        1,          // Принудительно 1 канал
        SampleRate
    );

    Audio::FSoundWavePCMWriter Writer;
    FString FileName = TEXT("Recording.wav");
    FString Directory = FPaths::ProjectSavedDir();
    FString OutFilePath;

    bool bSuccess = Writer.SynchronouslyWriteToWavFile(SampleBuffer, FileName, Directory, &OutFilePath);
    if (bSuccess)
    {
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("WAV file saved"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("GEngine is nullptr!"));
        }

        UE_LOG(LogTemp, Log, TEXT("%s: WAV file saved: %s"), *GetName(), *OutFilePath);
        OnRecognitionComplete(OutFilePath);

    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("%s: Failed to save WAV file"), *GetName());
    }
}

void AAudioCaptureActor::OnRecognitionComplete(const FString& AudioFilePath)
{
    AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [this, AudioFilePath]()
        {
            if (!VoiceRecognizer)
            {
                if (GEngine)
                {
                    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("VoiceRecognizer not initialized"));
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("GEngine is nullptr!"));
                }

                UE_LOG(LogTemp, Error, TEXT("VoiceRecognizer not initialized"));
                return;
            }

            std::string PathAnsi = TCHAR_TO_UTF8(*AudioFilePath);
            std::string RecognitionResultStd = VoiceRecognizer->convertAudio(PathAnsi.c_str());
            FString RecognitionResult = UTF8_TO_TCHAR(RecognitionResultStd.c_str());

            // Возвращаемся в игровой поток, чтобы вывести результат
            AsyncTask(ENamedThreads::GameThread, [this, RecognitionResult]()
                {
                    UE_LOG(LogTemp, Log, TEXT("Recognition Complete: %s"), *RecognitionResult);
                    if (GEngine)
                    {
                        GEngine->AddOnScreenDebugMessage(-1, 6.f, FColor::Green, RecognitionResult);
                    }
                });
        });
}


void AAudioCaptureActor::OnExitButtonClicked()
{
    if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
    {
        UKismetSystemLibrary::QuitGame(this, PC, EQuitPreference::Quit, true);
    }
}
