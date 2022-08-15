## SPDX-License-Identifier: GPL-2.0-only
project_git_repo=https://github.com/TrenchBoot/secure-kernel-loader.git
project_dir=secure-kernel-loader

all: skl

checkout:
	test -d $(project_dir) || \
	    git clone $(project_git_repo) $(project_dir)

skl: checkout
	$(MAKE) -C $(project_dir) DEBUG=y

clean:
	test -d $(project_dir) && $(MAKE) -C $(project_dir) clean || exit 0

distclean:
	rm -rf $(project_dir)

.PHONY: checkout skl clean distclean
