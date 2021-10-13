# Nvidia GC6 entry/exit flow

## Entry - Power off

1. Save LREN value to LTRE
1. Disable PCIe link
1. Set _STA to 0x5

## Exit - Power on

1. Enable PCIe link
1. Wait until GC6_FB_EN asserts
1. Restore LREN value from LTRE
1. Store 1 in CEDR (What is this?)
1. Assert GPU_EVENT
1. Wait for GC6_FB_EN to deassert
1. De-assert GPU_EVENT
1. Wait for GPU to power up (LNKS must be >= 7)
1. Set _STA to 0xF
