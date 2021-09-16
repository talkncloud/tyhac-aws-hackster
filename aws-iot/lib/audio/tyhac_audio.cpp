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

static int16_t i2s_readraw_buff[AUDIO_SAMPLE_SIZE];
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
    // i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM);

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

    if (!file)
    {
        Serial.println("TYHAC: Error writing to SD card");
        return false;
    };

    byte wav_header[WAVE_HEADER_SIZE];
    createWavHeader(wav_header, FLASH_RECORD_SIZE);
    file.write(wav_header, WAVE_HEADER_SIZE);

    // Initialise i2s
    initI2SMic();

    size_t bytes_read;

    int flash_wr_size = 0;
    Serial.println("TYHAC: Recording...");
    while (flash_wr_size < FLASH_RECORD_SIZE)
    {
        i2s_read(MIC_I2S_NUMBER, (char *)i2s_readraw_buff, AUDIO_SAMPLE_SIZE, &bytes_read, portMAX_DELAY);
        file.write((const byte *)i2s_readraw_buff, AUDIO_SAMPLE_SIZE);
        flash_wr_size += bytes_read;
    }

    file.close();
    Serial.println("TYHAC: Audio recording completed");
    return true;
}

/*
*   createWavHeader(wav_header, file_size)
*   Wav files have a specific set of requirements, this will create the file wav based on those requirements and
*   our audio requirements.
*
*   Source/Credit: https://github.com/Makerfabs/Project_ESP32-Voice-Interaction/tree/master/example/ESP32_Record_Play
*/
void createWavHeader(byte *wav_header, int wave_data_size)
{
    wav_header[0] = 'R';
    wav_header[1] = 'I';
    wav_header[2] = 'F';
    wav_header[3] = 'F';
    unsigned int fileSizeMinus8 = wave_data_size + 44 - 8;
    wav_header[4] = (byte)(fileSizeMinus8 & 0xFF);
    wav_header[5] = (byte)((fileSizeMinus8 >> 8) & 0xFF);
    wav_header[6] = (byte)((fileSizeMinus8 >> 16) & 0xFF);
    wav_header[7] = (byte)((fileSizeMinus8 >> 24) & 0xFF);
    wav_header[8] = 'W';
    wav_header[9] = 'A';
    wav_header[10] = 'V';
    wav_header[11] = 'E';
    wav_header[12] = 'f';
    wav_header[13] = 'm';
    wav_header[14] = 't';
    wav_header[15] = ' ';
    wav_header[16] = 0x10; // linear PCM
    wav_header[17] = 0x00;
    wav_header[18] = 0x00;
    wav_header[19] = 0x00;
    wav_header[20] = 0x01; // linear PCM
    wav_header[21] = 0x00;
    wav_header[22] = 0x01; // monoral
    wav_header[23] = 0x00;
    wav_header[24] = 0x44; // sampling rate 44100
    wav_header[25] = 0xAC;
    wav_header[26] = 0x00;
    wav_header[27] = 0x00;
    wav_header[28] = 0x88; // Byte/sec = 44100x2x1 = 88200
    wav_header[29] = 0x58;
    wav_header[30] = 0x01;
    wav_header[31] = 0x00;
    wav_header[32] = 0x02; // 16bit monoral
    wav_header[33] = 0x00;
    wav_header[34] = 0x10; // 16bit
    wav_header[35] = 0x00;
    wav_header[36] = 'd';
    wav_header[37] = 'a';
    wav_header[38] = 't';
    wav_header[39] = 'a';
    wav_header[40] = (byte)(wave_data_size & 0xFF);
    wav_header[41] = (byte)((wave_data_size >> 8) & 0xFF);
    wav_header[42] = (byte)((wave_data_size >> 16) & 0xFF);
    wav_header[43] = (byte)((wave_data_size >> 24) & 0xFF);
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