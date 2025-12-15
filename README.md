# SmartBook

A next-generation digital publishing platform that overcomes the limitations of standard ePub format by providing rich interactivity, persistent user data, and seamless integration of functional mini-applications and fillable forms.

## Overview

SmartBook is a cross-platform application suite consisting of:

- **Reader Application**: For consuming SmartBook cartridges with advanced features like multi-window support, embedded applications, form persistence, and comprehensive security model
- **Creator Tool**: For authoring SmartBook documents with WYSIWYG editing, form builder, embedded application support, and cartridge packaging

## Key Features

### Reader Application
- Multi-window support for simultaneous cartridge viewing
- Embedded JavaScript application execution with security controls
- Form data persistence within cartridge files
- Three-tier security model (CA-signed, self-signed, unsigned)
- Persistent trust management with tampering detection
- Fast library browsing with manifest-based metadata caching
- Dual view modes: Bookshelf (visual) and List (tabular)
- Full-text search capabilities
- Theming support (Light, Dark, Sepia, custom)

### Creator Tool
- WYSIWYG HTML content editing using Qt WebEngineView
- Rich text editing with direct HTML insertion
- Page and chapter management
- Visual form builder with JSON schema validation
- Embedded application definition and management
- CSS stylesheet editing
- JavaScript code management
- Cartridge signing at multiple trust levels
- Import from Markdown, DOCX, and HTML
- Version history and snapshots
- Content templates
- Auto-save and recovery
- Draft and publishing workflow
- Theming support matching Reader

## Technology Stack

- **Framework**: Qt (C++)
- **UI**: Qt Widgets with Qt Fusion Style
- **Database**: SQLite (single-file cartridge format)
- **Rendering**: Qt WebEngine (Chromium-based)
- **Documentation**: AsciiDoc

## Project Structure

```
SmartBook/
├── Documentation/          # Project documentation
│   ├── srs.adoc          # Software Requirements Specification
│   ├── ddd.adoc          # Detailed Design Document
│   ├── hld.adoc          # High-Level Design
│   ├── Product-Concept.adoc
│   ├── test-plan.adoc
│   ├── wireframes/       # UI wireframes
│   └── dev-journal/      # Development decision log
└── README.md
```

## Documentation

Comprehensive documentation is available in the `Documentation/` directory:

- **SRS** (`srs.adoc`): Complete requirements specification for both Reader and Creator Tool
- **DDD** (`ddd.adoc`): Detailed design including database schemas, algorithms, and component specifications
- **HLD** (`hld.adoc`): High-level architecture and component design
- **Product Concept** (`Product-Concept.adoc`): Project vision and core concepts
- **Test Plan** (`test-plan.adoc`): Test cases and acceptance criteria
- **Wireframes**: Visual design references for UI components

## SmartBook Format

SmartBook uses a single-file SQLite database container (`.sqlite` file) that includes:

- **Metadata**: Title, author, publication year, version, tags, cover image
- **Content Pages**: HTML content with chapter organization
- **Form Definitions**: JSON schema-based form definitions
- **User Data**: Persistent form data stored within cartridge
- **Embedded Applications**: JavaScript applications with manifest definitions
- **Security**: Digital signatures, content hashing, and trust management

## Security Model

SmartBook implements a three-tier security model:

1. **Level 1 (Commercial Trust)**: CA-signed certificates - applications whitelisted by default
2. **Level 2 (Self-Signed Trust)**: Self-signed certificates - requires user consent
3. **Level 3 (No Signature)**: Unsigned cartridges - requires user consent with persistent warning

All cartridges support persistent trust management with tampering detection via content hash verification.

## Development Status

**Phase 1**: Single-user application development (Current)
- Reader Application: Requirements and design complete
- Creator Tool: Requirements and design complete
- Implementation: Pending

**Phase 2**: Multi-user system (Planned)
- Centralized library server
- Role-Based Access Control (RBAC)
- Remote library synchronization

## Building

*Note: Build instructions will be added once source code is available.*

The project will use:
- Qt 6.x or later
- CMake or qmake build system
- C++17 or later

## Contributing

This project is currently in the design and specification phase. Contributions to documentation, requirements, and design are welcome.

## License

*License to be determined*

## Contact

*Contact information to be added*

---

**Last Updated**: December 14, 2025

