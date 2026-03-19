#pragma once

// =====================
// I2S MIC - INMP441, AMP - MAX98357
// =====================
#define I2S_SPK_MIC_BCLK    37
#define I2S_SPK_MIC_WS      38
#define I2S_SPK_DATA        36
#define I2S_SPK_SD          21
#define I2S_MIC_DATA        35

// Audio format
#define AUDIO_SAMPLE_RATE 16000
#define AUDIO_BITS        16
#define AUDIO_BUFFER_SIZE 1024
#define AUDIO_FRAME_SAMPLES 320   // 20ms
#define AUDIO_FRAME_BYTES (AUDIO_FRAME_SAMPLES * 2)

// =====================
// DRV8833 Motor Left
// =====================
#define M1_IN1  11
#define M1_IN2  12

// =====================
// DRV8833 Motor Right
// =====================
#define M2_IN1  13
#define M2_IN2  14
// =====================
// DRV8833 Motor Back
// =====================
#define M3_IN1  9
#define M3_IN2  10
// =====================
// DRV8833 Motor FAULT
// =====================
#define MOTOR_FAULT  18
// Back angle (ADC)
// =====================
#define BACK_ADC   5  // GPIO5 (ADC)

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
