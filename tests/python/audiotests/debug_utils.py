# Copyright © 2025 CCP ehf.

import os
import time

def debug_delay_if_enabled():
    """Add a delay if debug mode is enabled via environment variable or flag file, giving time to attach a debugger."""
    # Check environment variable first
    debug_delay = os.environ.get('DEBUG_DELAY', '0')
    
    # If no env var, check for flag file
    if debug_delay == '0':
        flag_file = os.path.join(os.path.dirname(__file__), '..', '..', '..', '.vscode', 'debug_delay_enabled.flag')
        if os.path.exists(flag_file):
            try:
                with open(flag_file, 'r') as f:
                    debug_delay = f.read().strip() or '5'
            except:
                debug_delay = '5'  # Default to 5 seconds
    
    try:
        delay_seconds = int(debug_delay)
        if delay_seconds > 0:
            print("DEBUG MODE: Waiting %d seconds for debugger attachment...\n" % delay_seconds)
            print("Attach debugger to exefile.exe now!\n")

            for i in range(delay_seconds, 0, -1):
                print("   %d...\n" % i)
                time.sleep(1)

            print("Continuing with test execution...\n")
    except ValueError:
        pass  
