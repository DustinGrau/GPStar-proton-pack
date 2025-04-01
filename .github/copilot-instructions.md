# Copilot Instructions

This document provides guidelines and instructions for using GitHub Copilot effectively in this project.

## Project Overview

This project is structured for development with PlatformIO, targeting ATMega and ESP32-based boards. The key components of the project include:

- **PlatformIO Configuration**: The `platformio.ini` file defines the build environment and dependencies.
- **Source Code**: The main application logic resides in `src/main.cpp`.
- **Headers**: Shared declarations and utilities are located in the `include/` directory.
- **Libraries**: External libraries are managed in the `libdeps/` directory.
- **Board Configurations**: Board-specific configurations are stored in the `boards/` directory.
- **Partition Tables**: Custom partition tables are defined in the `partitions/` directory.

## Coding Guidelines

1. **File Organization**:
   - Place all implementation code in `src/`.
   - Use the `include/` directory for header files that define shared constants, classes, and functions.

2. **Naming Conventions**:
   - Use `PascalCase` for class names (e.g., `Device`).
   - Use `camelCase` for variables and functions (e.g., `initializeWiFi`).
   - Use `UPPER_SNAKE_CASE` for constants (e.g., `MAX_CONNECTIONS`).

3. **PlatformIO-Specific Practices**:
   - Use the `platformio.ini` file to manage dependencies and build configurations.
   - Keep board-specific settings in the `boards/` directory.

4. **Code Style**:
   - Follow consistent indentation (e.g., 4 spaces per level).
   - Use comments to explain complex logic or hardware-specific code.
   - Avoid hardcoding values; use constants or configuration files instead.

## Development Patterns

1. **WiFi Configuration**:
   - Use the `ExtWiFi.h` header for WiFi-related functionality.
   - Centralize WiFi credentials in `Configuration.h`.

2. **Web Server**:
   - Use `Webhandler.h` to define routes and handle HTTP requests.
   - Serve static files (e.g., `Index.h`, `Style.h`) for the web interface.

3. **System Utilities**:
   - Use `System.h` for system-level utilities and initialization.

4. **Testing**:
   - Add unit tests in the `test/` directory.
   - Use PlatformIO's built-in testing framework to run tests.

## Using GitHub Copilot

To make the most of GitHub Copilot in this project:

1. **Context Awareness**:
   - Ensure that Copilot has access to relevant files by keeping them open in the editor.
   - Use meaningful function and variable names to guide Copilot's suggestions.

2. **Code Completion**:
   - Start typing a function or class name, and Copilot will suggest completions based on the project's existing patterns.

3. **Documentation**:
   - Write clear comments and docstrings to help Copilot generate accurate suggestions.

4. **Custom Functions**:
   - When creating new functions, follow the naming conventions and patterns used in existing files.

## Additional Notes

- Refer to the `platformio.ini` file for build configurations and dependencies.
- Use the `.vscode/` directory for editor-specific settings, such as debugging configurations.
- Keep the `lib/` directory organized and document any custom libraries in the `README` file.

By following these guidelines, you can ensure consistency and maintainability in your codebase while leveraging GitHub Copilot effectively.
