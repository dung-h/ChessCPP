#pragma once
#include <string>
#include <memory>
#include <array>
#include <tuple>
#include "Board.h"

#ifdef _WIN32
#include <windows.h>
#endif

// Add this declaration so it's visible to other files
std::string boardToFEN(const Board& board);

class StockfishConnector {
private:
    FILE* stockfishProcess;
    bool initialized;
    
    
#ifdef _WIN32
    // Windows-specific handles
    HANDLE childProcess;
    HANDLE childStdin;
    HANDLE childStdout;
#else
    // Linux-specific members
    // FILE* stockfishProcess;
    
    // New members for the separate pipe approach
    pid_t stockfish_pid;
    int stockfish_in_fd;
    int stockfish_out_fd;
    FILE* stockfishIn;  // For writing to Stockfish
    FILE* stockfishOut; // For reading from Stockfish
#endif
    
    std::string sendCommand(const std::string& cmd);
    std::string getEngineOutput();
    void writeCommand(const std::string& cmd);  
public:
    StockfishConnector();
    ~StockfishConnector();
    
    bool initialize(const std::string& pathToStockfish);
    std::tuple<int, int, int, int> getBestMove(const Board& board, int thinkingTimeMs = 1000);
    void close();
    bool ensureEngineRunning();
    void stopEngine() {
        try {
            sendCommand("stop");
        } catch (...) {
            // Handle exceptions
        }
    }
};

// Also add this declaration if used elsewhere
std::tuple<int, int, int, int> algebraicToCoordinates(const std::string& move);