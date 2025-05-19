ifeq (, $(shell command -v magick))
	CONVERT = convert -density 300
else
	CONVERT = magick -density 300
endif

DIA = dia
DOT = dot
EPSTOPDF = epstopdf

# You can set these variables from the command line.
SPHINXOPTS    = -W --keep-going
SPHINXBUILD   = sphinx-build
PAPER         =
BUILDDIR      = build

# Internal variables.
PAPEROPT_a4     = -D latex_paper_size=a4
PAPEROPT_letter = -D latex_paper_size=letter
