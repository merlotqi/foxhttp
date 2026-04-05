# Contributing to foxhttp

## Coding style

- **Types** (classes, structs, enums, type aliases intended as API types): `PascalCase`. Some historical typedefs remain snake_case (e.g. `form_data`); prefer the PascalCase aliases where provided (`FormData`, `MultipartData`).
- **Server timers**: public typedefs use `TimerId`, `TimerCallback`, `TimerClock`, `TimerTimePoint`, `TimerDuration` (avoid names like `clock_t` that collide with C/POSIX).
- **Strand pool**: callback types are `MetricsCallback` and `HealthCheckCallback`.
- **Config**: the merged view type is `foxhttp::config::ConfigManager::ConfigSnapshot` (field `strand_pool` for strand pool settings).
- **Functions and methods**: `snake_case`.
- **Variables and parameters**: `snake_case`; **private/protected members** typically end with `_`.
- **Implementation details** live under include paths `.../detail/...` and namespaces `detail` (singular), nested under the owning module (e.g. `foxhttp::parser::detail`).

The repository [`.clang-tidy`](.clang-tidy) configures `readability-identifier-naming` for headers under `foxhttp/`. Run selectively, for example:

```bash
clang-tidy -p build path/to/file.cpp
```

A full-tree run may still report violations in legacy or third-party-adjacent code; fix incrementally.

## Versioning

Public **type renames**, **namespace moves**, or **signature changes** are **breaking** and should be accompanied by a **major** version bump and migration notes (changelog or release text).

## Bulk refactors

For one-off mass renames or namespace alignment, use **[`scripts/full_refactor.py`](scripts/full_refactor.py)** as the canonical tool. Do not run [`scripts/apply_naming_refactor.py`](scripts/apply_naming_refactor.py) for new work; it is deprecated and kept for reference only.
