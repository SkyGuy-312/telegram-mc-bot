#include <tgbot/tgbot.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>
#include "ProcessController.hpp"

using json = nlohmann::json;
using namespace std;

// Struct to hold config
struct Config {
    string token;
    vector<int64_t> allowed_ids;
    string server_path;
    string server_jar;
    string java_args;
};

Config loadConfig() {
    std::ifstream f("config.json");
    if (!f.is_open()) {
        throw std::runtime_error("Could not open config.json");
    }
    json data = json::parse(f);
    Config cfg;
    cfg.token = data["telegram_token"];
    cfg.allowed_ids = data["allowed_ids"].get<vector<int64_t>>();
    cfg.server_path = data["server_path"];
    cfg.server_jar = data["server_jar"];
    cfg.java_args = data["java_args"];
    return cfg;
}

bool isAllowed(int64_t id, const vector<int64_t>& allowed) {
    return std::find(allowed.begin(), allowed.end(), id) != allowed.end();
}

int main() {
    try {
        Config cfg = loadConfig();
        TgBot::Bot bot(cfg.token);
        ProcessController server(cfg.server_path, cfg.server_jar, cfg.java_args);

        // Helper to send messages safely
        auto reply = [&](int64_t chatId, const string& text) {
            try {
                bot.getApi().sendMessage(chatId, text);
            } catch (exception& e) {
                cerr << "Error sending message: " << e.what() << endl;
            }
        };

        // Command: /start
        bot.getEvents().onCommand("start", [&](TgBot::Message::Ptr message) {
            if (!isAllowed(message->chat->id, cfg.allowed_ids) && !isAllowed(message->from->id, cfg.allowed_ids)) return;

            if (server.isRunning()) {
                reply(message->chat->id, "Server is already running!");
                return;
            }

            reply(message->chat->id, "Starting server...");
            bool started = server.start([&](const string& output) {
                // Log output to console.
                // NOTE: forwarding EVERYTHING to telegram might be too much spam.
                // We could implement a filter or a 'subscribe' command here.
                cout << "[MC]: " << output;
            });

            if (started) {
                 reply(message->chat->id, "Server process launched.");
            } else {
                 reply(message->chat->id, "Failed to launch server process.");
            }
        });

        // Command: /stop
        bot.getEvents().onCommand("stop", [&](TgBot::Message::Ptr message) {
            if (!isAllowed(message->chat->id, cfg.allowed_ids) && !isAllowed(message->from->id, cfg.allowed_ids)) return;
            
            if (!server.isRunning()) {
                reply(message->chat->id, "Server is not running.");
                return;
            }

            reply(message->chat->id, "Stopping server (sending /stop)...");
            server.stop();
            reply(message->chat->id, "Server stopped.");
        });
        
         // Command: /restart
        bot.getEvents().onCommand("restart", [&](TgBot::Message::Ptr message) {
            if (!isAllowed(message->chat->id, cfg.allowed_ids) && !isAllowed(message->from->id, cfg.allowed_ids)) return;
            
            reply(message->chat->id, "Restarting server...");
            server.restart([&](const string& output) {
                cout << "[MC]: " << output;
            });
            reply(message->chat->id, "Server restarted.");
        });

        // Command: /cmd <command>
        bot.getEvents().onCommand("cmd", [&](TgBot::Message::Ptr message) {
            if (!isAllowed(message->chat->id, cfg.allowed_ids) && !isAllowed(message->from->id, cfg.allowed_ids)) return;

            string cmd = message->text.substr(5); // Remove "/cmd "
            if (cmd.empty()) {
                reply(message->chat->id, "Usage: /cmd <command>");
                return;
            }

            server.writeCommand(cmd);
            reply(message->chat->id, "Command sent: " + cmd);
        });

        // Command: /status
        bot.getEvents().onCommand("status", [&](TgBot::Message::Ptr message) {
            if (!isAllowed(message->chat->id, cfg.allowed_ids) && !isAllowed(message->from->id, cfg.allowed_ids)) return;
             reply(message->chat->id, server.isRunning() ? "Server is ONLINE" : "Server is OFFLINE");
        });

        printf("Bot username: %s\n", bot.getApi().getMe()->username.c_str());
        TgBot::TgLongPoll longPoll(bot);
        while (true) {
            longPoll.start();
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
