# donors/ — file-level vendored snippets (D8)

Every subdirectory is a donor repo pinned to an exact commit, with a PROVENANCE.md
(commit SHA, license, per-file extraction reason). Donor files are read-only concept
references: project code under src/ re-implements the needed surfaces and never
#include-s donor files. Missing-license donors (hpcc, ns3-PB-FS, rdma-roce-sim) are
concept-extraction-only and must not appear here as file copies (R6).
