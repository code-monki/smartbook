# Smartbook Project - Top-Level Makefile
# Orchestrates builds across all components

.PHONY: help clean docs docs-html docs-html-pdf docs-html-pdf-chromium docs-html-pdf-weasyprint docs-pdf all test test-unit test-integration run install
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
PDF_DIR := $(DOCS_DIR)/pdf

# Default target
.DEFAULT_GOAL := help

help:
	@echo "Smartbook Project - Build System"
	@echo "================================="
	@echo ""
	@echo "Available targets:"
	@echo "  help              - Display this help message"
	@echo "  clean             - Remove all build artifacts"
	@echo "  docs              - Generate HTML and PDF documentation (default: Chromium)"
	@echo "  docs-html         - Generate HTML documentation only"
	@echo "  docs-html-pdf-chromium  - Convert HTML to PDF using Chromium/Chrome (recommended)"
	@echo "  docs-html-pdf-weasyprint - Convert HTML to PDF using WeasyPrint (alternative)"
	@echo "  docs-pdf          - Generate PDFs directly from AsciiDoc (may have limitations)"
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

docs: docs-html docs-html-pdf-chromium
	@echo ""
	@echo "Documentation generated:"
	@echo "  HTML files: $(DOCS_DIR)/html/"
	@echo "  PDF files:  $(PDF_DIR)/"
	@echo ""
	@echo "PDF generation methods available:"
	@echo "  make docs-html-pdf-chromium  # Chromium/Chrome headless (default, recommended)"
	@echo "  make docs-html-pdf-weasyprint # WeasyPrint (alternative)"
	@echo "  make docs-pdf                 # Asciidoctor PDF (may have list rendering issues)"

docs-mmd: docs-mmd-source docs-mmd-pdf

docs-mmd-source:
	@echo "Generating documentation as MultiMarkdown source files..."
	@mkdir -p $(DOCS_DIR)/mmd
	@which pandoc > /dev/null 2>&1 || (echo "Error: pandoc not found. Install with: brew install pandoc" && exit 1)
	@for doc in Documentation/*.adoc; do \
		if [ -f "$$doc" ]; then \
			doc_name=$$(basename "$$doc" .adoc); \
			echo "  Converting $$doc_name.adoc to MultiMarkdown..."; \
			pandoc --from=asciidoc --to=markdown_mmd "$$doc" -o "$(DOCS_DIR)/mmd/$$doc_name.mmd" 2>/dev/null || echo "Warning: Failed to convert $$doc"; \
		fi \
	done
	@echo "MultiMarkdown source files generated in $(DOCS_DIR)/mmd/"

docs-mmd-pdf:
	@echo "Generating PDFs from MultiMarkdown..."
	@mkdir -p $(PDF_DIR)
	@which pandoc > /dev/null 2>&1 || (echo "Error: pandoc not found. Install with: brew install pandoc" && exit 1)
	@for mmd in $(DOCS_DIR)/mmd/*.mmd; do \
		if [ -f "$$mmd" ]; then \
			doc_name=$$(basename "$$mmd" .mmd); \
			echo "  Converting $$doc_name.mmd to PDF..."; \
			pandoc --from=markdown_mmd --to=pdf "$$mmd" -o "$(PDF_DIR)/$$doc_name.pdf" 2>/dev/null || echo "Warning: Failed to convert $$mmd"; \
		fi \
	done
	@echo "PDFs generated from MultiMarkdown in $(PDF_DIR)/"
	@echo ""
	@echo "Note: MultiMarkdown preserves lists in table cells as HTML,"
	@echo "      which should render correctly in PDFs."

docs-html: docs-process-mermaid
	@echo "Generating documentation as HTML (recommended format)..."
	@mkdir -p $(DOCS_DIR)/html
	@mkdir -p $(DOCS_DIR)/html/images
	@which asciidoctor > /dev/null 2>&1 || (echo "Error: asciidoctor not found. Install with: gem install asciidoctor" && exit 1)
	@if [ -d "$(DOCS_DIR)/images" ] && [ -n "$$(ls -A $(DOCS_DIR)/images/*.svg 2>/dev/null)" ]; then \
		cp $(DOCS_DIR)/images/*.svg $(DOCS_DIR)/html/images/ 2>/dev/null || true; \
	fi
	@for doc in Documentation/*.adoc Documentation/*.adoc.processed; do \
		if [ -f "$$doc" ]; then \
			doc_basename=$$(basename "$$doc" .processed); \
			if [ "$$doc_basename" = "diagrams.adoc" ] && [ -f "Documentation/diagrams.adoc.processed" ]; then \
				echo "  Skipping $$doc_basename (using processed version with rendered diagrams instead)"; \
				continue; \
			fi; \
			echo "  Converting $$doc_basename to HTML..."; \
			asciidoctor -D $(DOCS_DIR)/html "$$doc" || echo "Warning: Failed to convert $$doc"; \
		fi; \
	done
	@if [ -f "Documentation/diagrams.adoc.processed" ]; then \
		rm -f Documentation/diagrams.adoc.processed; \
	fi
	@# Remove diagrams.html if diagrams.adoc.html exists (processed version is preferred)
	@if [ -f "$(DOCS_DIR)/html/diagrams.html" ] && [ -f "$(DOCS_DIR)/html/diagrams.adoc.html" ]; then \
		rm -f "$(DOCS_DIR)/html/diagrams.html"; \
		echo "  Removed diagrams.html (using diagrams.adoc.html with rendered diagrams instead)"; \
	fi
	@echo "HTML files generated in $(DOCS_DIR)/html/"
	@echo ""
	@echo "To create PDFs from HTML:"
	@echo "  make docs-html-pdf-chromium  # Use Chromium/Chrome headless (recommended)"
	@echo "  make docs-html-pdf-weasyprint # Use WeasyPrint (alternative)"
	@echo "  Or manually: Open HTML in browser → Print → Save as PDF"

docs-process-mermaid:
	@echo "Processing Mermaid diagrams..."
	@mkdir -p $(DOCS_DIR)/images
	@which mmdc > /dev/null 2>&1 || (echo "Warning: mermaid-cli (mmdc) not found. Mermaid diagrams will not be converted. Install with: npm install -g @mermaid-js/mermaid-cli" && exit 0)
	@which python3 > /dev/null 2>&1 || (echo "Warning: python3 not found. Mermaid diagrams will not be converted." && exit 0)
	@if [ -f "Documentation/diagrams.adoc" ]; then \
		echo "  Processing Mermaid diagrams in diagrams.adoc..."; \
		if [ -f "scripts/process-mermaid-diagrams.py" ]; then \
			python3 scripts/process-mermaid-diagrams.py "Documentation/diagrams.adoc" "$(DOCS_DIR)/images" || echo "  Warning: Mermaid processing script failed"; \
		else \
			echo "  Warning: Mermaid processing script not found at scripts/process-mermaid-diagrams.py"; \
		fi \
	fi
	@echo "Mermaid diagram processing complete."

docs-html-pdf-chromium: docs-html
	@echo "Converting HTML documentation to PDF using Chromium/Chrome headless..."
	@mkdir -p $(PDF_DIR)
	@CHROMIUM_BIN=""; \
	if command -v chromium > /dev/null 2>&1; then \
		CHROMIUM_BIN=chromium; \
	elif command -v chromium-browser > /dev/null 2>&1; then \
		CHROMIUM_BIN=chromium-browser; \
	elif command -v google-chrome > /dev/null 2>&1; then \
		CHROMIUM_BIN=google-chrome; \
	elif [ -f "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome" ]; then \
		CHROMIUM_BIN="/Applications/Google Chrome.app/Contents/MacOS/Google Chrome"; \
	elif [ -f "/Applications/Chromium.app/Contents/MacOS/Chromium" ]; then \
		CHROMIUM_BIN="/Applications/Chromium.app/Contents/MacOS/Chromium"; \
	fi; \
	if [ -z "$$CHROMIUM_BIN" ]; then \
		echo "Error: Chromium/Chrome not found."; \
		echo "Install Chromium or Chrome, or use: make docs-html-pdf-weasyprint"; \
		exit 1; \
	fi; \
	echo "Using: $$CHROMIUM_BIN"; \
	HTML_DIR=$$(cd $(DOCS_DIR)/html && pwd); \
	PDF_OUTPUT_DIR=$$(cd $(PDF_DIR) && pwd); \
	for html in $$HTML_DIR/*.html; do \
		if [ -f "$$html" ]; then \
			doc_name=$$(basename "$$html" .html); \
			if [ "$$doc_name" = "diagrams" ] && [ -f "$$HTML_DIR/diagrams.adoc.html" ]; then \
				echo "  Skipping $$doc_name.html (using diagrams.adoc.html with rendered diagrams instead)"; \
				continue; \
			fi; \
			echo "  Converting $$doc_name.html to PDF..."; \
			html_url="file://$$html"; \
			pdf_path="$$PDF_OUTPUT_DIR/$$doc_name.pdf"; \
			"$$CHROMIUM_BIN" --headless --disable-gpu --no-sandbox --disable-dev-shm-usage \
				--virtual-time-budget=2000 --run-all-compositor-stages-before-draw \
				--print-to-pdf="$$pdf_path" --print-to-pdf-no-header \
				"$$html_url" >/dev/null 2>&1; \
			if [ ! -f "$$pdf_path" ] || [ ! -s "$$pdf_path" ]; then \
				echo "Warning: Failed to convert $$doc_name.html (empty or missing PDF)"; \
				rm -f "$$pdf_path"; \
			fi; \
		fi; \
	done
	@echo "PDFs generated from HTML in $(PDF_DIR)/"

docs-html-pdf-weasyprint: docs-html
	@echo "Converting HTML documentation to PDF using WeasyPrint..."
	@mkdir -p $(PDF_DIR)
	@which weasyprint > /dev/null 2>&1 || (echo "Error: weasyprint not found. Install with: pip install weasyprint" && exit 1)
	@for html in $(DOCS_DIR)/html/*.html; do \
		if [ -f "$$html" ]; then \
			doc_name=$$(basename "$$html" .html); \
			if [ "$$doc_name" = "diagrams" ] && [ -f "$(DOCS_DIR)/html/diagrams.adoc.html" ]; then \
				echo "  Skipping $$doc_name.html (using diagrams.adoc.html with rendered diagrams instead)"; \
				continue; \
			fi; \
			echo "  Converting $$doc_name.html to PDF..."; \
			weasyprint "$$html" "$(PDF_DIR)/$$doc_name.pdf" || echo "Warning: Failed to convert $$doc_name.html"; \
		fi; \
	done
	@echo "PDFs generated from HTML in $(PDF_DIR)/"

docs-html-pdf: docs-html-pdf-chromium
	@echo "PDFs generated from HTML (using Chromium). Use 'make docs-html-pdf-weasyprint' for WeasyPrint alternative."

docs-latex:
	@echo "Generating documentation as LaTeX source files..."
	@mkdir -p $(DOCS_DIR)/latex
	@which pandoc > /dev/null 2>&1 || (echo "Error: pandoc not found. Install with: brew install pandoc" && exit 1)
	@for doc in Documentation/*.adoc; do \
		if [ -f "$$doc" ]; then \
			doc_name=$$(basename "$$doc" .adoc); \
			echo "  Converting $$doc_name.adoc to LaTeX..."; \
			pandoc --from=asciidoc --to=latex "$$doc" -o "$(DOCS_DIR)/latex/$$doc_name.tex" 2>/dev/null || echo "Warning: Failed to convert $$doc"; \
		fi \
	done
	@echo "LaTeX files generated in $(DOCS_DIR)/latex/"
	@echo "To compile LaTeX to PDF, install a LaTeX distribution (e.g., MacTeX) and run:"
	@echo "  cd $(DOCS_DIR)/latex && pdflatex <filename>.tex"

docs-docx:
	@echo "Generating documentation as DOCX (Word format)..."
	@mkdir -p $(DOCS_DIR)
	@which pandoc > /dev/null 2>&1 || (echo "Error: pandoc not found. Install with: brew install pandoc" && exit 1)
	@for doc in Documentation/*.adoc; do \
		if [ -f "$$doc" ]; then \
			doc_name=$$(basename "$$doc" .adoc); \
			echo "  Converting $$doc_name.adoc to DOCX..."; \
			pandoc --from=asciidoc --to=docx "$$doc" -o "$(DOCS_DIR)/$$doc_name.docx" 2>/dev/null || echo "Warning: Failed to convert $$doc"; \
		fi \
	done
	@echo "DOCX files generated in $(DOCS_DIR)/"

docs-pdf:
	@echo "Generating documentation as PDF..."
	@mkdir -p $(PDF_DIR)
	@which asciidoctor-pdf > /dev/null 2>&1 || (echo "Warning: asciidoctor-pdf not found. Install with: gem install asciidoctor-pdf" && echo "Skipping PDF generation..." && exit 0)
	@which mmdc > /dev/null 2>&1 || echo "Warning: mermaid-cli (mmdc) not found. Diagrams may not be converted."
	@for doc in Documentation/*.adoc; do \
		if [ -f "$$doc" ]; then \
			doc_name=$$(basename "$$doc" .adoc); \
			# Skip diagrams.adoc - it will be generated from HTML with rendered diagrams \
			if [ "$$doc_name" != "diagrams" ]; then \
				echo "  Processing $$doc_name to PDF..."; \
				asciidoctor-pdf -a pdf-theme=Documentation/pdf-theme.yml -D $(PDF_DIR) "$$doc" || echo "Warning: Failed to process $$doc"; \
			else \
				echo "  Skipping $$doc_name.adoc (use docs-html-pdf-chromium for rendered diagrams)"; \
			fi \
		fi \
	done
	@echo "PDF files generated in $(PDF_DIR)/"
	@echo "Note: PDFs may have list rendering limitations. Use docs-html-pdf-chromium for best results with diagrams."

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
