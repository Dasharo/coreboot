/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <intelblocks/cse.h>
#include <soc/pci_devs.h>

unsigned int soc_get_heci_dev(unsigned int heci_idx)
{
	if (heci_idx > 2)
		return 0;

	static const unsigned int heci_devs[] = {
		PCH_DEVFN_ME_HECI1,
		PCH_DEVFN_ME_HECI2,
		PCH_DEVFN_ME_HECI3,
	};

	return heci_devs[heci_idx];
}
