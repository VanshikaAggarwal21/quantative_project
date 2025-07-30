#pragma once

#include "types.h"
#include "order.h"
#include <map>
#include <unordered_map>
#include <vector>

/**
 * Efficient order book implementation for MBO to MBP conversion
 * 
 * Design Principles:
 * - Use std::map for automatic price level sorting
 * - Use std::unordered_map for O(1) order lookups
 * - Track changes to optimize MBP output generation
 * - Pre-allocate vectors to avoid reallocations
 */
class OrderBook {
private:
    // Price levels: price -> PriceLevel (automatically sorted)
    using LevelOrders = std::map<Price, PriceLevel, std::greater<Price>>;  // Bids: descending
    using AskLevels = std::map<Price, PriceLevel, std::less<Price>>;       // Asks: ascending
    
    LevelOrders bids_;
    AskLevels asks_;
    
    // Fast order lookup: order_id -> (price, side)
    struct OrderLocation {
        Price price;
        char side;
        
        OrderLocation() : price(0), side(0) {}
        OrderLocation(Price p, char s) : price(p), side(s) {}
    };
    std::unordered_map<OrderID, OrderLocation> order_lookup_;
    
    // Track if order book changed (for MBP output optimization)
    bool has_changes_{false};
    
    // Pre-allocated vectors for MBP output
    mutable std::vector<CompactPriceLevel> bid_levels_cache_;
    mutable std::vector<CompactPriceLevel> ask_levels_cache_;

public:
    OrderBook() {
        // Pre-allocate vectors to avoid reallocations
        bid_levels_cache_.reserve(MBP_LEVELS);
        ask_levels_cache_.reserve(MBP_LEVELS);
        order_lookup_.reserve(INITIAL_ORDER_CAPACITY);
    }
    
    /**
     * Apply an MBO record to the order book
     */
    void Apply(const MBORecord& record);
    
    /**
     * Get top N bid levels for MBP output
     */
    std::vector<CompactPriceLevel> GetTopBids(size_t levels = MBP_LEVELS) const;
    
    /**
     * Get top N ask levels for MBP output
     */
    std::vector<CompactPriceLevel> GetTopAsks(size_t levels = MBP_LEVELS) const;
    
    /**
     * Check if order book has changed since last reset
     */
    bool HasChanges() const { return has_changes_; }
    
    /**
     * Reset the change flag
     */
    void ResetChanges() { has_changes_ = false; }
    
    /**
     * Clear the entire order book
     */
    void Clear();
    
    /**
     * Get current best bid and ask
     */
    std::pair<CompactPriceLevel, CompactPriceLevel> GetBestBidAsk() const;
    
    /**
     * Get order book statistics
     */
    struct Statistics {
        size_t total_bid_levels;
        size_t total_ask_levels;
        size_t total_orders;
        Price best_bid;
        Price best_ask;
    };
    Statistics GetStatistics() const;

private:
    /**
     * Add an order to the order book
     */
    void AddOrder(const MBORecord& record);
    
    /**
     * Cancel an order from the order book
     */
    void CancelOrder(const MBORecord& record);
    
    /**
     * Modify an existing order
     */
    void ModifyOrder(const MBORecord& record);
    
    /**
     * Get or create a price level for the given side and price
     */
    PriceLevel& GetOrCreateLevel(char side, Price price);
    
    /**
     * Get a price level for the given side and price (const version)
     */
    const PriceLevel& GetLevel(char side, Price price) const;
    
    /**
     * Remove a price level if it's empty
     */
    void RemoveEmptyLevel(char side, Price price);
    
    /**
     * Mark that the order book has changed
     */
    void MarkChanged() { has_changes_ = true; }
    
    /**
     * Validate order book consistency (for debugging)
     */
    bool ValidateConsistency() const;
}; 