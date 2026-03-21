# Sphinx configuration for FoxHttp (reStructuredText under this directory).
# Build HTML: pip install sphinx && sphinx-build -b html . _build

project = "FoxHttp"
copyright = "FoxHttp contributors"
author = "FoxHttp"

release = "1.0.0"
version = "1.0"

extensions = []

templates_path = ["_templates"]
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store"]

html_theme = "alabaster"
html_static_path = ["_static"]

language = "en"
