/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef _SOC_IRQ_H_
#define _SOC_IRQ_H_

#define SDCARD_INT      3       /* Need to be shared by PMC and SCC only*/
#define LPSS_UART0_IRQ  4       /* Need to be shared by PMC and SCC only*/
#define LPSS_UART1_IRQ  5       /* Need to be shared by PMC and SCC only*/
#define LPSS_UART2_IRQ  6       /* Need to be shared by PMC and SCC only*/
#define LPSS_UART3_IRQ  7       /* Need to be shared by PMC and SCC only*/
#define PCH_IRQ10       10
#define PCH_IRQ11       11
#define XDCI_INT        13      /* Need to be shared by PMC and SCC only*/
#define GPIO_BANK_INT   14
#define NPK_INT         16
#define PIRQA_INT       16
#define PIRQB_INT       17
#define PIRQC_INT       18
#define SATA_INT        19
#define GEN_INT         19
#define PIRQD_INT       19
#define XHCI_INT        17      /* Need to be shared by PMC and SCC only*/
#define SMBUS_INT       20      /* PIRQE */
#define CSE_INT         20      /* PIRQE */
#define IUNIT_INT       21      /* PIRQF */
#define PIRQF_INT       21
#define PIRQG_INT       22
#define PUNIT_INT       24
#define AUDIO_INT       25
#define ISH_INT         26
#define I2C0_INT        27
#define I2C1_INT        28
#define I2C2_INT        29
#define I2C3_INT        30
#define I2C4_INT        31
#define I2C5_INT        32
#define I2C6_INT        33
#define I2C7_INT        34
#define SPI0_INT        35
#define SPI1_INT        36
#define SPI2_INT        37
#define UFS_INT         38
#define EMMC_INT        39
#define PMC_INT         40
#define SDIO_INT        42
#define ESPI_INT        43
#define CNVI_INT        44

#endif /* _SOC_IRQ_H_ */
