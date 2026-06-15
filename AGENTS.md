# AGENTS.md

This file provides guidance to AI agents when working with code in this repository.

## Project Overview

ns-3 is a discrete-event network simulator for Internet systems, written in C++ with Python bindings. The project uses CMake for building but provides a custom `ns3` wrapper script for easier command-line usage.

## Build System

### Essential Commands

**Configuration:**

```bash
./ns3 configure --enable-examples --enable-tests  # Basic setup
./ns3 configure --help                            # Show all options
```

**Building:**

```bash
./ns3 build                                       # Build entire project
./ns3 build [target]                             # Build specific target
```

**Running:**

```bash
./ns3 run "simple-global-routing"                  # Run example program
./ns3 run "program [args]"                       # Run with arguments
```

**Testing:**

```bash
./test.py                                        # Run all tests
./test.py -s module-name                         # Run specific module tests, where `module-name`
                                                 # is the name of the module to be tested.
./test.py -v                                     # Verbose output
```

**Utilities:**

```bash
./ns3 clean                                      # Clean build artifacts
./ns3 show targets                               # List available targets
```

## Architecture

### Core Structure

- **`src/`** - 49 official modules (core, network, internet, wifi, lte, etc.)
- **`contrib/`** - Third-party modules
- **`examples/`** - Example programs organized by topic
- **`scratch/`** - User simulation scripts
- **`utils/`** - Development utilities

### Module Organization

Each module follows this structure:

```text
src/module-name/
├── CMakeLists.txt      # Build configuration
├── model/              # Core implementation (.cc/.h)
├── helper/             # Helper classes for easier usage
├── test/               # Unit tests
├── examples/           # Usage examples
└── doc/                # Documentation
```

### Key Design Patterns

- **Attribute System**: Configuration through TypeId attributes
- **Callback System**: Event-driven programming with Callback<> templates
- **Smart Pointers**: Extensive use of Ptr<> for memory management
- **Logging**: Hierarchical logging system with NS_LOG_*
- **Tracing**: Packet and event tracing for analysis

## Development Workflow

### Creating New Modules

```bash
./utils/create-module.py module-name    # Generate module template
```

### Code Style

The full coding style guide is documented in `doc/contributing/source/coding-style.rst`.

- **Clang-format**: Formatting rules and current C++ standard alignment are in .clang-format
- **No Unicode symbols**: Do not use Unicode mathematical symbols (e.g., ≤, ≥, ×, ÷, ∑, π) or arrows (e.g., →, ←, ⇒, ↑) in comments or Doxygen documentation; use ASCII equivalents instead (e.g., `<=`, `>=`, `*`, `/`, `->`, `=>`)
- **Comments**: Comment judiciously. Add an inline comment only when code involves nuance or non-obvious handling - not for every statement, and not to explain straightforward operation. Avoid writing comments that narrate the change you are making (unless requested by the human reviewer) or describe what the previous/replaced code did. Full guidance is in `doc/contributing/source/coding-style.rst` (Comments section).

### Sphinx/reStructuredText Documentation

Full guidelines for writing model documentation in Sphinx reStructuredText are in `doc/contributing/source/models.rst`. An empty outline with the required section structure and elements for a new module can be found in `utils/create-module.py`. Note that existing modules are not yet universally conformant to this structure; when adding Sphinx documentation to an existing module, find an appropriate insertion point within the existing structure rather than restructuring the whole document.

### Doxygen Documentation

Full conventions are in `doc/contributing/source/coding-style.rst` (Comments section). Key rules:

- **Coverage**: All classes, methods, and member variables must have Doxygen comments. Exceptions: methods inherited from a parent class (docs are copied automatically) and default constructors/destructors.
- **Comment style**: Use C-style Javadoc blocks (`/** ... */`). Use `///< brief description` for inline member variable documentation.
- **Tag delimiter**: Use `@` not `\` for all tags (e.g., `@param`, `@return`, `@brief`). clang-format recognizes `@` as a Doxygen tag delimiter and formats accordingly.
- **Parameters and return values**: All must be documented with `@param` and `@return`.
- **Cross-references**: Use `@see` for cross-referencing other classes or methods.
- **Internal comments**: Use `@internal` / `@endinternal` for documentation not intended for public Doxygen output.
- **Grouping**: Use `@defgroup` and `@ingroup` to bind logically related classes within a module. Test classes should form an ancillary group with `@ingroup tests`.

Example:

```cpp
/**
 * Brief description of the class.
 */
class MyClass
{
  public:
    /**
     * Constructor.
     *
     * @param n Number of elements.
     */
    MyClass(int n);

    /**
     * Do something useful.
     *
     * @param x Input value.
     * @return Result of the operation.
     */
    int DoSomething(int x);

  private:
    int m_count; ///< Number of elements
};
```

### Testing Requirements

- All code must build with examples and tests enabled
- Tests must pass before commits
- Use `./test.py -s module-name` for module-specific testing

### Commit Guidelines

- Present tense, imperative mood ("Add feature" not "Added feature")
- Reference modules: "core, network: Add new feature"
  - if list of edited modules spans more than two, suppress listing
- 72 character limit for first line
- Use "(fixes #issue)" for bug fixes, after the list of modules but before
  the summary of the commit, where `#issue` is the number of the issue.
  If partially fixing an issue, use "(partially fixes #issue)".
- Avoid adding a commit message body to explain the commit; allow a
  human reviewer to add one if he or she deems necessary.

## Common Development Tasks

### Running Single Tests

```bash
./test.py -s core                       # Run core module tests
./test.py -e simple-global-routing      # Run specific example
```

Tests can be run with ns-3 logging enabled via the `NS_LOG` environment variable:

```bash
NS_LOG="BulkSendApplication" ./ns3 run 'test-runner --suite=applications-bulk-send'
```

Log level can be tuned to reduce noise. Note that filtering to a specific log level deselects prefix information, so `prefix_all` must be added to recover it:

```bash
NS_LOG="BulkSendApplication=level_info|prefix_all" ./ns3 run 'test-runner --suite=applications-bulk-send'
```

### Debugging

```bash
./ns3 run "program --help"              # Show program options
./ns3 run program --command-template "gdb --args %s"  # Run with gdb
```

### Building Specific Modules

```bash
./ns3 build core                        # Build only core module
./ns3 build wifi-simple-adhoc           # Build specific example
```

## Key Files

- **`.ns3rc`** - Build configuration settings
- **`CMakeLists.txt`** - Build system configuration
- **`test.py`** - Main test runner
- **`utils/create-module.py`** - Module generator
- **`.clang-format`** - Code formatting rules

## Module Dependencies

When working with modules, understand the dependency hierarchy:

- **core** - Base classes, logging, attributes
- **network** - Packets, nodes, devices
- **internet** - IP, TCP, UDP protocols
- **applications** - Application layer protocols
- **wifi/lte/etc.** - Technology-specific implementations

## Python Bindings

Python bindings are available but may not cover all C++ APIs. Use:

```python
from ns import ns
help(ns.ClassName)  # Get class documentation
```

## Documentation

Rendered (current release):

- **Manual**: <https://www.nsnam.org/docs/manual/html/>
- **API Reference**: <https://www.nsnam.org/docs/doxygen/>
- **Model Library**: <https://www.nsnam.org/docs/models/html/>

Source (in-tree, for editing):

- **Manual**: `doc/manual/source/`
- **Model docs**: `doc/models/source/`
- **Contributing guide**: `doc/contributing/source/`
