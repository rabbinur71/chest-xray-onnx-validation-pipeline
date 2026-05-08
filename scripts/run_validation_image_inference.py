#!/usr/bin/env python3

from pathlib import Path
import json

import cv2
import numpy as np
import torch
import torchxrayvision as xrv


PROJECT_ROOT = Path(__file__).resolve().parents[1]
IMAGE_REGISTRY_PATH = PROJECT_ROOT / "validation_dataset" / "metadata" / "image_registry.json"
REPORTS_DIR = PROJECT_ROOT / "validation_dataset" / "reports"


def load_image(path: Path) -> np.ndarray:
    image = cv2.imread(str(path), cv2.IMREAD_GRAYSCALE)

    if image is None:
        raise RuntimeError(f"Failed to load image: {path}")

    image = image.astype(np.float32)

    # Convert from [0,255] -> approximately [-1024,1024]
    image = (image / 255.0) * 2048.0 - 1024.0

    return image


def preprocess(image: np.ndarray) -> torch.Tensor:
    resized = cv2.resize(image, (224, 224), interpolation=cv2.INTER_AREA)

    tensor = torch.from_numpy(resized)
    tensor = tensor.unsqueeze(0).unsqueeze(0)
    tensor = tensor.contiguous().float()

    return tensor


def load_image_registry(path: Path) -> list[dict]:
    registry = json.loads(path.read_text(encoding="utf-8"))
    images = registry.get("images")

    if not isinstance(images, list) or not images:
        raise RuntimeError(f"Image registry has no images: {path}")

    return images


def build_findings(model: xrv.models.DenseNet, output: torch.Tensor) -> list[dict]:
    probabilities = output.detach().cpu().numpy()[0]

    return sorted(
        [
            {
                "label": label,
                "probability": float(probability),
            }
            for label, probability in zip(model.pathologies, probabilities)
        ],
        key=lambda item: item["probability"],
        reverse=True,
    )


def run_inference_for_entry(model: xrv.models.DenseNet, entry: dict) -> dict:
    image_id = entry["image_id"]
    image_path = PROJECT_ROOT / entry["relative_path"]
    report_path = REPORTS_DIR / f"{image_id}_inference_report.json"

    image = load_image(image_path)
    tensor = preprocess(image)

    with torch.no_grad():
        output = model(tensor)

    findings = build_findings(model, output)

    report = {
        "image_id": image_id,
        "category": entry.get("category"),
        "image": str(image_path),
        "input_tensor_shape": list(tensor.shape),
        "output_tensor_shape": list(output.shape),
        "top_findings": findings[:10],
        "all_findings": findings,
    }

    report_path.write_text(json.dumps(report, indent=2), encoding="utf-8")

    return {
        "image_id": image_id,
        "image_path": image_path,
        "report_path": report_path,
        "tensor_shape": list(tensor.shape),
        "output_shape": list(output.shape),
        "top_findings": findings[:10],
    }


def main() -> int:
    registry_entries = load_image_registry(IMAGE_REGISTRY_PATH)

    model = xrv.models.DenseNet(weights="densenet121-res224-all")
    model.eval()

    print(f"[INFO] Loaded image registry: {IMAGE_REGISTRY_PATH}")
    print(f"[INFO] Images scheduled for inference: {len(registry_entries)}")

    for entry in registry_entries:
        result = run_inference_for_entry(model, entry)

        print("[PASS] Validation image inference completed")
        print(f"[PASS] Image ID: {result['image_id']}")
        print(f"[PASS] Image: {result['image_path']}")
        print(f"[PASS] Report: {result['report_path']}")
        print(f"[PASS] Input tensor shape: {result['tensor_shape']}")
        print(f"[PASS] Output tensor shape: {result['output_shape']}")
        print("[INFO] Top findings:")

        for item in result["top_findings"]:
            print(f"  - {item['label']}: {item['probability']:.6f}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
