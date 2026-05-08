#!/usr/bin/env python3

from pathlib import Path
from datetime import datetime, timezone
import json
import platform
import sys

import cv2
import numpy as np
import onnx
import onnxruntime as ort
import torch
import torchxrayvision as xrv


PROJECT_ROOT = Path(__file__).resolve().parents[1]
ONNX_MODEL_PATH = PROJECT_ROOT / "models" / "chest_xray_model.onnx"
IMAGE_REGISTRY_PATH = PROJECT_ROOT / "validation_dataset" / "metadata" / "image_registry.json"
REPORT_PATH = PROJECT_ROOT / "reports" / "onnx_numerical_validation_report.json"

MODEL_NAME = "densenet121-res224-all"
ABSOLUTE_TOLERANCE = 1e-4


def utc_now_iso() -> str:
    return datetime.now(timezone.utc).isoformat()


def load_registry() -> list[dict]:
    data = json.loads(IMAGE_REGISTRY_PATH.read_text(encoding="utf-8"))
    images = data.get("images", [])
    if not images:
        raise RuntimeError("Image registry contains no images")
    return images


def load_image_as_tensor(image_path: Path) -> torch.Tensor:
    image = cv2.imread(str(image_path), cv2.IMREAD_GRAYSCALE)
    if image is None:
        raise RuntimeError(f"Failed to read image: {image_path}")

    image = image.astype(np.float32)
    image = (image / 255.0) * 2048.0 - 1024.0
    image = cv2.resize(image, (224, 224), interpolation=cv2.INTER_AREA)

    tensor = torch.from_numpy(image).unsqueeze(0).unsqueeze(0).contiguous().float()
    return tensor


def validate_onnx_model() -> None:
    model = onnx.load(str(ONNX_MODEL_PATH))
    onnx.checker.check_model(model)


def create_onnx_session() -> ort.InferenceSession:
    providers = ["CPUExecutionProvider"]
    return ort.InferenceSession(str(ONNX_MODEL_PATH), providers=providers)


def main() -> int:
    if not ONNX_MODEL_PATH.exists():
        raise RuntimeError(f"ONNX model missing: {ONNX_MODEL_PATH}")

    validate_onnx_model()

    torch.manual_seed(0)
    torch.set_num_threads(1)

    base_model = xrv.models.DenseNet(weights=MODEL_NAME)
    base_model.eval()

    class XRayVisionSigmoidWrapper(torch.nn.Module):
        def __init__(self, model):
            super().__init__()
            self.model = model

        def forward(self, x):
            features = self.model.features2(x)
            logits = self.model.classifier(features)
            return torch.sigmoid(logits)

    pytorch_model = XRayVisionSigmoidWrapper(base_model)
    pytorch_model.eval()

    session = create_onnx_session()

    input_name = session.get_inputs()[0].name
    output_name = session.get_outputs()[0].name

    registry_images = load_registry()
    per_image_results = []

    global_max_abs_delta = 0.0
    failed_images = []

    with torch.no_grad():
        for entry in registry_images:
            image_id = entry["image_id"]
            image_path = PROJECT_ROOT / entry["relative_path"]

            tensor = load_image_as_tensor(image_path)

            pytorch_output = pytorch_model(tensor).detach().cpu().numpy()
            onnx_output = session.run([output_name], {input_name: tensor.detach().cpu().numpy()})[0]

            if pytorch_output.shape != onnx_output.shape:
                raise RuntimeError(
                    f"Shape mismatch for {image_id}: PyTorch={pytorch_output.shape}, ONNX={onnx_output.shape}"
                )

            abs_delta = np.abs(pytorch_output - onnx_output)
            max_abs_delta = float(abs_delta.max())
            mean_abs_delta = float(abs_delta.mean())

            image_pass = max_abs_delta <= ABSOLUTE_TOLERANCE
            if not image_pass:
                failed_images.append(image_id)

            global_max_abs_delta = max(global_max_abs_delta, max_abs_delta)

            per_label = []
            for idx, label in enumerate(base_model.pathologies):
                per_label.append(
                    {
                        "index": idx,
                        "label": label,
                        "pytorch_probability": float(pytorch_output[0, idx]),
                        "onnx_probability": float(onnx_output[0, idx]),
                        "absolute_delta": float(abs_delta[0, idx])
                    }
                )

            per_image_results.append(
                {
                    "image_id": image_id,
                    "image_path": str(image_path),
                    "status": "PASS" if image_pass else "FAIL",
                    "input_shape": list(tensor.shape),
                    "pytorch_output_shape": list(pytorch_output.shape),
                    "onnx_output_shape": list(onnx_output.shape),
                    "max_absolute_delta": max_abs_delta,
                    "mean_absolute_delta": mean_abs_delta,
                    "top_delta_labels": sorted(
                        per_label,
                        key=lambda item: item["absolute_delta"],
                        reverse=True
                    )[:5],
                    "all_labels": per_label
                }
            )

    overall_status = "PASS" if not failed_images else "FAIL"

    report = {
        "document": "Major Phase 1 - Sub-Phase 1.7 ONNX Numerical Validation",
        "status": overall_status,
        "timestamp_utc": utc_now_iso(),
        "environment": {
            "python": sys.version,
            "platform": platform.platform(),
            "torch": torch.__version__,
            "torchxrayvision": getattr(xrv, "__version__", "unknown"),
            "onnxruntime": ort.__version__,
            "execution_provider": "CPUExecutionProvider"
        },
        "model": {
            "pytorch_model": MODEL_NAME,
            "onnx_model_path": str(ONNX_MODEL_PATH),
            "pathology_count": len(base_model.pathologies),
            "pathologies": list(base_model.pathologies)
        },
        "validation_policy": {
            "absolute_tolerance": ABSOLUTE_TOLERANCE,
            "comparison": "PyTorch output probabilities vs ONNX Runtime output probabilities",
            "input_source": "validation_dataset/metadata/image_registry.json"
        },
        "summary": {
            "image_count": len(registry_images),
            "global_max_absolute_delta": global_max_abs_delta,
            "failed_images": failed_images
        },
        "per_image_results": per_image_results
    }

    REPORT_PATH.write_text(json.dumps(report, indent=2), encoding="utf-8")

    print(f"[{overall_status}] ONNX numerical validation complete")
    print(f"[INFO] Images validated: {len(registry_images)}")
    print(f"[INFO] Global max absolute delta: {global_max_abs_delta:.10f}")
    print(f"[INFO] Tolerance: {ABSOLUTE_TOLERANCE}")
    print(f"[INFO] Report: {REPORT_PATH}")

    for result in per_image_results:
        print(
            f"[{result['status']}] {result['image_id']} "
            f"max_delta={result['max_absolute_delta']:.10f} "
            f"mean_delta={result['mean_absolute_delta']:.10f}"
        )

    if failed_images:
        raise RuntimeError(f"ONNX numerical validation failed for: {failed_images}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
