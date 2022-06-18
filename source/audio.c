#include <math.h>
#include <string.h>

#include <3ds.h>

#define SAMPLERATE 22050
#define SAMPLESPERBUF (SAMPLERATE / 30)
#define BYTESPERSAMPLE 4

ndspWaveBuf waveBuf[2];
u32 *audioBuffer;
bool fillBlock = false;
size_t stream_offset = 0;
int note = 4;
int notefreq[] = {
	262, 294, 339, 349, 392, 440,
	494, 440, 392, 349, 339, 294
};

void fill_buffer(void *audioBuffer,size_t offset, size_t size, int frequency ) {
	u32 *dest = (u32*)audioBuffer;

	for (int i=0; i<size; i++) {
		s16 sample = INT16_MAX * sin(frequency*(2*M_PI)*(offset+i)/SAMPLERATE);

		dest[i] = (sample<<16) | (sample & 0xffff);
	}

	DSP_FlushDataCache(audioBuffer,size);

}

void audio_init(void)
{
	audioBuffer = (u32*)linearAlloc(SAMPLESPERBUF*BYTESPERSAMPLE*2);
	
	ndspInit();

	ndspSetOutputMode(NDSP_OUTPUT_STEREO);

	ndspChnSetInterp(0, NDSP_INTERP_LINEAR);
	ndspChnSetRate(0, SAMPLERATE);
	ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);

	float mix[12];
	memset(mix, 0, sizeof(mix));
	mix[0] = 1.0;
	mix[1] = 1.0;
	ndspChnSetMix(0, mix);

	memset(waveBuf,0,sizeof(waveBuf));
	waveBuf[0].data_vaddr = &audioBuffer[0];
	waveBuf[0].nsamples = SAMPLESPERBUF;
	waveBuf[1].data_vaddr = &audioBuffer[SAMPLESPERBUF];
	waveBuf[1].nsamples = SAMPLESPERBUF;

	fill_buffer(audioBuffer, stream_offset, SAMPLESPERBUF * 2, notefreq[note]);

	stream_offset += SAMPLESPERBUF;

	ndspChnWaveBufAdd(0, &waveBuf[0]);
	ndspChnWaveBufAdd(0, &waveBuf[1]);
}

void audio_exit(void)
{
	ndspExit();

	linearFree(audioBuffer);
}

void audio_play(void)
{
	if (waveBuf[fillBlock].status == NDSP_WBUF_DONE) {
		fill_buffer(waveBuf[fillBlock].data_pcm16, stream_offset, waveBuf[fillBlock].nsamples,notefreq[note]);

		ndspChnWaveBufAdd(0, &waveBuf[fillBlock]);
		stream_offset += waveBuf[fillBlock].nsamples;

		fillBlock = !fillBlock;
	}
}
