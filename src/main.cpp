#include "mbo_processor.h"
#include "utils.h"
#include <iostream>
#include <stdexcept>
#include <chrono>

void PrintUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " <input_mbo_file> [output_mbp_file]\n";
    std::cout << "\n";
    std::cout << "Description:\n";
    std::cout << "  Converts Market By Order (MBO) data to Market By Price (MBP) format\n";
    std::cout << "  with top 10 price levels for both bid and ask sides.\n";
    std::cout << "\n";
    std::cout << "Arguments:\n";
    std::cout << "  input_mbo_file   Input MBO CSV file path\n";
    std::cout << "  output_mbp_file  Output MBP CSV file path (optional, defaults to mbp_output.csv)\n";
    std::cout << "\n";
    std::cout << "Example:\n";
    std::cout << "  " << program_name << " mbo.csv mbp_output.csv\n";
    std::cout << "  " << program_name << " data/mbo.csv\n";
}

int main(int argc, char* argv[]) {
    try {
        // Enable fast I/O for better performance
        utils::EnableFastIO();
        
        // Parse command line arguments
        if (argc < 2 || argc > 3) {
            PrintUsage(argv[0]);
            return 1;
        }
        
        std::string input_file = argv[1];
        std::string output_file = (argc == 3) ? argv[2] : "mbp_output.csv";
        
        std::cout << "=== MBO to MBP Converter ===\n";
        std::cout << "Input file:  " << input_file << "\n";
        std::cout << "Output file: " << output_file << "\n";
        std::cout << "============================\n\n";
        
        // Start timing
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Create processor and process file
        MBOProcessor processor(output_file);
        
        // Configure processor
        processor.SetSkipFirstRecord(true);  // Skip initial clear record as per requirements
        processor.SetValidateOutput(true);   // Validate output format
        processor.SetPerformanceMonitoring(true);  // Enable performance monitoring
        
        // Process the file
        processor.ProcessFile(input_file);
        
        // End timing
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        // Get final statistics
        auto stats = processor.GetStats();
        
        std::cout << "\n=== Conversion Complete ===\n";
        std::cout << "Total processing time: " << duration.count() << "ms\n";
        std::cout << "Records processed: " << stats.records_processed << "\n";
        std::cout << "MBP records generated: " << stats.mbp_records_generated << "\n";
        std::cout << "Processing rate: " << stats.records_per_second << " records/sec\n";
        std::cout << "Output saved to: " << output_file << "\n";
        std::cout << "==========================\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown error occurred" << std::endl;
        return 1;
    }
} 