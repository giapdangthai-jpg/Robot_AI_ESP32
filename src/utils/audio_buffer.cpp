#include "audio_buffer.h"
#include <Arduino.h>
#include <esp_heap_caps.h>

// g_micBuf: capture buffer, filled by mic_stream_task, consumed by mic_upload_task
// g_spkBuf: playback buffer, filled by WebSocket RX, consumed by ws_audio_play_task
AudioBuffer g_micBuf;
AudioBuffer g_spkBuf;

// Set true while speaker is playing; mic_upload_task discards frames to prevent echo
volatile bool g_speakerActive = false;

// Allocate ring buffer of `capacitySamples` int16_t samples
// preferPsram=true tries SPIRAM first (for large speaker buffer); falls back to DRAM
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

    // Fallback: any available heap if preferred region is full
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

// Push `samples` int16_t values into the ring buffer (mutex-protected)
// Returns false if not enough free space (caller should log/count drops)
bool AudioBuffer::push(const int16_t* data, size_t samples) {
    if (!_mutex || !_buf || !data || samples == 0) return false;
    xSemaphoreTake(_mutex, portMAX_DELAY);

    if (samples > (_capacity - _count)) {
        xSemaphoreGive(_mutex);
        return false;  // buffer full — frame dropped
    }

    for (size_t i = 0; i < samples; i++) {
        _buf[_writeIdx++] = data[i];
        if (_writeIdx >= _capacity) _writeIdx = 0;
    }
    _count += samples;

    xSemaphoreGive(_mutex);
    return true;
}

// Pop exactly `samples` int16_t values from the ring buffer (mutex-protected)
// Returns false if fewer samples are available — caller should retry next tick
bool AudioBuffer::pop(int16_t* data, size_t samples) {
    if (!_mutex || !_buf || !data || samples == 0) return false;
    xSemaphoreTake(_mutex, portMAX_DELAY);

    if (_count < samples) {
        xSemaphoreGive(_mutex);
        return false;  // not enough data yet
    }

    for (size_t i = 0; i < samples; i++) {
        data[i] = _buf[_readIdx++];
        if (_readIdx >= _capacity) _readIdx = 0;
    }
    _count -= samples;

    xSemaphoreGive(_mutex);
    return true;
}
