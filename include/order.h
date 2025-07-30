#pragma once

#include "types.h"
#include "utils.h"
#include <vector>
#include <unordered_map>

/**
 * Represents an individual order in the order book
 */
struct Order {
    OrderID order_id;
    Price price;
    Size size;
    char side;
    Timestamp timestamp;
    
    Order() = default;
    
    Order(OrderID id, Price p, Size sz, char s, Timestamp ts)
        : order_id(id), price(p), size(sz), side(s), timestamp(ts) {}
    
    bool IsValid() const {
        return order_id != 0 && utils::IsValidPrice(price) && 
               utils::IsValidSize(size) && utils::IsValidSide(side);
    }
};

/**
 * Represents a price level containing multiple orders
 */
struct PriceLevel {
    Price price{kUndefPrice};
    Size total_size{0};
    uint32_t order_count{0};
    
    // Map of order_id to order size for efficient lookups
    std::unordered_map<OrderID, Size> orders;
    
    PriceLevel() = default;
    
    explicit PriceLevel(Price p) : price(p) {}
    
    /**
     * Check if this price level is empty
     */
    bool IsEmpty() const { 
        return price == kUndefPrice || order_count == 0; 
    }
    
    /**
     * Add an order to this price level
     */
    void AddOrder(OrderID order_id, Size size) {
        orders[order_id] = size;
        total_size += size;
        order_count++;
    }
    
    /**
     * Remove an order from this price level
     */
    void RemoveOrder(OrderID order_id) {
        auto it = orders.find(order_id);
        if (it != orders.end()) {
            total_size -= it->second;
            order_count--;
            orders.erase(it);
            
            // If no orders left, mark as empty
            if (order_count == 0) {
                price = kUndefPrice;
                total_size = 0;
            }
        }
    }
    
    /**
     * Modify an existing order's size
     */
    void ModifyOrder(OrderID order_id, Size new_size) {
        auto it = orders.find(order_id);
        if (it != orders.end()) {
            total_size -= it->second;  // Remove old size
            total_size += new_size;    // Add new size
            it->second = new_size;
        }
    }
    
    /**
     * Get the size of a specific order
     */
    Size GetOrderSize(OrderID order_id) const {
        auto it = orders.find(order_id);
        return (it != orders.end()) ? it->second : 0;
    }
    
    /**
     * Check if an order exists in this price level
     */
    bool HasOrder(OrderID order_id) const {
        return orders.find(order_id) != orders.end();
    }
    
    /**
     * Clear all orders from this price level
     */
    void Clear() {
        orders.clear();
        total_size = 0;
        order_count = 0;
        price = kUndefPrice;
    }
};

/**
 * Compact representation of a price level for MBP output
 */
struct CompactPriceLevel {
    Price price{kUndefPrice};
    Size size{0};
    uint32_t count{0};
    
    CompactPriceLevel() = default;
    
    CompactPriceLevel(Price p, Size sz, uint32_t ct) 
        : price(p), size(sz), count(ct) {}
    
    explicit CompactPriceLevel(const PriceLevel& level)
        : price(level.price), size(level.total_size), count(level.order_count) {}
    
    bool IsEmpty() const { return price == kUndefPrice; }
    
    operator bool() const { return !IsEmpty(); }
}; 