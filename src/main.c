
/* main.c - chromatic guitar tuner */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <signal.h>
#include "libfft.h"
#include <portaudio.h>
#include <stdbool.h>
#include <string.h>
#include <float.h>

/* Constants */
#define SAMPLE_RATE 8000
#define FFT_SIZE 8192
#define FFT_EXP_SIZE 13
#define LOWPASS_CUTOFF 330.0f

/* Note Names */
const char *NOTES[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

/* Function Declarations */
void applyHanWindow(float *data, int size, const float *window);
void computeSecondOrderLowPassParameters(float srate, float f, float *a, float *b);
float processSecondOrderFilter(float x, float *mem, const float *a, const float *b);
void signalHandler(int signum);
int findNearestNote(float freq, const float *freqTable, const char **noteNameTable,
                    float *nearestNotePitch, int tableSize);

/* Global Variables */
static volatile bool running = true;
// Pre-calculated Hanning window
static float window[FFT_SIZE];

int main(int argc, char **argv)
{
   PaStreamParameters inputParameters;
   float a[2], b[3], mem1[4], mem2[4];
   float data[FFT_SIZE];
   float datai[FFT_SIZE];
   float freqTable[FFT_SIZE / 2];
   const char *noteNameTable[FFT_SIZE / 2] = {NULL};
   float notePitchTable[FFT_SIZE / 2];
   void *fft = NULL;
   PaStream *stream = NULL;
   PaError err = paNoError;
   struct sigaction action;

   // Signal Handling
   action.sa_handler = signalHandler;
   sigemptyset(&action.sa_mask);
   action.sa_flags = 0;
   sigaction(SIGINT, &action, NULL);
   sigaction(SIGHUP, &action, NULL);
   sigaction(SIGTERM, &action, NULL);

   // Initialize FFT, Filter, and Window
   fft = initfft(FFT_EXP_SIZE);
   computeSecondOrderLowPassParameters(SAMPLE_RATE, LOWPASS_CUTOFF, a, b);
   memset(mem1, 0, sizeof(mem1));
   memset(mem2, 0, sizeof(mem2));

   // Pre-calculate Hanning window
   for (int i = 0; i < FFT_SIZE; ++i)
   {
      window[i] = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (FFT_SIZE - 1.0f)));
   }

   // Frequency and Note Tables
   for (int i = 0; i < FFT_SIZE / 2; ++i)
   {
      freqTable[i] = (SAMPLE_RATE * i) / (float)FFT_SIZE;
   }

   for (int i = 0; i < 127; ++i)
   {
      float pitch = 440.0f / 32.0f * powf(2.0f, (i - 9.0f) / 12.0f);
      if (pitch > SAMPLE_RATE / 2.0f)
      {
         break;
      }

      // Find closest frequency
      // TODO: Binary search could be implemented here
      float minDiff = FLT_MAX;
      int index = -1;
      for (int j = 0; j < FFT_SIZE / 2; ++j)
      {
         float diff = fabsf(freqTable[j] - pitch);
         if (diff < minDiff)
         {
            minDiff = diff;
            index = j;
         }
      }
      if (index != -1)
      {
         noteNameTable[index] = NOTES[i % 12];
         notePitchTable[index] = pitch;
      }
   }

   // Portaudio Initialization
   err = Pa_Initialize();
   if (err != paNoError)
      goto error;

   inputParameters.device = Pa_GetDefaultInputDevice();
   inputParameters.channelCount = 1;
   inputParameters.sampleFormat = paFloat32;
   inputParameters.suggestedLatency = Pa_GetDeviceInfo(inputParameters.device)->defaultHighInputLatency;
   inputParameters.hostApiSpecificStreamInfo = NULL;

   printf("Opening %s\n", Pa_GetDeviceInfo(inputParameters.device)->name);
   err = Pa_OpenStream(&stream, &inputParameters, NULL, SAMPLE_RATE, FFT_SIZE, paClipOff, NULL, NULL);
   if (err != paNoError)
      goto error;
   err = Pa_StartStream(stream);
   if (err != paNoError)
      goto error;

   // Main Loop
   while (running)
   {
      // Read Audio Data
      err = Pa_ReadStream(stream, data, FFT_SIZE);
      if (err && err != paInputOverflowed)
         goto error; // Ignore overflows but handle other errors

      // Apply Low-Pass Filter
      for (int j = 0; j < FFT_SIZE; ++j)
      {
         data[j] = processSecondOrderFilter(data[j], mem1, a, b);
         data[j] = processSecondOrderFilter(data[j], mem2, a, b);
      }

      // Apply Window
      applyHanWindow(data, FFT_SIZE, window);

      // FFT
      memset(datai, 0, sizeof(datai));
      applyfft(fft, data, datai, false);

      // Find Peak Frequency
      float maxVal = -1.0f;
      int maxIndex = -1;
      for (int j = 0; j < FFT_SIZE / 2; ++j)
      {
         float v = data[j] * data[j] + datai[j] * datai[j];
         if (v > maxVal)
         {
            maxVal = v;
            maxIndex = j;
         }
      }

      float freq = freqTable[maxIndex];

      float nearestNotePitch;
      // Find nearest note and pitch
      int nearestNoteIndex = findNearestNote(freq, freqTable, noteNameTable, &nearestNotePitch, FFT_SIZE / 2);

      const char *nearestNoteName = (nearestNoteIndex != -1) ? noteNameTable[nearestNoteIndex] : "";

      // Calculate cents sharp
      float centsSharp = 1200.0f * log2f(freq / nearestNotePitch);

      // Output
      printf("\033[2J\033[1;1H"); // Clear screen
      printf("Tuner listening. Control-C to exit.\n");
      printf("%f Hz, %d : %f\n", freq, maxIndex, maxVal * 1000.0f);

      if (nearestNoteIndex != -1)
      {
         printf("Nearest Note: %s\n", nearestNoteName);

         if (fabsf(centsSharp) > 0.01f)
         { // Only print if significantly different from zero
            if (centsSharp > 0)
            {
               printf("%f cents sharp.\n", centsSharp);
            }
            else
            {
               printf("%f cents flat.\n", -centsSharp);
            }
         }
         else
         {
            printf("in tune!\n");
         }

         // Tuning Indicator
         int chars = 30;
         printf("\n");
         if (fabsf(centsSharp) < 0.01f || centsSharp >= 0)
         {
            for (int i = 0; i < chars; ++i)
               printf(" ");
         }
         else
         {
            for (int i = 0; i < (int)(chars + centsSharp); ++i)
               printf(" ");
            for (int i = (chars + centsSharp < 0 ? 0 : (int)(chars + centsSharp)); i < chars; ++i)
               printf("=");
         }

         printf(" %2s ", nearestNoteName);
         if (fabsf(centsSharp) > 0.01f)
         {
            for (int i = 0; i < chars && i < (int)centsSharp; ++i)
               printf("=");
         }

         printf("\n");
      }
      else
      {
         printf("No note detected.\n");
      }

      fflush(stdout); // Important for real-time output
   }

   // Cleanup
   err = Pa_StopStream(stream);
   if (err != paNoError)
      goto error;

   destroyfft(fft);
   Pa_Terminate();
   return 0;

error:
   if (stream)
   {
      Pa_AbortStream(stream);
      Pa_CloseStream(stream);
   }

   Pa_Terminate();
   fprintf(stderr, "An error occurred: %s\n", Pa_GetErrorText(err));
   return 1;
}

void applyHanWindow(float *data, int size, const float *window)
{
   for (int i = 0; i < size; ++i)
   {
      data[i] *= window[i];
   }
}

void computeSecondOrderLowPassParameters(float srate, float f, float *a, float *b)
{
   float a0;
   float w0 = 2.0f * M_PI * f / srate;
   float cosw0 = cosf(w0);
   float sinw0 = sinf(w0);
   float alpha = sinw0 / 2.0f * sqrtf(2.0f);

   a0 = 1.0f + alpha;
   a[0] = (-2.0f * cosw0) / a0;
   a[1] = (1.0f - alpha) / a0;
   b[0] = (1.0f - cosw0) / (2.0f * a0);
   b[1] = (1.0f - cosw0) / a0;
   b[2] = b[0];
}

float processSecondOrderFilter(float x, float *mem, const float *a, const float *b)
{
   float ret = b[0] * x + b[1] * mem[0] + b[2] * mem[1] - a[0] * mem[2] - a[1] * mem[3];
   mem[1] = mem[0];
   mem[0] = x;
   mem[3] = mem[2];
   mem[2] = ret;
   return ret;
}

void signalHandler(int signum)
{
   running = false;
}

int findNearestNote(float freq, const float *freqTable, const char **noteNameTable,
                    float *nearestNotePitch, int tableSize)
{

   float minDiff = FLT_MAX;
   int nearestIndex = -1;

   for (int i = 0; i < tableSize; ++i)
   {
      // Make sure a note exists at this index
      if (noteNameTable[i] != NULL)
      {
         float diff = fabsf(freqTable[i] - freq);
         if (diff < minDiff)
         {
            minDiff = diff;
            nearestIndex = i;
         }
      }
   }

   if (nearestIndex != -1)
   {
      *nearestNotePitch = freqTable[nearestIndex];
      return nearestIndex;
   }
   else
   {
      // Or handle the case where no note is found differently
      *nearestNotePitch = 0.0f;
      return -1; // Indicate no nearest note found
   }
}