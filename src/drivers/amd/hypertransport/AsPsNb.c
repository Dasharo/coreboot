/* SPDX-License-Identifier: GPL-2.0-only */

#include "comlib.h"
#include "AsPsDefs.h"
#include "AsPsNb.h"

u8 get_num_of_node_nb(void);
u8 translate_node_id_to_device_id_nb(u8 nodeId);

/**
 * Return the minimum possible NbCOF (in 100MHz) for the system.
 *
 * This function can be run on any core and is used by the HT & Memory init
 * code in Phase 1.
 *
 * @return minNbCOF (in multiple of half of CLKIN, 100MHz).
 */
u8 get_min_nb_cof(void)
{
	u8 num_of_node, i, j, device_id, nb_did, nb_fid, next_nb_fid;
	u32 dtemp;

	nb_did = 0;
	nb_fid = 0;

	/* get number of node in the system */
	num_of_node = get_num_of_node_nb();

	/* go through each node for the minimum NbCOF (in multiple of CLKIN/2) */
	for (i = 0; i < num_of_node; i++)
	{
		/* stub function for APIC ID virtualization for large MP system later */
		device_id = translate_node_id_to_device_id_nb(i);

		/* read all P-state spec registers for NbDid = 1 */
		for (j = 0; j < 5; j++)
		{
			amd_pci_read(MAKE_SBDFO(
					0, 0,
					device_id, FN_4,
					PS_SPEC_REG + (j * PCI_REG_LEN)),
					&dtemp); /* F4x1E0 + j * 4 */
			/* get NbDid */
			if (dtemp & NB_DID_MASK)
				nb_did = 1;
		}
		/* if F3x1FC[NbCofVidUpdate]=0, NbFid =  default value */
		amd_pci_read(MAKE_SBDFO(0,0,device_id,FN_3,PRCT_INFO), &dtemp); /*F3x1FC*/
		if (!(dtemp & NB_CV_UPDATE)) /* F3x1FC[NbCofVidUpdated]=0, use default VID */
		{
			amd_pci_read(MAKE_SBDFO(0,0,device_id,FN_3,CPTC0), &dtemp); /*F3xD4*/
			next_nb_fid = (u8) (dtemp & BIT_MASK_5);
			if (nb_did)
				next_nb_fid = (u8) (next_nb_fid >> 1);
		}
		else
		{
			/* check PVI/SPI */
			/* F3xA0 */
			amd_pci_read(MAKE_SBDFO(0,0,device_id,FN_3,PW_CTL_MISC), &dtemp);
			if (dtemp & PVI_MODE) /* PVI */
			{
				/* F3x1FC */
				amd_pci_read(MAKE_SBDFO(0,0,device_id,FN_3,PRCT_INFO), &dtemp);
				next_nb_fid = (u8) (dtemp >> UNI_NB_FID_BIT);
				next_nb_fid &= BIT_MASK_5;
				/* if (nb_did)
					next_nb_fid = next_nb_fid  >> 1; */
			}
			else /* SVI */
			{
				/*F3x1FC*/
				amd_pci_read(MAKE_SBDFO(0,0,device_id,FN_3,PRCT_INFO), &dtemp);
				next_nb_fid = (u8) ((dtemp >> UNI_NB_FID_BIT) & BIT_MASK_5);
				next_nb_fid = (u8) (next_nb_fid + ((dtemp >> SPLT_NB_FID_OFFSET) & BIT_MASK_3));
				/* if (nb_did)
					next_nb_fid = next_nb_fid >> 1; */
			}
		}
		if (i == 0)
			nb_fid = next_nb_fid;
		else if (nb_fid > next_nb_fid)
		nb_fid = next_nb_fid;
	}

	/* add the base and convert to 100MHz divide by 2 if DID = 1 */
	if (nb_did)
		nb_fid = (u8) (nb_fid + 4);
	else
		nb_fid = (u8) ((nb_fid + 4) << 1);
	return nb_fid;
}

u8 get_num_of_node_nb(void)
{
	u32 dtemp;

	amd_pci_read(MAKE_SBDFO(0,0,24,0,0x60), &dtemp);
	dtemp = (dtemp >> 4) & BIT_MASK_3;
	dtemp++;
	return (u8)dtemp;
}

/**
 * Return the PCI device ID for PCI access using node ID.
 *
 * This function may need to change node ID to device ID in big MP systems.
 *
 * @param nodeId Node ID of the node.
 * @return PCI device ID of the node.
 */
u8 translate_node_id_to_device_id_nb(u8 nodeId)
{
	return (u8)(nodeId + PCI_DEV_BASE);
}
