#pragma once

#include "types.h"
#include "orderbook.h"
#include "records.h"
#include "utils.h"
#include <fstream>
#include <string>
#include <memory>

/**
 * Main processor for converting MBO data to MBP format
 * 
 * Design Principles:
 * - Single responsibility: Process MBO records and generate MBP output
 * - Performance optimization: Buffered I/O, change tracking
 * - Error handling: Robust error handling and validation
 * - Memory efficiency: Minimal allocations during processing
 */
class MBOProcessor {
private:
    OrderBook order_book_;
    std::ofstream output_file_;
    std::string output_buffer_;
    uint64_t record_count_{0};
    uint64_t mbp_record_count_{0};
    utils::PerformanceMonitor performance_monitor_;
    
    // Configuration
    bool skip_first_record_{true};  // Skip the initial clear record
    bool validate_output_{true};    // Validate output format
    bool enable_performance_monitoring_{true};
    
    // Buffering for performance
    static constexpr size_t OUTPUT_BUFFER_SIZE = 64 * 1024;  // 64KB

public:
    /**
     * Constructor
     * @param output_filename Output MBP file path
     */
    explicit MBOProcessor(const std::string& output_filename);
    
    /**
     * Destructor - ensures proper cleanup
     */
    ~MBOProcessor();
    
    /**
     * Process MBO file and generate MBP output
     * @param input_filename Input MBO file path
     */
    void ProcessFile(const std::string& input_filename);
    
    /**
     * Process a single MBO record
     * @param record The MBO record to process
     */
    void ProcessRecord(const MBORecord& record);
    
    /**
     * Write MBP record to output
     * @param record The MBP record to write
     */
    void WriteMBPRecord(const MBPRecord& record);
    
    /**
     * Write CSV header to output file
     */
    void WriteHeader();
    
    /**
     * Flush output buffer to file
     */
    void FlushOutput();
    
    /**
     * Get processing statistics
     */
    struct ProcessingStats {
        uint64_t records_processed;
        uint64_t mbp_records_generated;
        uint64_t processing_time_ms;
        double records_per_second;
    };
    ProcessingStats GetStats() const;
    
    /**
     * Set configuration options
     */
    void SetSkipFirstRecord(bool skip) { skip_first_record_ = skip; }
    void SetValidateOutput(bool validate) { validate_output_ = validate; }
    void SetPerformanceMonitoring(bool enable) { enable_performance_monitoring_ = enable; }

private:
    /**
     * Initialize output file and write header
     */
    void InitializeOutput();
    
    /**
     * Create MBP record from current order book state
     * @param mbo_record The original MBO record
     * @return MBP record with current order book state
     */
    MBPRecord CreateMBPRecord(const MBORecord& mbo_record);
    
    /**
     * Validate MBP record before writing
     * @param record The MBP record to validate
     */
    void ValidateMBPRecord(const MBPRecord& record) const;
    
    /**
     * Handle special cases (like initial clear record)
     * @param record The MBO record to check
     * @return True if this is a special case that should be handled differently
     */
    bool HandleSpecialCase(const MBORecord& record);
    
    /**
     * Update performance monitoring
     */
    void UpdatePerformanceStats();
    
    /**
     * Report final statistics
     */
    void ReportFinalStats();
}; 