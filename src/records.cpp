#include "records.h"
#include "utils.h"
#include <stdexcept>

MBORecord MBORecord::Parse(const std::string& line) {
    auto fields = utils::SplitCSVLine(line);
    
    if (fields.size() != 15) {
        throw std::runtime_error("Invalid MBO record: expected 15 fields, got " + 
                                std::to_string(fields.size()));
    }
    
    MBORecord record;
    
    try {
        record.ts_recv = std::string(fields[0]);
        record.ts_event = std::string(fields[1]);
        record.rtype = utils::ParseUint8(fields[2]);
        record.publisher_id = utils::ParseUint16(fields[3]);
        record.instrument_id = utils::ParseUint32(fields[4]);
        record.action = fields[5][0];  // Single character
        record.side = fields[6][0];    // Single character
        record.price = utils::ParsePrice(fields[7]);
        record.size = utils::ParseUint32(fields[8]);
        record.channel_id = utils::ParseUint8(fields[9]);
        record.order_id = utils::ParseUint64(fields[10]);
        record.flags = utils::ParseUint8(fields[11]);
        record.ts_in_delta = utils::ParseInt32(fields[12]);
        record.sequence = utils::ParseUint32(fields[13]);
        record.symbol = std::string(fields[14]);
    } catch (const std::exception& e) {
        throw std::runtime_error("Failed to parse MBO record: " + std::string(e.what()));
    }
    
    return record;
}

std::string MBPRecord::ToCSV() const {
    std::ostringstream oss;
    
    // Write metadata fields
    oss << ",";  // Empty first field (index will be added by processor)
    oss << ts_recv << ",";
    oss << ts_event << ",";
    oss << static_cast<int>(rtype) << ",";
    oss << publisher_id << ",";
    oss << instrument_id << ",";
    oss << action << ",";
    oss << side << ",";
    oss << depth << ",";
    oss << utils::FormatPrice(price) << ",";
    oss << size << ",";
    oss << static_cast<int>(flags) << ",";
    oss << ts_in_delta << ",";
    oss << sequence << ",";
    
    // Write bid and ask levels interleaved (top 10)
    for (int i = 0; i < MBP_LEVELS; ++i) {
        oss << utils::FormatPrice(bid_prices[i]) << ",";
        oss << bid_sizes[i] << ",";
        oss << bid_counts[i] << ",";
        oss << utils::FormatPrice(ask_prices[i]) << ",";
        oss << ask_sizes[i] << ",";
        oss << ask_counts[i] << ",";
    }
    
    // Write final fields
    oss << symbol << ",";
    oss << order_id;
    
    return oss.str();
}

MBPRecord MBPRecord::FromOrderBook(const MBORecord& mbo_record, 
                                  const std::vector<CompactPriceLevel>& bids,
                                  const std::vector<CompactPriceLevel>& asks) {
    MBPRecord mbp_record;
    
    // Copy metadata from MBO record
    mbp_record.ts_recv = mbo_record.ts_recv;
    mbp_record.ts_event = mbo_record.ts_event;
    mbp_record.rtype = 10;  // MBP record type
    mbp_record.publisher_id = mbo_record.publisher_id;
    mbp_record.instrument_id = mbo_record.instrument_id;
    mbp_record.action = mbo_record.action;
    mbp_record.side = mbo_record.side;
    mbp_record.depth = 0;  // Will be set based on the action
    mbp_record.price = mbo_record.price;
    mbp_record.size = mbo_record.size;
    mbp_record.flags = mbo_record.flags;
    mbp_record.ts_in_delta = mbo_record.ts_in_delta;
    mbp_record.sequence = mbo_record.sequence;
    mbp_record.symbol = mbo_record.symbol;
    mbp_record.order_id = mbo_record.order_id;
    
    // Set depth based on action type
    if (mbo_record.action == ACTION_CANCEL) {
        mbp_record.depth = 1;  // Cancel action has depth 1
    }
    
    // Copy bid levels (up to 10)
    for (size_t i = 0; i < bids.size() && i < MBP_LEVELS; ++i) {
        mbp_record.SetBidLevel(i, bids[i].price, bids[i].size, bids[i].count);
    }
    
    // Copy ask levels (up to 10)
    for (size_t i = 0; i < asks.size() && i < MBP_LEVELS; ++i) {
        mbp_record.SetAskLevel(i, asks[i].price, asks[i].size, asks[i].count);
    }
    
    return mbp_record;
} 