#ifndef CPU_AMD_MICROCODE_H
#define CPU_AMD_MICROCODE_H

void amd_update_microcode_from_cbfs(void);
void update_microcode(u32 cpu_deviceid);

#endif /* CPU_AMD_MICROCODE_H */
