#pragma once

#include "types.h"
#include "utils.h"
#include "order.h"
#include <string>
#include <array>
#include <sstream>
#include <iomanip>

/**
 * Market By Order (MBO) record structure
 * Represents a single order book event
 */
struct MBORecord {
    std::string ts_recv;
    std::string ts_event;
    uint8_t rtype;
    uint16_t publisher_id;
    uint32_t instrument_id;
    char action;
    char side;
    Price price;
    Size size;
    uint8_t channel_id;
    OrderID order_id;
    uint8_t flags;
    int32_t ts_in_delta;
    Sequence sequence;
    std::string symbol;
    
    MBORecord() = default;
    
    /**
     * Parse MBO record from CSV line
     */
    static MBORecord Parse(const std::string& line);
    
    /**
     * Check if this is a top-of-book message
     */
    bool IsTopOfBook() const { 
        return (flags & FLAG_TOB) != 0; 
    }
    
    /**
     * Check if this is the last message in an event
     */
    bool IsLast() const { 
        return (flags & FLAG_LAST) != 0; 
    }
    
    /**
     * Check if this is a snapshot message
     */
    bool IsSnapshot() const { 
        return (flags & FLAG_SNAPSHOT) != 0; 
    }
    
    /**
     * Check if this is an MBP message
     */
    bool IsMBP() const { 
        return (flags & FLAG_MBP) != 0; 
    }
    
    /**
     * Check if this record is valid
     */
    bool IsValid() const {
        return utils::IsValidAction(action) && 
               utils::IsValidSide(side) &&
               (action == ACTION_CLEAR || utils::IsValidPrice(price)) &&
               (action == ACTION_CLEAR || utils::IsValidSize(size));
    }
    
    /**
     * Check if this action affects the order book
     */
    bool AffectsOrderBook() const {
        return action == ACTION_ADD || action == ACTION_CANCEL || 
               action == ACTION_MODIFY || action == ACTION_CLEAR;
    }
};

/**
 * Market By Price (MBP) record structure
 * Represents the top 10 price levels for both bid and ask sides
 */
struct MBPRecord {
    std::string ts_recv;
    std::string ts_event;
    uint8_t rtype;
    uint16_t publisher_id;
    uint32_t instrument_id;
    char action;
    char side;
    uint32_t depth;
    Price price;
    Size size;
    uint8_t flags;
    int32_t ts_in_delta;
    Sequence sequence;
    
    // Top 10 price levels for bid side (descending order)
    std::array<Price, MBP_LEVELS> bid_prices;
    std::array<Size, MBP_LEVELS> bid_sizes;
    std::array<uint32_t, MBP_LEVELS> bid_counts;
    
    // Top 10 price levels for ask side (ascending order)
    std::array<Price, MBP_LEVELS> ask_prices;
    std::array<Size, MBP_LEVELS> ask_sizes;
    std::array<uint32_t, MBP_LEVELS> ask_counts;
    
    std::string symbol;
    OrderID order_id;
    
    MBPRecord() {
        // Initialize arrays with default values
        bid_prices.fill(kUndefPrice);
        bid_sizes.fill(0);
        bid_counts.fill(0);
        ask_prices.fill(kUndefPrice);
        ask_sizes.fill(0);
        ask_counts.fill(0);
    }
    
    /**
     * Convert to CSV line format
     */
    std::string ToCSV() const;
    
    /**
     * Create MBP record from MBO record and order book state
     */
    static MBPRecord FromOrderBook(const MBORecord& mbo_record, 
                                  const std::vector<CompactPriceLevel>& bids,
                                  const std::vector<CompactPriceLevel>& asks);
    
    /**
     * Set bid price level data
     */
    void SetBidLevel(int level, Price price, Size size, uint32_t count) {
        if (level >= 0 && level < MBP_LEVELS) {
            bid_prices[level] = price;
            bid_sizes[level] = size;
            bid_counts[level] = count;
        }
    }
    
    /**
     * Set ask price level data
     */
    void SetAskLevel(int level, Price price, Size size, uint32_t count) {
        if (level >= 0 && level < MBP_LEVELS) {
            ask_prices[level] = price;
            ask_sizes[level] = size;
            ask_counts[level] = count;
        }
    }
    
    /**
     * Get bid price level data
     */
    CompactPriceLevel GetBidLevel(int level) const {
        if (level >= 0 && level < MBP_LEVELS) {
            return CompactPriceLevel(bid_prices[level], bid_sizes[level], bid_counts[level]);
        }
        return CompactPriceLevel();
    }
    
    /**
     * Get ask price level data
     */
    CompactPriceLevel GetAskLevel(int level) const {
        if (level >= 0 && level < MBP_LEVELS) {
            return CompactPriceLevel(ask_prices[level], ask_sizes[level], ask_counts[level]);
        }
        return CompactPriceLevel();
    }
}; 