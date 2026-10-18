"""Run a Windows PE target through WOW64 for CMake try_run probes."""
from __future__ import annotations

import subprocess
import sys

if len(sys.argv) < 2:
    raise SystemExit("expected a target executable")
result = subprocess.run(sys.argv[1:])
raise SystemExit(result.returncode)
