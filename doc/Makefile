# Minimal makefile for Sphinx documentation
#

# You can set these variables from the command line, and also
# from the environment for the first two.
SPHINXOPTS    ?=
SPHINXBUILD   ?= sphinx-build
SOURCEDIR     = .
BUILDDIR      = _build

# Control whether to run rsync (default: on)
# Run `make html SYNC=off `to not sync the doc files with rsync
SYNC          ?= on

# Detect OS and set rsync command accordingly
ifeq ($(OS),Windows_NT)
# Under windows, we use the wsl since rsync is not available natively under windows
    RSYNC_CMD = wsl rsync
else
    RSYNC_CMD = rsync
endif

# Put it first so that "make" without argument is like "make help".
help:
	@$(SPHINXBUILD) -M help "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

.PHONY: help Makefile

# Catch-all target: route all unknown targets to Sphinx using the new
# "make mode" option.  $(O) is meant as a shortcut for $(SPHINXOPTS).
%: Makefile
ifeq ($(SYNC),on)
	$(RSYNC_CMD) -avm --include="*/" --include="*.rst" --include="*.png"  --include="*.jpg" --include="*.zip" --include="*.pdf" --exclude="*" $(SOURCEDIR)/../samples $(SOURCEDIR)/../scripts $(SOURCEDIR)/../boards $(SOURCEDIR)/../lib $(SOURCEDIR)
endif
	@$(SPHINXBUILD) -M $@ "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)
