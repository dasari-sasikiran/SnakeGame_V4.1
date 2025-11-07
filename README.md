# 🐍 Snake Game v4.1 (ESP32 TFT + IR + Voice + Web Control)

A feature-rich Snake Game on ESP32 combining IR, Web, and Voice inputs — powered by Cloudflare-secured web console and real-time TFT graphics.

### Version
**v4.1 – Final Stable Build**

Developed by **DASARI SASIKIRAN**  with iterative guidance from ChatGPT (R&D → Development → Testing → Execution).

---

## 📘 Overview

**Snake Game v4.1** is an advanced ESP32-based version of the classic Snake Game.  
It combines **real-time graphics**, **multiple input modes**, and a **web-based voice console** secured using **Cloudflare Tunnelling**.

### 🔹 Key Features
- **TFT Display (ILI9341)** with full-color graphics  
- **IR Remote**, **Web Console**, and **Voice Commands** for control  
- **Buzzer Sound Effects** for gameplay feedback  
- **Cloudflare Tunnel** for secure HTTPS remote access  
- **Wi-Fi Web Dashboard** (SPIFFS-hosted HTML/JS/CSS)  
- **Edge Impulse Voice Model Integration**  
- **Modular architecture**: display, game, web, IR, buzzer, and voice systems  

This version transitions from the earlier SH1106 OLED builds to a **240×320 ILI9341 TFT**, providing a richer, smoother visual experience.

---

## ⚙️ Hardware Specifications

| Component | Description |
|------------|-------------|
| **MCU** | ESP32 Dev Board (38-pin, dual-core, 240 MHz) |
| **Display** | 2.4”/2.8” SPI TFT (Driver: ILI9341) |
| **IR Receiver** | VS1838B / HX1838 / TSOP38238 |
| **Buzzer** | Active buzzer (HYDZ or equivalent) |
| **Optional Mic** | Electret or MEMS (used by browser voice input) |
| **Storage** | SPIFFS filesystem |
| **Power** | 5 V (USB or regulated source) |

---

## 🧩 Wiring Diagram

| Function | GPIO | Notes |
|-----------|------|-------|
| TFT SCK | 18 | SPI Clock |
| TFT MOSI | 23 | SPI Data (MOSI) |
| TFT MISO | 19 | (optional) |
| TFT CS | 5 | Chip Select |
| TFT DC | 2 | Data/Command |
| TFT RST | 4 | Hardware Reset |
| IR Receiver | 15 | Data pin |
| Buzzer | 27 | Active buzzer signal |
| VCC / GND | 3.3 V or 5 V / GND | Shared by all peripherals |

---

## 📦 Software Dependencies

**PlatformIO Configuration**

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
board_build.filesystem = spiffs
lib_deps = 
    adafruit/Adafruit ILI9341
    adafruit/Adafruit GFX Library
    madhephaestus/ESP32 IRremote
    me-no-dev/AsyncTCP
    me-no-dev/ESP Async WebServer
    bodmer/TJpg_Decoder


🧠 Source Structure

src/
 ├── main.cpp              → Setup, loop, and initialization
 ├── game.cpp/.h           → Core snake logic and scoring
 ├── display.cpp/.h        → Rendering and HUD drawing
 ├── buzzer.cpp/.h         → Sound effect patterns
 ├── ir_control.cpp/.h     → IR remote decoding
 ├── web_control.cpp/.h    → WebSocket & HTTP server
 ├── voice.cpp/.h          → Voice inference interface
 ├── voice_actions.cpp     → Voice-to-action mapping
 ├── config.h              → GPIO, display, and constants
data/
 ├── index.html, script.js, style.css → Web dashboard assets
platformio.ini             → Build environment


🌐 Web Control & Voice Console

1. Connect ESP32 to Wi-Fi (SSID/PASS configured in `web_control.cpp`).
2. After boot, open the **Serial Monitor (115200 baud)** to see the local IP.
3. Open `http://<your-esp32-ip>/` to access the dashboard.
4. Use on-screen buttons, IR remote, or microphone input to control the snake.
5. Voice input is processed via a browser and sent to ESP32 through WebSocket.

## 🛡️ Cloudflare Tunnel (Secure HTTPS Access)

To make the local ESP32 web console securely available over HTTPS using **Cloudflare Tunnel**, follow these steps:

### 1️⃣ Install Cloudflare CLI (Cloudflared)

Download and install `cloudflared` from:
👉 [https://developers.cloudflare.com/cloudflare-one/connections/connect-apps/install-and-setup/installation/](https://developers.cloudflare.com/cloudflare-one/connections/connect-apps/install-and-setup/installation/)

### 2️⃣ Authenticate with your Cloudflare account

```bash
cloudflared login
```

Select your domain from the Cloudflare dashboard.

### 3️⃣ Start a secure tunnel to your ESP32

Assuming your ESP32 web server is on `192.168.1.120:80`:

```bash
cloudflared tunnel --url http://192.168.1.120:80
```

### 4️⃣ Copy the temporary HTTPS URL

After execution, Cloudflare will provide a public link, such as:

```
https://snakegame-kiran.cloudflareTunnel.com
```

Use this link to access your **voice-powered web console** securely from any device.

### 5️⃣ (Optional) Create a permanent tunnel

```bash
cloudflared tunnel create snakegame
cloudflared tunnel route dns snakegame snakegame.yourdomain.com
cloudflared tunnel run snakegame
```

Now your ESP32 web interface is reachable at:

```
https://snakegame.yourdomain.com
```

---

## 🔊 Buzzer Feedback

| Event          | Pattern                                 |
| -------------- | --------------------------------------- |
| Startup        | 2 short beeps                           |
| Food Eaten     | 1 short beep (100 ms)                   |
| Game Over      | Rapid short beeps with increasing delay |
| Pause / Resume | Quick click                             |
| Voice Command  | Optional click                          |

---

## 🕹️ IR Remote Mapping

| Action       | Example IR Code (HEX) |
| ------------ | --------------------- |
| Up           | `0xC00058`            |
| Down         | `0xC00059`            |
| Left         | `0xC0005A`            |
| Right        | `0xC0005B`            |
| Pause        | `0xC0005C`            |
| Reset        | `0xC0000C`            |
| Sound Toggle | `0xC0000D`            |

*(Update codes in `config.h` as needed for your remote.)*

---

## ⚙️ Build & Flash Instructions

1. Install **PlatformIO** (VS Code extension or CLI).
2. Open `SnakeGame_V4.1` folder.
3. Connect ESP32 board via USB.
4. Build and upload firmware:

   ```bash
   pio run -t upload
   ```
5. Upload web assets to SPIFFS:

   bash
   pio run -t uploadfs

6. Open Serial Monitor (`115200 baud`) to view IP and logs.
7. Open browser at displayed IP or via your Cloudflare HTTPS URL.


## 🧾 Notes

* Voice inference model: `snake-voice-console_inferencing.h`
* Supports **IR**, **Web**, and **Voice** control simultaneously.
* Designed for **ESP32-D0WDQ6-V3** or equivalent.
* Mask Wi-Fi credentials before public release.
* Recommended to remove `.pio/` build directory before sharing.


## 🏁 Authors

**Developed by:**

* DASARI SASIKIRAN
* R&D and System Guidance by ChatGPT
