#pragma once

#include "types.h"
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <iostream>

namespace utils {

/**
 * Split a CSV line into fields using string_view for zero-copy parsing
 * @param line The CSV line to parse
 * @return Vector of string_view fields
 */
std::vector<std::string_view> SplitCSVLine(std::string_view line);

/**
 * Parse a string to uint64_t without string allocation
 * @param str The string to parse
 * @return Parsed uint64_t value
 */
uint64_t ParseUint64(std::string_view str);

/**
 * Parse a string to uint32_t without string allocation
 * @param str The string to parse
 * @return Parsed uint32_t value
 */
uint32_t ParseUint32(std::string_view str);

/**
 * Parse a string to int32_t without string allocation
 * @param str The string to parse
 * @return Parsed int32_t value
 */
int32_t ParseInt32(std::string_view str);

/**
 * Parse a string to uint8_t without string allocation
 * @param str The string to parse
 * @return Parsed uint8_t value
 */
uint8_t ParseUint8(std::string_view str);

/**
 * Parse a string to uint16_t without string allocation
 * @param str The string to parse
 * @return Parsed uint16_t value
 */
uint16_t ParseUint16(std::string_view str);

/**
 * Parse a string price to int64_t (handles scientific notation)
 * @param price_str The price string (e.g., "5.510000000")
 * @return Price as int64_t (e.g., 5510000000 for 5.51)
 */
Price ParsePrice(std::string_view price_str);

/**
 * Convert int64_t price to string for CSV output
 * @param price The price as int64_t
 * @return Formatted price string with 9 decimal places
 */
std::string FormatPrice(Price price);

/**
 * Parse timestamp string to uint64_t
 * @param timestamp_str The timestamp string
 * @return Timestamp as uint64_t
 */
Timestamp ParseTimestamp(std::string_view timestamp_str);

/**
 * Format timestamp for output
 * @param timestamp The timestamp as uint64_t
 * @return Formatted timestamp string
 */
std::string FormatTimestamp(Timestamp timestamp);

/**
 * Check if a price is valid (not undefined)
 * @param price The price to check
 * @return True if price is valid
 */
bool IsValidPrice(Price price);

/**
 * Check if a size is valid (greater than 0)
 * @param size The size to check
 * @return True if size is valid
 */
bool IsValidSize(Size size);

/**
 * Check if a side is valid
 * @param side The side to check
 * @return True if side is valid
 */
bool IsValidSide(char side);

/**
 * Check if an action is valid
 * @param action The action to check
 * @return True if action is valid
 */
bool IsValidAction(char action);

/**
 * Enable fast I/O for better performance
 */
void EnableFastIO();

/**
 * Performance monitoring class
 */
class PerformanceMonitor {
private:
    size_t records_processed_{0};
    size_t mbp_records_generated_{0};
    size_t peak_memory_usage_{0};

public:
    std::chrono::high_resolution_clock::time_point start_time_;
    
    void Start() { 
        start_time_ = std::chrono::high_resolution_clock::now(); 
    }
    
    void RecordProcessed() { records_processed_++; }
    void MBPRecordGenerated() { mbp_records_generated_++; }
    void UpdateMemoryUsage(size_t usage) { 
        if (usage > peak_memory_usage_) peak_memory_usage_ = usage; 
    }
    
    void Report() const {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time_);
        
        std::cout << "=== Performance Report ===\n";
        std::cout << "Records processed: " << records_processed_ << "\n";
        std::cout << "MBP records generated: " << mbp_records_generated_ << "\n";
        std::cout << "Processing time: " << duration.count() << "ms\n";
        std::cout << "Processing rate: " << (records_processed_ * 1000.0 / duration.count()) << " records/sec\n";
        std::cout << "Peak memory usage: " << peak_memory_usage_ << " bytes\n";
        std::cout << "========================\n";
    }
};

} // namespace utils 