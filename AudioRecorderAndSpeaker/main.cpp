#include <iostream>
#include <Windows.h>
#include <MMSystem.h>
using namespace std;

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32")


int main()
{
	const int NUMPTS = 44100 * 10;
	int sampleRate = 44100;
	short int waveIn[NUMPTS];

	HWAVEIN hWaveIn;
	WAVEHDR WaveInHdr;
	MMRESULT result;
	HWAVEOUT hWaveOut;

	WAVEFORMATEX pFormat;
	pFormat.wFormatTag = WAVE_FORMAT_PCM;
	pFormat.nChannels = 2;
	pFormat.nSamplesPerSec = sampleRate;
	pFormat.nAvgBytesPerSec = 4 * sampleRate;
	pFormat.nBlockAlign = 4;
	pFormat.wBitsPerSample = 16;
	pFormat.cbSize = 0;

	result = waveInOpen(&hWaveIn, WAVE_MAPPER, &pFormat, 0, 0, WAVE_FORMAT_DIRECT);

	if (result)
	{
		char fault[256];
		waveInGetErrorTextA(result, fault, 256);
		MessageBoxA(NULL, fault, "Failed to open waveform input device.", MB_OK | MB_ICONEXCLAMATION);
		return 1;
	}

	WaveInHdr.lpData = (LPSTR)waveIn;
	WaveInHdr.dwBufferLength = 2 * NUMPTS;
	WaveInHdr.dwBytesRecorded = 0;
	WaveInHdr.dwUser = 0;
	WaveInHdr.dwFlags = 0;
	WaveInHdr.dwLoops = 0;
	waveInPrepareHeader(hWaveIn, &WaveInHdr, sizeof(WAVEHDR));

	result = waveInAddBuffer(hWaveIn, &WaveInHdr, sizeof(WAVEHDR));
	if (result)
	{
		MessageBoxA(NULL, "Failed to read block from device", NULL, MB_OK | MB_ICONEXCLAMATION);
		return 1;
	}

	result = waveInStart(hWaveIn);
	if (result)
	{
		MessageBoxA(NULL, "Failed to start recording", NULL, MB_OK | MB_ICONEXCLAMATION);
		return 1;
	}

	cout << "Recording..." << endl;
	Sleep((NUMPTS / sampleRate) * 1000); //Sleep while recording

	cout << "Playing..." << endl;

	if (waveOutOpen(&hWaveOut, WAVE_MAPPER, &pFormat, 0, 0, WAVE_FORMAT_DIRECT))
	{
		MessageBoxA(NULL, "Failed to replay", NULL, MB_OK | MB_ICONEXCLAMATION);
	}

	waveOutWrite(hWaveOut, &WaveInHdr, sizeof(WaveInHdr)); //<-- The line I forgot before
	Sleep((NUMPTS / sampleRate) * 1000); //Sleep for as long as there was recorded

	waveOutUnprepareHeader(hWaveOut, &WaveInHdr, sizeof(WAVEHDR));
	waveInUnprepareHeader(hWaveIn, &WaveInHdr, sizeof(WAVEHDR));
	waveInClose(hWaveIn);
	waveOutClose(hWaveOut);

	return 0;
}
