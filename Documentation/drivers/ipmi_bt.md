# IPMI BT driver

The driver can be found in `src/drivers/ipmi/` (same as KCS). It works with BMC
that provide a BT I/O interface as specified in the [IPMI] standard.

The driver detects the IPMI version and reserves the I/O space in coreboot's
resource allocator.

## For developers

To use the driver, select the `IPMI_BT` Kconfig and add the following PNP
device (in example for the BT at 0xe4):

```
 chip drivers/ipmi
   device pnp e4.0 on end         # IPMI BT
 end
```

**Note:** The I/O base address needs to be aligned to 4.

The following registers can be set:

* `wait_for_bmc`
  * Boolean
  * Wait for BMC to boot. This can be used if the BMC takes a long time to boot
    after PoR.
* `bmc_boot_timeout`
  * Integer
  * The timeout in seconds to wait for the IPMI service to be loaded.
    Will be used if wait_for_bmc is true.


[IPMI]: https://www.intel.com/content/dam/www/public/us/en/documents/product-briefs/ipmi-second-gen-interface-spec-v2-rev1-1.pdf
