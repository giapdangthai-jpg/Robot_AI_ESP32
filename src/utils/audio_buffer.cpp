#include "audio_buffer.h"

int16_t AudioBuffer::buffer[8192];
size_t AudioBuffer::writeIndex = 0;
size_t AudioBuffer::readIndex = 0;
SemaphoreHandle_t AudioBuffer::mutex = nullptr;

void AudioBuffer::init() {
    mutex = xSemaphoreCreateMutex();
}

bool AudioBuffer::push(int16_t* data, size_t samples) {

    if (!mutex) return false;
    xSemaphoreTake(mutex, portMAX_DELAY);

    for (size_t i = 0; i < samples; i++) {
        buffer[writeIndex++] = data[i];
        if (writeIndex >= 8192) writeIndex = 0;
    }

    xSemaphoreGive(mutex);
    return true;
}

bool AudioBuffer::pop(int16_t* data, size_t samples) {

    if (!mutex) return false;
    xSemaphoreTake(mutex, portMAX_DELAY);
    
    if (readIndex == writeIndex) {
    xSemaphoreGive(mutex);
    return false;
    }

    for (size_t i = 0; i < samples; i++) {
        data[i] = buffer[readIndex++];
        if (readIndex >= 8192) readIndex = 0;
    }

    xSemaphoreGive(mutex);
    return true;
}
