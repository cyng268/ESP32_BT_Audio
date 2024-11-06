#include <driver/i2s.h>

// DumbDisplay configuration
#define BLUETOOTH "ESP32BT"
#include "esp32dumbdisplay.h"
DumbDisplay dumbdisplay(new DDBluetoothSerialIO(BLUETOOTH));

// INMP441 I2S pin configuration
#define I2S_WS 23
#define I2S_SD 21
#define I2S_SCK 22
#define I2S_PORT I2S_NUM_0

// Audio configuration
#define SoundSampleRate 16000
#define SoundNumChannels 1
#define StreamBufferNumBytes 256 

#define I2S_DMA_BUF_LEN 1024
#define I2S_DMA_BUF_COUNT 8

// Global variables
const char* SoundName = "recorded_sound";
bool started = false;
int amplifyFactor = 10;
const int MaxAmplifyFactor = 20;
int soundChunkId = -1;
int what = 0;  // 0=none, 1=mic, 2=record, 3=play
bool updateAmplifyFactor = false;

// Buffer for audio data
int16_t StreamBuffer[StreamBufferNumBytes / 2];

// DumbDisplay layers
PlotterDDLayer* plotterLayer;
LcdDDLayer* micTabLayer;
LcdDDLayer* recTabLayer;
LcdDDLayer* playTabLayer;
LcdDDLayer* startBtnLayer;
LcdDDLayer* stopBtnLayer;
LcdDDLayer* amplifyLblLayer;
LedGridDDLayer* amplifyMeterLayer;

// Connection version tracker
DDConnectVersionTracker cvTracker(-1);

void i2s_install() {
    const i2s_config_t i2s_config = {
        .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SoundSampleRate,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = I2S_DMA_BUF_COUNT,
        .dma_buf_len = I2S_DMA_BUF_LEN,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };
    i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
}

void i2s_setpin() {
    const i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK,
        .ws_io_num = I2S_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_SD
    };
    i2s_set_pin(I2S_PORT, &pin_config);
}

void setup() {
    Serial.begin(115200);
    
    // Initialize I2S
    i2s_install();
    i2s_setpin();
    i2s_start(I2S_PORT);

    // Set up DumbDisplay
    dumbdisplay.recordLayerSetupCommands();
    
    // Create layers
    plotterLayer = dumbdisplay.createPlotterLayer(300, 100);
    plotterLayer->border(1, "gray");
    
    micTabLayer = dumbdisplay.createLcdLayer(8, 1);
    micTabLayer->writeCenteredLine("MIC");
    micTabLayer->border(1, "gray");
    micTabLayer->enableFeedback("f");
    
    recTabLayer = dumbdisplay.createLcdLayer(8, 1);
    recTabLayer->writeCenteredLine("REC");
    recTabLayer->border(1, "gray");
    recTabLayer->enableFeedback("f");
    
    playTabLayer = dumbdisplay.createLcdLayer(8, 1);
    playTabLayer->writeCenteredLine("PLAY");
    playTabLayer->border(1, "gray");
    playTabLayer->enableFeedback("f");
    
    startBtnLayer = dumbdisplay.createLcdLayer(8, 1);
    startBtnLayer->writeCenteredLine("Start");
    startBtnLayer->border(1, "gray");
    startBtnLayer->enableFeedback("f");
    
    stopBtnLayer = dumbdisplay.createLcdLayer(8, 1);
    stopBtnLayer->writeCenteredLine("Stop");
    stopBtnLayer->border(1, "gray");
    stopBtnLayer->enableFeedback("f");
    
    amplifyLblLayer = dumbdisplay.createLcdLayer(20, 1);
    amplifyLblLayer->writeCenteredLine("Amplification");
    
    amplifyMeterLayer = dumbdisplay.createLedGridLayer(MaxAmplifyFactor, 1, 1, 2);
    amplifyMeterLayer->onColor("darkblue");
    amplifyMeterLayer->offColor("lightgray");
    amplifyMeterLayer->border(0.2, "blue");
    amplifyMeterLayer->enableFeedback("fa:rpt50");

    // Layout configuration
    DDAutoPinConfig builder('V');
    builder
        .addLayer(plotterLayer)
        .beginGroup('H')
            .addLayer(micTabLayer)
            .addLayer(recTabLayer)
            .addLayer(playTabLayer)
        .endGroup()
        .beginGroup('H')
            .addLayer(startBtnLayer)
            .addLayer(stopBtnLayer)
        .endGroup()
        .beginGroup('S')
            .addLayer(amplifyLblLayer)
            .addLayer(amplifyMeterLayer)
        .endGroup();
    dumbdisplay.configAutoPin(builder.build());
    
    dumbdisplay.playbackLayerSetupCommands("esp32ddmice");

    // Set idle callback
    // dumbdisplay.setIdleCalback([](long idleForMillis) {
    //     started = false;
    // });
}

void loop() {
    if (cvTracker.checkChanged(dumbdisplay)) {
        updateAmplifyFactor = true;
    }

    // Handle UI feedback
    if (micTabLayer->getFeedback()) {
        what = 1;
    } else if (recTabLayer->getFeedback()) {
        what = 2;
    } else if (playTabLayer->getFeedback()) {
        what = 3;
    }

    const DDFeedback* feedback = amplifyMeterLayer->getFeedback();
    if (feedback != NULL) {
        amplifyFactor = feedback->x + 1;
        updateAmplifyFactor = true;
    }

    if (updateAmplifyFactor) {
        for (int i = 0; i < MaxAmplifyFactor; i++) {
            // amplifyMeterLayer->turnLed(i, 0, i < amplifyFactor);
        }
        updateAmplifyFactor = false;
    }

    if (startBtnLayer->getFeedback()) {
        if (!started) {
            if (what == 1) {
                soundChunkId = dumbdisplay.streamSound16(SoundSampleRate, SoundNumChannels);
                dumbdisplay.writeComment(String("STARTED mic streaming with chunk id [") + soundChunkId + "]");
                Serial.println("startstream");
            } else if (what == 2) {
                soundChunkId = dumbdisplay.saveSoundChunked16(SoundName, SoundSampleRate, SoundNumChannels);
                Serial.println("start record");
                dumbdisplay.writeComment(String("STARTED record streaming with chunk id [") + soundChunkId + "]");
            }
            started = true;
        }
    }

    if (stopBtnLayer->getFeedback()) {
        started = false;
    }

    if (started && (what == 1 || what == 2)) {
        size_t bytesRead = 0;
        esp_err_t result = i2s_read(I2S_PORT, &StreamBuffer, StreamBufferNumBytes, &bytesRead, portMAX_DELAY);

        if (result == ESP_OK) {
            int samplesRead = bytesRead / 2;
            
            // Apply amplification
            for (int i = 0; i < samplesRead; i++) {
                StreamBuffer[i] *= amplifyFactor;
            }

            // Plot audio waveform
            plotterLayer->clear();
            for (int i = 0; i < samplesRead; i++) {
                // plotterLayer->plot(i, StreamBuffer[i] / 32768.0);
            }

            // Send audio data
            dumbdisplay.sendSoundChunk16(soundChunkId, StreamBuffer, samplesRead, false);
        }
    }
}