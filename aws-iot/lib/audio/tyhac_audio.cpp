/*
* Description:  This handles all aspects of audio for the main program.
*               If audio is detected from the external sound sensor (LM393) the internal mic (SPM1423) records
*               x amount of seconds of audio. The audio is recorded in WAV format and written to the SD CARD.
*  
* Author:       Mick Jacobsson - (https://www.talkncloud.com)
*/

#include <tyhac_audio.h>
#include "env.h"

File file;

// static int16_t i2s_readraw_buff[AUDIO_SAMPLE_SIZE];
static uint8_t i2s_readraw_buff[AUDIO_SAMPLE_SIZE];
int data_offset = 0;
const int WAVE_HEADER_SIZE = 44;
int micSensor = 0;

/*
*   initI2SMic()
*   Uses i2s to read from the internal m5 stack core 2 microphone. The SPM1423 microphone is built into the unit.
*   The sampling rate and bits etc can be adjusted in the .h for this program.
*   
*
*   Source/Credit: https://github.com/m5stack/M5Core2
*                  https://www.youtube.com/watch?v=CwIWpBqa-nM
*                  https://github.com/atomic14/m5stack-core2-audio-monitor * Extra credit for the great work by atomic14
*/
bool initI2SMic()
{
    esp_err_t err = ESP_OK;

    i2s_driver_uninstall(MIC_I2S_NUMBER); // Uninstall the I2S driver.
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM), // Set the I2S operating mode.
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT, // Fixed 12-bit stereo MSB.
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT, // Set the channel format.
        .communication_format = I2S_COMM_FORMAT_I2S,  // Set the format of the communication.
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,     // Set the interrupt flag.
        .dma_buf_count = 2,                           // DMA buffer count.
        .dma_buf_len = 1024,                          // DMA buffer length.
    };

    // Install and drive I2S.
    err += i2s_driver_install(MIC_I2S_NUMBER, &i2s_config, 0, NULL);

    i2s_pin_config_t tx_pin_config;
    tx_pin_config.bck_io_num = CONFIG_I2S_BCK_PIN; // Link the BCK to the CONFIG_I2S_BCK_PIN pin.
    tx_pin_config.ws_io_num = CONFIG_I2S_LRCK_PIN;
    tx_pin_config.data_out_num = CONFIG_I2S_DATA_PIN;
    tx_pin_config.data_in_num = CONFIG_I2S_DATA_IN_PIN;
    err += i2s_set_pin(MIC_I2S_NUMBER, &tx_pin_config);                                           // Set the I2S pin number.
    err += i2s_set_clk(MIC_I2S_NUMBER, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO); // Set the clock and bitwidth used by I2S Rx and Tx.

    return true;
}

/*
*   recordAudio()
*   Records audio to the SD card. The same file will be used, audio will be overwritten. The audio is read is i2s
*   with the parameters being set above. The audio length is based on the file size and how it fills, adjust the
*   duration (RECORD_TIME) as needed.
*
*   Source/Credit: https://github.com/m5stack/M5Core2
*                  https://github.com/espressif/esp-idf/blob/316988bd2d3379912e40c159de953318afe5050e/examples/peripherals/i2s/i2s_audio_recorder_sdcard/main/i2s_recorder_main.c
*/
bool recordAudio()
{

    // Create file for writing on SD card
    SD.remove(TYHAC_AUDIO_SAMPLE);
    file = SD.open(TYHAC_AUDIO_SAMPLE, FILE_WRITE);

    // Check for file errors
    if (!file)
    {
        Serial.println("TYHAC: Error writing to SD card");
        return false;
    };

    // Create the wav file header
    char wav_header_fmt[WAVE_HEADER_SIZE];
    wavHeader(wav_header_fmt, FLASH_RECORD_SIZE, int(SAMPLE_RATE));

    // Write the headers
    file.write((uint8_t *)wav_header_fmt, WAVE_HEADER_SIZE);

    // Initialise i2s
    initI2SMic();

    size_t bytes_read;

    int flash_wr_size = 0;
    Serial.println("TYHAC: Recording...");
    // Fill the wav file until the file size
    while (flash_wr_size < FLASH_RECORD_SIZE)
    {
        i2s_read(MIC_I2S_NUMBER, i2s_readraw_buff, AUDIO_SAMPLE_SIZE, &bytes_read, portMAX_DELAY);
        file.write(i2s_readraw_buff, bytes_read);
        flash_wr_size += bytes_read;
    }

    // clean up
    file.close();
    Serial.println("TYHAC: Audio recording completed");
    return true;
}

/*
*   wavHeader(char* wav_header, uint32_t wav_size, uint32_t sample_rate)
*   Wav files have a specific set of requirements, this will create the file wav based on those requirements and
*   our audio requirements.
*
*   Source/Credit:  https://github.com/espressif/esp-idf/blob/316988bd2d3379912e40c159de953318afe5050e/examples/peripherals/i2s/i2s_audio_recorder_sdcard/main/i2s_recorder_main.c
*                   https://github.com/Makerfabs/Project_ESP32-Voice-Interaction/tree/master/example/ESP32_Record_Play
*/
void wavHeader(char *wav_header, uint32_t wav_size, uint32_t sample_rate)
{

    // See this for reference: http://soundfile.sapp.org/doc/WaveFormat/
    uint32_t file_size = wav_size + WAVE_HEADER_SIZE - 8;
    uint32_t byte_rate = 1 * SAMPLE_RATE * (AUDIO_BIT / 8);

    const char set_wav_header[] = {
        'R', 'I', 'F', 'F',                                                  // ChunkID
        file_size, file_size >> 8, file_size >> 16, file_size >> 24,         // ChunkSize
        'W', 'A', 'V', 'E',                                                  // Format
        'f', 'm', 't', ' ',                                                  // Subchunk1ID
        0x10, 0x00, 0x00, 0x00,                                              // Subchunk1Size (16 for PCM)
        0x01, 0x00,                                                          // AudioFormat (1 for PCM)
        0x01, 0x00,                                                          // NumChannels (1 channel)
        sample_rate, sample_rate >> 8, sample_rate >> 16, sample_rate >> 24, // SampleRate
        byte_rate, byte_rate >> 8, byte_rate >> 16, byte_rate >> 24,         // ByteRate
        0x02, 0x00,                                                          // BlockAlign
        0x10, 0x00,                                                          // BitsPerSample (16 bits)
        'd', 'a', 't', 'a',                                                  // Subchunk2ID
        wav_size, wav_size >> 8, wav_size >> 16, wav_size >> 24,             // Subchunk2Size
    };

    memcpy(wav_header, set_wav_header, sizeof(set_wav_header));
}

/*
*   audioSensor()
*   Reads the analogue pin of the LM393 mic unit. If the value is above a certain value (MIC_SEN_VALUE)
*   returns true, otherwise false. 
*
*   Unit: https://shop.m5stack.com/products/microphone-unit-lm393
*/
bool audioSensor()
{
    micSensor = analogRead(MIC_Unit);
    if (micSensor >= MIC_SEN_VALUE)
    {
        Serial.println("TYHAC: Cough detected - " + String(micSensor));
        return true;
    }
    else
    {
        // Serial.println("TYHAC: Reading audio sensor - " + String(micSensor));
        return false;
    }
}