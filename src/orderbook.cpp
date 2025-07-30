#include "orderbook.h"
#include "records.h"
#include <stdexcept>
#include <iostream>

void OrderBook::Apply(const MBORecord& record) {
    if (!record.IsValid()) {
        throw std::invalid_argument("Invalid MBO record");
    }
    
    switch (record.action) {
        case ACTION_ADD:
            AddOrder(record);
            break;
        case ACTION_CANCEL:
            CancelOrder(record);
            break;
        case ACTION_MODIFY:
            ModifyOrder(record);
            break;
        case ACTION_CLEAR:
            Clear();
            break;
        case ACTION_TRADE:
        case ACTION_FILL:
        case ACTION_NONE:
            // These actions don't affect the order book
            break;
        default:
            throw std::invalid_argument("Unknown action: " + std::string(1, record.action));
    }
}

void OrderBook::AddOrder(const MBORecord& record) {
    // Check if order already exists
    if (order_lookup_.find(record.order_id) != order_lookup_.end()) {
        throw std::runtime_error("Order ID " + std::to_string(record.order_id) + " already exists");
    }
    
    // Add order to the appropriate price level
    PriceLevel& level = GetOrCreateLevel(record.side, record.price);
    level.AddOrder(record.order_id, record.size);
    
    // Track the order location
    order_lookup_[record.order_id] = OrderLocation(record.price, record.side);
    
    MarkChanged();
}

void OrderBook::CancelOrder(const MBORecord& record) {
    auto it = order_lookup_.find(record.order_id);
    if (it == order_lookup_.end()) {
        // Order not found - this might be normal for some edge cases
        return;
    }
    
    Price price = it->second.price;
    char side = it->second.side;
    
    // Remove from price level
    PriceLevel& level = GetOrCreateLevel(side, price);
    level.RemoveOrder(record.order_id);
    
    // Remove from order lookup
    order_lookup_.erase(it);
    
    // Remove empty price level
    RemoveEmptyLevel(side, price);
    
    MarkChanged();
}

void OrderBook::ModifyOrder(const MBORecord& record) {
    auto it = order_lookup_.find(record.order_id);
    if (it == order_lookup_.end()) {
        // Order not found, treat as add
        AddOrder(record);
        return;
    }
    
    Price old_price = it->second.price;
    char old_side = it->second.side;
    
    // If price or side changed, we need to move the order
    if (old_price != record.price || old_side != record.side) {
        // Remove from old level
        PriceLevel& old_level = GetOrCreateLevel(old_side, old_price);
        old_level.RemoveOrder(record.order_id);
        RemoveEmptyLevel(old_side, old_price);
        
        // Add to new level
        PriceLevel& new_level = GetOrCreateLevel(record.side, record.price);
        new_level.AddOrder(record.order_id, record.size);
        
        // Update order lookup
        it->second = OrderLocation(record.price, record.side);
    } else {
        // Same price and side, just modify size
        PriceLevel& level = GetOrCreateLevel(record.side, record.price);
        level.ModifyOrder(record.order_id, record.size);
    }
    
    MarkChanged();
}

void OrderBook::Clear() {
    bids_.clear();
    asks_.clear();
    order_lookup_.clear();
    MarkChanged();
}

std::vector<CompactPriceLevel> OrderBook::GetTopBids(size_t levels) const {
    bid_levels_cache_.clear();
    
    for (const auto& [price, level] : bids_) {
        if (bid_levels_cache_.size() >= levels) break;
        if (!level.IsEmpty()) {
            bid_levels_cache_.emplace_back(level);
        }
    }
    
    return bid_levels_cache_;
}

std::vector<CompactPriceLevel> OrderBook::GetTopAsks(size_t levels) const {
    ask_levels_cache_.clear();
    
    for (const auto& [price, level] : asks_) {
        if (ask_levels_cache_.size() >= levels) break;
        if (!level.IsEmpty()) {
            ask_levels_cache_.emplace_back(level);
        }
    }
    
    return ask_levels_cache_;
}

std::pair<CompactPriceLevel, CompactPriceLevel> OrderBook::GetBestBidAsk() const {
    CompactPriceLevel best_bid, best_ask;
    
    // Get best bid (highest price)
    if (!bids_.empty()) {
        const auto& level = bids_.begin()->second;
        if (!level.IsEmpty()) {
            best_bid = CompactPriceLevel(level);
        }
    }
    
    // Get best ask (lowest price)
    if (!asks_.empty()) {
        const auto& level = asks_.begin()->second;
        if (!level.IsEmpty()) {
            best_ask = CompactPriceLevel(level);
        }
    }
    
    return {best_bid, best_ask};
}

OrderBook::Statistics OrderBook::GetStatistics() const {
    Statistics stats{};
    
    stats.total_bid_levels = bids_.size();
    stats.total_ask_levels = asks_.size();
    stats.total_orders = order_lookup_.size();
    
    // Get best bid
    if (!bids_.empty()) {
        stats.best_bid = bids_.begin()->first;
    } else {
        stats.best_bid = kUndefPrice;
    }
    
    // Get best ask
    if (!asks_.empty()) {
        stats.best_ask = asks_.begin()->first;
    } else {
        stats.best_ask = kUndefPrice;
    }
    
    return stats;
}

PriceLevel& OrderBook::GetOrCreateLevel(char side, Price price) {
    if (side == BID_SIDE) {
        auto& level = bids_[price];
        if (level.price == kUndefPrice) {
            level.price = price;
        }
        return level;
    } else if (side == ASK_SIDE) {
        auto& level = asks_[price];
        if (level.price == kUndefPrice) {
            level.price = price;
        }
        return level;
    } else {
        throw std::invalid_argument("Invalid side: " + std::string(1, side));
    }
}

const PriceLevel& OrderBook::GetLevel(char side, Price price) const {
    if (side == BID_SIDE) {
        auto it = bids_.find(price);
        if (it == bids_.end()) {
            throw std::runtime_error("Bid level not found at price " + std::to_string(price));
        }
        return it->second;
    } else if (side == ASK_SIDE) {
        auto it = asks_.find(price);
        if (it == asks_.end()) {
            throw std::runtime_error("Ask level not found at price " + std::to_string(price));
        }
        return it->second;
    } else {
        throw std::invalid_argument("Invalid side: " + std::string(1, side));
    }
}

void OrderBook::RemoveEmptyLevel(char side, Price price) {
    if (side == BID_SIDE) {
        auto it = bids_.find(price);
        if (it != bids_.end() && it->second.IsEmpty()) {
            bids_.erase(it);
        }
    } else if (side == ASK_SIDE) {
        auto it = asks_.find(price);
        if (it != asks_.end() && it->second.IsEmpty()) {
            asks_.erase(it);
        }
    }
}

bool OrderBook::ValidateConsistency() const {
    // Check that all orders in lookup exist in their price levels
    for (const auto& [order_id, location] : order_lookup_) {
        try {
            const PriceLevel& level = GetLevel(location.side, location.price);
            if (!level.HasOrder(order_id)) {
                std::cerr << "Order " << order_id << " not found in price level" << std::endl;
                return false;
            }
        } catch (const std::exception& e) {
            std::cerr << "Order " << order_id << " references invalid price level: " << e.what() << std::endl;
            return false;
        }
    }
    
    // Check that all orders in price levels are tracked in lookup
    for (const auto& [price, level] : bids_) {
        for (const auto& [order_id, size] : level.orders) {
            auto it = order_lookup_.find(order_id);
            if (it == order_lookup_.end() || it->second.price != price || it->second.side != BID_SIDE) {
                std::cerr << "Bid order " << order_id << " not properly tracked in lookup" << std::endl;
                return false;
            }
        }
    }
    
    for (const auto& [price, level] : asks_) {
        for (const auto& [order_id, size] : level.orders) {
            auto it = order_lookup_.find(order_id);
            if (it == order_lookup_.end() || it->second.price != price || it->second.side != ASK_SIDE) {
                std::cerr << "Ask order " << order_id << " not properly tracked in lookup" << std::endl;
                return false;
            }
        }
    }
    
    return true;
} 