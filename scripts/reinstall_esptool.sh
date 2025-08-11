#!/bin/bash

set -e

PYTHON_BIN_PATH="$HOME/Library/Python/3.12/bin"
ZSHRC_FILE="$HOME/.zshrc"

echo "Uninstalling existing esptool versions..."

# Uninstall esptool via pip (user install)
python3 -m pip uninstall -y esptool || echo "No esptool found to uninstall with pip."

# Uninstall esptool via pipx (if installed)
if command -v pipx >/dev/null 2>&1; then
    pipx uninstall esptool || echo "No esptool found to uninstall with pipx."
else
    echo "pipx not found, skipping pipx uninstall."
fi

echo "Installing latest esptool (user install)..."
python3 -m pip install --user --upgrade esptool

echo "Checking PATH setup in $ZSHRC_FILE..."

if grep -Fq "export PATH=\"$PYTHON_BIN_PATH:\$PATH\"" "$ZSHRC_FILE"; then
    echo "PATH already configured in $ZSHRC_FILE."
else
    echo "Adding Python user bin directory to PATH in $ZSHRC_FILE..."
    echo -e "\n# Added by esptool reinstall script\nexport PATH=\"$PYTHON_BIN_PATH:\$PATH\"" >> "$ZSHRC_FILE"
    echo "Added. Run 'source $ZSHRC_FILE' or restart your terminal to apply changes."
fi

echo
echo "Installation complete. Verify by running:"
echo "  which esptool"
echo "  esptool version"
