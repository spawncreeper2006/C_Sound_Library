#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

#define PRINT_ERROR(a, args...) printf("ERROR %s() %s Line %d: " a "\n", __FUNCTION__, __FILE__, __LINE__, ##args);

#if RAND_MAX == 32767
#define rand32() ((rand()%lt%lt16) + (rand()%lt%lt1) + (rand()&1))
#else
#define rand32() rand()
#endif

typedef struct {
	uint32_t samples;
	int32_t *data;
} sound_t;

sound_t hello;

bool LoadWav(const char *filename, sound_t *sound);

uint32_t SAMPLING_RATE;


//uint32_t CHUNK_SIZE = 2000;


//#define CHUNK_SIZE 2000

uint16_t CHUNK_SIZE;

HWAVEOUT wave_out;
WAVEHDR header[2] = {0};


//int16_t chunks[2][CHUNK_SIZE] = {0};

int16_t* chunks[2];


bool chunk_swap = false;
int16_t *to;
bool quit = false;
static uint32_t sound_position;


void CALLBACK WaveOutProc(HWAVEOUT wave_out_handle, UINT message, DWORD_PTR instance, DWORD_PTR param1, DWORD_PTR param2);


int play_samples(sound_t sound, uint32_t sample_rate, uint32_t sound_pos)
{
    hello = sound;
    SAMPLING_RATE = sample_rate;
    sound_position = sound_pos;

    CHUNK_SIZE = SAMPLING_RATE/24;


    //for (int i =0; i<2000; i++)
    //{
        chunks[0] = (int16_t*)malloc(CHUNK_SIZE*sizeof(int16_t));
        chunks[1] = (int16_t*)malloc(CHUNK_SIZE*sizeof(int16_t));

    //}

    	{
		WAVEFORMATEX format = {
			.wFormatTag = WAVE_FORMAT_PCM,
			.nChannels = 1,
			.nSamplesPerSec = SAMPLING_RATE,
			.wBitsPerSample = 16,
			.cbSize = 0,
		};
		format.nBlockAlign = format.nChannels * format.wBitsPerSample / 8;
		format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
		if(waveOutOpen(&wave_out, WAVE_MAPPER, &format, (DWORD_PTR)WaveOutProc, (DWORD_PTR)NULL, CALLBACK_FUNCTION) != MMSYSERR_NOERROR) {
			PRINT_ERROR("waveOutOpen failed");
			return -1;
		}
	}

	if(waveOutSetVolume(wave_out, 0xFFFFFFFF) != MMSYSERR_NOERROR) {
		PRINT_ERROR("waveOutSetVolume failed");
		return -1;
	}


	for(int i = 0; i < 2; ++i) {
		header[i].lpData = (CHAR*)chunks[i];
		header[i].dwBufferLength = CHUNK_SIZE * 2;
		if(waveOutPrepareHeader(wave_out, &header[i], sizeof(header[i])) != MMSYSERR_NOERROR) {
			PRINT_ERROR("waveOutPrepareHeader failed");
			return -1;
		}

		if(waveOutWrite(wave_out, &header[i], sizeof(header[i])) != MMSYSERR_NOERROR) {
			PRINT_ERROR("waveOutWrite[%d] failed", i);
			return -1;
		}
	}


	while(!quit);



}

sound_t load_sound(const char* filename)
{
    sound_t sound_to_load;
    if(!LoadWav(filename, &sound_to_load))
    {
		PRINT_ERROR("Failed to load sound");
		return sound_to_load;
	}
    return sound_to_load;
}





int main() {

    uint32_t sound_pos = 48000;
    SAMPLING_RATE = 60000;



    sound_t sound = load_sound("Afterbloom.wav");

    play_samples(sound, SAMPLING_RATE, sound_pos); //needs to take sound_t struct, curr_sample and sample_rate as parameters and return done pointer so it can be manipulated

	return 0;
}

void CALLBACK WaveOutProc(HWAVEOUT wave_out_handle, UINT message, DWORD_PTR instance, DWORD_PTR param1, DWORD_PTR param2) {


	switch(message) {
		case WOM_CLOSE: {  } break;
		case WOM_OPEN:  {  } break;
		case WOM_DONE:  {
			//printf("WOM_DONE\n");

			to = chunks[chunk_swap];

			for(int j = 0; j < CHUNK_SIZE; ++j) {
				if(sound_position < hello.samples) {
					*(to++) = hello.data[sound_position++];
				}
				else {
					quit = true;
					*(to++) = 0;
				}
			}
			if(waveOutWrite(wave_out, &header[chunk_swap], sizeof(header[chunk_swap])) != MMSYSERR_NOERROR) {
				PRINT_ERROR("waveOutWrite failed\n");
			}
			chunk_swap = !chunk_swap;
		} break;
	}
}

// Loads ONLY 16-bit 1-channel PCM .WAV files. Allocates sound->data and fills with the pcm data. Fills sound->samples with the number of ELEMENTS in sound->data. EG for 2-bytes per sample single channel, sound->samples = HALF of the number of bytes in sound->data.
bool LoadWav(const char *filename, sound_t *sound) {
	bool return_value = true;
	FILE *file;
	char magic[4];
	int32_t filesize;
	int32_t format_length;		// 16
	int16_t format_type;		// 1 = PCM
	int16_t num_channels;		// 1
	int32_t sample_rate;		// 44100
	int32_t bytes_per_second;	// sample_rate * num_channels * bits_per_sample / 8
	int16_t block_align;		// num_channels * bits_per_sample / 8
	int16_t bits_per_sample;	// 16
	int32_t data_size;

	file = fopen(filename, "rb");
	if(file == NULL) {
		PRINT_ERROR("%s: Failed to open file", filename);
		return false;
	}

	fread(magic, 1, 4, file);
	if(magic[0] != 'R' || magic[1] != 'I' || magic[2] != 'F' || magic[3] != 'F') {
		PRINT_ERROR("%s First 4 bytes should be \"RIFF\", are \"%4s\"", filename, magic);
		return_value = false;
		goto CLOSE_FILE;
	}

	fread(&filesize, 4, 1, file);

	fread(magic, 1, 4, file);
	if(magic[0] != 'W' || magic[1] != 'A' || magic[2] != 'V' || magic[3] != 'E') {
		PRINT_ERROR("%s 4 bytes should be \"WAVE\", are \"%4s\"", filename, magic);
		return_value = false;
		goto CLOSE_FILE;
	}

	fread(magic, 1, 4, file);
	if(magic[0] != 'f' || magic[1] != 'm' || magic[2] != 't' || magic[3] != ' ') {
		PRINT_ERROR("%s 4 bytes should be \"fmt/0\", are \"%4s\"", filename, magic);
		return_value = false;
		goto CLOSE_FILE;
	}

	fread(&format_length, 4, 1, file);
	fread(&format_type, 2, 1, file);
	if(format_type != 1) {
		PRINT_ERROR("%s format type should be 1, is %d", filename, format_type);
		return_value = false;
		goto CLOSE_FILE;
	}

	fread(&num_channels, 2, 1, file);
	//if(num_channels != 1) {
	//	PRINT_ERROR("%s Number of channels should be 1, is %d", filename, num_channels);
	//	return_value = false;
	//	goto CLOSE_FILE;
	//}

	fread(&sample_rate, 4, 1, file);
	//if(sample_rate != 44100) {
	//	PRINT_ERROR("%s Sample rate should be 44100, is %d", filename, sample_rate);
	//	return_value = false;
	//	goto CLOSE_FILE;
	//}

	fread(&bytes_per_second, 4, 1, file);
	fread(&block_align, 2, 1, file);
	fread(&bits_per_sample, 2, 1, file);
	//if(bits_per_sample != 16) {
	//	PRINT_ERROR("%s bits per sample should be 16, is %d", filename, bits_per_sample);
	//	return_value = false;
	//	goto CLOSE_FILE;
	//}

	fread(magic, 1, 4, file);
	if(magic[0] != 'd' || magic[1] != 'a' || magic[2] != 't' || magic[3] != 'a') {
		PRINT_ERROR("%s 4 bytes should be \"data\", are \"%4s\"", filename, magic);
		return_value = false;
		goto CLOSE_FILE;
	}

	fread(&data_size, 4, 1, file);

	sound->data = malloc(data_size);

	if(sound->data == NULL) {
		PRINT_ERROR("%s Failed to allocate %d bytes for data", filename, data_size);
		return_value = false;
		goto CLOSE_FILE;
	}

	if(fread(sound->data, 1, data_size, file) != data_size) {
		PRINT_ERROR("%s Failed to read data bytes", filename);
		return_value = false;
		free(sound->data);
		goto CLOSE_FILE;
	}

	sound->samples = data_size / 2;

	CLOSE_FILE:
	fclose(file);

	return return_value;
}
