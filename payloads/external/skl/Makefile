## SPDX-License-Identifier: GPL-2.0-only
project_git_repo=$(CONFIG_SKL_REPOSITORY)
project_dir=secure-kernel-loader

all: skl

checkout:
	if [ ! -d "$(project_dir)" ]; then \
		git clone --recurse-submodules "$(project_git_repo)" -j5; \
	fi
	cd "$(project_dir)"; \
		if ! git rev-parse --verify -q $(CONFIG_SKL_TAG_OR_REV)^{object} >/dev/null; then \
			echo "    $(CONFIG_SKL_TAG_OR_REV) is not a valid git reference"; \
			exit 1;\
		fi; \
		if git status --ignore-submodules=dirty | grep -q "nothing to commit, working tree clean"; then \
			echo "    Checking out secure-kernel-loader $(CONFIG_SKL_TAG_OR_REV)"; \
			git checkout --detach $(CONFIG_SKL_TAG_OR_REV) -f; \
		else \
			echo "    Working directory not clean; will not overwrite"; \
		fi; \
		git submodule update --init --checkout


skl: checkout
	$(MAKE) -C $(project_dir) DEBUG=y

clean:
	test -d $(project_dir) && $(MAKE) -C $(project_dir) clean || exit 0

distclean:
	rm -rf $(project_dir)

.PHONY: checkout skl clean distclean
