🔹 config/

👉 KHÔNG để logic ở đây

pinmap.h
→ Tất cả GPIO (motor, audio, I2S, ADC)

secrets.h
→ Wi-Fi, IP PC AI

🔹 core/

👉 “xương sống” robot

system.cpp

init Wi-Fi

init motor

init audio

state.cpp

IDLE

LISTENING

MOVING

SLEEP

🔹 motor/

👉 Tách driver và logic

drv8833.cpp
→ PWM + DIR thấp cấp

motor.cpp
→ moveForward(), stop(), waveTail()

🔹 audio/

👉 I2S là phải tách riêng

mic_i2s.cpp

read audio frame

speaker_i2s.cpp

play buffer

🔹 network/

👉 WebSocket + Wi-Fi

wifi_mgr.cpp

websocket.cpp

🔹 ai/

👉 Giao tiếp với PC AI

ai_bridge.h

sendAudio()

receiveCommand()

command_parser.cpp

“sit”

“stand”

“walk”

🔹 utils/

👉 Debug, log, buffer