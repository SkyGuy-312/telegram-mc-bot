# Minecraft Telegram Bot (C++)

A lightweight, native C++ application to control your Minecraft server remotely via Telegram.

## Features
*   **Remote Control**: Start, Stop, Restart your server from Telegram.
*   **Command Injection**: Send console commands (e.g., `/cmd say Hello`) directly to the server.
*   **Secure**: Whitelist-based access control (User IDs).
*   **No Port Forwarding**: Uses Telegram Long Polling, so it works behind firewalls and NATs without router configuration.
*   **Native Performance**: Written in C++ using Windows API for reliable process management.

## Prerequisites
*   **Windows OS**
*   **Telegram Bot Token**: Create one via [@BotFather](https://t.me/BotFather).
*   **Telegram User ID**: Get yours via [@userinfobot](https://t.me/userinfobot).

## Installation & Build

This project includes a helper script `build_and_run.bat` that constructs the entire environment for you.

1.  **Clone/Download** this repository.
2.  **Run `build_and_run.bat`**.
    *   This script will automatically download **CMake** (if missing).
    *   It will clone **vcpkg** (Microsoft's package manager).
    *   It will compile all C++ dependencies (`tgbot-cpp`, `curl`, `openssl`).
    *   *Note: First run may take 5-10 minutes.*

## Configuration

1.  Rename `config.example.json` to `config.json`.
2.  Open `config.json` and fill in your details:

```json
{
    "telegram_token": "YOUR_BOT_TOKEN_FROM_BOT_FATHER",
    "allowed_ids": [123456789],
    "server_path": "C:\\Path\\To\\Minecraft\\Server",
    "server_jar": "server.jar",
    "java_args": "-Xmx4G"
}
```

*   `allowed_ids`: List of Telegram User IDs allowed to use the bot.
*   `server_path`: **Absolute path** to the folder where your `server.jar` is located.

## Usage

Run `build_and_run.bat` to start the bot.

**Bot Commands:**
*   `/start` - Launches the Minecraft server process.
*   `/stop` - specificies "stop" command to server stdin and waits for process to exit.
*   `/restart` - Stops and restarts the server.
*   `/status` - Checks if server is running.
*   `/cmd <text>` - Sends text to the server console.

## Directory Structure
*   `src/`: Source code (`main.cpp`, `ProcessController`).
*   `build_and_run.bat`: One-click setup script.
*   `CMakeLists.txt`: Build configuration.
*   `vcpkg.json`: Dependency manifest.
