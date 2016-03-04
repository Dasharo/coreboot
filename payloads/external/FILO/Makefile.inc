FILOGIT-$(CONFIG_FILO_GENERIC)=http://review.coreboot.org/p/filo.git
FILOGIT-$(CONFIG_FILO_ELTAN)=git@vcs.eltan.com:eltan/filo.git

ifeq ($(CONFIG_FILO_GENERIC),y)
#
# Using the generic FILO REPO
#
TAG-$(CONFIG_FILO_MASTER)=origin/master
NAME-$(CONFIG_FILO_MASTER)=MASTER
TAG-$(CONFIG_FILO_STABLE)=4dbb31a64fe5b1c7e3025ab34619220609897646
NAME-$(CONFIG_FILO_STABLE)=STABLE
endif

ifeq ($(CONFIG_FILO_ELTAN),y)
#
# Using the eltan FILO REPO
#
TAG-$(CONFIG_FILO_MASTER)=origin/master
NAME-$(CONFIG_FILO_MASTER)=MASTER
TAG-$(CONFIG_FILO_STABLE)=$(CONFIG_ELTAN_FILO_TAG)
NAME-$(CONFIG_FILO_STABLE)=STABLE
endif

unexport KCONFIG_AUTOHEADER
unexport KCONFIG_AUTOCONFIG
unexport KCONFIG_DEPENDENCIES
unexport KCONFIG_SPLITCONFIG
unexport KCONFIG_TRISTATE
unexport KCONFIG_NEGATIVES

all: filo

checkout:
ifneq ($(wildcard filo/.*),)
	echo "Found filo no need to get a new one"
else
	echo "    GIT FILO $(FILOGIT-y) $(NAME-y) $(TAG-y)"
	test -d filo || \
		git clone $(FILOGIT-y)
	cd filo && \
		git checkout master && \
		git remote update && \
		test -n $(TAG-y) && \
			git branch -f $(NAME-y) $(TAG-y) && \
			git checkout $(NAME-y)
endif

config: libpayload
	echo "    CONFIG     FILO $(NAME-y)"
	$(MAKE) -C filo defconfig LIBCONFIG_PATH=../../../libpayload
	echo "CONFIG_MENULST_FILE=\"$(CONFIG_MENULST_FILE)\"" >> filo/.config
	echo "CONFIG_MENULST_TIMEOUT=$(CONFIG_MENULST_TIMEOUT)" >> filo/.config
	# This shows how to force a previously unset .config option *on*
	#echo "CONFIG_VGAHOOKS=y" >> filo/.config
	# This shows how to force a previously set .config option *off*
	#echo "# CONFIG_SMBIOS is not set" >> filo/.config
	$(MAKE) -C filo oldconfig LIBCONFIG_PATH=../../../libpayload

filo: config
	echo "    MAKE       FILO $(NAME-y)"
	$(MAKE) -C filo

libpayload: checkout
ifneq ($(wildcard filo/build/libpayload),)
	echo "Already have a libpayload no need to build new"
else
	cd ../../libpayload && $(MAKE) defconfig && \
		$(MAKE) && $(MAKE) DESTDIR=../external/FILO/filo/build install
endif

clean:
	test -d filo && $(MAKE) -C filo clean || exit 0

distclean:
	rm -rf filo

.PHONY: checkout config filo clean distclean

