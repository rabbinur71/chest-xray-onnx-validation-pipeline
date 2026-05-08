#!/usr/bin/env python3

from pathlib import Path
import json
from datetime import datetime, timezone

import torch
import torch.nn as nn
import torchxrayvision as xrv


PROJECT_ROOT = Path(__file__).resolve().parents[1]
MODELS_DIR = PROJECT_ROOT / "models"
REPORTS_DIR = PROJECT_ROOT / "reports"

ONNX_PATH = MODELS_DIR / "chest_xray_model.onnx"
EXPORT_REPORT_PATH = REPORTS_DIR / "onnx_export_report.json"

MODEL_NAME = "densenet121-res224-all"


class XRayVisionSigmoidWrapper(nn.Module):
    """
    ONNX-safe TorchXRayVision wrapper.

    Exports:
        features2(x) -> classifier(features) -> sigmoid(logits)

    Intentionally excludes TorchXRayVision op_norm() because its boolean
    masked assignment path exports into an ONNX graph that fails at runtime.
    Threshold/status interpretation will be handled later by the C rule engine.
    """

    def __init__(self, base_model: xrv.models.DenseNet) -> None:
        super().__init__()
        self.base_model = base_model

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        features = self.base_model.features2(x)
        logits = self.base_model.classifier(features)
        probabilities = torch.sigmoid(logits)
        return probabilities


def utc_now_iso() -> str:
    return datetime.now(timezone.utc).isoformat()


def main() -> int:
    MODELS_DIR.mkdir(parents=True, exist_ok=True)
    REPORTS_DIR.mkdir(parents=True, exist_ok=True)

    torch.manual_seed(0)
    torch.set_num_threads(1)

    base_model = xrv.models.DenseNet(weights=MODEL_NAME)
    base_model.eval()

    export_model = XRayVisionSigmoidWrapper(base_model)
    export_model.eval()

    dummy_input = torch.zeros((1, 1, 224, 224), dtype=torch.float32)

    input_names = ["input_image"]
    output_names = ["pathology_probabilities"]

    torch.onnx.export(
        export_model,
        dummy_input,
        ONNX_PATH,
        export_params=True,
        opset_version=17,
        do_constant_folding=True,
        input_names=input_names,
        output_names=output_names,
        dynamic_axes=None
    )

    if not ONNX_PATH.exists() or ONNX_PATH.stat().st_size <= 0:
        raise RuntimeError(f"ONNX export failed or produced empty file: {ONNX_PATH}")

    report = {
        "document": "Major Phase 1 - Sub-Phase 1.6 ONNX Export Report",
        "status": "PASS",
        "timestamp_utc": utc_now_iso(),
        "model": {
            "library": "torchxrayvision",
            "base_class": "xrv.models.DenseNet",
            "export_wrapper": "XRayVisionSigmoidWrapper",
            "weights": MODEL_NAME,
            "pathology_count": len(base_model.pathologies),
            "pathologies": list(base_model.pathologies)
        },
        "onnx_export": {
            "onnx_path": str(ONNX_PATH),
            "opset_version": 17,
            "input_names": input_names,
            "output_names": output_names,
            "input_shape": [1, 1, 224, 224],
            "input_dtype": "float32",
            "output_shape_expected": [1, len(base_model.pathologies)],
            "output_semantics": "sigmoid probabilities without TorchXRayVision op_norm",
            "dynamic_axes": None,
            "constant_folding": True,
            "file_size_bytes": ONNX_PATH.stat().st_size
        },
        "engineering_decision": {
            "excluded_postprocessing": "TorchXRayVision op_norm",
            "reason": "op_norm uses boolean masked assignment that exported into runtime-invalid ONNX Reshape/GatherND/ScatterND graph",
            "future_handling": "Threshold and finding-status interpretation will be implemented in the C rule engine"
        }
    }

    EXPORT_REPORT_PATH.write_text(json.dumps(report, indent=2), encoding="utf-8")

    print("[PASS] ONNX export completed")
    print(f"[PASS] ONNX model: {ONNX_PATH}")
    print(f"[PASS] Export report: {EXPORT_REPORT_PATH}")
    print(f"[PASS] File size bytes: {ONNX_PATH.stat().st_size}")
    print("[PASS] Export semantics: sigmoid probabilities without op_norm")
    print(f"[PASS] Input shape: {[1, 1, 224, 224]}")
    print(f"[PASS] Expected output shape: {[1, len(base_model.pathologies)]}")
    print("[PASS] Output labels preserved:")
    for idx, label in enumerate(base_model.pathologies):
        print(f"  {idx:02d} -> {label}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
