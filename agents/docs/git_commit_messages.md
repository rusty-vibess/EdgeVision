# Git Commit Messages

## Allowed Prefixes

- `task`
- `fix`
- `cleanup`
- `chore`
- `feat`

## Prefix Rules

- Use `feat` only for very large commits that deliver a full feature, not a small task.
- Use UK spelling throughout the whole commit message.

## Structure

- Use exactly one title line.
- Use `prefix(scope): Title Case summary`.
- Use `1` to `4` body lines.
- Use `-` for every body line.
- Keep every body line short, direct, and concrete.

## Template

```text
prefix(scope): Title Case summary

- Short concrete point
- Short concrete point
- Short concrete point
```

## Example

```text
fix(builder): Reset InfiniTAM scene state on pipeline initialisation

- Reset the InfiniTAM scene once when the builder pipeline is first created
- Keep viewer rendering on the direct CUDA path without extra fail-closed guards
- Add a builder regression check that the bootstrap scene hash remains valid
```

## Style Rules

- Prefer short noun-verb statements over explanation.
- Prefer module or subsystem names for `scope`.
- Do not use long prose paragraphs.
- Do not use nested bullets.
- Do not exceed `4` body lines.
