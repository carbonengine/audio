#!/bin/bash
# filepath: /Users/eric/Documents/code/carbon-audio/launch-vscode-with-branch.sh

if [ $# -eq 0 ]; then
    echo "Usage: launch-vscode-with-branch.sh \"path/to/branch\""
    echo "Example: launch-vscode-with-branch.sh \"/Users/username/p4/eve-frontier/branches/development/MAINLINE\""
    exit 1
fi

BRANCH_PATH="$1"
echo "Setting CCP_EVE_PERFORCE_BRANCH_PATH to: $BRANCH_PATH"

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Set the environment variable for this session
export CCP_EVE_PERFORCE_BRANCH_PATH="$BRANCH_PATH"

# Also set it permanently by adding to shell profile
# Detect which shell profile to use
if [ -n "$ZSH_VERSION" ]; then
    PROFILE_FILE="$HOME/.zshrc"
elif [ -n "$BASH_VERSION" ]; then
    PROFILE_FILE="$HOME/.bash_profile"
else
    PROFILE_FILE="$HOME/.profile"
fi

# Remove any existing CCP_EVE_PERFORCE_BRANCH_PATH lines and add the new one
grep -v "export CCP_EVE_PERFORCE_BRANCH_PATH=" "$PROFILE_FILE" > "$PROFILE_FILE.tmp" 2>/dev/null || touch "$PROFILE_FILE.tmp"
echo "export CCP_EVE_PERFORCE_BRANCH_PATH=\"$BRANCH_PATH\"" >> "$PROFILE_FILE.tmp"
mv "$PROFILE_FILE.tmp" "$PROFILE_FILE"

echo "Environment variable set successfully"
echo "Launching VS Code with the new environment..."
echo "Opening VS Code in directory: $SCRIPT_DIR"

# Launch VS Code with the script's directory and environment
# Use open command which works better from AppleScript
if command -v code &> /dev/null; then
    cd "$SCRIPT_DIR" && code .
else
    open -a "Visual Studio Code" "$SCRIPT_DIR"
fi

echo "VS Code launched with CCP_EVE_PERFORCE_BRANCH_PATH=$BRANCH_PATH"