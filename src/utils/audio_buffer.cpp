#include "audio_buffer.h"
#include <Arduino.h>
#include <esp_heap_caps.h>

AudioBuffer g_micBuf;
AudioBuffer g_spkBuf;

bool AudioBuffer::init(size_t capacitySamples, bool preferPsram) {
    if (!_mutex) {
        _mutex = xSemaphoreCreateMutex();
    }
    if (!_mutex || capacitySamples == 0) {
        return false;
    }

    if (_buf) {
        heap_caps_free(_buf);
        _buf = nullptr;
    }

    const size_t bytes = capacitySamples * sizeof(int16_t);
    _usingPsram = false;

    if (preferPsram) {
        _buf = static_cast<int16_t*>(heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
        _usingPsram = (_buf != nullptr);
    } else {
        _buf = static_cast<int16_t*>(heap_caps_malloc(bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    }

    if (!_buf) {
        _buf = static_cast<int16_t*>(heap_caps_malloc(bytes, MALLOC_CAP_8BIT));
        if (_buf) {
            _usingPsram = false;
        }
    }

    if (!_buf) {
        Serial.printf("[AUDIO_BUF] alloc failed samples=%u bytes=%u\n",
                      (unsigned)capacitySamples, (unsigned)bytes);
        _capacity = 0;
        return false;
    }

    _capacity = capacitySamples;
    _writeIdx = 0;
    _readIdx  = 0;
    _count    = 0;
    Serial.printf("[AUDIO_BUF] init samples=%u bytes=%u mem=%s\n",
                  (unsigned)_capacity,
                  (unsigned)bytes,
                  _usingPsram ? "PSRAM" : "DRAM");
    return true;
}

void AudioBuffer::clear() {
    if (!_mutex || !_buf) return;
    xSemaphoreTake(_mutex, portMAX_DELAY);
    _writeIdx = 0;
    _readIdx  = 0;
    _count    = 0;
    xSemaphoreGive(_mutex);
}

bool AudioBuffer::push(const int16_t* data, size_t samples) {
    if (!_mutex || !_buf || !data || samples == 0) return false;
    xSemaphoreTake(_mutex, portMAX_DELAY);

    if (samples > (_capacity - _count)) {
        xSemaphoreGive(_mutex);
        return false;
    }

    for (size_t i = 0; i < samples; i++) {
        _buf[_writeIdx++] = data[i];
        if (_writeIdx >= _capacity) _writeIdx = 0;
    }
    _count += samples;

    xSemaphoreGive(_mutex);
    return true;
}

bool AudioBuffer::pop(int16_t* data, size_t samples) {
    if (!_mutex || !_buf || !data || samples == 0) return false;
    xSemaphoreTake(_mutex, portMAX_DELAY);

    if (_count < samples) {
        xSemaphoreGive(_mutex);
        return false;
    }

    for (size_t i = 0; i < samples; i++) {
        data[i] = _buf[_readIdx++];
        if (_readIdx >= _capacity) _readIdx = 0;
    }
    _count -= samples;

    xSemaphoreGive(_mutex);
    return true;
}
