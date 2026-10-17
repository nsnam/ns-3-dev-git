PYTHON ?= python3
SCRIPT = ../../examples/leo-orbital-manipulations.py
FIGURES_DIR = figures

.PHONY: all leo-60s-0deg leo-600s-0deg leo-600s-30deg leo-600s-30deg-raan-180 leo-600s-30deg-planeRot-60

all: leo-60s-0deg leo-600s-0deg leo-600s-30deg leo-600s-30deg-raan-180 leo-600s-30deg-planeRot-60

leo-60s-0deg:
	cd $(FIGURES_DIR) && $(PYTHON) $(SCRIPT) --time-step-sec=60 --inclination-deg=0.0 --raan-deg=0 --plane-rotation-deg=0.0

leo-600s-0deg:
	cd $(FIGURES_DIR) && $(PYTHON) $(SCRIPT) --time-step-sec=600 --inclination-deg=0.0 --raan-deg=0 --plane-rotation-deg=0.0

leo-600s-30deg:
	cd $(FIGURES_DIR) && $(PYTHON) $(SCRIPT) --time-step-sec=600 --inclination-deg=30.0 --raan-deg=0 --plane-rotation-deg=0.0

leo-600s-30deg-raan-180:
	cd $(FIGURES_DIR) && $(PYTHON) $(SCRIPT) --time-step-sec=600 --inclination-deg=30.0 --raan-deg=180 --plane-rotation-deg=0.0

leo-600s-30deg-planeRot-60:
	cd $(FIGURES_DIR) && $(PYTHON) $(SCRIPT) --time-step-sec=600 --inclination-deg=30.0 --raan-deg=0 --plane-rotation-deg=60.0
