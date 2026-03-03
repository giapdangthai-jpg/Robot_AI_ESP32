#include "audio_buffer.h"

AudioBuffer g_micBuf;
AudioBuffer g_spkBuf;

void AudioBuffer::init() {
    _mutex = xSemaphoreCreateMutex();
    _writeIdx = 0;
    _readIdx  = 0;
    _count    = 0;
}

void AudioBuffer::clear() {
    if (!_mutex) return;
    xSemaphoreTake(_mutex, portMAX_DELAY);
    _writeIdx = 0;
    _readIdx  = 0;
    _count    = 0;
    xSemaphoreGive(_mutex);
}

bool AudioBuffer::push(const int16_t* data, size_t samples) {
    if (!_mutex || !data || samples == 0) return false;
    xSemaphoreTake(_mutex, portMAX_DELAY);

    if (samples > (kCapacity - _count)) {
        xSemaphoreGive(_mutex);
        return false;
    }

    for (size_t i = 0; i < samples; i++) {
        _buf[_writeIdx++] = data[i];
        if (_writeIdx >= kCapacity) _writeIdx = 0;
    }
    _count += samples;

    xSemaphoreGive(_mutex);
    return true;
}

bool AudioBuffer::pop(int16_t* data, size_t samples) {
    if (!_mutex || !data || samples == 0) return false;
    xSemaphoreTake(_mutex, portMAX_DELAY);

    if (_count < samples) {
        xSemaphoreGive(_mutex);
        return false;
    }

    for (size_t i = 0; i < samples; i++) {
        data[i] = _buf[_readIdx++];
        if (_readIdx >= kCapacity) _readIdx = 0;
    }
    _count -= samples;

    xSemaphoreGive(_mutex);
    return true;
}
