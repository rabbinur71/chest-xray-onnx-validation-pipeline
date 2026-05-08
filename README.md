# Offline AI-Assisted Chest X-Ray Analysis System
## Phase 1 — AI Model Acquisition & Validation

A reproducible offline chest X-ray AI validation and ONNX deployment pipeline designed as the technical foundation for a future native C-based medical inference engine.

---

# Project Overview

This repository contains the completed implementation and validation work for:

```text
Major Phase 1 — AI Model Acquisition & Validation
```

The project establishes a deterministic and deployment-ready inference pipeline for chest X-ray analysis using:
- TorchXRayVision
- PyTorch
- ONNX
- ONNX Runtime

The validated ONNX model produced in this phase is intended for future integration into:
- a native C inference engine,
- a GTK desktop application,
- and an offline doctor-assistive workflow.

This repository is focused on:
- model validation,
- preprocessing determinism,
- runtime stability,
- ONNX deployment,
- and numerical verification.

---

# Current Status

## Major Phase 1 Status

```text
COMPLETE
```

Completed successfully:
- clinical and technical scope definition
- pretrained model evaluation and selection
- Python validation environment
- deterministic preprocessing pipeline
- controlled validation dataset
- baseline inference validation
- ONNX export pipeline
- ONNX numerical equivalence validation
- runtime contract documentation
- technical handoff preparation

---

# Key Technical Achievements

## 1. Validated Chest X-Ray AI Model

Selected model:

```text
TorchXRayVision DenseNet121
weights = densenet121-res224-all
```

The model supports multi-label chest radiograph analysis including:
- Pneumonia
- Cardiomegaly
- Pneumothorax
- Effusion
- Atelectasis
- Consolidation
- Edema
- Lung Opacity
- and additional thoracic findings.

---

## 2. Deterministic Inference Pipeline

Validated runtime flow:

```text
Chest X-Ray
    ↓
Preprocessing
    ↓
Tensor Conversion
    ↓
PyTorch / ONNX Runtime
    ↓
Probability Output
```

The following contracts were formally frozen:
- input image size
- normalization behavior
- tensor shape
- pathology ordering
- output semantics

These contracts will later be reproduced in the native C runtime.

---

## 3. Successful ONNX Deployment

Final validated deployment artifact:

```text
models/chest_xray_model.onnx
```

The ONNX model:
- loads successfully,
- executes successfully,
- and passes numerical equivalence validation.

---

## 4. ONNX Numerical Equivalence Validation

Validation objective:

```text
PyTorch output ≈ ONNX Runtime output
```

Validation results:

| Metric | Result |
|---|---|
| Images validated | 4 |
| Validation status | PASS |
| Failed images | 0 |
| Tolerance | `1e-4` |
| Global max delta | `1.788e-07` |

The ONNX model is therefore considered:
- numerically stable,
- runtime-safe,
- and deployment-ready for future C integration.

---

# Repository Structure

```text
.
├── config/
│   └── model_config.json
│
├── docs/
│
├── models/
│   ├── chest_xray_model.onnx
│   ├── model_metadata.json
│   └── failed_exports/
│
├── reports/
│   ├── environment_validation_report.json
│   ├── model_download_report.json
│   ├── baseline_inference_validation_report.json
│   ├── onnx_export_report.json
│   └── onnx_numerical_validation_report.json
│
├── scripts/
│   ├── validate_environment.py
│   ├── download_model.py
│   ├── baseline_inference_validation.py
│   ├── run_validation_image_inference.py
│   ├── export_model_to_onnx.py
│   └── validate_onnx_numerical_equivalence.py
│
├── validation_dataset/
│   ├── normal/
│   ├── pneumonia/
│   ├── cardiomegaly/
│   ├── pneumothorax/
│   ├── metadata/
│   └── reports/
│
├── requirements.cpu.txt
├── setup.sh
└── README.md
```

---

# Runtime Contracts

## Input Tensor Contract

| Property | Value |
|---|---|
| Layout | NCHW |
| Shape | `[1,1,224,224]` |
| Type | `float32` |
| Channels | 1 |
| Semantic Type | chest radiograph |

---

## Preprocessing Contract

Normalization formula:

```python
image = (image / 255.0) * 2048.0 - 1024.0
```

Final tensor:

```text
[1,1,224,224]
float32
```

---

## Output Tensor Contract

| Property | Value |
|---|---|
| Shape | `[1,18]` |
| Type | `float32` |
| Semantics | sigmoid probabilities |

---

# Official Output Label Mapping

| Index | Label |
|---|---|
| 00 | Atelectasis |
| 01 | Consolidation |
| 02 | Infiltration |
| 03 | Pneumothorax |
| 04 | Edema |
| 05 | Emphysema |
| 06 | Fibrosis |
| 07 | Effusion |
| 08 | Pneumonia |
| 09 | Pleural_Thickening |
| 10 | Cardiomegaly |
| 11 | Nodule |
| 12 | Mass |
| 13 | Hernia |
| 14 | Lung Lesion |
| 15 | Fracture |
| 16 | Lung Opacity |
| 17 | Enlarged Cardiomediastinum |

---

# Setup

## Linux Environment

From repository root:

```bash
chmod +x setup.sh
./setup.sh
source .venv/bin/activate
```

---

# Validation Workflow

## 1. Validate Environment

```bash
python scripts/validate_environment.py
```

---

## 2. Download Model

```bash
python scripts/download_model.py
```

---

## 3. Baseline Inference Validation

```bash
python scripts/baseline_inference_validation.py
```

---

## 4. Run Validation Dataset Inference

```bash
python scripts/run_validation_image_inference.py
```

---

## 5. Export ONNX Model

```bash
python scripts/export_model_to_onnx.py
```

---

## 6. Validate ONNX Numerical Equivalence

```bash
python scripts/validate_onnx_numerical_equivalence.py
```

---

# Controlled Validation Dataset

Validation categories currently include:
- normal
- pneumonia
- cardiomegaly
- pneumothorax

Purpose:
- engineering validation
- preprocessing regression testing
- ONNX verification
- future C runtime comparison

This dataset is NOT intended for:
- clinical claims,
- statistical conclusions,
- or medical performance assertions.

---

# Important Engineering Decision

TorchXRayVision `op_norm()` postprocessing was intentionally excluded from the final ONNX export because:
- boolean masked assignment exported into unstable ONNX graph behavior,
- runtime reshape failures occurred under ONNX Runtime.

Final export architecture:

```text
features2(x)
    ↓
classifier(features)
    ↓
sigmoid(logits)
```

Clinical interpretation logic will later be implemented inside:
- the native C rule engine,
- not inside the ONNX model itself.

This improves:
- maintainability,
- explainability,
- auditability,
- deterministic runtime behavior,
- and future regulatory traceability.

---

# Current Limitations

Current repository scope:

| Area | Status |
|---|---|
| Chest X-ray only | YES |
| PNG/JPG only | YES |
| Offline only | YES |
| CPU inference only | YES |
| Small validation dataset | YES |
| Doctor-assistive only | YES |

Not yet implemented:
- DICOM ingestion
- GUI application
- C inference engine
- SQLite persistence
- PDF export
- Grad-CAM visualization
- PACS integration
- clinical validation studies

---

# Intended Next Phase

Next planned milestone:

```text
Major Phase 2 — Core C Engine Development
```

Planned deliverables:
- native C preprocessing engine
- ONNX Runtime C integration
- rule engine
- JSON result generation
- SQLite persistence
- command-line prototype

---

# Safety Statement

This repository is:
- an engineering validation platform,
- not a clinically approved diagnostic device.

The software is intended as:

```text
Doctor-assistive clinical decision support software
```

It must not be used as:
- autonomous diagnosis,
- final clinical authority,
- or a replacement for physician review.

Expected future wording:

```text
Finding suspicious for pneumonia.
Doctor review required.
```

NOT:

```text
Patient has pneumonia.
```

---

# Long-Term Vision

With:
- curated large-scale datasets,
- radiologist-reviewed annotations,
- formal training/fine-tuning pipelines,
- independent validation,
- and regulatory-quality engineering controls,

this architecture can evolve into:
- deployable offline medical imaging software,
- low-resource diagnostic assistance systems,
- and cross-platform radiology support applications.

---

# Final Technical Outcome

Major Phase 1 successfully demonstrated that:

```text
A pretrained chest X-ray AI model can be:
validated,
stabilized,
exported,
numerically verified,
and prepared for integration into a native offline medical inference engine.
```

The project is now ready to proceed into:
- native C engine development,
- ONNX Runtime C integration,
- and full offline inference implementation.

---