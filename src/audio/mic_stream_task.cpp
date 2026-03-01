#include <driver/i2s.h>
#include "../config/pinmap.h"
#include "../utils/audio_buffer.h"
#include "vad.h"

static void micTask(void* pv) {

    int16_t frame[AUDIO_FRAME_SAMPLES];
    size_t bytesRead;

    while (true) {

        i2s_read(I2S_NUM_0,
                 frame,
                 AUDIO_FRAME_BYTES,
                 &bytesRead,
                 portMAX_DELAY);

        if (bytesRead == AUDIO_FRAME_BYTES) {

            if (VAD::isSpeech(frame, AUDIO_FRAME_SAMPLES)) {
                AudioBuffer::push(frame, AUDIO_FRAME_SAMPLES);
            }
        }
    }
}

