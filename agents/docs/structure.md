# Documentation Structure

## Core Rule

- Public API and usage docs should live beside the public headers they describe.
- Prefer a local `README.md` under `include/<module>/...` for module or domain API contracts.
- Use `docs/` for cross-module architecture, rationale, decisions, and broader explanation.

## Local README Rules

- A local `README.md` is the canonical entrypoint for understanding a public API area.
- Place it at the nearest public include boundary that owns the API being described.
- For example:
  - `src/model/include/model/scene/README.md`
  - `src/viewer/include/viewer/README.md`
- Use `README.md` for discoverability. Do not invent a more specific filename unless there are multiple docs in the same directory.

## What Belongs In A Local README

- Public API surface
- Responsibility split between the main public types
- Primary read/write/request/response/control flow
- Important semantic rules that are not obvious from declarations alone
- Short usage-oriented guidance for auditors, maintainers, and agents

## What Does Not Belong There

- Broad cross-module architecture narrative
- Historical rationale or decision logs
- Deep implementation walkthroughs of private helpers
- Build/environment material unrelated to the local API contract

## Style Rules

- Be brief, precise, and audit-friendly.
- Prefer headings and short bullet lists over long prose.
- Use the language of the API exactly as it appears in code.
- Do not repeat obvious declaration details if the header already states them clearly.
- Add a short `TL;DR` at the end when the API has important responsibility boundaries.

## Agent Behavior

- When investigating a code area, check for the nearest module-local `README.md` first.
- Do not ingest the whole `docs/` folder by default when a nearby API README exists.
- Use `docs/` after the local README when cross-module context or rationale is still needed.

## Template

- Use [Local README Template](./local_readme_template.md) when creating a new local API README.
