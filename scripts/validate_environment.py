#!/usr/bin/env python3
"""
Validate the Phase 1.3 Python environment for the Offline Doctor-Assistive
X-ray Analysis System.

This script performs dependency import checks, records package/runtime metadata,
and writes a machine-readable validation report.
"""

from __future__ import annotations

import importlib
import json
import platform
import sys
from dataclasses import asdict, dataclass
from datetime import datetime, timezone
from pathlib import Path
from typing import Dict, List, Optional


PROJECT_ROOT = Path(__file__).resolve().parents[1]
CONFIG_PATH = PROJECT_ROOT / "config" / "model_config.json"


@dataclass(frozen=True)
class DependencyStatus:
    module: str
    import_name: str
    version: Optional[str]
    ok: bool
    error: Optional[str] = None


REQUIRED_MODULES: Dict[str, str] = {
    "torch": "torch",
    "torchvision": "torchvision",
    "torchxrayvision": "torchxrayvision",
    "onnx": "onnx",
    "onnxruntime": "onnxruntime",
    "numpy": "numpy",
    "Pillow": "PIL",
    "cv2": "cv2",
    "pandas": "pandas",
    "matplotlib": "matplotlib",
    "jsonschema": "jsonschema",
    "pytest": "pytest",
}


def load_config() -> dict:
    if not CONFIG_PATH.exists():
        raise FileNotFoundError(f"Missing configuration file: {CONFIG_PATH}")
    return json.loads(CONFIG_PATH.read_text(encoding="utf-8"))


def check_dependency(module_name: str, import_name: str) -> DependencyStatus:
    try:
        module = importlib.import_module(import_name)
        version = getattr(module, "__version__", None)
        return DependencyStatus(
            module=module_name,
            import_name=import_name,
            version=version,
            ok=True,
        )
    except Exception as exc:  # noqa: BLE001 - validation must capture all import failures
        return DependencyStatus(
            module=module_name,
            import_name=import_name,
            version=None,
            ok=False,
            error=str(exc),
        )


def main() -> int:
    config = load_config()
    report_path = PROJECT_ROOT / config["artifacts"]["environment_report_path"]
    report_path.parent.mkdir(parents=True, exist_ok=True)

    dependency_results: List[DependencyStatus] = [
        check_dependency(module, import_name)
        for module, import_name in REQUIRED_MODULES.items()
    ]

    failed = [item for item in dependency_results if not item.ok]

    report = {
        "timestamp_utc": datetime.now(timezone.utc).isoformat(),
        "project": config["project"],
        "phase": config["phase"],
        "subphase": config["subphase"],
        "python": {
            "executable": sys.executable,
            "version": sys.version,
            "platform": platform.platform(),
        },
        "dependencies": [asdict(item) for item in dependency_results],
        "status": "PASS" if not failed else "FAIL",
    }

    report_path.write_text(json.dumps(report, indent=2), encoding="utf-8")

    if failed:
        for item in failed:
            print(f"[FAIL] {item.module}: {item.error}")
        print(f"[FAIL] Environment validation report written: {report_path}")
        return 1

    print(f"[PASS] Environment validation report written: {report_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
