# Formalizer Top: Dashboard definitions and more

This directory contains the definition files for Formalizer dashboards and several other
generated pages.

- `index.json`: Edit this file to change the main dashboard.
- `admin.json`: Edit this file to change the admin dashboard.
- `granular-controls.html`: Edit this HTML document to change the granular controls dashboard.
- `integrity.html`: TBD.
- `select.html`: Edit this HTML document to change the standard Node selection page.

## Updating dashboards that are defined by a JSON file

For example, to update the main dashboard:

```
fzdashboard -D index
cd ~/src/formalizer && make executables
```

The two commands above will:

1. Generate the dashboard HTML file from the JSON definitions.
2. Update the active dashboard (as well as any other Formalizer components).

For more information, see `fzdashboard -h`.

---
Randal A. Koene, 2022
