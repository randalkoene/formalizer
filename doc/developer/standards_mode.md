# Standards Mode

Some of the Formalizer page are missing a DOCTYPE at the top, which puts Chrome into Quirks Mode for that page. Switching to Standards Mode can lead to a few visual changes that I
didn't want to deal with right away. Nevertheless, I asked Gemini about this, and below are its suggestions. An example of a page where this matters is the Log Page.

---

It sounds like you’ve just stumbled upon one of the oldest "time-traveling" quirks of web development.

When you omit the `<!DOCTYPE html>`, you are essentially telling Chrome to pretend it’s a browser from the late 90s. This is **Quirks Mode**. When you add it, you trigger **Standards Mode**, where the browser follows modern CSS rules.

The visual shifts you're seeing are the result of how CSS was interpreted back in the day versus how it works now.

---

## Why the appearance changed

### 1. The Font Size Shift

In **Quirks Mode**, font sizes inside tables don't always inherit correctly from the `<body>` or the parent element. More importantly, older browsers interpreted font scaling differently. In Standards Mode, a `15px` font is strictly `15px` based on the modern box model, whereas Quirks Mode might have been applying a slight "fudge factor" or inheriting a different base size.

### 2. The Table Row "Padding"

This is the most common culprit. In **Standards Mode**, images and table cells are treated as inline elements that sit on a "baseline" (like a letter 'p' or 'g' in a sentence). This leaves a tiny gap at the bottom for "descenders." Quirks Mode ignores this and collapses the cell tightly around the content.

---

## How to fix it (The "Hybrid" approach)

You definitely want to stay in **Standards Mode**—it’s more predictable and keeps your site from breaking as browsers update. To get your "Quirks" look back without sacrificing the Doctype, try these CSS overrides:

### For the Table Rows

To remove that extra "padding" (which is actually whitespace), set your table cells to `vertical-align: bottom` or make the content inside them display as blocks.

```css
td {
    vertical-align: bottom; /* Removes the gap for text descenders */
    padding: 0;             /* Ensures no modern padding is applied */
}

```

### For the Font Size

Since Quirks Mode often failed to inherit styles into tables, you should explicitly define your font properties for table elements to ensure they match the rest of your page.

```css
body, table, td, th {
    font-size: 15px;
    line-height: 1.2; /* Adjust this to match your previous 'vibe' */
}

```

### The "Box Model" Fix

If your layout feels "squished" or "stretched," it might be the box model. Quirks Mode used the "Border Box" model (where padding is inside the width), while modern CSS uses "Content Box." You can force the modern browser to use the old-school (and arguably more intuitive) sizing:

```css
html {
  box-sizing: border-box;
}
*, *:before, *:after {
  box-sizing: inherit;
}

```

---

By applying these styles, you get the best of both worlds: a modern, compliant document that looks exactly like your old-school layout.
