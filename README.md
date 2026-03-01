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

--------------------------------------------------------------------------
I. KIẾN TRÚC THEO TẦNG (Layered Architecture)
1️⃣ Hardware Layer

I2S mic

I2S DAC

Motor driver

WiFi chip

2️⃣ Driver / Utils Layer
utils/
 ├── ringbuffer
 ├── audio_buffer
 ├── logger


Chức năng:

Không chứa business logic

Chỉ cung cấp công cụ nền

3️⃣ Core Layer (Trái tim hệ thống)
core/
 ├── event_bus
 ├── state
 ├── system

🔥 Vai trò cực kỳ quan trọng
Module	Vai trò
event_bus	Giao tiếp giữa các module
state	Quản lý trạng thái robot
system	Khởi tạo toàn bộ

👉 Đây là trung tâm điều phối.

4️⃣ Network Layer
network/
 ├── wifi_mgr
 ├── websocket
 ├── websocket_task
 ├── ws_stream_task

Phân tầng nội bộ
wifi_mgr        → quản lý WiFi
websocket       → API WebSocket
websocket_task  → FreeRTOS xử lý message
ws_stream_task  → streaming audio

5️⃣ AI Layer
ai/
 ├── ai_bridge
 ├── command_parser

Flow:
WebSocket → ai_bridge → command_parser → event_bus


Ví dụ:

{ "type": "answer", "text": "Turn left" }


↓

command_parser.parse()


↓

event_bus.publish(MOTOR_LEFT)


↓

motor chạy

6️⃣ Application Layer (Main)
main.cpp


Chỉ làm:

system.init();


Không chứa logic.

🔁 III. DATA FLOW TOÀN HỆ THỐNG
🎤 Khi có mic
Mic → audio_buffer → ws_stream_task → PC (Whisper)
PC → LLM → TTS → trả audio
ESP → audio_player → loa

📝 Khi gửi text (hiện tại)
ESP sendAsk()
↓
websocket_task
↓
PC LLM
↓
websocket trả answer
↓
ai_bridge
↓
command_parser
↓
event_bus
↓
motor / speaker

🧠 IV. FreeRTOS Task Architecture

Tôi đề xuất:

TaskWiFi
TaskWebSocket
TaskStream
TaskAI
TaskMotor


Mỗi task giao tiếp bằng:

event_bus
queue
ringbuffer
