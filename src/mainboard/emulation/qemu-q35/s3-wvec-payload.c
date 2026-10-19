/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Tiny qemu-q35 payload that programs FACS.firmware_waking_vector like an OS
 * would, then halts. s3-verify.sh triggers S3 via PM1_CNT; on wake coreboot
 * should jump to the stub which prints Q35VEC to COM1.
 *
 * Place the stub below DCACHE_RAM_BASE (0x10000). QEMU CAR is DRAM, and
 * bootblock zeros the allocated CAR window on every reset including S3.
 *
 * Build:
 *   gcc -m32 -nostdlib -fno-pic -fno-stack-protector -static \
 *     -Wl,-T,s3-wvec.ld -Wl,--build-id=none -s \
 *     -o s3-wvec.elf s3-wvec-payload.c
 */

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

/* 0x1000 => real-mode 0100:0000. Must stay below qemu-q35 CAR at 0x10000. */
#define STUB_ADDR 0x00001000u

static void outb(u16 port, u8 val)
{
	__asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static void serial_puts(const char *s)
{
	for (; *s; s++)
		outb(0x3f8, (u8)*s);
}

static int sig4(const void *p, const char *s)
{
	const u8 *a = p;
	const u8 *b = (const u8 *)s;
	return a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3];
}

static int sig8(const void *p, const char *s)
{
	const u8 *a = p;
	const u8 *b = (const u8 *)s;
	int i;

	for (i = 0; i < 8; i++) {
		if (a[i] != b[i])
			return 0;
	}
	return 1;
}

/* 16-bit real-mode stub at STUB_ADDR (vector 0x1000 => seg:off 0100:0000). */
static void install_stub(void)
{
	u8 *p = (u8 *)STUB_ADDR;
	/* cli */
	p[0] = 0xfa;
	/* mov dx, 0x3f8 */
	p[1] = 0xba;
	p[2] = 0xf8;
	p[3] = 0x03;
	/* mov al, 'Q' ; out dx, al */
	p[4] = 0xb0;
	p[5] = 'Q';
	p[6] = 0xee;
	p[7] = 0xb0;
	p[8] = '3';
	p[9] = 0xee;
	p[10] = 0xb0;
	p[11] = '5';
	p[12] = 0xee;
	p[13] = 0xb0;
	p[14] = 'V';
	p[15] = 0xee;
	p[16] = 0xb0;
	p[17] = 'E';
	p[18] = 0xee;
	p[19] = 0xb0;
	p[20] = 'C';
	p[21] = 0xee;
	/* hlt ; jmp $ */
	p[22] = 0xf4;
	p[23] = 0xeb;
	p[24] = 0xfe;
}

void _start(void)
{
	u8 *p;
	u8 *rsdp = 0;
	u32 rsdt_addr;
	u32 *rsdt;
	u32 length;
	u32 n;
	u32 i;
	u32 fadt = 0;
	u32 facs = 0;
	u32 *wvec;

	serial_puts("S3-PAYLOAD: looking for FACS\n");

	for (p = (u8 *)0xe0000; p < (u8 *)0xfffff; p += 16) {
		if (sig8(p, "RSD PTR ")) {
			rsdp = p;
			break;
		}
	}
	if (!rsdp) {
		serial_puts("S3-PAYLOAD: no RSDP\n");
		goto halt;
	}

	rsdt_addr = *(u32 *)(rsdp + 16);
	if (!rsdt_addr) {
		serial_puts("S3-PAYLOAD: no RSDT\n");
		goto halt;
	}
	rsdt = (u32 *)rsdt_addr;
	if (!sig4(rsdt, "RSDT")) {
		serial_puts("S3-PAYLOAD: bad RSDT\n");
		goto halt;
	}
	length = rsdt[1];
	n = (length - 36) / 4;
	for (i = 0; i < n; i++) {
		u8 *t = (u8 *)rsdt[9 + i]; /* 36/4 = 9 */
		if (sig4(t, "FACP")) {
			fadt = (u32)t;
			break;
		}
	}
	if (!fadt) {
		serial_puts("S3-PAYLOAD: no FADT\n");
		goto halt;
	}

	/* firmware_ctrl at FADT+36 */
	facs = *(u32 *)(fadt + 36);
	if (!facs) {
		serial_puts("S3-PAYLOAD: no FACS ptr\n");
		goto halt;
	}
	if (!sig4((void *)facs, "FACS")) {
		serial_puts("S3-PAYLOAD: bad FACS\n");
		goto halt;
	}

	install_stub();
	wvec = (u32 *)(facs + 12);
	*wvec = STUB_ADDR;
	serial_puts("S3-PAYLOAD: programmed\n");

halt:
	for (;;)
		__asm__ volatile("hlt");
}
