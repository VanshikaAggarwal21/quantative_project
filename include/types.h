#pragma once

#include <cstdint>
#include <string>
#include <array>
#include <vector>
#include <map>
#include <unordered_map>

// Core type aliases for better readability
using OrderID = uint64_t;
using Price = int64_t;
using Size = uint32_t;
using Timestamp = uint64_t;
using Sequence = uint32_t;

// Constants
constexpr int MBP_LEVELS = 10;
constexpr Price kUndefPrice = INT64_MAX;  // 9223372036854775807

// Side constants
constexpr char BID_SIDE = 'B';
constexpr char ASK_SIDE = 'A';
constexpr char NEUTRAL_SIDE = 'N';

// Action constants
constexpr char ACTION_ADD = 'A';
constexpr char ACTION_CANCEL = 'C';
constexpr char ACTION_MODIFY = 'M';
constexpr char ACTION_TRADE = 'T';
constexpr char ACTION_FILL = 'F';
constexpr char ACTION_CLEAR = 'R';
constexpr char ACTION_NONE = 'N';

// Flag constants
constexpr uint8_t FLAG_LAST = 128;    // F_LAST
constexpr uint8_t FLAG_TOB = 64;      // F_TOB
constexpr uint8_t FLAG_SNAPSHOT = 32; // F_SNAPSHOT
constexpr uint8_t FLAG_MBP = 16;      // F_MBP

// Performance constants
constexpr size_t BUFFER_SIZE = 64 * 1024;  // 64KB
constexpr size_t BATCH_SIZE = 1000;
constexpr size_t INITIAL_ORDER_CAPACITY = 1000;

// Forward declarations
struct MBORecord;
struct MBPRecord;
struct PriceLevel;
struct Order;
class OrderBook;
class SequenceTracker;
class MBOProcessor;

// Side enum for type safety
enum class Side : char {
    Bid = BID_SIDE,
    Ask = ASK_SIDE,
    None = NEUTRAL_SIDE
};

// Action enum for type safety
enum class Action : char {
    Add = ACTION_ADD,
    Cancel = ACTION_CANCEL,
    Modify = ACTION_MODIFY,
    Trade = ACTION_TRADE,
    Fill = ACTION_FILL,
    Clear = ACTION_CLEAR,
    None = ACTION_NONE
}; 