

set -e 

echo "=========================================="
echo "MBO to MBP Converter - Test Suite"
echo "=========================================="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Function to run a test
run_test() {
    local test_name="$1"
    local input_file="$2"
    local expected_output="$3"
    local test_description="$4"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    echo -e "\n${BLUE}Running Test: ${test_name}${NC}"
    echo "Description: ${test_description}"
    
    # Create output directory if it doesn't exist
    mkdir -p test/output
    
    # Run the converter
    local output_file="test/output/${test_name}_output.csv"
    local start_time=$(date +%s)
    
    if ./build/reconstruction_vanshika "$input_file" "$output_file" > /dev/null 2>&1; then
        local end_time=$(date +%s)
        local duration=$(( (end_time - start_time) * 1000 ))  # Convert to milliseconds
        if [ "$duration" -lt 0 ]; then
            duration=1  # Fallback for timing issues
        fi
        
        # Check if output file exists and has content
        if [ -f "$output_file" ] && [ -s "$output_file" ]; then
            local line_count=$(wc -l < "$output_file")
            echo -e "  ${GREEN}✓ PASSED${NC} - Generated ${line_count} records in ${duration}ms"
            PASSED_TESTS=$((PASSED_TESTS + 1))
        else
            echo -e "  ${RED}✗ FAILED${NC} - No output generated"
            FAILED_TESTS=$((FAILED_TESTS + 1))
        fi
    else
        echo -e "  ${RED}✗ FAILED${NC} - Execution error"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
}

# Function to validate output format
validate_output_format() {
    local test_name="$1"
    local output_file="test/output/${test_name}_output.csv"
    
    echo -e "\n${YELLOW}Validating Output Format: ${test_name}${NC}"
    
    if [ ! -f "$output_file" ]; then
        echo -e "  ${RED}✗ FAILED${NC} - Output file not found"
        return 1
    fi
    
    # Check header
    local header=$(head -1 "$output_file")
    if [[ "$header" == "ts_recv" ]] && [[ "$header" == "bid_px_00" ]] && [[ "$header" == "ask_px_00" ]]; then
        echo -e "  ${GREEN}✓ PASSED${NC} - Header format correct"
    else
        echo -e "  ${RED}✗ FAILED${NC} - Header format incorrect"
        return 1
    fi
    
    # Check data records
    local data_lines=$(tail -n +2 "$output_file" | wc -l)
    if [ "$data_lines" -gt 0 ]; then
        echo -e "  ${GREEN}✓ PASSED${NC} - ${data_lines} data records generated"
    else
        echo -e "  ${RED}✗ FAILED${NC} - No data records generated"
        return 1
    fi
    
    # Check first data record format
    local first_record=$(tail -n +2 "$output_file" | head -1)
    if [[ "$first_record" =~ ^[0-9]+, ]]; then
        echo -e "  ${GREEN}✓ PASSED${NC} - Record format correct (starts with index)"
    else
        echo -e "  ${RED}✗ FAILED${NC} - Record format incorrect"
        return 1
    fi
}

# Function to check performance
check_performance() {
    local test_name="$1"
    local input_file="$2"
    local output_file="test/output/${test_name}_output.csv"
    
    echo -e "\n${YELLOW}Performance Check: ${test_name}${NC}"
    
    local start_time=$(date +%s)
    ./build/reconstruction_vanshika "$input_file" "$output_file" > /dev/null 2>&1
    local end_time=$(date +%s)
    local duration=$(( (end_time - start_time) * 1000 ))
    
    local input_lines=$(wc -l < "$input_file")
    local output_lines=$(wc -l < "$output_file")
    if [ "$duration" -eq 0 ]; then
        duration=1  # Avoid division by zero
    fi
    local records_per_second=$(( (input_lines * 1000) / duration ))
    if [ "$records_per_second" -lt 0 ]; then
        records_per_second=0
    fi
    
    echo -e "  Processing time: ${duration}ms"
    echo -e "  Input records: $((input_lines - 1))"
    echo -e "  Output records: $((output_lines - 1))"
    echo -e "  Performance: ${records_per_second} records/sec"
    
    if [ "$records_per_second" -gt 10000 ]; then
        echo -e "  ${GREEN}✓ PASSED${NC} - Performance acceptable"
    else
        echo -e "  ${YELLOW}⚠ WARNING${NC} - Performance below threshold"
    fi
}

# Main test execution
echo -e "\n${BLUE}Building project...${NC}"
if make clean && make > /dev/null 2>&1; then
    echo -e "${GREEN}✓ Build successful${NC}"
else
    echo -e "${RED}✗ Build failed${NC}"
    exit 1
fi

echo -e "\n${BLUE}Running test cases...${NC}"

# Test 1: Simple functionality test
run_test "simple" "test/test_data_simple.csv" "" "Basic order book operations (Add, Cancel, Trade)"

# Test 2: Edge cases test
run_test "edge_cases" "test/test_data_edge_cases.csv" "" "Edge cases (zero size, extreme prices, many orders)"

# Validate outputs
validate_output_format "simple"
validate_output_format "edge_cases"

# Performance checks
check_performance "simple" "test/test_data_simple.csv"
check_performance "edge_cases" "test/test_data_edge_cases.csv"

# Test summary
echo -e "\n=========================================="
echo -e "${BLUE}Test Summary${NC}"
echo -e "=========================================="
echo -e "Total Tests: ${TOTAL_TESTS}"
echo -e "${GREEN}Passed: ${PASSED_TESTS}${NC}"
echo -e "${RED}Failed: ${FAILED_TESTS}${NC}"

if [ "$FAILED_TESTS" -eq 0 ]; then
    echo -e "\n${GREEN}🎉 All tests passed!${NC}"
    exit 0
else
    echo -e "\n${RED}❌ Some tests failed!${NC}"
    exit 1
fi