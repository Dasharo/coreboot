package mtl

import (
	"strings"
	"fmt"

	"review.coreboot.org/coreboot.git/util/intelp2m/platforms/common"
	"review.coreboot.org/coreboot.git/util/intelp2m/platforms/snr"
	"review.coreboot.org/coreboot.git/util/intelp2m/platforms/cnl"
	"review.coreboot.org/coreboot.git/util/intelp2m/config"
	"review.coreboot.org/coreboot.git/util/intelp2m/fields"
)

const (
	PAD_CFG_DW0_RO_FIELDS = (0x1 << 27) | (0x1 << 24) | (0x3 << 21) | (0xf << 16) | 0xfc
	PAD_CFG_DW1_RO_FIELDS = 0xfdffc3ff
)

const (
	PAD_CFG_DW0 = common.PAD_CFG_DW0
	PAD_CFG_DW1 = common.PAD_CFG_DW1
	MAX_DW_NUM  = common.MAX_DW_NUM
)

type InheritanceMacro interface {
	Pull()
	GpiMacroAdd()
	GpoMacroAdd()
	NativeFunctionMacroAdd()
	NoConnMacroAdd()
}

type PlatformSpecific struct {
	InheritanceMacro
}

// RemmapRstSrc - remmap Pad Reset Source Config
func (PlatformSpecific) RemmapRstSrc() {
	macro := common.GetMacro()
	if config.TemplateGet() != config.TempInteltool {
		// Use reset source remapping only if the input file is inteltool.log dump
		return
	}
	if strings.Contains(macro.PadIdGet(), "GPD") {
		// See reset map for the MeteorLake GPD group at
                // https://github.com/coreboot/coreboot/blob/master/src/soc/intel/meteorlake/gpio.c#L10
                // remmap is not required because it is the same as common.
		return
	}

	dw0 := macro.Register(PAD_CFG_DW0)
	var remapping = map[uint8]uint32{
		0: common.RST_RSMRST << common.PadRstCfgShift,
		1: common.RST_DEEP   << common.PadRstCfgShift,
		2: common.RST_PLTRST << common.PadRstCfgShift,
		3: common.RST_PWROK  << common.PadRstCfgShift,
	}
	resetsrc, valid := remapping[dw0.GetResetConfig()]
	if valid {
		// dw0.SetResetConfig(resetsrc)
		ResetConfigFieldVal := (dw0.ValueGet() & 0x3fffffff) | remapping[dw0.GetResetConfig()]
		dw0.ValueSet(ResetConfigFieldVal)
	} else {
		fmt.Println("Invalid Pad Reset Config [ 0x", resetsrc ," ] for ", macro.PadIdGet())
	}
	dw0.CntrMaskFieldsClear(common.PadRstCfgMask)
}

// Adds The Pad Termination (TERM) parameter from PAD_CFG_DW1 to the macro
// as a new argument
func (platform PlatformSpecific) Pull() {
	platform.InheritanceMacro.Pull()
}

// Adds PAD_CFG_GPI macro with arguments
func (platform PlatformSpecific) GpiMacroAdd() {
	platform.InheritanceMacro.GpiMacroAdd()
}

// Adds PAD_CFG_GPO macro with arguments
func (platform PlatformSpecific) GpoMacroAdd() {
	platform.InheritanceMacro.GpoMacroAdd()
}

// Adds PAD_CFG_NF macro with arguments
func (platform PlatformSpecific) NativeFunctionMacroAdd() {
	platform.InheritanceMacro.NativeFunctionMacroAdd()
}

// Adds PAD_NC macro
func (platform PlatformSpecific) NoConnMacroAdd() {
	platform.InheritanceMacro.NoConnMacroAdd()
}


// GenMacro - generate pad macro
// dw0 : DW0 config register value
// dw1 : DW1 config register value
// return: string of macro
//         error
func (PlatformSpecific) GenMacro(id string, dw0 uint32, dw1 uint32, ownership uint8) string {
	macro := common.GetInstanceMacro(
		PlatformSpecific{
			InheritanceMacro: cnl.PlatformSpecific{
				InheritanceMacro: snr.PlatformSpecific{},
			},
		},
		fields.InterfaceGet(),
	)
	macro.Clear()
	macro.Register(PAD_CFG_DW0).CntrMaskFieldsClear(common.AllFields)
	macro.Register(PAD_CFG_DW1).CntrMaskFieldsClear(common.AllFields)
	macro.PadIdSet(id).SetPadOwnership(ownership)
	macro.Register(PAD_CFG_DW0).ValueSet(dw0).ReadOnlyFieldsSet(PAD_CFG_DW0_RO_FIELDS)
	macro.Register(PAD_CFG_DW1).ValueSet(dw1).ReadOnlyFieldsSet(PAD_CFG_DW1_RO_FIELDS)
	return macro.Generate()
}
