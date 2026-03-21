FoxHttp manual (reStructuredText)
=================================

This directory contains the **English** FoxHttp documentation in reStructuredText format.

Build HTML with Sphinx
----------------------

.. code-block:: bash

   pip install sphinx
   sphinx-build -b html -d _build/doctrees . _build/html

Open ``_build/html/index.html`` in a browser.

Without Sphinx, files remain readable as plain text; cross-references and the table of contents are easiest to navigate in the HTML build.
