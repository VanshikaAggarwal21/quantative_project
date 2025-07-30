# Makefile for MBO to MBP Converter
# Performance-optimized build configuration

# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -DNDEBUG
LDFLAGS = 

# Directories
SRCDIR = src
BUILDDIR = build
DATADIR = data
OUTPUTDIR = $(DATADIR)/output
INCLUDEDIR = include

# Source files
SOURCES = $(wildcard $(SRCDIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(BUILDDIR)/%.o)

# Target executable
TARGET = $(BUILDDIR)/reconstruction_vanshika

# Include paths
INCLUDES = -I$(INCLUDEDIR)

# Default target
all: $(TARGET)

# Build executable
$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $(TARGET)
	@echo "Build complete!"

# Compile source files
$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(BUILDDIR)
	@echo "Compiling $<..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(BUILDDIR)
	@echo "Clean complete!"

# Run with sample data
run: $(TARGET)
	@mkdir -p $(OUTPUTDIR)
	@echo "Running with sample data..."
	./$(TARGET) $(DATADIR)/mbo.csv $(OUTPUTDIR)/mbp_output.csv

# Validate output against expected result
validate: run
	@echo "Validating output..."
	@if [ -f $(DATADIR)/mbp.csv ]; then \
		echo "Comparing output with expected result..."; \
		diff $(OUTPUTDIR)/mbp_output.csv $(DATADIR)/mbp.csv || echo "Output differs from expected"; \
	else \
		echo "Expected output file $(DATADIR)/mbp.csv not found"; \
	fi

# Performance test
perf: $(TARGET)
	@echo "Running performance test..."
	@mkdir -p $(OUTPUTDIR)
	@time ./$(TARGET) $(DATADIR)/mbo.csv $(OUTPUTDIR)/mbp_perf_test.csv

# Debug build (with debug symbols and no optimization)
debug: CXXFLAGS = -std=c++17 -O0 -g -Wall -Wextra -DDEBUG
debug: $(TARGET)

# Profile-guided optimization build
pgo: CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -DNDEBUG -fprofile-generate
pgo: LDFLAGS = -fprofile-generate
pgo: $(TARGET)

# Use profile-guided optimization (run after pgo target and sample data)
pgo-use: CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -DNDEBUG -fprofile-use
pgo-use: LDFLAGS = -fprofile-use
pgo-use: $(TARGET)

# Install dependencies (if needed)
install-deps:
	@echo "No external dependencies required - using standard C++ library only"

# Show help
help:
	@echo "Available targets:"
	@echo "  all        - Build the executable (default)"
	@echo "  clean      - Remove build artifacts"
	@echo "  run        - Run with sample data"
	@echo "  validate   - Run and validate against expected output"
	@echo "  perf       - Run performance test"
	@echo "  debug      - Build with debug symbols"
	@echo "  pgo        - Build with profile-guided optimization generation"
	@echo "  pgo-use    - Build using profile-guided optimization"
	@echo "  install-deps - Install dependencies (none required)"
	@echo "  help       - Show this help message"

# Create necessary directories
setup:
	@echo "Setting up project directories..."
	mkdir -p $(BUILDDIR) $(OUTPUTDIR)
	@echo "Setup complete!"

# Check if all required files exist
check-files:
	@echo "Checking required files..."
	@if [ ! -f $(DATADIR)/mbo.csv ]; then \
		echo "ERROR: $(DATADIR)/mbo.csv not found"; \
		exit 1; \
	fi
	@if [ ! -f $(DATADIR)/mbp.csv ]; then \
		echo "WARNING: $(DATADIR)/mbp.csv not found (validation will be skipped)"; \
	fi
	@echo "File check complete!"

# Full build and test
test: setup check-files all validate
	@echo "Full build and test complete!"

.PHONY: all clean run validate perf debug pgo pgo-use install-deps help setup check-files test 