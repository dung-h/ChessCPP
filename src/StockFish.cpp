#include "StockFish.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>
#include <cstdio>
#include <string>
#include <unistd.h>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#else
#include <sys/wait.h>
#include <fcntl.h>
#endif

StockfishConnector::StockfishConnector() : stockfishProcess(nullptr), initialized(false)
#ifdef _WIN32
                                           ,
                                           childProcess(NULL), childStdin(NULL), childStdout(NULL)
#endif
{
}

StockfishConnector::~StockfishConnector()
{
    close();
}

#ifdef _WIN32

void StockfishConnector::close()
{
    if (initialized)
    {
        try
        {
            // Tell engine to quit
            sendCommand("quit");

            // Wait for process to terminate
            WaitForSingleObject(childProcess, 1000);

            // Close handles
            CloseHandle(childStdin);
            CloseHandle(childStdout);
            CloseHandle(childProcess);

            childProcess = NULL;
            childStdin = NULL;
            childStdout = NULL;
            initialized = false;
        }
        catch (...)
        {
            std::cerr << "Error closing Stockfish process" << std::endl;
        }
    }
}

std::string StockfishConnector::sendCommand(const std::string &cmd)
{
    if (!initialized || !childProcess || !childStdin)
    {
        return "";
    }

    // Add newline to command if not present
    std::string fullCmd = cmd;
    if (fullCmd.empty() || fullCmd.back() != '\n')
    {
        fullCmd += "\n";
    }

    // Write command to pipe
    DWORD bytesWritten;
    if (!WriteFile(childStdin, fullCmd.c_str(), fullCmd.length(), &bytesWritten, NULL))
    {
        DWORD error = GetLastError();
        std::cerr << "DEBUG: Failed to write to pipe. Error code: " << error << std::endl;
        return "";
    }

    // Force flush the pipe to ensure the command is sent immediately
    if (!FlushFileBuffers(childStdin))
    {
        DWORD error = GetLastError();
        std::cerr << "DEBUG: Failed to flush pipe. Error code: " << error << std::endl;
    }

    // Add a small delay to ensure Stockfish has time to process the command
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // For isready command, wait for readyok
    if (cmd == "isready")
    {
        return getEngineOutput();
    }

    // For all other commands, return empty string
    return "";
}

bool StockfishConnector::initialize(const std::string &pathToStockfish)
{
    if (initialized)
    {
        close();
    }

    // Security attributes for pipe inheritance
    SECURITY_ATTRIBUTES saAttr;
    ZeroMemory(&saAttr, sizeof(saAttr));
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    // Create pipes for stdin/stdout
    HANDLE hChildStdin_Read = NULL, hChildStdin_Write = NULL;
    HANDLE hChildStdout_Read = NULL, hChildStdout_Write = NULL;

    // Create a pipe for the child process's STDOUT
    if (!CreatePipe(&hChildStdout_Read, &hChildStdout_Write, &saAttr, 0))
    {
        std::cerr << "Failed to create stdout pipe, error: " << GetLastError() << std::endl;
        return false;
    }

    // Create a pipe for the child process's STDIN
    if (!CreatePipe(&hChildStdin_Read, &hChildStdin_Write, &saAttr, 0))
    {
        std::cerr << "Failed to create stdin pipe, error: " << GetLastError() << std::endl;
        CloseHandle(hChildStdout_Read);
        CloseHandle(hChildStdout_Write);
        return false;
    }

    // Ensure the read/write handles to the pipes aren't inherited
    if (!SetHandleInformation(hChildStdout_Read, HANDLE_FLAG_INHERIT, 0) ||
        !SetHandleInformation(hChildStdin_Write, HANDLE_FLAG_INHERIT, 0))
    {
        std::cerr << "Failed to set handle information, error: " << GetLastError() << std::endl;
        CloseHandle(hChildStdin_Read);
        CloseHandle(hChildStdin_Write);
        CloseHandle(hChildStdout_Read);
        CloseHandle(hChildStdout_Write);
        return false;
    }

    // Create the child process
    PROCESS_INFORMATION piProcInfo;
    STARTUPINFO siStartInfo;

    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));

    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdError = hChildStdout_Write;
    siStartInfo.hStdOutput = hChildStdout_Write;
    siStartInfo.hStdInput = hChildStdin_Read;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    // Create the child process - use the exact filename without quotes first
    BOOL success = CreateProcess(
        NULL,                                       // No module name (use command line)
        const_cast<LPSTR>(pathToStockfish.c_str()), // Command line
        NULL,                                       // Process security attributes
        NULL,                                       // Primary thread security attributes
        TRUE,                                       // Handles are inherited
        CREATE_NO_WINDOW,                           // Creation flags - don't create a window
        NULL,                                       // Use parent's environment
        NULL,                                       // Use parent's current directory
        &siStartInfo,                               // STARTUPINFO pointer
        &piProcInfo                                 // PROCESS_INFORMATION pointer
    );

    if (!success)
    {
        DWORD error = GetLastError();
        std::cerr << "Failed to create Stockfish process, error code: " << error << std::endl;

        // Try again with quotes if there are spaces in the path
        if (pathToStockfish.find(' ') != std::string::npos)
        {
            std::string quotedPath = "\"" + pathToStockfish + "\"";
            success = CreateProcess(
                NULL, const_cast<LPSTR>(quotedPath.c_str()),
                NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL,
                &siStartInfo, &piProcInfo);

            if (!success)
            {
                std::cerr << "Failed with quoted path too, error: " << GetLastError() << std::endl;
                CloseHandle(hChildStdin_Read);
                CloseHandle(hChildStdin_Write);
                CloseHandle(hChildStdout_Read);
                CloseHandle(hChildStdout_Write);
                return false;
            }
        }
        else
        {
            CloseHandle(hChildStdin_Read);
            CloseHandle(hChildStdin_Write);
            CloseHandle(hChildStdout_Read);
            CloseHandle(hChildStdout_Write);
            return false;
        }
    }

    // Store handles for later use
    childProcess = piProcInfo.hProcess;
    childStdin = hChildStdin_Write;
    childStdout = hChildStdout_Read;

    // Close unused handles
    CloseHandle(piProcInfo.hThread);
    CloseHandle(hChildStdin_Read);
    CloseHandle(hChildStdout_Write);

    // Set initialized to true here so getEngineOutput will work
    // We're not fully initialized, but the pipes are ready
    initialized = true;

    // Wait a moment for the process to start
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Initialize the UCI protocol
    // Write directly to the pipe
    std::string uciCommand = "uci\r\n";
    DWORD bytesWritten = 0;
    if (!WriteFile(childStdin, uciCommand.c_str(), uciCommand.size(), &bytesWritten, NULL))
    {
        std::cerr << "Failed to write UCI command to Stockfish, error: " << GetLastError() << std::endl;
        close();
        return false;
    }
    FlushFileBuffers(childStdin);

    // Read the output from the pipe
    std::string output = getEngineOutput();

    if (output.find("uciok") == std::string::npos)
    {
        // Try sending the command again
        DWORD bytesWritten2 = 0;
        WriteFile(childStdin, uciCommand.c_str(), uciCommand.size(), &bytesWritten2, NULL);
        FlushFileBuffers(childStdin);

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        std::string additionalOutput = getEngineOutput();

        output += additionalOutput;
        if (output.find("uciok") == std::string::npos)
        {
            std::cerr << "Failed to initialize UCI protocol" << std::endl;
            close();
            return false;
        }
    }

    return true;
}

std::string StockfishConnector::getEngineOutput()
{
    // Check for valid handles instead of just initialized flag
    if (!childProcess || !childStdout)
    {
        std::cerr << "DEBUG: getEngineOutput called with invalid handles" << std::endl;
        return "";
    }

    std::string output;
    char buffer[4096];
    DWORD bytesRead;
    DWORD totalBytesAvail;
    DWORD bytesLeft;
    bool foundTerminator = false;
    auto startTime = std::chrono::steady_clock::now();
    const auto timeout = std::chrono::seconds(30); // Increased from 5 to 30 seconds

    int loopCount = 0;
    bool anyBytesReceived = false;

    // Read with timeout
    while (!foundTerminator &&
           (std::chrono::steady_clock::now() - startTime) < timeout)
    {
        loopCount++;

        // Check if data is available
        BOOL pipeResult = PeekNamedPipe(childStdout, NULL, 0, NULL, &totalBytesAvail, &bytesLeft);
        if (!pipeResult)
        {
            DWORD error = GetLastError();
            std::cerr << "DEBUG: Failed to peek pipe. Error code: " << error << std::endl;
            break;
        }

        if (loopCount % 50 == 0)
        { // Report status every 500ms
            // Status logging can be re-enabled if needed
        }

        if (totalBytesAvail > 0)
        {
            anyBytesReceived = true;

            // Data available, read it
            ZeroMemory(buffer, sizeof(buffer));
            BOOL readResult = ReadFile(childStdout, buffer, std::min<DWORD>(sizeof(buffer) - 1, totalBytesAvail), &bytesRead, NULL);
            if (readResult && bytesRead > 0)
            {
                buffer[bytesRead] = '\0';
                output += buffer;

                // Check for terminating conditions
                if (output.find("bestmove") != std::string::npos ||
                    output.find("readyok") != std::string::npos ||
                    output.find("uciok") != std::string::npos)
                {
                    foundTerminator = true;
                }
            }
            else
            {
                DWORD error = GetLastError();
                std::cerr << "DEBUG: Failed to read from pipe. Error code: " << error << std::endl;
                break;
            }
        }
        else
        {
            // No data yet, sleep briefly
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    // If we timed out with no response, send a "stop" command to force Stockfish to respond
    if (!foundTerminator && !anyBytesReceived)
    {
        std::cerr << "DEBUG: No response from Stockfish after " << std::chrono::duration_cast<std::chrono::seconds>(timeout).count() << " seconds. Sending 'stop' command." << std::endl;

        // Send stop command to force Stockfish to respond
        std::string stopCmd = "stop\r\n";
        DWORD bytesWritten;
        WriteFile(childStdin, stopCmd.c_str(), stopCmd.size(), &bytesWritten, NULL);
        FlushFileBuffers(childStdin);

        // Wait up to 2 additional seconds for a response
        auto stopStartTime = std::chrono::steady_clock::now();
        const auto stopTimeout = std::chrono::seconds(2);

        while ((std::chrono::steady_clock::now() - stopStartTime) < stopTimeout)
        {
            BOOL pipeResult = PeekNamedPipe(childStdout, NULL, 0, NULL, &totalBytesAvail, &bytesLeft);
            if (pipeResult && totalBytesAvail > 0)
            {
                ZeroMemory(buffer, sizeof(buffer));
                ReadFile(childStdout, buffer, std::min<DWORD>(sizeof(buffer) - 1, totalBytesAvail), &bytesRead, NULL);
                buffer[bytesRead] = '\0';
                output += buffer;

                // Look for bestmove in response
                if (output.find("bestmove") != std::string::npos)
                {
                    foundTerminator = true;
                    break;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    // Handle partial responses where we might have data but no terminator
    else if (!foundTerminator && anyBytesReceived)
    {
        std::cerr << "DEBUG: Got incomplete response from Stockfish. Sending 'stop' command." << std::endl;

        std::string stopCmd = "stop\r\n";
        DWORD bytesWritten;
        WriteFile(childStdin, stopCmd.c_str(), stopCmd.size(), &bytesWritten, NULL);
        FlushFileBuffers(childStdin);

        // Wait briefly for final response
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Read any remaining data
        BOOL pipeResult = PeekNamedPipe(childStdout, NULL, 0, NULL, &totalBytesAvail, &bytesLeft);
        if (pipeResult && totalBytesAvail > 0)
        {
            ZeroMemory(buffer, sizeof(buffer));
            ReadFile(childStdout, buffer, std::min<DWORD>(sizeof(buffer) - 1, totalBytesAvail), &bytesRead, NULL);
            buffer[bytesRead] = '\0';
            output += buffer;
        }
    }

    return output;
}

void StockfishConnector::writeCommand(const std::string &cmd)
{
    if (!initialized || !childProcess || !childStdin || childStdin == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Cannot write command - pipe or process invalid" << std::endl;
        return;
    }

    // Add newline to command if not present
    std::string fullCmd = cmd;
    if (fullCmd.empty() || fullCmd.back() != '\n')
    {
        fullCmd += "\n";
    }

    // Write command to pipe
    DWORD bytesWritten;
    if (!WriteFile(childStdin, fullCmd.c_str(), fullCmd.length(), &bytesWritten, NULL))
    {
        DWORD error = GetLastError();
        std::cerr << "DEBUG: Failed to write command: " << cmd << ". Error code: " << error << std::endl;
        return;
    }

    // Force flush the pipe to ensure the command is actually sent
    if (!FlushFileBuffers(childStdin))
    {
        DWORD error = GetLastError();
        std::cerr << "DEBUG: Failed to flush pipe. Error code: " << error << std::endl;
    }

    // Give Stockfish a moment to process the command
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
}

#else
// LINUX IMPLEMENTATION

bool StockfishConnector::initialize(const std::string &pathToStockfish)
{
    if (initialized)
    {
        close();
    }
    
    std::cout << "Attempting to initialize Stockfish at: " << pathToStockfish << std::endl;
    
    // Find executable path
    std::string execPath = pathToStockfish;
    if (access(execPath.c_str(), X_OK) != 0) {
        // Try adding ./ if not already present
        if (pathToStockfish.substr(0,2) != "./") {
            std::string altPath = "./" + pathToStockfish;
            std::cout << "Trying alternate path with ./ prefix: " << altPath << std::endl;
            
            if (access(altPath.c_str(), X_OK) == 0) {
                execPath = altPath;
            } else if (access("/usr/bin/stockfish", X_OK) == 0) {
                execPath = "/usr/bin/stockfish";
            } else if (access("/usr/games/stockfish", X_OK) == 0) {
                execPath = "/usr/games/stockfish";
            } else {
                std::cerr << "Stockfish not found in any location" << std::endl;
                return false;
            }
        } else {
            std::cerr << "Stockfish not executable at: " << execPath << std::endl;
            return false;
        }
    }
    
    // Create two pairs of pipes
    int stdin_pipe[2];  // Parent writes to [1], child reads from [0]
    int stdout_pipe[2]; // Child writes to [1], parent reads from [0]
    
    if (pipe(stdin_pipe) < 0 || pipe(stdout_pipe) < 0) {
        std::perror("Failed to create pipes");
        return false;
    }
    
    // Fork a child process
    pid_t child_pid = fork();
    
    if (child_pid < 0) {
        std::perror("Failed to fork process");
        ::close(stdin_pipe[0]);
        ::close(stdin_pipe[1]);
        ::close(stdout_pipe[0]);
        ::close(stdout_pipe[1]);
        return false;
    }
    
    if (child_pid == 0) {
        // Child process
        
        // Close unused pipe ends
        ::close(stdin_pipe[1]);
        ::close(stdout_pipe[0]);
        
        // Redirect stdin to read from parent
        dup2(stdin_pipe[0], STDIN_FILENO);
        ::close(stdin_pipe[0]);
        
        // Redirect stdout to write to parent
        dup2(stdout_pipe[1], STDOUT_FILENO);
        ::close(stdout_pipe[1]);
        
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull >= 0) {
            dup2(devnull, STDERR_FILENO);
            ::close(devnull);
        }
        

        // Execute stockfish
        execlp(execPath.c_str(), execPath.c_str(), NULL);
        
        // If we get here, execlp failed
        std::perror("Failed to execute Stockfish");
        exit(1);
    }
    
    // Parent process
    
    // Close unused pipe ends
    ::close(stdin_pipe[0]);
    ::close(stdout_pipe[1]);
    
    // Store file descriptors for communication
    stockfish_in_fd = stdin_pipe[1];
    stockfish_out_fd = stdout_pipe[0];
    stockfish_pid = child_pid;
    
    // Convert file descriptors to FILE* for easier I/O
    stockfishIn = fdopen(stockfish_in_fd, "w");
    stockfishOut = fdopen(stockfish_out_fd, "r");
    
    if (!stockfishIn || !stockfishOut) {
        std::perror("Failed to create FILE streams");
        close();
        return false;
    }
    
    std::cout << "Stockfish process started successfully" << std::endl;
    
    // Initialize UCI protocol
    fprintf(stockfishIn, "uci\n");
    fflush(stockfishIn);
    
    std::string output;
    char buffer[4096];
    int timeout = 0;
    while (fgets(buffer, sizeof(buffer), stockfishOut) != nullptr) {
        output += buffer;
        std::cout << "UCI response: " << buffer;
        if (output.find("uciok") != std::string::npos) {
            break;
        }
        
        // Add timeout check
        timeout++;
        if (timeout > 100) {
            std::cerr << "Timeout waiting for UCI acknowledgement" << std::endl;
            close();
            return false;
        }
    }
    
    if (output.find("uciok") == std::string::npos) {
        std::cerr << "Failed to initialize UCI protocol" << std::endl;
        close();
        return false;
    }
    
    std::cout << "UCI protocol initialized" << std::endl;
    
    // Ensure engine is ready
    fprintf(stockfishIn, "isready\n");
    fflush(stockfishIn);
    
    output.clear();
    timeout = 0;
    while (fgets(buffer, sizeof(buffer), stockfishOut) != nullptr) {
        output += buffer;
        std::cout << "isready response: " << buffer;
        if (output.find("readyok") != std::string::npos) {
            break;
        }
        
        // Add timeout check
        timeout++;
        if (timeout > 100) {
            std::cerr << "Timeout waiting for ready acknowledgement" << std::endl;
            close();
            return false;
        }
    }
    
    if (output.find("readyok") == std::string::npos) {
        std::cerr << "Stockfish engine not ready" << std::endl;
        close();
        return false;
    }
    
    std::cout << "Stockfish initialization successful!" << std::endl;
    initialized = true;
    return true;
}

void StockfishConnector::close()
{
    if (initialized) {
        // Send quit command
        if (stockfishIn) {
            fprintf(stockfishIn, "quit\n");
            fflush(stockfishIn);
            fclose(stockfishIn);
            stockfishIn = nullptr;
        }
        
        if (stockfishOut) {
            fclose(stockfishOut);
            stockfishOut = nullptr;
        }
        
        // Wait for process to terminate
        if (stockfish_pid > 0) {
            int status;
            waitpid(stockfish_pid, &status, 0);
            stockfish_pid = 0;
        }
        
        initialized = false;
    }
}

std::string StockfishConnector::sendCommand(const std::string &cmd)
{
    if (!initialized || !stockfishIn || !stockfishOut)
    {
        return "";
    }

    // Send command to Stockfish
    fprintf(stockfishIn, "%s\n", cmd.c_str());
    fflush(stockfishIn);

    // Get output
    return getEngineOutput();
}

std::string StockfishConnector::getEngineOutput()
{
    if (!initialized || !stockfishIn || !stockfishOut)
    {
        return "";
    }

    std::string output;
    char buffer[4096];

    auto startTime = std::chrono::steady_clock::now();
    const auto timeout = std::chrono::seconds(30);

    while ((std::chrono::steady_clock::now() - startTime) < timeout)
    {
        if (fgets(buffer, sizeof(buffer), stockfishOut) != nullptr)
        {
            output += buffer;

            // Check if we've reached the end of a response
            if (output.find("bestmove") != std::string::npos ||
                output.find("readyok") != std::string::npos ||
                output.find("uciok") != std::string::npos)
            {
                break;
            }
        }
        else if (feof(stockfishOut) || ferror(stockfishOut))
        {
            break;
        }
        else
        {
            // No data yet, sleep briefly
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    return output;
}

void StockfishConnector::writeCommand(const std::string &cmd)
{
    if (!initialized || !stockfishIn)
    {
        std::cerr << "Cannot write command - pipe or process invalid" << std::endl;
        return;
    }
    
    // Add newline to command if not present
    std::string fullCmd = cmd;
    if (fullCmd.empty() || fullCmd.back() != '\n')
    {
        fullCmd += "\n";
    }
    
    // Write command to pipe
    fprintf(stockfishIn, "%s", fullCmd.c_str());
    fflush(stockfishIn);

    // Give Stockfish a moment to process the command
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
}

#endif

// Common functions for both platforms

std::string boardToFEN(const Board &board)
{
    std::stringstream fen;

    // In standard FEN notation, the 8th rank (top) is first, 1st rank (bottom) is last
    // Since your board is inverted, we need to invert it back
    for (int row = 7; row >= 0; --row)
    { // Iterate from 7 to 0 (top to bottom in standard notation)
        int emptyCount = 0;
        for (int col = 0; col < 8; ++col)
        {
            auto piece = board.getPiece(7 - row, col); // Flip row index to match your board
            if (piece)
            {
                if (emptyCount > 0)
                {
                    fen << emptyCount;
                    emptyCount = 0;
                }

                char pieceChar;
                switch (piece->getType())
                {
                case PieceType::Pawn:
                    pieceChar = 'p';
                    break;
                case PieceType::Knight:
                    pieceChar = 'n';
                    break;
                case PieceType::Bishop:
                    pieceChar = 'b';
                    break;
                case PieceType::Rook:
                    pieceChar = 'r';
                    break;
                case PieceType::Queen:
                    pieceChar = 'q';
                    break;
                case PieceType::King:
                    pieceChar = 'k';
                    break;
                default:
                    pieceChar = '?';
                    break;
                }

                // Swap colors too - your white is standard black and vice versa
                if (piece->getColor() == Color::Black)
                {
                    pieceChar = std::toupper(pieceChar);
                }

                fen << pieceChar;
            }
            else
            {
                emptyCount++;
            }
        }

        if (emptyCount > 0)
        {
            fen << emptyCount;
        }

        if (row > 0)
        {
            fen << '/';
        }
    }

    // Active color - invert this too
    fen << ' ' << (board.getCurrentTurn() == Color::Black ? 'w' : 'b') << ' ';

    // Castling rights need to be inverted too
    bool hasCastling = false;
    if (board.canCastleKingside(Color::Black))
    {
        fen << 'K';
        hasCastling = true;
    }
    if (board.canCastleQueenside(Color::Black))
    {
        fen << 'Q';
        hasCastling = true;
    }
    if (board.canCastleKingside(Color::White))
    {
        fen << 'k';
        hasCastling = true;
    }
    if (board.canCastleQueenside(Color::White))
    {
        fen << 'q';
        hasCastling = true;
    }

    if (!hasCastling)
    {
        fen << '-';
    }

    // En passant target square needs to be flipped too
    fen << ' ';
    auto epTarget = board.getEnPassantTarget();
    if (epTarget.first != -1 && epTarget.second != -1)
    {
        char file = 'a' + epTarget.second;
        char rank = '1' + (7 - epTarget.first); // Flip the rank
        fen << file << rank;
    }
    else
    {
        fen << '-';
    }

    // Halfmove clock and fullmove number (simplified)
    fen << " 0 1";

    return fen.str();
}

std::tuple<int, int, int, int> algebraicToCoordinates(const std::string &move)
{
    if (move.length() < 4)
    {
        return std::make_tuple(-1, -1, -1, -1);
    }

    int fromCol = move[0] - 'a';
    int fromRow = 7 - (move[1] - '1'); // Convert from standard to your inverted board
    int toCol = move[2] - 'a';
    int toRow = 7 - (move[3] - '1'); // Convert from standard to your inverted board

    // Parse promotion if present
    if (move.length() > 4 && move[4] == 'q')
    {
        // Handle promotion specifically if needed
    }

    // Ensure coordinates are valid
    if (fromCol < 0 || fromCol > 7 || fromRow < 0 || fromRow > 7 ||
        toCol < 0 || toCol > 7 || toRow < 0 || toRow > 7)
    {
        return std::make_tuple(-1, -1, -1, -1);
    }

    return std::make_tuple(fromRow, fromCol, toRow, toCol);
}

std::tuple<int, int, int, int> StockfishConnector::getBestMove(const Board &board, int thinkingTimeMs)
{
#ifdef _WIN32
    if (!initialized || !childProcess || childProcess == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Stockfish not initialized or process invalid" << std::endl;
        return std::make_tuple(-1, -1, -1, -1);
    }
    
    DWORD exitCode;
    if (GetExitCodeProcess(childProcess, &exitCode) && exitCode != STILL_ACTIVE)
    {
        std::cerr << "Stockfish process has terminated with code: " << exitCode << std::endl;
        initialized = false;
        childProcess = NULL;
        childStdin = NULL;
        childStdout = NULL;
        return std::make_tuple(-1, -1, -1, -1);
    }
#else
    if (!initialized || !stockfishIn || !stockfishOut)
    {
        std::cerr << "Stockfish not initialized or process invalid" << std::endl;
        return std::make_tuple(-1, -1, -1, -1);
    }
#endif

    // Convert board to FEN notation
    std::string fen = boardToFEN(board);
    
    // First make sure engine is ready - this is important!
    writeCommand("isready");
    std::string readyOutput = getEngineOutput();
    if (readyOutput.find("readyok") == std::string::npos)
    {
        std::cerr << "Engine not responding to isready" << std::endl;
        return std::make_tuple(-1, -1, -1, -1);
    }

    // Set position - no response expected for this command
    writeCommand("position fen " + fen);

    // Send the go command - THIS is where we wait for the actual move
    writeCommand("go depth 5");

    // Get the analysis results
    std::string output = getEngineOutput();
    
    // If no bestmove found, try sending a stop command
    if (output.find("bestmove") == std::string::npos)
    {
        writeCommand("stop");
        std::string additionalOutput = getEngineOutput();
        output += additionalOutput;
    }

    // Parse best move
    size_t pos = output.find("bestmove");
    if (pos != std::string::npos)
    {
        std::string bestMoveFull = output.substr(pos + 9);                     // Skip "bestmove "
        std::string bestMove = bestMoveFull.substr(0, bestMoveFull.find(' ')); // Get part before space

        // Extract coordinates from the first 4 characters of the move
        return algebraicToCoordinates(bestMove.substr(0, 4));
    }

    std::cerr << "Failed to get best move from Stockfish" << std::endl;
    return std::make_tuple(-1, -1, -1, -1);
}

bool StockfishConnector::ensureEngineRunning()
{
#ifdef _WIN32
    if (initialized && childProcess)
    {
        DWORD exitCode;
        if (GetExitCodeProcess(childProcess, &exitCode) && exitCode == STILL_ACTIVE)
        {
            return true; // Process is running
        }
    }
#else
    // Linux check - if already initialized, don't try to restart
    if (initialized && stockfishIn && stockfishOut && stockfish_pid > 0)
    {
        // Check if process is still alive by sending a non-disruptive command
        if (fprintf(stockfishIn, "isready\n") > 0) {
            fflush(stockfishIn);
            // Successfully wrote to the pipe - process is likely still alive
            return true;
        }
    }
#endif
    // Only try to restart if we aren't already in the process of initializing
    static bool initializing = false;
    if (initializing) {
        return false;
    }
    
    initializing = true;
    close();

#ifdef _WIN32
    bool result = initialize("stockfish.exe");
#else
    bool result = initialize("./stockfish");  // Use with ./ prefix on Linux
#endif
    initializing = false;

    if (!result)
    {
        std::cerr << "Failed to ensure Stockfish engine is running" << std::endl;
        return false;
    }

    std::cout << "Stockfish engine is running" << std::endl;
    return true;
}