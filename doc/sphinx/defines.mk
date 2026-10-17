ifeq (, $(shell command -v magick))
	CONVERT = convert -density 300
else
	CONVERT = magick -density 300
endif

# Decompress gzip-compressed .dia files before handing them to dia: recent
# libxml2 (2.14+) no longer auto-decompresses XML input, breaking dia 0.97.x.
# SPHINX_DIR is this file's own directory, so the wrapper path holds for every
# Makefile that includes defines.mk, regardless of its depth in the tree.
SPHINX_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
DIA = $(SPHINX_DIR)dia-wrapper.sh
DOT = dot
EPSTOPDF = epstopdf

# You can set these variables from the command line.
SPHINXOPTS    = -W --keep-going
SPHINXBUILD   = sphinx-build
PAPER         =
BUILDDIR      = build
export BUILDDIR   # so dia-wrapper.sh localizes its temp dir under $(BUILDDIR)

# Internal variables.
PAPEROPT_a4     = -D latex_paper_size=a4
PAPEROPT_letter = -D latex_paper_size=letter
