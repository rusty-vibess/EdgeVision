# Local README Template

Use this structure for public API documentation placed beside public headers.

## Title

- Name the API area directly.
- Example: `# Model Scene API`

## Opening Line

- One sentence only.
- State that the public API for this area lives in the current directory.

## Public API

### Facade

- List the main public facade type(s)
- List the key entrypoint methods only

### Granted Access / Handle / Request Types

- List the main supporting public types
- List only the methods that define their meaning

### Public Value Types

- Point to nearby `types/` headers when relevant

## Internal Implementation

### Internal Types

- Name the key internal type(s) only if needed to explain responsibility boundaries
- Keep this short

## Flow

### Read / Request / Input Flow

- Step-by-step bullets

### Write / Commit / Output Flow

- Step-by-step bullets

## Other Notes

- Only include non-obvious semantic rules not already covered above

## TL;DR

- 2-4 bullets
- Extreme brevity
- Restate the responsibility split and the main semantic rule
