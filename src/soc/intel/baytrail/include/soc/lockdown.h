#include <stdbool.h>


void platform_lockdown_config(void *unused);
void enable_smm_bwp(void);
bool wpd_status(void);
void disable_smm_bwp(void);
