#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <iostream>
#include <mmdeviceapi.h>
#include <Audioclient.h>
#include <audiopolicy.h>
#include <atlstr.h>
#include <functiondiscoverykeys_devpkey.h>
#include <time.h>
#include <cstdio>

#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

#define SAFE_RELEASE(punk)  \
              if ((punk) != NULL)  \
                { (punk)->Release(); (punk) = NULL; }

WAVEFORMATEX* MixFormat(WAVEFORMATEXTENSIBLE* _MixFormat) { return &(_MixFormat->Format); }
UINT32 SamplesPerSecond(WAVEFORMATEXTENSIBLE* _MixFormat) { return _MixFormat->Format.nSamplesPerSec; }
UINT32 BytesPerSample(WAVEFORMATEXTENSIBLE* _MixFormat) { return _MixFormat->Format.wBitsPerSample / 8; }

void Write_WAV(BYTE* data, int dataSize)
{
    FILE* fp;
    const BYTE RIFF[] = { 'R', 'I', 'F', 'F' };
    const int RIFF_Chunksize = dataSize + 32;
    const BYTE WAVE[] = { 'W', 'A', 'V', 'E' };
    const BYTE FMT[] = { 'f', 'm', 't', ' ' };
    const int FMT_Chuncksize = 16;
    const BYTE FMT_AudioFormat[] = { 0x01, 0x00 }; //little
    const BYTE FMT_NumberOfChannel[] = { 0x02, 0x00 }; //little
    const int FMT_SampleRate = 44100;
    const int FMT_ByteRate = 352800;
    const BYTE FMT_BlockAlign[] = { 0x04, 0x00 }; // little
    const BYTE FMT_BitPerSample[] = { 0x08, 0x00 }; //little
    const BYTE WaveData[] = { 'd', 'a', 't', 'a' };
    unsigned long littleEndian_datasize = dataSize;

    fp = fopen("D:\\test.wav", "w");
    fwrite(RIFF, 1, 4, fp);
    fwrite(&RIFF_Chunksize, 1, 4, fp);
    fwrite(WAVE, 1, 4, fp);
    fwrite(FMT, 1, 4, fp);
    fwrite(&FMT_Chuncksize, 1, 4, fp);
    fwrite(FMT_AudioFormat, 1, 2, fp);
    fwrite(FMT_NumberOfChannel, 1, 2, fp);
    fwrite(&FMT_SampleRate, 1, 4, fp);
    fwrite(&FMT_ByteRate, 1, 4, fp);
    fwrite(FMT_BlockAlign, 1, 2, fp);
    fwrite(FMT_BitPerSample, 1, 2, fp);
    fwrite(WaveData, 1, 4, fp);
    fwrite(&littleEndian_datasize, 1, 4, fp);
    fwrite(data, 1, dataSize, fp);
    // fprintf(fp, (const char*)WaveData);

}

int main()
{
    HRESULT hr;

    IPropertyStore* propertyStore = nullptr;
    PROPVARIANT name;

    IMMDeviceEnumerator* m_DeviceEnumerator = nullptr;
    IAudioClient* m_AudioClient = nullptr;
    IAudioCaptureClient* m_CaptureClient = nullptr;
    IMMDevice* CaptureDevice;

    WAVEFORMATEXTENSIBLE* TargetFormat;
    int FrameSize;
    UINT32 TargetLatency = 20;
    int TargetDuration = 10;

    UINT32 BufferSize;

    size_t CaptureBufferSize;

    hr = CoInitialize(NULL);
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&m_DeviceEnumerator);

    // Device Setup
    hr = m_DeviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &CaptureDevice);

    hr = CaptureDevice->OpenPropertyStore(STGM_READ, &propertyStore);
    PropVariantInit(&name);
    hr = propertyStore->GetValue(PKEY_Device_FriendlyName, &name);
    std::cout << CW2A(name.pwszVal) << std::endl;

    hr = CaptureDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&m_AudioClient);
    if (FAILED(hr))
        std::cout << "Error Activate" << std::endl;

    // Load Format
    hr = m_AudioClient->GetMixFormat((WAVEFORMATEX**)&TargetFormat);
    if (FAILED(hr))
        std::cout << "Error GetMixFormat" << std::endl;

    FrameSize = (TargetFormat->Format.wBitsPerSample / 8) * TargetFormat->Format.nChannels;

    hr = m_AudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, TargetLatency * 10000, 0, MixFormat(TargetFormat), NULL);
    if (FAILED(hr))
        std::cout << "Error Initialize" << std::endl;

    hr = m_AudioClient->GetBufferSize(&BufferSize);
    if (FAILED(hr))
        std::cout << "Error GetBufferSize" << std::endl;

    hr = m_AudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&m_CaptureClient);
    if (FAILED(hr))
        std::cout << "Error GetService" << std::endl;

    // Capture Start
    CaptureBufferSize = SamplesPerSecond(TargetFormat) * TargetDuration * FrameSize;
    std::cout << CaptureBufferSize << std::endl;
    BYTE* CaptureBuffer = new BYTE[CaptureBufferSize];

    hr = m_AudioClient->Start();
    if (FAILED(hr))
        std::cout << "Error Start" << std::endl;

    bool Playing = true;
    time_t start = time(NULL), end;
    UINT CurrentBufferIndex = 0;
    UINT32 framesAvailable;
    DWORD  flags;
    BYTE* data;
    UINT32 packetLength = 0;

    std::cout << TargetDuration << "초 녹음을 시작합니다." << std::endl;
    while (Playing)
    {
        Sleep(30);
        end = time(NULL);

        if (end - start >= TargetDuration)
        {
            hr = m_AudioClient->Stop();
            if (FAILED(hr))
                std::cout << "Error Stop" << std::endl;
            std::cout << sizeof(CaptureBuffer) << std::endl;
            Playing = false;
            break;
        }

        hr = m_CaptureClient->GetBuffer(&data, &framesAvailable, &flags, NULL, NULL);
        if (FAILED(hr))
        {
            std::cout << "녹음 된 바이트를 얻지 못했습니다." << std::endl;
            break;
        }

        const UINT BytesToCopy = framesAvailable * FrameSize;

        if (BytesToCopy == 0)
            continue;

        if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
            ZeroMemory(&CaptureBuffer[CurrentBufferIndex], BytesToCopy);
        else
            CopyMemory(&CaptureBuffer[CurrentBufferIndex], data, BytesToCopy);

        CurrentBufferIndex += BytesToCopy;
        hr = m_CaptureClient->ReleaseBuffer(framesAvailable);
    }
    std::cout << CurrentBufferIndex << std::endl;

    Write_WAV(CaptureBuffer, CurrentBufferIndex);

    return 0;
}