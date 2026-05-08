#!/usr/bin/env python3
"""
Download and instantiate the approved TorchXRayVision DenseNet model.

This script intentionally performs only model acquisition and metadata capture.
Numerical inference validation belongs to Sub-Phase 1.4.
"""

from __future__ import annotations

import json
import os
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Dict

import torch
import torchxrayvision as xrv


PROJECT_ROOT = Path(__file__).resolve().parents[1]
CONFIG_PATH = PROJECT_ROOT / "config" / "model_config.json"


def load_config() -> Dict[str, Any]:
    if not CONFIG_PATH.exists():
        raise FileNotFoundError(f"Missing configuration file: {CONFIG_PATH}")
    return json.loads(CONFIG_PATH.read_text(encoding="utf-8"))


def configure_runtime(config: Dict[str, Any]) -> None:
    runtime = config["runtime"]
    if runtime.get("deterministic", True):
        torch.manual_seed(0)
        torch.use_deterministic_algorithms(True, warn_only=True)
    torch.set_num_threads(int(runtime.get("torch_num_threads", 1)))


def acquire_model(config: Dict[str, Any]) -> torch.nn.Module:
    primary = config["primary_model"]
    cache_dir = PROJECT_ROOT / primary["cache_dir"]
    cache_dir.mkdir(parents=True, exist_ok=True)

    # TorchXRayVision uses this location for cached pretrained weights.
    os.environ["TORCHXRAYVISION_CACHE"] = str(cache_dir)

    model = xrv.models.DenseNet(
        weights=primary["weights"],
        apply_sigmoid=bool(primary["apply_sigmoid"]),
    )
    model.eval()
    return model


def main() -> int:
    config = load_config()
    configure_runtime(config)

    model = acquire_model(config)
    primary = config["primary_model"]

    metadata_path = PROJECT_ROOT / config["artifacts"]["metadata_path"]
    report_path = PROJECT_ROOT / config["artifacts"]["model_download_report_path"]
    metadata_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.parent.mkdir(parents=True, exist_ok=True)

    targets = list(getattr(model, "targets", []))
    metadata = {
        "timestamp_utc": datetime.now(timezone.utc).isoformat(),
        "provider": primary["provider"],
        "architecture": primary["architecture"],
        "weights": primary["weights"],
        "input_size": primary["input_size"],
        "expected_channels": primary["expected_channels"],
        "tensor_layout": primary["tensor_layout"],
        "apply_sigmoid": primary["apply_sigmoid"],
        "targets": targets,
        "target_count": len(targets),
        "torch_version": torch.__version__,
        "torchxrayvision_version": getattr(xrv, "__version__", "unknown"),
    }

    metadata_path.write_text(json.dumps(metadata, indent=2), encoding="utf-8")

    dummy = torch.zeros(
        1,
        int(primary["expected_channels"]),
        int(primary["input_size"]),
        int(primary["input_size"]),
        dtype=torch.float32,
    )
    with torch.no_grad():
        output = model(dummy)

    report = {
        "timestamp_utc": datetime.now(timezone.utc).isoformat(),
        "status": "PASS",
        "metadata_path": str(metadata_path.relative_to(PROJECT_ROOT)),
        "output_shape": list(output.shape),
        "output_dtype": str(output.dtype),
        "finite_output": bool(torch.isfinite(output).all().item()),
        "target_count": len(targets),
    }
    report_path.write_text(json.dumps(report, indent=2), encoding="utf-8")

    print(f"[PASS] Model acquired: {primary['weights']}")
    print(f"[PASS] Metadata written: {metadata_path}")
    print(f"[PASS] Acquisition report written: {report_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
