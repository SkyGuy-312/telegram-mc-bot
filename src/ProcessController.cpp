#include "ProcessController.hpp"
#include <iostream>
#include <vector>

ProcessController::ProcessController(const std::string& serverPath, const std::string& jarName, const std::string& javaArgs)
    : m_serverPath(serverPath), m_jarName(jarName), m_javaArgs(javaArgs), m_running(false) {
    ZeroMemory(&m_piProcInfo, sizeof(PROCESS_INFORMATION));
}

ProcessController::~ProcessController() {
    stop();
    if (m_logThread.joinable()) {
        m_logThread.join();
    }
}

bool ProcessController::start(OutputCallback callback) {
    if (isRunning()) return false;

    m_onOutput = callback;

    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    // Create a pipe for the child process's STDOUT.
    if (!CreatePipe(&m_hChildStd_OUT_Rd, &m_hChildStd_OUT_Wr, &saAttr, 0)) return false;
    SetHandleInformation(m_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0);

    // Create a pipe for the child process's STDIN.
    if (!CreatePipe(&m_hChildStd_IN_Rd, &m_hChildStd_IN_Wr, &saAttr, 0)) return false;
    SetHandleInformation(m_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA siStartInfo;
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdError = m_hChildStd_OUT_Wr;
    siStartInfo.hStdOutput = m_hChildStd_OUT_Wr;
    siStartInfo.hStdInput = m_hChildStd_IN_Rd;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    std::string cmdLine = "java " + m_javaArgs + " -jar " + m_jarName + " nogui";
    
    // CreateProcess requires a mutable string
    std::vector<char> cmdVec(cmdLine.begin(), cmdLine.end());
    cmdVec.push_back(0);

    BOOL bSuccess = CreateProcessA(
        NULL,
        cmdVec.data(),
        NULL,
        NULL,
        TRUE,
        0,
        NULL,
        m_serverPath.c_str(),
        &siStartInfo,
        &m_piProcInfo
    );

    if (!bSuccess) {
        return false;
    }

    // Close handles to the stdin and stdout pipes no longer needed by the child process.
    // If they are not closed, there is no way for the parent to know that the child has sent all its data.
    CloseHandle(m_hChildStd_OUT_Wr);
    CloseHandle(m_hChildStd_IN_Rd);
    m_hChildStd_OUT_Wr = NULL;
    m_hChildStd_IN_Rd = NULL;

    m_running = true;
    m_logThread = std::thread(&ProcessController::outputReaderThread, this);

    return true;
}

void ProcessController::stop() {
    if (!isRunning()) return;
    writeCommand("stop");
    
    // Wait for process to exit
    WaitForSingleObject(m_piProcInfo.hProcess, 10000); // Wait up to 10s
    
    m_running = false;
    if (m_logThread.joinable()) m_logThread.join();

    CloseHandle(m_piProcInfo.hProcess);
    CloseHandle(m_piProcInfo.hThread);
    CloseHandle(m_hChildStd_OUT_Rd);
    CloseHandle(m_hChildStd_IN_Wr);

    ZeroMemory(&m_piProcInfo, sizeof(PROCESS_INFORMATION));
    m_hChildStd_OUT_Rd = NULL;
    m_hChildStd_IN_Wr = NULL;
}

void ProcessController::kill() {
     if (m_piProcInfo.hProcess) {
        TerminateProcess(m_piProcInfo.hProcess, 1);
        m_running = false;
     }
}

void ProcessController::restart(OutputCallback callback) {
    stop();
    start(callback);
}

void ProcessController::writeCommand(const std::string& command) {
    if (!isRunning() || !m_hChildStd_IN_Wr) return;
    
    std::string fullCmd = command + "\n";
    DWORD dwWritten;
    WriteFile(m_hChildStd_IN_Wr, fullCmd.c_str(), fullCmd.length(), &dwWritten, NULL);
}

bool ProcessController::isRunning() {
    if (!m_piProcInfo.hProcess) return false;
    DWORD exitCode;
    if (GetExitCodeProcess(m_piProcInfo.hProcess, &exitCode)) {
        return exitCode == STILL_ACTIVE;
    }
    return false;
}

void ProcessController::outputReaderThread() {
    const int BUFSIZE = 4096;
    CHAR chBuf[BUFSIZE];
    DWORD dwRead;
    BOOL bSuccess = FALSE;

    while (m_running) {
        bSuccess = ReadFile(m_hChildStd_OUT_Rd, chBuf, BUFSIZE - 1, &dwRead, NULL);
        if (!bSuccess || dwRead == 0) break;

        chBuf[dwRead] = 0; // Null terminate
        if (m_onOutput) {
            m_onOutput(std::string(chBuf));
        }
    }
}
