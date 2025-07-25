# DMC (Deterministic Markov Chain) 

DMCA is a symbolic semantic memory system designed to complement and extend traditional vector databases. It serves as the foundation for FixCom's intelligent, AI-assisted release note generator

DMCA is to power the backend of FixCom, a tool that automates Git commit summarization, changelog generation, and release note authoring using small quantized models.

## FixCom already:
- Parses commits into hunks
- Summarizes each hunk independently using tiny LLMs (less context rot)
- Aggregates hunks into commit-level summaries
- Tags hunks using Ctags, and stores keywords in a vector DB
- Segments release notes by gathering hunks related to a given keyword
- **However, this system is limited: the vector DB lacks semantic understanding, relationship modeling, and context merging.**

## DMCA addresses the limitations of static keyword search by constructing a symbolic Markov graph, where:
- Nodes represent source-level entities (e.g., functions, classes, variables) and semantic concepts (e.g., "thread-safe", "render pipeline").
- Edges represent probabilistic transitions between symbols based on co-occurrence, file proximity, call relations, or summarizer keywords.
- Commits are modeled as subgraphs, giving them semantic “footprints.”
- Features (like threading changes, renderer refactors, etc.) are inferred as connected regions of the graph.

## This allows DMCA to:
- Discover semantically related commits automatically
- Merge related concepts and track refactors over time
- Build structured, modular release notes per feature
- Provide full explainability without large LLMs

```
        Git Repo
            │
            ▼
    [ FixCom Hunks ]
            │
    +-----┴------+-----------------------------+
    |            |                             |
Small Model   ➜ Summary                  Extract CTags
    |                                         |
Commit Summary ◄───────────── Tag/Concept Mapping
    |                                         |
    +-------------→ [ DMCA Graph ] ←----------+
                        │
                        ▼
        Graph Regions + Commit Subgraphs
                        │
                        ▼
                [ DMCA Output ]
                        │
                        ▼
        FixCom Generates Release Notes
```