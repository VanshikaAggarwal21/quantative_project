#include "mbo_processor.h"
#include <iostream>
#include <stdexcept>
#include <chrono>

MBOProcessor::MBOProcessor(const std::string& output_filename) {
    // Enable fast I/O for better performance
    utils::EnableFastIO();
    
    // Open output file
    output_file_.open(output_filename);
    if (!output_file_.is_open()) {
        throw std::runtime_error("Failed to open output file: " + output_filename);
    }
    
    // Initialize output
    InitializeOutput();
    
    // Start performance monitoring
    if (enable_performance_monitoring_) {
        performance_monitor_.Start();
    }
}

MBOProcessor::~MBOProcessor() {
    try {
        FlushOutput();
        if (enable_performance_monitoring_) {
            ReportFinalStats();
        }
    } catch (const std::exception& e) {
        std::cerr << "Error during cleanup: " << e.what() << std::endl;
    }
}

void MBOProcessor::ProcessFile(const std::string& input_filename) {
    std::ifstream input_file(input_filename);
    if (!input_file.is_open()) {
        throw std::runtime_error("Failed to open input file: " + input_filename);
    }
    
    std::string line;
    
    // Skip header line
    if (!std::getline(input_file, line)) {
        throw std::runtime_error("Input file is empty or cannot be read");
    }
    
    // Process each line
    while (std::getline(input_file, line)) {
        try {
            auto record = MBORecord::Parse(line);
            ProcessRecord(record);
            
            if (enable_performance_monitoring_) {
                performance_monitor_.RecordProcessed();
                UpdatePerformanceStats();
            }
        } catch (const std::exception& e) {
            std::cerr << "Error processing line " << (record_count_ + 1) << ": " << e.what() << std::endl;
            // Continue processing other records
        }
    }
    
    // Final flush
    FlushOutput();
}

void MBOProcessor::ProcessRecord(const MBORecord& record) {
    // Handle special cases first
    if (HandleSpecialCase(record)) {
        return;
    }
    
    // Apply record to order book
    order_book_.Apply(record);
    record_count_++;
    
    // Only generate MBP output for A, C, R, or T actions
    if (record.action == ACTION_ADD || record.action == ACTION_CANCEL || record.action == ACTION_CLEAR || record.action == ACTION_TRADE) {
        if (order_book_.HasChanges()) {
            auto mbp_record = CreateMBPRecord(record);
            WriteMBPRecord(mbp_record);
            order_book_.ResetChanges();
            mbp_record_count_++;
            
            if (enable_performance_monitoring_) {
                performance_monitor_.MBPRecordGenerated();
            }
        }
    }
}

void MBOProcessor::WriteMBPRecord(const MBPRecord& record) {
    if (validate_output_) {
        ValidateMBPRecord(record);
    }
    
    // Add index and record to output buffer
    output_buffer_ += std::to_string(mbp_record_count_) + record.ToCSV() + '\n';
    
    // Flush if buffer is full
    if (output_buffer_.size() >= OUTPUT_BUFFER_SIZE) {
        FlushOutput();
    }
}

void MBOProcessor::WriteHeader() {
    std::string header = ",ts_recv,ts_event,rtype,publisher_id,instrument_id,action,side,depth,price,size,flags,ts_in_delta,sequence";
    
    // Add bid and ask level headers interleaved with zero-padded two-digit format
    for (int i = 0; i < MBP_LEVELS; ++i) {
        std::string level_str = (i < 10) ? "0" + std::to_string(i) : std::to_string(i);
        header += ",bid_px_" + level_str + ",bid_sz_" + level_str + ",bid_ct_" + level_str;
        header += ",ask_px_" + level_str + ",ask_sz_" + level_str + ",ask_ct_" + level_str;
    }
    
    header += ",symbol,order_id\n";
    
    output_file_ << header;
}

void MBOProcessor::FlushOutput() {
    if (!output_buffer_.empty()) {
        output_file_.write(output_buffer_.data(), output_buffer_.size());
        output_buffer_.clear();
    }
}

MBOProcessor::ProcessingStats MBOProcessor::GetStats() const {
    ProcessingStats stats{};
    stats.records_processed = record_count_;
    stats.mbp_records_generated = mbp_record_count_;
    
    // Calculate processing time and rate
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - performance_monitor_.start_time_);
    stats.processing_time_ms = duration.count();
    
    if (stats.processing_time_ms > 0) {
        stats.records_per_second = (record_count_ * 1000.0) / stats.processing_time_ms;
    }
    
    return stats;
}

void MBOProcessor::InitializeOutput() {
    WriteHeader();
}

MBPRecord MBOProcessor::CreateMBPRecord(const MBORecord& mbo_record) {
    // Get current order book state
    auto bids = order_book_.GetTopBids(MBP_LEVELS);
    auto asks = order_book_.GetTopAsks(MBP_LEVELS);
    
    // Create MBP record
    return MBPRecord::FromOrderBook(mbo_record, bids, asks);
}

void MBOProcessor::ValidateMBPRecord(const MBPRecord& record) const {
    // Basic validation checks
    if (record.rtype != 10) {
        throw std::runtime_error("Invalid MBP record type: " + std::to_string(record.rtype));
    }
    
    if (!utils::IsValidAction(record.action)) {
        throw std::runtime_error("Invalid action in MBP record: " + std::string(1, record.action));
    }
    
    if (!utils::IsValidSide(record.side)) {
        throw std::runtime_error("Invalid side in MBP record: " + std::string(1, record.side));
    }
    
    // Validate price levels
    for (int i = 0; i < MBP_LEVELS; ++i) {
        // Check bid levels
        if (record.bid_prices[i] != kUndefPrice) {
            if (record.bid_sizes[i] == 0 || record.bid_counts[i] == 0) {
                throw std::runtime_error("Invalid bid level " + std::to_string(i) + ": price set but size/count is 0");
            }
        }
        
        // Check ask levels
        if (record.ask_prices[i] != kUndefPrice) {
            if (record.ask_sizes[i] == 0 || record.ask_counts[i] == 0) {
                throw std::runtime_error("Invalid ask level " + std::to_string(i) + ": price set but size/count is 0");
            }
        }
    }
}

bool MBOProcessor::HandleSpecialCase(const MBORecord& record) {
    // Process all records including reset records
    // Reset records should generate MBP records with empty order book
    // We don't skip any records - all should be processed
    // For reset records, we need to clear the order book and generate an MBP record
    if (record.action == ACTION_CLEAR) {
        order_book_.Clear();
        record_count_++;
        
        // Generate MBP record for reset
        auto mbp_record = CreateMBPRecord(record);
        WriteMBPRecord(mbp_record);
        mbp_record_count_++;
        
        if (enable_performance_monitoring_) {
            performance_monitor_.MBPRecordGenerated();
        }
        
        return true;  // Skip normal processing since we handled it here
    }
    
    return false;
}

void MBOProcessor::UpdatePerformanceStats() {
    // Update memory usage (simplified - in production, use proper memory tracking)
    size_t estimated_memory = order_book_.GetStatistics().total_orders * sizeof(OrderID) * 2;
    performance_monitor_.UpdateMemoryUsage(estimated_memory);
}

void MBOProcessor::ReportFinalStats() {
    auto stats = GetStats();
    
    std::cout << "\n=== Processing Complete ===\n";
    std::cout << "Records processed: " << stats.records_processed << "\n";
    std::cout << "MBP records generated: " << stats.mbp_records_generated << "\n";
    std::cout << "Processing time: " << stats.processing_time_ms << "ms\n";
    std::cout << "Processing rate: " << stats.records_per_second << " records/sec\n";
    
    // Order book statistics
    auto ob_stats = order_book_.GetStatistics();
    std::cout << "Final order book state:\n";
    std::cout << "  Bid levels: " << ob_stats.total_bid_levels << "\n";
    std::cout << "  Ask levels: " << ob_stats.total_ask_levels << "\n";
    std::cout << "  Total orders: " << ob_stats.total_orders << "\n";
    
    if (ob_stats.best_bid != kUndefPrice) {
        std::cout << "  Best bid: " << utils::FormatPrice(ob_stats.best_bid) << "\n";
    }
    if (ob_stats.best_ask != kUndefPrice) {
        std::cout << "  Best ask: " << utils::FormatPrice(ob_stats.best_ask) << "\n";
    }
    
    std::cout << "==========================\n";
} 