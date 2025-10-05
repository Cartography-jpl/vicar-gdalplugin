# Install conda environment
# --------------------------------------
# Things the user can change. Note you can change this by creating a Makefile.local
# to set these variables, or just set it on the make line.

# Should we install the latest conda packages, or the "pixi.lock" set known to
# work (which will often lag the latest conda packages). Most of the time
# latest should work fine, but there might be the occasional breakage (e.g.,
# conda moves to a newer version of python and our various packages haven't
# been rebuilt yet the newer python, a python package update breaks
# something).
PIXI_LOCKED=yes

mkfile_dir := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
# Location of environment we create
ENV_DIR=$(mkfile_dir)/../../vicar-gdalplugin-build/build-pixi

# Location of afids conda packages (you can check this out from
# git@github.jpl.nasa.gov:Cartography/afids-conda-package.git
CONDA_PACKAGE_DIR=$(mkfile_dir)/../../afids-conda-package/afids-conda-channel

# Include a Makefile.local to override things, if found
-include Makefile.local
# ---------------------------------------------------------------------------
# Past this point are the rules, shouldn't need to modify these
# ---------------------------------------------------------------------------

OS := $(shell uname -s)

ifeq ($(OS),Darwin)
  PIXI_FILE_INSTALL=$(if $(filter $(CONDA_FIXED),yes), $(mkfile_dir)/osx/pixi.toml $(mkfile_dir)/osx/pixi.lock, $(mkfile_dir)/pixi.toml)
  PIXI_COMPILER=compilers
else
  PIXI_FILE_INSTALL=$(if $(filter $(CONDA_FIXED),yes), $(mkfile_dir)/linux/pixi.toml $(mkfile_dir)/linux/pixi.lock, $(mkfile_dir)/pixi.toml)
  PIXI_COMPILER=gcc gxx gfortran
endif

$(ENV_DIR): $(PIXI_FILE_INSTALL)
	-rm -rf $(ENV_DIR)
	pixi init -c conda-forge -c $(CONDA_PACKAGE_DIR) $(ENV_DIR)
	cd $(ENV_DIR) && pixi config set --local run-post-link-scripts insecure && cp $^ . && pixi add $(PIXI_COMPILER) && pixi install
	- cd $(ENV_DIR) && pixi task rm configure
	cd $(ENV_DIR) && pixi task add configure "$(mkfile_dir)/../configure --prefix=\$$CONDA_PREFIX"

# Recreate the environment, deleting if already there
# ------------------------------------------------------------
recreate-env:
	-rm -rf $(ENV_DIR)
	$(MAKE) create-env

# Build the conda envirornment, if needed or out of date.
# ------------------------------------------------------------
create-env: $(ENV_DIR)

