# Smartbook Project - Top-Level Makefile
# Orchestrates builds across all components

.PHONY: help clean docs all test test-unit test-integration run install
.PHONY: linux linux-debian-arm64 linux-debian-x86_64 linux-ubuntu-x86_64 linux-fedora-x86_64 linux-arch-x86_64
.PHONY: macos macos-arm64 macos-x86_64
.PHONY: windows windows-x86_64

# Detect OS and architecture
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

# Normalize architecture
ifeq ($(UNAME_M),x86_64)
    ARCH := x86_64
else ifeq ($(UNAME_M),amd64)
    ARCH := x86_64
else ifeq ($(UNAME_M),aarch64)
    ARCH := arm64
else ifeq ($(UNAME_M),arm64)
    ARCH := arm64
else
    ARCH := $(UNAME_M)
endif

# Detect Linux distribution
ifeq ($(UNAME_S),Linux)
    ifneq ($(wildcard /etc/os-release),)
        DISTRO_ID := $(shell grep "^ID=" /etc/os-release | cut -d= -f2 | tr -d '"' | tr '[:upper:]' '[:lower:]')
    else
        DISTRO_ID := unknown
    endif
endif

# Build directory
BUILD_DIR := build
DOCS_DIR := $(BUILD_DIR)/docs

# Default target
.DEFAULT_GOAL := help

help:
	@echo "Smartbook Project - Build System"
	@echo "================================="
	@echo ""
	@echo "Available targets:"
	@echo "  help              - Display this help message"
	@echo "  clean             - Remove all build artifacts"
	@echo "  docs              - Generate all documentation PDFs"
	@echo "  all               - Clean, build all platforms, generate docs, run tests"
	@echo "  install           - Install built binaries (Unix/macOS only, use install-windows.ps1 on Windows)"
	@echo "  test              - Build and run all tests"
	@echo "  test-unit         - Build and run unit tests only"
	@echo "  test-integration  - Build and run integration tests only"
	@echo "  run               - Run reader application (default)"
	@echo "  run-reader        - Run reader application"
	@echo "  run-creator       - Run creator tool"
	@echo "  run-server        - Run server application (Phase 2)"
	@echo ""
	@echo "Linux targets:"
	@echo "  linux             - Build for current Linux system (auto-detect)"
	@echo "  linux-debian-arm64    - Debian on ARM64"
	@echo "  linux-debian-x86_64   - Debian on x86_64"
	@echo "  linux-ubuntu-x86_64   - Ubuntu on x86_64"
	@echo "  linux-fedora-x86_64   - Fedora on x86_64"
	@echo "  linux-arch-x86_64     - Arch Linux on x86_64"
	@echo ""
	@echo "macOS targets:"
	@echo "  macos             - Build universal binary (auto-detect)"
	@echo "  macos-arm64       - macOS on Apple Silicon"
	@echo "  macos-x86_64      - macOS on Intel"
	@echo ""
	@echo "Windows targets:"
	@echo "  windows-x86_64    - Windows on x86_64"
	@echo ""
	@echo "Current system: $(UNAME_S) $(ARCH)"
	@if [ "$(UNAME_S)" = "Linux" ]; then \
		echo "Detected distribution: $(DISTRO_ID)"; \
	fi

clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)
	@cd common && $(MAKE) clean 2>/dev/null || true
	@cd reader && $(MAKE) clean 2>/dev/null || true
	@cd creator && $(MAKE) clean 2>/dev/null || true
	@cd server && $(MAKE) clean 2>/dev/null || true
	@cd test && $(MAKE) clean 2>/dev/null || true
	@echo "Clean complete."

docs:
	@echo "Generating documentation PDFs..."
	@mkdir -p $(DOCS_DIR)
	@which asciidoctor > /dev/null 2>&1 || (echo "Error: asciidoctor not found. Install with: gem install asciidoctor" && exit 1)
	@which mmdc > /dev/null 2>&1 || echo "Warning: mermaid-cli (mmdc) not found. Diagrams may not be converted."
	@echo "Using HTML → PDF workflow for better list rendering..."
	@echo "Step 1: Generating HTML from AsciiDoc..."
	@mkdir -p $(DOCS_DIR)/html
	@for doc in Documentation/*.adoc; do \
		if [ -f "$$doc" ]; then \
			echo "  Converting $$doc to HTML..."; \
			asciidoctor -D $(DOCS_DIR)/html "$$doc" || echo "Warning: Failed to convert $$doc to HTML"; \
		fi \
	done
	@echo "Step 2: Converting HTML to PDF..."
	@if command -v wkhtmltopdf > /dev/null 2>&1; then \
		for html in $(DOCS_DIR)/html/*.html; do \
			if [ -f "$$html" ]; then \
				pdf_name=$$(basename "$$html" .html).pdf; \
				echo "  Converting $$(basename $$html) to PDF..."; \
				wkhtmltopdf --page-size Letter --margin-top 0.75in --margin-bottom 0.75in --margin-left 0.75in --margin-right 0.75in --print-media-type "$$html" "$(DOCS_DIR)/$$pdf_name" 2>/dev/null || echo "Warning: Failed to convert $$html"; \
			fi \
		done; \
	elif command -v weasyprint > /dev/null 2>&1; then \
		for html in $(DOCS_DIR)/html/*.html; do \
			if [ -f "$$html" ]; then \
				pdf_name=$$(basename "$$html" .html).pdf; \
				echo "  Converting $$(basename $$html) to PDF..."; \
				weasyprint "$$html" "$(DOCS_DIR)/$$pdf_name" 2>/dev/null || echo "Warning: Failed to convert $$html"; \
			fi \
		done; \
	else \
		echo "Error: No PDF converter found. Install one of:"; \
		echo "  - wkhtmltopdf: brew install wkhtmltopdf (macOS) or apt-get install wkhtmltopdf (Linux)"; \
		echo "  - weasyprint: pip install weasyprint"; \
		echo ""; \
		echo "Falling back to asciidoctor-pdf (may have list rendering issues)..."; \
		@which asciidoctor-pdf > /dev/null 2>&1 || (echo "Error: asciidoctor-pdf not found. Install with: gem install asciidoctor-pdf" && exit 1); \
		for doc in Documentation/*.adoc; do \
			if [ -f "$$doc" ]; then \
				echo "Processing $$doc..."; \
				asciidoctor-pdf -a pdf-theme=Documentation/pdf-theme.yml -D $(DOCS_DIR) "$$doc" || echo "Warning: Failed to process $$doc"; \
			fi \
		done; \
	fi
	@echo "Documentation generated in $(DOCS_DIR)/"

# Linux platform targets
linux:
	@if [ "$(DISTRO_ID)" = "debian" ]; then \
		$(MAKE) linux-debian-$(ARCH); \
	elif [ "$(DISTRO_ID)" = "ubuntu" ]; then \
		$(MAKE) linux-ubuntu-$(ARCH); \
	elif [ "$(DISTRO_ID)" = "fedora" ]; then \
		$(MAKE) linux-fedora-$(ARCH); \
	elif [ "$(DISTRO_ID)" = "arch" ] || [ "$(DISTRO_ID)" = "archlinux" ]; then \
		$(MAKE) linux-arch-$(ARCH); \
	else \
		echo "Unknown Linux distribution: $(DISTRO_ID)"; \
		echo "Please use specific target: linux-debian-x86_64, linux-ubuntu-x86_64, etc."; \
		exit 1; \
	fi

linux-debian-arm64 linux-debian-x86_64 linux-ubuntu-x86_64 linux-fedora-x86_64 linux-arch-x86_64:
	@PLATFORM=$(subst linux-,,$@); \
	echo "Building for Linux: $$PLATFORM..."; \
	mkdir -p $(BUILD_DIR)/$$PLATFORM; \
	cd common && $(MAKE) PLATFORM=$$PLATFORM BUILD_DIR=../$(BUILD_DIR)/$$PLATFORM; \
	cd ../reader && $(MAKE) PLATFORM=$$PLATFORM BUILD_DIR=../$(BUILD_DIR)/$$PLATFORM; \
	cd ../creator && $(MAKE) PLATFORM=$$PLATFORM BUILD_DIR=../$(BUILD_DIR)/$$PLATFORM; \
	echo "Linux build complete: $$PLATFORM"

# macOS platform targets
macos:
	@echo "Building universal binary for macOS..."
	@$(MAKE) macos-arm64
	@$(MAKE) macos-x86_64
	@echo "Creating universal binary..."
	@lipo -create \
		$(BUILD_DIR)/macos-arm64/lib/libsmartbook_common.dylib \
		$(BUILD_DIR)/macos-x86_64/lib/libsmartbook_common.dylib \
		-output $(BUILD_DIR)/macos-universal/lib/libsmartbook_common.dylib 2>/dev/null || \
		echo "Note: Universal binary creation requires both architectures built first"

macos-arm64 macos-x86_64:
	@ARCH=$(subst macos-,,$@); \
	echo "Building for macOS: $$ARCH..."; \
	mkdir -p $(BUILD_DIR)/macos-$$ARCH; \
	cd common && $(MAKE) PLATFORM=macos-$$ARCH BUILD_DIR=../$(BUILD_DIR)/macos-$$ARCH; \
	cd ../reader && $(MAKE) PLATFORM=macos-$$ARCH BUILD_DIR=../$(BUILD_DIR)/macos-$$ARCH; \
	cd ../creator && $(MAKE) PLATFORM=macos-$$ARCH BUILD_DIR=../$(BUILD_DIR)/macos-$$ARCH; \
	echo "macOS build complete: $$ARCH"

# Windows platform targets
windows-x86_64:
	@echo "Building for Windows: x86_64..."
	@echo "Note: Windows builds use PowerShell scripts. Run:"
	@echo "  .\build-windows.ps1"
	@echo "Or use CMake directly:"
	@echo "  cmake -B build/windows-x86_64 -S ."
	@echo "  cmake --build build/windows-x86_64"
	@if command -v powershell >/dev/null 2>&1 && [ -f "build-windows.ps1" ]; then \
		powershell -ExecutionPolicy Bypass -File build-windows.ps1 -Target all; \
	elif [ -f "build-windows.ps1" ]; then \
		echo "Error: PowerShell not found. Use PowerShell to build on Windows."; \
	else \
		echo "Error: build-windows.ps1 not found. Use PowerShell to build on Windows."; \
	fi

# Test targets
test: test-build
	@echo "Running all tests..."
	@cd test && $(MAKE) run

test-build:
	@echo "Building test suite..."
	@cd test && $(MAKE) build

test-unit: test-build
	@echo "Running unit tests..."
	@cd test && $(MAKE) run-unit

test-integration: test-build
	@echo "Running integration tests..."
	@cd test && $(MAKE) run-integration

# Run targets
run: run-reader

run-reader:
	@if [ "$(UNAME_S)" = "Linux" ]; then \
		PLATFORM=linux-$(DISTRO_ID)-$(ARCH); \
		READER_BIN="$(BUILD_DIR)/$$PLATFORM/reader/smartbook-reader"; \
		if [ -f "$$READER_BIN" ]; then \
			$$READER_BIN; \
		elif [ -f "$(BUILD_DIR)/$$PLATFORM/bin/smartbook-reader" ]; then \
			$(BUILD_DIR)/$$PLATFORM/bin/smartbook-reader; \
		else \
			echo "Reader not built. Run: make $$PLATFORM"; \
			exit 1; \
		fi \
	elif [ "$(UNAME_S)" = "Darwin" ]; then \
		PLATFORM=macos-$(ARCH); \
		READER_BIN="$(BUILD_DIR)/$$PLATFORM/reader/smartbook-reader.app/Contents/MacOS/smartbook-reader"; \
		if [ -f "$$READER_BIN" ]; then \
			$$READER_BIN; \
		elif [ -f "$(BUILD_DIR)/$$PLATFORM/bin/smartbook-reader.app/Contents/MacOS/smartbook-reader" ]; then \
			$(BUILD_DIR)/$$PLATFORM/bin/smartbook-reader.app/Contents/MacOS/smartbook-reader; \
		else \
			echo "Reader not built. Run: make $$PLATFORM"; \
			exit 1; \
		fi \
	elif [ "$(UNAME_S)" = "MINGW" ] || [ "$(UNAME_S)" = "MSYS" ]; then \
		READER_BIN="$(BUILD_DIR)/windows-x86_64/reader/smartbook-reader.exe"; \
		if [ -f "$$READER_BIN" ]; then \
			$$READER_BIN; \
		elif [ -f "$(BUILD_DIR)/windows-x86_64/bin/smartbook-reader.exe" ]; then \
			$(BUILD_DIR)/windows-x86_64/bin/smartbook-reader.exe; \
		else \
			echo "Reader not built. Run: make windows-x86_64"; \
			exit 1; \
		fi \
	else \
		echo "Unknown operating system: $(UNAME_S)"; \
		exit 1; \
	fi

run-creator:
	@if [ "$(UNAME_S)" = "Linux" ]; then \
		PLATFORM=linux-$(DISTRO_ID)-$(ARCH); \
		CREATOR_BIN="$(BUILD_DIR)/$$PLATFORM/creator/smartbook-creator"; \
		if [ -f "$$CREATOR_BIN" ]; then \
			$$CREATOR_BIN; \
		elif [ -f "$(BUILD_DIR)/$$PLATFORM/bin/smartbook-creator" ]; then \
			$(BUILD_DIR)/$$PLATFORM/bin/smartbook-creator; \
		else \
			echo "Creator not built. Run: make $$PLATFORM"; \
			exit 1; \
		fi \
	elif [ "$(UNAME_S)" = "Darwin" ]; then \
		PLATFORM=macos-$(ARCH); \
		CREATOR_BIN="$(BUILD_DIR)/$$PLATFORM/creator/smartbook-creator.app/Contents/MacOS/smartbook-creator"; \
		if [ -f "$$CREATOR_BIN" ]; then \
			$$CREATOR_BIN; \
		elif [ -f "$(BUILD_DIR)/$$PLATFORM/bin/smartbook-creator.app/Contents/MacOS/smartbook-creator" ]; then \
			$(BUILD_DIR)/$$PLATFORM/bin/smartbook-creator.app/Contents/MacOS/smartbook-creator; \
		else \
			echo "Creator not built. Run: make $$PLATFORM"; \
			exit 1; \
		fi \
	elif [ "$(UNAME_S)" = "MINGW" ] || [ "$(UNAME_S)" = "MSYS" ]; then \
		CREATOR_BIN="$(BUILD_DIR)/windows-x86_64/creator/smartbook-creator.exe"; \
		if [ -f "$$CREATOR_BIN" ]; then \
			$$CREATOR_BIN; \
		elif [ -f "$(BUILD_DIR)/windows-x86_64/bin/smartbook-creator.exe" ]; then \
			$(BUILD_DIR)/windows-x86_64/bin/smartbook-creator.exe; \
		else \
			echo "Creator not built. Run: make windows-x86_64"; \
			exit 1; \
		fi \
	else \
		echo "Unknown operating system: $(UNAME_S)"; \
		exit 1; \
	fi

run-server:
	@echo "Server application not yet implemented (Phase 2)"
	@exit 1

# Install target (Unix/macOS only - Windows uses install-windows.ps1)
install:
	@echo "Installing Smartbook..."
	@if [ "$(UNAME_S)" = "Darwin" ] || [ "$(UNAME_S)" = "Linux" ]; then \
		PREFIX=$${PREFIX:-/usr/local}; \
		echo "Installing to $$PREFIX..."; \
		cd common && $(MAKE) install PREFIX=$$PREFIX; \
		cd ../reader && $(MAKE) install PREFIX=$$PREFIX; \
		cd ../creator && $(MAKE) install PREFIX=$$PREFIX; \
		echo "Installation complete!"; \
	else \
		echo "Error: Install not supported via Makefile on Windows."; \
		echo "Use: .\install-windows.ps1"; \
	fi

# All target - complete build
all: clean
	@echo "Starting complete build process..."
	@echo "Step 1: Building all platforms..."
	@$(MAKE) linux-debian-x86_64 || true
	@$(MAKE) linux-ubuntu-x86_64 || true
	@$(MAKE) linux-fedora-x86_64 || true
	@$(MAKE) linux-arch-x86_64 || true
	@if [ "$(UNAME_S)" = "Darwin" ]; then \
		$(MAKE) macos-arm64 || true; \
		$(MAKE) macos-x86_64 || true; \
	fi
	@echo "Step 2: Generating documentation..."
	@$(MAKE) docs || true
	@echo "Step 3: Building and running tests..."
	@$(MAKE) test || true
	@echo "Complete build finished."
