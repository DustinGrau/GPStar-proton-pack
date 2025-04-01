# Copilot Instructions

This document provides guidelines and instructions for using GitHub Copilot effectively in this project.

## Project Overview

This project is structured for development with PlatformIO, targeting ESP32-based boards. The key components of the project include:

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

## Commenting and Editing Styles

1. **Thought Process and Plan of Action**:
   - When making recommendations or edits, clearly lay out the thought process and the plan of action.
   - Provide a "before" and "after" comparison for edits, allowing for review and approval before changes are finalized.

2. **Commenting Code**:
   - Functions and methods should include comments explaining their purpose, inputs, outputs, and any side effects.
   - Objects and classes should be documented to describe their role in the system and their key attributes or methods.
   - For code involving complex math or algorithms, provide detailed comments explaining the logic and purpose of the calculations.

   Example:
   ````cpp
   // Function: calculateTrajectory
   // Purpose: Computes the trajectory of a projectile based on initial velocity and angle.
   // Inputs:
   //   - float velocity: The initial velocity of the projectile (m/s).
   //   - float angle: The launch angle of the projectile (degrees).
   // Outputs:
   //   - float: The computed trajectory distance (meters).
   float calculateTrajectory(float velocity, float angle) {
       // Convert angle to radians for trigonometric calculations
       float angleRadians = angle * (PI / 180.0);

       // Use the physics formula: distance = (velocity^2 * sin(2 * angle)) / gravity
       float distance = (pow(velocity, 2) * sin(2 * angleRadians)) / GRAVITY;

       return distance;
   }
