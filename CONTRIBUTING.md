Good evening, Sir. Lenerd here — clean request. We’ll **update `CONTRIBUTING.md` so it explicitly defers commit message rules to a dedicated document** (e.g. `COMMIT_GUIDELINES.md`), which is the correct, scalable approach.

Below is the **updated version of the file**, with **only the relevant change made**, while keeping the rest intact and professional.

You can replace your current `CONTRIBUTING.md` with this verbatim.

---

# Contributing to Torilate

Thank you for your interest in contributing to **Torilate**. Contributions of all kinds—bug fixes, features, documentation improvements, and discussions—are welcome and appreciated.

Torilate is a systems-level networking project, and contributions are expected to follow its architectural and security principles.

---

## 1. Before You Contribute

Before starting work on a significant change:

* **Open an issue** to discuss the proposed change, bug, or feature
* Alternatively, reach out to the maintainers via the repository’s preferred communication channels

This helps ensure alignment on design, scope, and direction before code is written.

---

## 2. Contribution Workflow

All changes to Torilate happen through **Pull Requests**.

### Standard workflow

1. **Fork** the repository
2. Create a new branch from `main`
3. Make your changes in your fork
4. Commit your changes following the project’s commit message guidelines
5. Push your branch to your fork
6. Open a Pull Request against the main repository

---

## 3. Code Standards

When contributing code, please adhere to the following:

* Follow the existing **layered architecture**
* Do not introduce cross-layer dependencies
* Keep platform-specific code isolated (e.g., Windows vs POSIX)
* Avoid adding OS headers outside the `net` layer
* Prefer clarity and correctness over cleverness

All code should compile cleanly with warnings enabled.

---

## 4. Testing

If your change introduces new behavior:

* Add tests where applicable
* Manually verify functionality using known endpoints or reproducible steps
* Document how the change was tested in the Pull Request description

Torilate currently relies on manual testing, but reproducibility is important.

---

## 5. Documentation

If your change affects:

* Public behavior
* CLI usage
* Architecture or design
* Protocol handling

Please update the relevant documentation, including:

* `README.md`
* `ARCHITECTURE.md`
* Inline comments where appropriate

---

## 6. Commit Messages

All commits **must follow the project’s commit message format and tagging rules**.

Please refer to:

➡ **`COMMIT_GUIDELINES.md`**

Pull Requests containing commits that do not follow these guidelines may be requested to rebase or amend their commit history before review.

---

## 7. Pull Request Guidelines

When opening a Pull Request:

* Clearly describe **what** the change does
* Explain **why** the change is needed
* Reference any related issues
* Keep Pull Requests focused and reasonably scoped

Large or breaking changes may be requested to be split into smaller PRs.

---

## 8. Code of Conduct

Be respectful and professional in all interactions.

Harassment, abuse, or hostile behavior will not be tolerated.

---

## 9. Licensing

By contributing to this repository, you agree that your contributions will be licensed under the same license as the project.

---

Thank you for helping improve Torilate.

If you have questions or need guidance, feel free to open an issue and ask.

— **Trident Apollo**
