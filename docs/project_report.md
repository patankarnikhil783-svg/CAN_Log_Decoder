# CAN Log Decoder: Project Report

This report completes the sections of the mini-project template that are not
part of the proposal, the detailed design or the README. The section numbers
follow the template.

- Proposal: [project_proposal.md](project_proposal.md)
- Detailed design: [detailed_design.md](detailed_design.md)
- README: [../README.md](../README.md)

---

## 16. Repository Structure

Final project structure:

```
CAN_Log_Decoder/
|-- src/
|   |-- main.c
|   |-- menu.c
|   |-- file_handler.c
|   |-- parser.c
|   |-- validation.c
|   |-- decoder.c
|   |-- frame_store.c
|   `-- report.c
|
|-- include/
|   |-- types.h
|   |-- menu.h
|   |-- file_handler.h
|   |-- parser.h
|   |-- validation.h
|   |-- decoder.h
|   |-- frame_store.h
|   `-- report.h
|
|-- tests/
|   `-- test_can_log_decoder.cpp
|
|-- data/
|   `-- sample_can_log.csv
|
|-- docs/
|   |-- project_proposal.md
|   |-- detailed_design.md
|   |-- project_report.md
|   |-- architecture.png
|   |-- module_dependencies.png
|   |-- flow_main.png
|   `-- flow_decode.png
|
|-- Makefile
|-- README.md
|-- .gitignore
`-- LICENSE
```

All 30 unit tests are in one test file, grouped into test suites by function.
This replaces the six test files planned in the first version of the design.

---

## 17. Git and GitHub Plan

### 17.1 Repository Information

| Field | Value |
|---|---|
| Repository name | CAN_Log_Decoder |
| Repository visibility | Public |
| Main branch | main |
| GitHub link | https://github.com/patankarnikhil783-svg/CAN_Log_Decoder |

### 17.2 Commit Milestones

| Milestone | Commit Message | Status |
|---|---|---|
| Repository creation | Initial project structure with Makefile and LICENSE (4c5851d) | Complete |
| Proposal | Add scenario, scope, requirements, architecture and initial modules (f86d5c6 to acb7b87) | Complete |
| Detailed design | Add detailed design: data design, modules, functions, validation, files, errors, test plan (5a0d97c to aa84b4b) | Complete |
| Data design | Add shared types and constants (5642836) | Complete |
| Core module | Add parser module with unit tests (e9a07aa) | Complete |
| File handling | Add validation, frame store, file handler, decoder, report and menu modules (6cd1127) | Complete |
| Application | Implement main application logic (f03de9a) | Complete |
| Build fix | Fix syntax error in openLogFile | Complete |
| Coding standards | Apply coding standards: braces, whitespace and function length | Complete |
| Unit testing | Replace parser tests with full gTest suite | Complete |
| Sample data | Add sample CAN log | Complete |
| Makefile | Initial Makefile in 4c5851d, verified with `make all`, `make run`, `make test` and `make clean` | Complete |
| Documentation | Document build, run and test steps in README; add project report | Complete |

Run `git log --oneline` to see the commit hash of each milestone.

### 17.3 Files to Ignore

`.gitignore` contains:

```
build/
*.o
*.out
*.exe
can_decoder
*.log
core
.vscode/
*.swp
```

---

## 18. Implementation Status

| Work Item | Status | Evidence or Commit |
|---|---|---|
| Proposal | Complete | `docs/project_proposal.md` |
| Detailed design | Complete | `docs/detailed_design.md` |
| Shared types (`types.h`) | Complete | 5642836 |
| Parser | Complete | e9a07aa |
| Validation | Complete | 6cd1127 |
| Decoder | Complete | 6cd1127 |
| Frame store | Complete | 6cd1127 |
| Report | Complete | 6cd1127 |
| Menu | Complete | 6cd1127 |
| File handling | Complete | `src/file_handler.c`, 6cd1127 |
| Main application | Complete | f03de9a |
| Unit tests | Complete | `tests/test_can_log_decoder.cpp`, 30 tests |
| Makefile | Complete | `Makefile` |
| README | Complete | `README.md` |
| Repository verification | Complete after the final push: clone into a new folder and run `make all`, `make test`, `make run` | Section 19 |

---

## 19. Test Execution Results

### 19.1 Build Result

Command:

```
make all
```

Result: **Pass**

Warnings or errors: None. The C code is compiled with
`-std=c99 -Wall -Wextra -Wpedantic`. Running `make all` a second time prints
`Nothing to be done for 'all'.`

### 19.2 Unit-Test Result

Command:

```
make test
```

| Item | Result |
|---|---|
| Total tests | 30 |
| Passed | 30 |
| Failed | 0 |

Test-output evidence:

```
[==========] 30 tests from 10 test suites ran.
[  PASSED  ] 30 tests.
```

| Test suite | Tests | Kinds of cases |
|---|---|---|
| ParseHexValue | 3 | Positive, boundary (0xFF and 0x100), negative |
| ParseFrameLine | 4 | Positive, negative (field count, invalid byte), boundary (identifier and length limits) |
| CalculateChecksum | 1 | Positive |
| ValidateFrame | 5 | Positive, negative (identifier, length, checksum), check order |
| ExtractBits | 4 | Positive, boundary (last bit), negative (length, NULL) |
| ConvertRawValue | 1 | Positive (factor and offset) |
| DecodeFrame | 3 | Positive (all three messages), negative (unknown identifier, NULL) |
| FrameStore | 6 | Positive, boundary (capacity of 1000), statistics, filtering |
| ParseMenuChoice | 2 | Boundary (0 and 6, white space), negative |
| StatusToText | 1 | Every status has a message |

### 19.3 Application Test Results

These are the system tests ST-01 to ST-19 from the detailed design, run on the
real program with `data/sample_can_log.csv` and generated files.

| Scenario | Input or Condition | Expected Result | Actual Result | Status |
|---|---|---|---|---|
| Normal use | Load and decode `data/sample_can_log.csv` | 8 lines read, 1 rejected, 5 decoded, 1 checksum error, 1 unknown identifier | Same | Pass |
| Normal use | Statistics after decoding | 0x100: 3 received, 3 valid. 0x200: 2 received, 1 valid, 1 error. 0x300: 1 received, 1 valid. 1 unknown identifier | Same | Pass |
| Normal use | Frames of identifier 0x100 | Three frames, engine speed 2822.5, 2957.0 and 2957.0 rpm | Same | Pass |
| Normal use | Error frames | Line 6 (checksum, expected F5, received 00) and line 7 (unknown identifier) | Same | Pass |
| Normal use | Single frame `1A 2C 78 80 00 00 00 CE` on 0x100 | 2822.5 rpm, 80 deg C, 51.2 % | Same | Pass |
| Invalid input | Menu choice `9`, `abc`, empty line | "Invalid choice" each time | Same | Pass |
| Invalid input | Single frame with last byte `00` | "Checksum mismatch (expected CE, received 00)" | Same | Pass |
| Invalid input | Option 2 before loading, options 3 to 5 before decoding | "No file loaded. Choose 1 first." and "No frames stored. Choose 2 first." | Same | Pass |
| Invalid input | Line with 300 characters | Line rejected as too long, following lines processed | Same | Pass |
| Boundary value | Log with 1001 frames | "Frame store is full (1000 frames)", 1000 frames decoded, summary shown | Same | Pass |
| Boundary value | Log with 1000 frames | Finishes in less than one second | About 2 ms | Pass |
| Boundary value | Ctrl+D at the menu | Program ends normally | Same | Pass |
| Missing record | Load `missing.csv` three times | "Cannot open the file" three times, then back to the menu | Same | Pass |
| File failure | Load an empty file | "The file has no header line" | Same | Pass |
| File failure | Load a folder name (`data`) | "Error while reading the file" | Same | Pass |
| Build | `make clean`, then `make all` and `make test` | Everything rebuilt, all tests pass | Same | Pass |

---

## 20. README Content Checklist

`README.md` contains:

- [x] Project title
- [x] Engineering domain
- [x] Problem statement
- [x] Proposed solution
- [x] Features
- [x] Architecture diagram
- [x] Module descriptions
- [x] Folder structure
- [x] C concepts used
- [x] Coding standards
- [x] Prerequisites
- [x] Build instructions
- [x] Run instructions
- [x] Test instructions
- [x] Sample input
- [x] Sample output
- [x] Unit-testing summary
- [x] Error-handling summary
- [x] Team members (student information)
- [x] Known limitations
- [x] Future enhancements
- [x] GitHub repository link

---

## 21. Demonstration Plan

| Sequence | Demonstration Item | Approximate Time |
|---|---|---|
| 1 | Introduce the engineering scenario: decoding recorded CAN traffic by hand is slow and error-prone | 1 min |
| 2 | Explain the problem statement | 1 min |
| 3 | Present the requirements (FR-01 to FR-18, NFR-01 to NFR-13) | 1 min |
| 4 | Explain the architecture and module dependencies | 2 min |
| 5 | Show the repository structure | 1 min |
| 6 | Build using `make all` | 1 min |
| 7 | Run using `make run` | 1 min |
| 8 | Demonstrate the features: load and decode the sample, statistics, frames by identifier, error frames, single frame | 3 min |
| 9 | Demonstrate invalid-input handling: bad menu choice, missing file, empty file, wrong checksum | 2 min |
| 10 | Execute `make test` (30 tests pass) | 1 min |
| 11 | Show the commit history with `git log --oneline` | 1 min |
| 12 | Explain limitations and improvements | 1 min |

---

## 22. Reflection

### 22.1 What Worked Well?

Writing the proposal and the detailed design before any code made the
implementation straightforward. The signal table, the status codes and the
function specifications were already decided, so each module could be written
and tested against its specification. Keeping parsing, validation and decoding
free of input and output made them easy to unit test. Describing the signals in
one constant table meant the decoder needed only one generic bit-extraction
function.

### 22.2 Challenges Faced

1. Extracting signals that sit at any bit position, such as the brake bit and
   the three gear bits, and checking the boundary at the last bit.
2. Reading input safely: long lines, Windows line endings, empty input and the
   end of input at the menu.
3. Keeping the code consistent with the coding standards written in the design:
   braces on every statement, no trailing whitespace and functions of 50 lines
   or fewer.
4. Keeping the repository correct: an early commit did not compile because of a
   stray character, and the test layout changed from six planned files to one.

### 22.3 How Were the Challenges Resolved?

1. A single `extractBits` function builds a 64-bit value from the data bytes,
   shifts it and applies a mask, with unit tests for the boundaries.
2. Every read is bounded. Over-long lines are drained and rejected, and CR/LF
   is removed. Each input function returns a status code, so the end of input
   ends the program cleanly.
3. I checked the code against the standards, split the long functions in
   `main.c`, `file_handler.c` and `menu.c`, and fixed the brace style.
4. I rebuilt from a clean state, ran `make test`, and cloned the repository
   into a new folder to check what a reviewer sees.

### 22.4 What Did You Learn?

- **C programming:** bit manipulation with unsigned types, bounded string and
  file input, and passing results through output parameters and status codes.
- **Modular design:** one responsibility per module, a thin `main.c` and no
  input or output in logic modules.
- **Coding standards:** a written standard is only useful if the code is
  checked against it.
- **Unit testing:** positive, negative and boundary cases for each function,
  with independent and repeatable tests.
- **Makefiles:** separate compilation, dependency files so only changed files
  rebuild, and a test build that leaves out `main.o`.
- **Git and GitHub:** small commits with clear messages, a `.gitignore` for
  generated files, and verifying the published repository by cloning it.

### 22.5 Known Limitations

1. Only three fixed messages (0x100, 0x200 and 0x300) are supported.
2. Only standard 11-bit identifiers and 8-byte frames are accepted.
3. Signals are unsigned and little-endian only.
4. At most 1000 frames are kept in memory.

### 22.6 Future Enhancements

1. Read signal definitions from a DBC file.
2. Support signed and big-endian signals and extended 29-bit identifiers.
3. Export decoded signals to a CSV file for plotting.

---

## 23. Final Submission Checklist

### Proposal and Design

- [x] Project title is clear.
- [x] Engineering scenario is realistic.
- [x] Problem statement is specific.
- [x] Target users are identified.
- [x] Scope is defined.
- [x] Functional requirements are documented.
- [x] Non-functional requirements are documented.
- [x] Architecture diagram is included.
- [x] Modules are identified.
- [x] Data structures are designed.
- [x] Major functions are specified.
- [x] Error-handling strategy is defined.
- [x] Unit-test plan is complete.

### Implementation

- [x] Multiple C source files are used.
- [x] Header files are provided.
- [x] `main.c` contains high-level control only.
- [x] Structures are used meaningfully.
- [x] Functions have clear parameters and return values.
- [x] Input validation is implemented.
- [x] File operations are checked.
- [x] Errors are handled.
- [x] Coding standards are followed.
- [x] Code compiles without warnings.
- [x] No unnecessary or unused code remains.

### Unit Testing

- [x] gTest is used.
- [x] At least three important functions are tested.
- [x] At least ten meaningful tests are included.
- [x] Positive tests are included.
- [x] Negative tests are included.
- [x] Boundary tests are included.
- [x] Tests are independent and repeatable.
- [x] `make test` works.
- [x] All tests pass.

### Makefile

- [x] `make all` works.
- [x] `make run` works.
- [x] `make test` works.
- [x] `make clean` works.

### Git and GitHub

- [x] Git was used throughout development.
- [x] Multiple meaningful commits are present.
- [x] `.gitignore` is configured.
- [x] Generated files are not committed.
- [x] GitHub contains the complete project (tick after the final push).
- [x] The repository was cloned and verified independently (tick after the check).
- [x] The correct repository link is submitted.

### Documentation

- [x] README is complete.
- [x] Architecture is documented.
- [x] Repository structure is documented.
- [x] Build instructions are correct.
- [x] Run instructions are correct.
- [x] Test instructions are correct.
- [x] Sample output is included.
- [x] Limitations are documented.
- [x] Future enhancements are documented.

---

## 24. Final Submission Information

| Field | Value |
|---|---|
| Project title | CAN Log Decoder in C |
| Engineering domain | Automotive Embedded Systems (In-Vehicle Networks) |
| Student name | Nikhil Shripad Patankar |
| Roll number | 612509029 |
| Division / batch | Instrumentation and Control Engineering |
| GitHub repository link | https://github.com/patankarnikhil783-svg/CAN_Log_Decoder |
| Project proposal location | docs/project_proposal.md |
| Detailed design location | docs/detailed_design.md |
| README location | README.md |
| Build command | `make all` |
| Run command | `make run` |
| Unit-test command | `make test` |
| Clean command | `make clean` |
| Submission date | 12/11/2026 |
