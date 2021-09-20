#include "Arduino.h"
#include <driver/i2s.h>
#include <M5Core2.h>

#define CONFIG_I2S_BCK_PIN 12
#define CONFIG_I2S_LRCK_PIN 0
#define CONFIG_I2S_DATA_PIN 2
#define CONFIG_I2S_DATA_IN_PIN 34

#define MIC_I2S_NUMBER I2S_NUM_0

#define MIC_Unit 36 // LM393 mic unit
#define MIC_SEN_VALUE 2050 // Threshold to return true to record audio. Gain adjusted to 50%.

#define RECORD_TIME             10 // seconds
#define SAMPLE_RATE             44100
#define AUDIO_BIT               16
#define AUDIO_SAMPLE_SIZE       (AUDIO_BIT * 1024)
#define SAMPLES_NUM             ((SAMPLE_RATE/100) * 2)
#define BYTE_RATE               (1 * SAMPLE_RATE * ((AUDIO_BIT / 8))
#define FLASH_RECORD_SIZE       (BYTE_RATE * RECORD_TIME))

void wavHeader(char* wav_header, uint32_t wav_size, uint32_t sample_rate);
bool recordAudio();
bool audioSensor();