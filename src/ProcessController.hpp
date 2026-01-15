#pragma once

#include <string>
#include <windows.h>
#include <functional>
#include <thread>
#include <atomic>

class ProcessController {
public:
    using OutputCallback = std::function<void(const std::string&)>;

    ProcessController(const std::string& serverPath, const std::string& jarName, const std::string& javaArgs);
    ~ProcessController();

    bool start(OutputCallback callback);
    void stop(); // Graceful stop via "stop" command
    void kill(); // Force kill
    void restart(OutputCallback callback);
    void writeCommand(const std::string& command);
    bool isRunning();

private:
    void outputReaderThread();

    std::string m_serverPath;
    std::string m_jarName;
    std::string m_javaArgs;

    HANDLE m_hChildStd_OUT_Rd = NULL;
    HANDLE m_hChildStd_OUT_Wr = NULL;
    HANDLE m_hChildStd_IN_Rd = NULL;
    HANDLE m_hChildStd_IN_Wr = NULL;

    PROCESS_INFORMATION m_piProcInfo;
    std::atomic<bool> m_running;
    std::thread m_logThread;
    OutputCallback m_onOutput;
};
