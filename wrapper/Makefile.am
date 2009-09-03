if ENABLE_PHP
   PHP_DIR = php
endif

if ENABLE_PYTHON
   PYTHON_DIR = python
endif

if ENABLE_RUBY
   RUBY_DIR = ruby
endif

if ENABLE_VALA
   VALA_DIR = vala
endif

# Only include directories that were enabled at ./configure time
SUBDIRS = $(PHP_DIR) $(PYTHON_DIR) $(RUBY_DIR) $(VALA_DIR)

# Force DIST_SUBDIRS equal to SUBDIRS. Otherwise "distclean" target will
# break in unconfigured directories. This is allowed because all wrapper
# directories (configured and unconfigured) are also listed in EXTRA_DIST.
DIST_SUBDIRS = $(SUBDIRS)
