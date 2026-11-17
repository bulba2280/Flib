#!/usr/bin/env python3
"""Build the static HTML documentation from the Markdown sources."""

import re
from pathlib import Path

import markdown

HERE = Path(__file__).resolve().parent

PAGES = [
    ("index", "Overview"),
    ("files", "Files"),
    ("dirs", "Directories"),
    ("examples", "Examples"),
    ("changelog", "Changelog"),
    ("license", "License"),
]

TEMPLATE = """<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>{title}</title>
<link rel="stylesheet" href="style.css">
</head>
<body>
<nav>{nav}</nav>
<main>
{body}
</main>
<footer>flib 1.1.0</footer>
</body>
</html>
"""


def build_nav(current):
    links = []
    for name, label in PAGES:
        if name == current:
            links.append(f'<span class="current">{label}</span>')
        else:
            links.append(f'<a href="{name}.html">{label}</a>')
    return "\n".join(links)


def convert(source):
    text = source.read_text()
    body = markdown.markdown(text, extensions=["fenced_code", "tables"])
    return re.sub(r'href="([^"]+)\.md"', r'href="\1.html"', body)


def title_of(source, fallback):
    for line in source.read_text().splitlines():
        if line.startswith("# "):
            return line[2:].strip()
    return fallback


def main():
    for name, label in PAGES:
        source = HERE / f"{name}.md"
        if not source.exists():
            continue
        title = title_of(source, label)
        page_title = title if name == "index" else f"{title} - flib"
        html = TEMPLATE.format(
            title=page_title,
            nav=build_nav(name),
            body=convert(source),
        )
        (HERE / f"{name}.html").write_text(html)
        print(f"built {name}.html")


if __name__ == "__main__":
    main()
