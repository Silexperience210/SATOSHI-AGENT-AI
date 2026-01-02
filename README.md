# SATOSHI AGENT AI

Intelligent Voice Bitcoin Assistant for ESP32-S3

## Description

SATOSHI AGENT AI is an advanced AI-powered voice assistant for Bitcoin enthusiasts, running on ESP32-S3 microcontroller with touchscreen display. It features voice wake word "SATOSHI", AI conversations via Claude API, real-time Bitcoin price monitoring, mining pool management, LNbits wallet integration, and more.

## Features

- **Voice Activation**: Wake word "SATOSHI" for hands-free operation
- **AI Chat**: Powered by Anthropic Claude API for intelligent conversations
- **Bitcoin Monitoring**: Real-time price updates and market data
- **Mining Management**: Support for Bitaxe and Braiins mining pools
- **Lightning Network**: LNbits wallet integration for payments
- **Text-to-Speech**: Multiple TTS providers (Groq, Google, OpenAI)
- **Speech-to-Text**: Whisper API for voice recognition
- **Touchscreen UI**: 2.8" TFT display with capacitive touch
- **WiFi Setup**: Captive portal for easy configuration

## Hardware Requirements

- **ESP32-S3 Development Board**: Freenove ESP32-S3 with 2.8" touchscreen display
  - Purchase: [Freenove ESP32-S3 2.8" Touchscreen](https://www.freenove.com/product/fnk0085/)
- **Microphone**: Built-in or external I2S microphone
- **Speaker**: 8Ω speaker for audio output
  - Purchase: [8Ω Speaker on Amazon](https://www.amazon.com/s?k=8+ohm+speaker+3w)
- **Power Supply**: 5V USB or battery

## Software Requirements

- **PlatformIO**: For building and flashing
- **Arduino Framework**: ESP32 support
- **Libraries**:
  - GFX Library for Arduino
  - ArduinoJson
  - QRCode

## Installation

1. Clone the repository:
   ```bash
   git clone https://github.com/Silexperience210/SATOSHI-AGENT-AI.git
   cd SATOSHI-AGENT-AI
   ```

2. Install PlatformIO if not already installed.

3. Open the project in VS Code with PlatformIO extension.

4. Configure your API keys in the captive portal setup.

5. Build and upload:
   ```bash
   platformio run --target upload
   ```

## Usage

1. Power on the device.
2. Connect to the WiFi access point "SATOSHI-AGENT-AI".
3. Open browser and configure WiFi, API keys, etc.
4. Say "SATOSHI" to wake the assistant.
5. Ask questions about Bitcoin, manage mining, or make payments.

## API Keys Required

- Anthropic Claude API key
- Groq API key (for TTS)
- Google TTS API key (optional)
- LNbits server details
- Braiins API token (optional)

## Contributing

Contributions welcome! Please fork and submit pull requests.

## License

CeCILL License - see LICENSE file for details.

## Disclaimer

This project is for educational purposes. Use at your own risk. Bitcoin and crypto trading involves risk.
