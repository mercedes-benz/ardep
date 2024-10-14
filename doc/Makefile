# Minimal makefile for Sphinx documentation
#

# You can set these variables from the command line, and also
# from the environment for the first two.
SPHINXOPTS    ?=
SPHINXBUILD   ?= sphinx-build
SOURCEDIR     = .
BUILDDIR      = _build

DOXYGENBUILD  = doxygen
DOXYGENCONFIG = ardep.doxyfile

# Put it first so that "make" without argument is like "make help".
help:
	echo "${HELLO}"
	@$(SPHINXBUILD) -M help "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

.PHONY: help Makefile
	

# Catch-all target: route all unknown targets to Sphinx using the new
# "make mode" option.  $(O) is meant as a shortcut for $(SPHINXOPTS).
%: Makefile
	mkdir -p $(BUILDDIR)/html
	rsync -avm --exclude="/doc" --include="*/" --include="*.rst" --include="*.png"  --include="*.jpg" --include="*.zip" --include="*.pdf" --exclude="*" $(SOURCEDIR)/.. $(SOURCEDIR)/_board
	$(DOXYGENBUILD) $(DOXYGENCONFIG)
	@$(SPHINXBUILD) -M $@ "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)
