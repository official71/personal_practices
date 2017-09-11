/*
 * Copyright (C) ST-Ericsson SA 2010
 *
 * License Terms: GNU General Public License v2
 *
 * Authors: Sundar Iyer <sundar.iyer@stericsson.com> for ST-Ericsson
 *          Bengt Jonsson <bengt.g.jonsson@stericsson.com> for ST-Ericsson
 *          Daniel Willerud <daniel.willerud@stericsson.com> for ST-Ericsson
 */

#ifndef __LINUX_MFD_AB8500_REGULATOR_H
#define __LINUX_MFD_AB8500_REGULATOR_H

#include <linux/platform_device.h>

/* AB8500 regulators */
enum ab8500_regulator_id {
	AB8500_LDO_AUX1,
	AB8500_LDO_AUX2,
	AB8500_LDO_AUX3,
	AB8500_LDO_INTCORE,
	AB8500_LDO_TVOUT,
	AB8500_LDO_AUDIO,
	AB8500_LDO_ANAMIC1,
	AB8500_LDO_ANAMIC2,
	AB8500_LDO_DMIC,
	AB8500_LDO_ANA,
	AB8500_NUM_REGULATORS,
};

/* AB8505 regulators */
enum ab8505_regulator_id {
	AB8505_LDO_AUX1,
	AB8505_LDO_AUX2,
	AB8505_LDO_AUX3,
	AB8505_LDO_AUX4,
	AB8505_LDO_AUX5,
	AB8505_LDO_AUX6,
	AB8505_LDO_INTCORE,
	AB8505_LDO_ADC,
	AB8505_LDO_USB,
	AB8505_LDO_AUDIO,
	AB8505_LDO_ANAMIC1,
	AB8505_LDO_ANAMIC2,
	AB8505_LDO_AUX8,
	AB8505_LDO_ANA,
	AB8505_SYSCLKREQ_2,
	AB8505_SYSCLKREQ_4,
	AB8505_NUM_REGULATORS,
};

/* AB9540 regulators */
enum ab9540_regulator_id {
	AB9540_LDO_AUX1,
	AB9540_LDO_AUX2,
	AB9540_LDO_AUX3,
	AB9540_LDO_AUX4,
	AB9540_LDO_INTCORE,
	AB9540_LDO_TVOUT,
	AB9540_LDO_USB,
	AB9540_LDO_AUDIO,
	AB9540_LDO_ANAMIC1,
	AB9540_LDO_ANAMIC2,
	AB9540_LDO_DMIC,
	AB9540_LDO_ANA,
	AB9540_SYSCLKREQ_2,
	AB9540_SYSCLKREQ_4,
	AB9540_NUM_REGULATORS,
};

/* AB8540 regulators */
enum ab8540_regulator_id {
	AB8540_LDO_AUX1,
	AB8540_LDO_AUX2,
	AB8540_LDO_AUX3,
	AB8540_LDO_AUX4,
	AB8540_LDO_AUX5,
	AB8540_LDO_AUX6,
	AB8540_LDO_INTCORE,
	AB8540_LDO_TVOUT,
	AB8540_LDO_AUDIO,
	AB8540_LDO_ANAMIC1,
	AB8540_LDO_ANAMIC2,
	AB8540_LDO_DMIC,
	AB8540_LDO_ANA,
	AB8540_LDO_SDIO,
	AB8540_SYSCLKREQ_2,
	AB8540_SYSCLKREQ_4,
	AB8540_NUM_REGULATORS,
};

/* AB8500, AB8505, and AB9540 register initialization */
struct ab8500_regulator_reg_init {
	int id;
	u8 mask;
	u8 value;
};

#define INIT_REGULATOR_REGISTER(_id, _mask, _value)	\
	{						\
		.id = _id,				\
		.mask = _mask,				\
		.value = _value,			\
	}

/* AB8500 registers */
enum ab8500_regulator_reg {
	AB8500_REGUREQUESTCTRL2,
	AB8500_REGUREQUESTCTRL3,
	AB8500_REGUREQUESTCTRL4,
	AB8500_REGUSYSCLKREQ1HPVALID1,
	AB8500_REGUSYSCLKREQ1HPVALID2,
	AB8500_REGUHWHPREQ1VALID1,
	AB8500_REGUHWHPREQ1VALID2,
	AB8500_REGUHWHPREQ2VALID1,
	AB8500_REGUHWHPREQ2VALID2,
	AB8500_REGUSWHPREQVALID1,
	AB8500_REGUSWHPREQVALID2,
	AB8500_REGUSYSCLKREQVALID1,
	AB8500_REGUSYSCLKREQVALID2,
	AB8500_REGUMISC1,
	AB8500_VAUDIOSUPPLY,
	AB8500_REGUCTRL1VAMIC,
	AB8500_VPLLVANAREGU,
	AB8500_VREFDDR,
	AB8500_EXTSUPPLYREGU,
	AB8500_VAUX12REGU,
	AB8500_VRF1VAUX3REGU,
	AB8500_VAUX1SEL,
	AB8500_VAUX2SEL,
	AB8500_VRF1VAUX3SEL,
	AB8500_REGUCTRL2SPARE,
	AB8500_REGUCTRLDISCH,
	AB8500_REGUCTRLDISCH2,
	AB8500_NUM_REGULATOR_REGISTERS,
};

/* AB8505 registers */
enum ab8505_regulator_reg {
	AB8505_REGUREQUESTCTRL1,
	AB8505_REGUREQUESTCTRL2,
	AB8505_REGUREQUESTCTRL3,
	AB8505_REGUREQUESTCTRL4,
	AB8505_REGUSYSCLKREQ1HPVALID1,
	AB8505_REGUSYSCLKREQ1HPVALID2,
	AB8505_REGUHWHPREQ1VALID1,
	AB8505_REGUHWHPREQ1VALID2,
	AB8505_REGUHWHPREQ2VALID1,
	AB8505_REGUHWHPREQ2VALID2,
	AB8505_REGUSWHPREQVALID1,
	AB8505_REGUSWHPREQVALID2,
	AB8505_REGUSYSCLKREQVALID1,
	AB8505_REGUSYSCLKREQVALID2,
	AB8505_REGUVAUX4REQVALID,
	AB8505_REGUMISC1,
	AB8505_VAUDIOSUPPLY,
	AB8505_REGUCTRL1VAMIC,
	AB8505_VSMPSAREGU,
	AB8505_VSMPSBREGU,
	AB8505_VSAFEREGU, /* NOTE! PRCMU register */
	AB8505_VPLLVANAREGU,
	AB8505_EXTSUPPLYREGU,
	AB8505_VAUX12REGU,
	AB8505_VRF1VAUX3REGU,
	AB8505_VSMPSASEL1,
	AB8505_VSMPSASEL2,
	AB8505_VSMPSASEL3,
	AB8505_VSMPSBSEL1,
	AB8505_VSMPSBSEL2,
	AB8505_VSMPSBSEL3,
	AB8505_VSAFESEL1, /* NOTE! PRCMU register */
	AB8505_VSAFESEL2, /* NOTE! PRCMU register */
	AB8505_VSAFESEL3, /* NOTE! PRCMU register */
	AB8505_VAUX1SEL,
	AB8505_VAUX2SEL,
	AB8505_VRF1VAUX3SEL,
	AB8505_VAUX4REQCTRL,
	AB8505_VAUX4REGU,
	AB8505_VAUX4SEL,
	AB8505_REGUCTRLDISCH,
	AB8505_REGUCTRLDISCH2,
	AB8505_REGUCTRLDISCH3,
	AB8505_CTRLVAUX5,
	AB8505_CTRLVAUX6,
	AB8505_NUM_REGULATOR_REGISTERS,
};

/* AB9540 registers */
enum ab9540_regulator_reg {
	AB9540_REGUREQUESTCTRL1,
	AB9540_REGUREQUESTCTRL2,
	AB9540_REGUREQUESTCTRL3,
	AB9540_REGUREQUESTCTRL4,
	AB9540_REGUSYSCLKREQ1HPVALID1,
	AB9540_REGUSYSCLKREQ1HPVALID2,
	AB9540_REGUHWHPREQ1VALID1,
	AB9540_REGUHWHPREQ1VALID2,
	AB9540_REGUHWHPREQ2VALID1,
	AB9540_REGUHWHPREQ2VALID2,
	AB9540_REGUSWHPREQVALID1,
	AB9540_REGUSWHPREQVALID2,
	AB9540_REGUSYSCLKREQVALID1,
	AB9540_REGUSYSCLKREQVALID2,
	AB9540_REGUVAUX4REQVALID,
	AB9540_REGUMISC1,
	AB9540_VAUDIOSUPPLY,
	AB9540_REGUCTRL1VAMIC,
	AB9540_VSMPS1REGU,
	AB9540_VSMPS2REGU,
	AB9540_VSMPS3REGU, /* NOTE! PRCMU register */
	AB9540_VPLLVANAREGU,
	AB9540_EXTSUPPLYREGU,
	AB9540_VAUX12REGU,
	AB9540_VRF1VAUX3REGU,
	AB9540_VSMPS1SEL1,
	AB9540_VSMPS1SEL2,
	AB9540_VSMPS1SEL3,
	AB9540_VSMPS2SEL1,
	AB9540_VSMPS2SEL2,
	AB9540_VSMPS2SEL3,
	AB9540_VSMPS3SEL1, /* NOTE! PRCMU register */
	AB9540_VSMPS3SEL2, /* NOTE! PRCMU register */
	AB9540_VAUX1SEL,
	AB9540_VAUX2SEL,
	AB9540_VRF1VAUX3SEL,
	AB9540_REGUCTRL2SPARE,
	AB9540_VAUX4REQCTRL,
	AB9540_VAUX4REGU,
	AB9540_VAUX4SEL,
	AB9540_REGUCTRLDISCH,
	AB9540_REGUCTRLDISCH2,
	AB9540_REGUCTRLDISCH3,
	AB9540_NUM_REGULATOR_REGISTERS,
};

/* AB8540 registers */
enum ab8540_regulator_reg {
	AB8540_REGUREQUESTCTRL1,
	AB8540_REGUREQUESTCTRL2,
	AB8540_REGUREQUESTCTRL3,
	AB8540_REGUREQUESTCTRL4,
	AB8540_REGUSYSCLKREQ1HPVALID1,
	AB8540_REGUSYSCLKREQ1HPVALID2,
	AB8540_REGUHWHPREQ1VALID1,
	AB8540_REGUHWHPREQ1VALID2,
	AB8540_REGUHWHPREQ2VALID1,
	AB8540_REGUHWHPREQ2VALID2,
	AB8540_REGUSWHPREQVALID1,
	AB8540_REGUSWHPREQVALID2,
	AB8540_REGUSYSCLKREQVALID1,
	AB8540_REGUSYSCLKREQVALID2,
	AB8540_REGUVAUX4REQVALID,
	AB8540_REGUVAUX5REQVALID,
	AB8540_REGUVAUX6REQVALID,
	AB8540_REGUVCLKBREQVALID,
	AB8540_REGUVRF1REQVALID,
	AB8540_REGUMISC1,
	AB8540_VAUDIOSUPPLY,
	AB8540_REGUCTRL1VAMIC,
	AB8540_VHSIC,
	AB8540_VSDIO,
	AB8540_VSMPS1REGU,
	AB8540_VSMPS2REGU,
	AB8540_VSMPS3REGU,
	AB8540_VPLLVANAREGU,
	AB8540_EXTSUPPLYREGU,
	AB8540_VAUX12REGU,
	AB8540_VRF1VAUX3REGU,
	AB8540_VSMPS1SEL1,
	AB8540_VSMPS1SEL2,
	AB8540_VSMPS1SEL3,
	AB8540_VSMPS2SEL1,
	AB8540_VSMPS2SEL2,
	AB8540_VSMPS2SEL3,
	AB8540_VSMPS3SEL1,
	AB8540_VSMPS3SEL2,
	AB8540_VAUX1SEL,
	AB8540_VAUX2SEL,
	AB8540_VRF1VAUX3SEL,
	AB8540_REGUCTRL2SPARE,
	AB8540_VAUX4REQCTRL,
	AB8540_VAUX4REGU,
	AB8540_VAUX4SEL,
	AB8540_VAUX5REQCTRL,
	AB8540_VAUX5REGU,
	AB8540_VAUX5SEL,
	AB8540_VAUX6REQCTRL,
	AB8540_VAUX6REGU,
	AB8540_VAUX6SEL,
	AB8540_VCLKBREQCTRL,
	AB8540_VCLKBREGU,
	AB8540_VCLKBSEL,
	AB8540_VRF1REQCTRL,
	AB8540_REGUCTRLDISCH,
	AB8540_REGUCTRLDISCH2,
	AB8540_REGUCTRLDISCH3,
	AB8540_REGUCTRLDISCH4,
	AB8540_VSIMSYSCLKCTRL,
	AB8540_VANAVPLLSEL,
	AB8540_NUM_REGULATOR_REGISTERS,
};

/* AB8500 external regulators */
struct ab8500_ext_regulator_cfg {
	bool hwreq; /* requires hw mode or high power mode */
};

enum ab8500_ext_regulator_id {
	AB8500_EXT_SUPPLY1,
	AB8500_EXT_SUPPLY2,
	AB8500_EXT_SUPPLY3,
	AB8500_NUM_EXT_REGULATORS,
};

/* AB8500 regulator platform data */
struct ab8500_regulator_platform_data {
	int num_reg_init;
	struct ab8500_regulator_reg_init *reg_init;
	int num_regulator;
	struct regulator_init_data *regulator;
	int num_ext_regulator;
	struct regulator_init_data *ext_regulator;
};

#ifdef CONFIG_REGULATOR_AB8500_DEBUG
int ab8500_regulator_debug_init(struct platform_device *pdev);
int ab8500_regulator_debug_exit(struct platform_device *pdev);
#else
static inline int ab8500_regulator_debug_init(struct platform_device *pdev)
{
	return 0;
}
static inline int ab8500_regulator_debug_exit(struct platform_device *pdev)
{
	return 0;
}
#endif

/* AB8500 external regulator functions. */
int ab8500_ext_regulator_init(struct platform_device *pdev);
void ab8500_ext_regulator_exit(struct platform_device *pdev);

#endif
