# Chest X-Ray ONNX Validation Pipeline

This repository contains a reproducible Python validation environment for a chest X-ray inference workflow built with TorchXRayVision, PyTorch, and ONNX Runtime.

The project is focused on validating a pretrained chest X-ray model before later integration into a larger offline diagnostic system. It covers:

- environment validation
- pretrained model download and verification
- baseline PyTorch inference checks
- inference over a small validation image registry
- ONNX export
- ONNX numerical equivalence diagnostics

## Purpose

The main goal of this repository is to verify that a chest X-ray model can be:

1. loaded reliably in Python
2. run consistently on a controlled validation dataset
3. exported to ONNX
4. compared between PyTorch and ONNX Runtime before downstream deployment work

This repository is a validation and diagnostics workspace. It is not a production medical product.

## Current Status

Implemented and available:

- Python setup automation with `setup.sh`
- environment validation script
- model download script
- baseline inference validation
- validation dataset registry-driven inference
- ONNX export flow
- ONNX numerical validation workflow
- ONNX failure diagnostics for the current export path

Current blocking issue:

- the current ONNX export of `torchxrayvision.models.DenseNet(weights="densenet121-res224-all")` includes TorchXRayVision postprocessing logic (`op_norm`) that does not execute correctly in ONNX Runtime for real validation inputs

What this means:

- PyTorch inference works
- ONNX export produces a file
- ONNX Runtime session creation succeeds
- numerical validation currently fails because the exported postprocessing branch is not runtime-safe for general inputs

## Repository Structure

```text
.
├── config/                          # model and project configuration
├── docs/                            # phase notes and implementation documentation
├── models/                          # exported model artifacts and metadata
├── reports/                         # generated validation reports
├── scripts/                         # validation, export, and diagnostic scripts
├── validation_dataset/              # small curated validation dataset and metadata
├── requirements.cpu.txt             # Python dependency list
├── setup.sh                         # local environment bootstrap
└── README.md
```

Important files:

- `scripts/validate_environment.py`
- `scripts/download_model.py`
- `scripts/baseline_inference_validation.py`
- `scripts/run_validation_image_inference.py`
- `scripts/export_model_to_onnx.py`
- `scripts/validate_onnx_numerical_equivalence.py`
- `validation_dataset/metadata/image_registry.json`
- `docs/phase_1_subphase_1_3_python_validation_environment.md`

## Setup

From the project root:

```bash
./setup.sh
source .venv/bin/activate
```

Installations and scripts in this repository are intended for a CPU-based Python workflow.

## Typical Workflow

### 1. Validate the environment

```bash
python scripts/validate_environment.py
```

### 2. Download the pretrained model

```bash
python scripts/download_model.py
```

### 3. Run baseline PyTorch inference validation

```bash
python scripts/baseline_inference_validation.py
```

### 4. Run inference on the validation dataset

```bash
python scripts/run_validation_image_inference.py
```

### 5. Export the model to ONNX

```bash
python scripts/export_model_to_onnx.py
```

### 6. Run ONNX numerical validation

```bash
python scripts/validate_onnx_numerical_equivalence.py
```

## Validation Dataset

The repository includes a small validation dataset registry under:

`validation_dataset/metadata/image_registry.json`

This registry defines the images used for controlled checks, including category labels and expected qualitative behavior for selected findings.

Current validation categories include examples such as:

- normal
- pneumonia
- cardiomegaly
- pneumothorax

The dataset here is intended for engineering validation only. It is too small to support any clinical or statistical claims.

## ONNX Numerical Validation Note

The current ONNX numerical validation phase is intentionally documented with the present failure state.

Diagnosis so far:

- the DenseNet feature extractor and classifier path export successfully
- the failure occurs in the exported TorchXRayVision `op_norm` postprocessing branch
- ONNX Runtime fails at a `Reshape` node generated from boolean-mask based normalization logic

Diagnostic report:

- `tmp_onnx_diagnostics/onnx_reshape_failure_diagnostic_report.md`

This is a known engineering limitation in the current state of the repository, not a silent or ignored defect.

## Limitations

This repository has several important limitations:

- It is a validation environment, not a production inference service.
- It does not provide a clinical decision support workflow.
- It does not perform report generation for clinical use.
- It uses a very small curated validation dataset.
- It currently depends on TorchXRayVision pretrained weights and behavior.
- The exported ONNX model path is not yet fully validated for runtime numerical equivalence.
- ONNX export currently requires a safer wrapper approach to avoid TorchXRayVision `op_norm` export issues.
- No claims should be made about diagnostic accuracy, safety, or deployment readiness based on this repository alone.

## Intended Next Engineering Step

The next corrective step is to re-export the model through a wrapper that excludes the problematic TorchXRayVision `op_norm` branch, then re-run ONNX numerical validation against the same wrapped PyTorch path.

## Documentation

Detailed implementation notes are available in:

- `docs/phase_1_subphase_1_3_python_validation_environment.md`

