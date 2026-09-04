# CitySense Aggregator — C++ Structured Data Processing

A C++ data-processing project that transforms an approximately **2,000-row urban congestion and pollution dataset** into consistent structured analytical outputs. The project emphasizes validation, schema discipline, modular processing, and JSON/CSV serialization.

> **Team project note.** CitySense was completed as a group project. This README describes the parts I can defend in an interview: the C++ processing pipeline, validation/error handling, schema-consistent JSON/CSV output, and modularity for additional metrics/zones. It should not be read as a claim of sole authorship of the entire repository.

## What the project demonstrates

- C++ structured-data processing.
- Input validation and explicit error handling.
- JSON/CSV serialization with consistent schemas.
- Modular components that can be extended to additional metrics and geographic zones.
- Repeatable transformation from raw rows to analytical outputs.

## Processing flow

```text
Raw congestion / pollution data
        │
        ▼
Input parsing and validation
        │
        ▼
Structured domain records
        │
        ▼
Aggregation / metric processing
        │
        ├──────────────► JSON output
        │
        └──────────────► CSV output
```

## Design priorities

### 1. Data quality before output
Malformed or inconsistent input should be detected rather than silently propagated. Validation and error handling are part of the processing contract.

### 2. Schema consistency
JSON and CSV outputs use predictable fields and formatting so downstream analysis can rely on a stable structure.

### 3. Extensibility
Processing components were organized so additional zones, metrics, or derived outputs can be added without rewriting the entire pipeline.

### 4. Reproducibility
Given the same input and configuration, the program should generate the same structured result, making debugging and verification practical.

## Suggested review path for a recruiter or engineer

After restoring a normal working tree, I recommend reviewing:

1. the entry point / main processing flow;
2. data model or record structures;
3. validation and parsing logic;
4. JSON serialization;
5. CSV serialization;
6. aggregation/metric components;
7. sample input and generated output files, if retained in the repository.

## Local repository note

The local copy currently shown in my portfolio workspace is a **bare Git repository** (`citysense-group-project-group-13.git`) containing Git objects/refs rather than a checked-out working tree. To edit this README locally, first create a normal working copy, for example:

```powershell
git clone D:\Courses\git\citysense-group-project-group-13.git D:\Courses\git\citysense-group-project-group-13
cd D:\Courses\git\citysense-group-project-group-13
```

Then place this file at the repository root as `README.md`, commit, and push to the public remote.

## Portfolio accessibility check

Before putting this repository on a resume, open the GitHub URL in a private/incognito browser window. If it returns 404 or requires organization membership, make a permitted public portfolio copy (with team/course authorization) or remove the external resume link until it is accessible.

## Technologies

- C++
- JSON
- CSV
- Validation and error handling
- Structured data processing
- Git/GitHub

## Status

**Completed team course project.** Use as portfolio evidence for C++ data processing, validation, serialization, and modular software design.
