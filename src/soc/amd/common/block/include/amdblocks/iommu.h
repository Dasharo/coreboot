/* SPDX-License-Identifier: GPL-2.0-only */

#define IOMMU_CAP_BASE_LO	0x44
#define   IOMMU_ENABLE		(1 << 0)
#define IOMMU_CAP_BASE_HI	0x48

#define IOMMU_VF_BASE_LO	0xd0
#define IOMMU_VF_BASE_HI	0xd4

#define IOMMU_VF_CNTL_BASE_LO	0xd8
#define IOMMU_VF_CNTL_BASE_HI	0xdc