#!/usr/bin/env python3
"""
Major Phase 1 - Sub-Phase 1.4
Baseline Model Inference Validation

Purpose:
- Load the approved TorchXRayVision DenseNet model.
- Execute deterministic baseline inference.
- Record input tensor contract.
- Record output label contract.
- Validate probability vector integrity.
- Generate formal JSON validation report.

This script uses a deterministic synthetic radiograph-like image only to validate
runtime behavior, tensor structure, label extraction, and inference stability.
Clinical validation with real X-ray images follows after this baseline passes.
"""

from __future__ import annotations

import json
import math
import platform
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

import numpy as np
import torch
import torchxrayvision as xrv


PROJECT_ROOT = Path(__file__).resolve().parents[1]
REPORTS_DIR = PROJECT_ROOT / "reports"
REPORT_PATH = REPORTS_DIR / "baseline_inference_validation_report.json"


def utc_now_iso() -> str:
    return datetime.now(timezone.utc).isoformat()


def build_synthetic_radiograph_tensor() -> torch.Tensor:
    """
    Build deterministic synthetic input in TorchXRayVision expected range.

    TorchXRayVision models commonly expect single-channel radiograph tensors
    with values approximately in [-1024, 1024].

    Shape:
        [1, 1, 224, 224]
    """
    height = 224
    width = 224

    y = np.linspace(-1.0, 1.0, height, dtype=np.float32)
    x = np.linspace(-1.0, 1.0, width, dtype=np.float32)
    grid_x, grid_y = np.meshgrid(x, y)

    radial = np.sqrt((grid_x * 0.85) ** 2 + (grid_y * 1.15) ** 2)
    image = 1.0 - np.clip(radial, 0.0, 1.0)

    # Convert to approximate TorchXRayVision intensity range.
    image = (image * 2048.0) - 1024.0
    image = image.astype(np.float32)

    tensor = torch.from_numpy(image).unsqueeze(0).unsqueeze(0)
    return tensor.contiguous()


def summarize_tensor(tensor: torch.Tensor) -> dict[str, Any]:
    detached = tensor.detach().cpu()
    return {
        "shape": list(detached.shape),
        "dtype": str(detached.dtype),
        "min": float(detached.min().item()),
        "max": float(detached.max().item()),
        "mean": float(detached.mean().item()),
        "std": float(detached.std().item()),
        "is_contiguous": bool(detached.is_contiguous()),
        "has_nan": bool(torch.isnan(detached).any().item()),
        "has_inf": bool(torch.isinf(detached).any().item()),
    }


def validate_probabilities(probabilities: torch.Tensor) -> None:
    if probabilities.ndim != 2:
        raise RuntimeError(f"Expected output tensor rank 2, got shape {list(probabilities.shape)}")

    if probabilities.shape[0] != 1:
        raise RuntimeError(f"Expected batch size 1, got shape {list(probabilities.shape)}")

    if torch.isnan(probabilities).any().item():
        raise RuntimeError("Model output contains NaN values")

    if torch.isinf(probabilities).any().item():
        raise RuntimeError("Model output contains infinite values")

    min_value = float(probabilities.min().item())
    max_value = float(probabilities.max().item())

    if min_value < 0.0 or max_value > 1.0:
        raise RuntimeError(
            f"Expected probabilities in [0, 1], got range [{min_value}, {max_value}]"
        )


def main() -> int:
    REPORTS_DIR.mkdir(parents=True, exist_ok=True)

    torch.manual_seed(0)
    np.random.seed(0)
    torch.set_num_threads(1)

    model_name = "densenet121-res224-all"
    model = xrv.models.DenseNet(weights=model_name)
    model.eval()

    input_tensor = build_synthetic_radiograph_tensor()

    with torch.no_grad():
        output_1 = model(input_tensor)
        output_2 = model(input_tensor)

    validate_probabilities(output_1)
    validate_probabilities(output_2)

    max_repeat_delta = float(torch.max(torch.abs(output_1 - output_2)).item())
    if max_repeat_delta > 1e-7:
        raise RuntimeError(f"Inference is not deterministic. Max delta: {max_repeat_delta}")

    labels = list(model.pathologies)
    output_values = output_1.detach().cpu().numpy()[0].astype(float)

    if len(labels) != int(output_1.shape[1]):
        raise RuntimeError(
            f"Label count mismatch. labels={len(labels)}, output_width={int(output_1.shape[1])}"
        )

    ranked_findings = sorted(
        [
            {
                "label": label,
                "probability": float(probability),
            }
            for label, probability in zip(labels, output_values)
        ],
        key=lambda item: item["probability"],
        reverse=True,
    )

    report = {
        "document": "Major Phase 1 - Sub-Phase 1.4 Baseline Model Inference Validation",
        "status": "PASS",
        "timestamp_utc": utc_now_iso(),
        "environment": {
            "python": sys.version,
            "platform": platform.platform(),
            "torch": torch.__version__,
            "torchxrayvision": getattr(xrv, "__version__", "unknown"),
            "device": "cpu",
            "torch_num_threads": torch.get_num_threads(),
        },
        "model": {
            "library": "torchxrayvision",
            "class": "xrv.models.DenseNet",
            "weights": model_name,
            "pathology_count": len(labels),
            "pathologies": labels,
        },
        "input_contract": {
            "semantic_type": "single chest radiograph image tensor",
            "tensor_layout": "NCHW",
            "batch_size": 1,
            "channels": 1,
            "height": 224,
            "width": 224,
            "value_range_expected": "approximately [-1024, 1024]",
            "dtype": "float32",
            "summary": summarize_tensor(input_tensor),
        },
        "output_contract": {
            "tensor_shape": list(output_1.shape),
            "dtype": str(output_1.dtype),
            "activation_interpretation": "multi-label probabilities",
            "probability_min": float(output_1.min().item()),
            "probability_max": float(output_1.max().item()),
            "probability_mean": float(output_1.mean().item()),
            "has_nan": bool(torch.isnan(output_1).any().item()),
            "has_inf": bool(torch.isinf(output_1).any().item()),
            "max_repeat_inference_delta": max_repeat_delta,
        },
        "top_ranked_findings": ranked_findings[:10],
        "all_findings": ranked_findings,
        "clinical_use_statement": (
            "This baseline validates model runtime behavior only. It is not clinical validation "
            "and must not be interpreted as diagnostic evidence."
        ),
    }

    REPORT_PATH.write_text(json.dumps(report, indent=2), encoding="utf-8")

    print(f"[PASS] Baseline inference validation complete")
    print(f"[PASS] Report written: {REPORT_PATH}")
    print(f"[PASS] Model: {model_name}")
    print(f"[PASS] Input tensor shape: {list(input_tensor.shape)}")
    print(f"[PASS] Output tensor shape: {list(output_1.shape)}")
    print(f"[PASS] Pathology count: {len(labels)}")
    print(f"[PASS] Max repeat inference delta: {max_repeat_delta:.10f}")
    print("[INFO] Top 5 baseline outputs:")
    for item in ranked_findings[:5]:
        print(f"  - {item['label']}: {item['probability']:.6f}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
