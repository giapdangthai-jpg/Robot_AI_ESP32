#pragma once

// =====================
// I2S MIC - INMP441
// =====================
#define I2S_MIC_BCLK   4
#define I2S_MIC_WS     5
#define I2S_MIC_DATA   6

// Audio format
#define AUDIO_SAMPLE_RATE 16000
#define AUDIO_BITS        16
#define AUDIO_BUFFER_SIZE 1024
#define AUDIO_FRAME_SAMPLES 320   // 20ms
#define AUDIO_FRAME_BYTES (AUDIO_FRAME_SAMPLES * 2)


// =====================
// I2S AMP - MAX98357
// =====================
#define I2S_SPK_BCLK   7
#define I2S_SPK_WS     8
#define I2S_SPK_DATA   9

// =====================
// DRV8833 Motor Left
// =====================
#define M1_IN1  10
#define M1_IN2  11

// =====================
// DRV8833 Motor Right
// =====================
#define M2_IN1  12
#define M2_IN2  13
// =====================
// DRV8833 Motor Back
// =====================
#define M3_IN1  14
#define M3_IN2  15

// Back angle (ADC)
// =====================
#define BACK_ADC   1  // GPIO1 (ADC)

// =====================
// UART Debug
// =====================
#define DEBUG_BAUD 115200

// =====================
// RGB LED - WS2812
// =====================
#define RGB_PIN 48

// =====================
// BOOT button (built-in on ESP32-S3 DevKit)
// Hold 3s at boot to reset WiFi credentials
// =====================
#define BOOT_BTN_PIN  0
#define WIFI_RESET_HOLD_MS 3000
