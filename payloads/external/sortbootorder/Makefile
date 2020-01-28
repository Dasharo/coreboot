TAG-$(SORTBOOTORDER_MASTER)=origin/master
TAG-$(SORTBOOTORDER_STABLE)=v4.6.17
TAG-$(SORTBOOTORDER_REVISION)=$(SORTBOOTORDER_REVISION_ID)

project_git_repo=https://github.com/pcengines/sortbootorder.git
project_name=sortbootorder
project_dir=$(CURDIR)/sortbootorder

unexport KCONFIG_AUTOHEADER
unexport KCONFIG_AUTOCONFIG
unexport KCONFIG_DEPENDENCIES
unexport KCONFIG_SPLITCONFIG
unexport KCONFIG_TRISTATE
unexport KCONFIG_NEGATIVES

all: build

$(project_dir):
	echo "    Cloning sortbootorder from Git"
	git clone $(project_git_repo) $(project_dir)

fetch: $(project_dir)
ifeq ($(TAG-y),)
	echo "Error: The specified tag is invalid"
ifeq ($(SORTBOOTORDER_REVISION),y)
	echo "Error: There is no revision specified for sortbootorder"
	false
endif
	false
endif
	cd sortbootorder; git show $(TAG-y) >/dev/null 2>&1 ; \
		if [ $$? -ne 0 ] || [ "$(TAG-y)" = "origin/master" ]; then \
		echo "    Fetching new commits from the sortbootorder repo"; \
		git fetch; fi

checkout: fetch
	echo "    Checking out sortbootorder revision $(TAG-y)"
	cd sortbootorder; git checkout master; git branch -D coreboot 2>/dev/null; git checkout -b coreboot $(TAG-y)

build: checkout
	echo "    MAKE       sortbootorder"
	$(MAKE) -C sortbootorder VERSION=$(TAG-y) APU1=$(TARGET_APU1)

clean:
	test -d sortbootorder && $(MAKE) -C sortbootorder clean || exit 0

distclean:
	rm -rf sortbootorder

print-repo-info:
	echo "$(project_git_repo) $(project_dir)"

.PHONY: checkout build clean distclean fetch sortbootorder print-repo-info
