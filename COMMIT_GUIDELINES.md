
# Commit Message Guidelines

Clear, consistent commit messages make the history readable, searchable, and meaningful. All contributions to **Torilate** are expected to follow the format and conventions outlined below.

---

## 1. Commit Message Format

Each commit message **must** follow this structure:

```
[TAG]: Short, imperative description of the change
```

### Rules

* The tag must be uppercase and enclosed in square brackets
* The description should be concise and written in the **imperative mood**
* Avoid trailing periods
* Keep the subject line under 72 characters when possible

---

## 2. Allowed Tags

| Tag          | Meaning                                        |
| ------------ | ---------------------------------------------- |
| `[FIX]`      | Fixed a bug or issue                           |
| `[ADD]`      | Added new files or directories                 |
| `[FEAT]`     | Introduced a new feature                       |
| `[DELETE]`   | Deleted files or directories                   |
| `[REMOVED]`  | Removed an existing feature                    |
| `[UPDATE]`   | Updated existing logic or file content         |
| `[UPD_FEAT]` | Updated or enhanced an existing feature        |
| `[CONF]`     | Updated or added configuration files           |
| `[DOCS]`     | Updated or added documentaitons files          |
| `[BUILD]`    | Modified/added build system or build files     |
| `[REFACTOR]` | Refactored code without changing behavior      |
| `[REBASE]`   | Major restructuring or rewrite of the codebase |

---

## 3. Examples

### ✅ Good Examples

```
[FEAT]: Add SOCKS4a hostname support
[FIX]: Handle partial socket reads correctly
[REFACTOR]: Isolate Winsock initialization into net layer
[BUILD]: Update CMake to support platform-specific sockets
[CONF]: Add editorconfig for consistent formatting
```

### ❌ Bad Examples

```
fixed stuff
update
changes
[FEAT] added something
```

---

## 4. Scope and Granularity

* Each commit should represent **one logical change**
* Avoid mixing unrelated changes in a single commit
* Large changes should be broken into smaller, reviewable commits

---

## 5. When in Doubt

If a change could reasonably fit multiple tags:

* Choose the **most specific** tag
* Prefer `[FEAT]` over `[ADD]` when behavior changes
* Prefer `[REFACTOR]` when logic is reorganized without altering output

---

Following these guidelines helps keep Torilate’s history clean and easy to reason about.

Thank you for contributing.

— **Trident Apollo**


