/*
 * board/amlogic/atr_01_v1/atr_01_v1.c
 *
 * Copyright (C) 2025 leftymods. All rights reserved.
 *
 * ATR-01 (AtriStation) board support for Amlogic SM1/S905X3
 * Features: eMMC only (no NAND), LED Ring (IS31FL3236), Smart Speaker
 */

#include <common.h>
#include <malloc.h>
#include <errno.h>
#include <environment.h>
#include <fdt_support.h>
#include <libfdt.h>
#include <asm/cpu_id.h>
#include <asm/arch/secure_apb.h>
#ifdef CONFIG_SYS_I2C_AML
#include <aml_i2c.h>
#endif
#ifdef CONFIG_SYS_I2C_MESON
#include <amlogic/i2c.h>
#endif
#include <dm.h>
#ifdef CONFIG_AML_VPU
#include <vpu.h>
#endif
#include <vpp.h>
#ifdef CONFIG_AML_V2_FACTORY_BURN
#include <amlogic/aml_v2_burning.h>
#endif
#ifdef CONFIG_AML_HDMITX20
#include <amlogic/hdmi.h>
#endif
#ifdef CONFIG_AML_LCD
#include <amlogic/aml_lcd.h>
#endif
#include <asm/arch/eth_setup.h>
#include <phy.h>
#include <linux/sizes.h>
#include <asm-generic/gpio.h>
#include <dm.h>
#ifdef CONFIG_AML_SPIFC
#include <amlogic/spifc.h>
#endif
#ifdef CONFIG_AML_SPICC
#include <amlogic/spicc.h>
#endif

DECLARE_GLOBAL_DATA_PTR;

// LED Ring IS31FL3236 I2C addresses
#define IS31FL3236_ADDR1 0x3C
#define IS31FL3236_ADDR2 0x3F
#define LED_RING_I2C_BUS 0

// LED Ring registers
#define IS31FL3236_REG_SHUTDOWN 0x00
#define IS31FL3236_REG_PWM1 0x01
#define IS31FL3236_REG_UPDATE 0x25
#define IS31FL3236_REG_GLOBAL_CTRL 0x4A

int serial_set_pin_port(unsigned long port_base)
{
	return 0;
}

int dram_init(void)
{
	gd->ram_size = PHYS_SDRAM_1_SIZE;
	return 0;
}

void secondary_boot_func(void)
{
}

#ifdef ETHERNET_INTERNAL_PHY
void internalPhyConfig(struct phy_device *phydev)
{
}

static int dwmac_meson_cfg_pll(void)
{
	writel(0x39C0040A, P_ETH_PLL_CTL0);
	writel(0x927C0000, P_ETH_PLL_CTL1);
	writel(0x827D0000, P_ETH_PLL_CTL2);
	writel(0xD7930000, P_ETH_PLL_CTL3);
	writel(0x02140000, P_ETH_PLL_CTL4);
	writel(0x727D0000, P_ETH_PLL_CTL5);
	writel(0x927C0000, P_ETH_PLL_CTL6);
	writel(0x00140000, P_ETH_PLL_CTL7);
	return 0;
}

static int dwmac_meson_cfg_io(void)
{
	setbits_le32(P_PAD_GPIO_BANK_0_REG5, 0xff);
	setbits_le32(P_PAD_GPIO_BANK_0_REG4, 0xff);
	setbits_le32(P_PAD_GPIO_BANK_0_REG3, 0xff);
	return 0;
}

static int dwmac_meson_cfg_gpios(void)
{
	writel(readl(P_PERIPHS_PIN_MUX_2) | (1 << 0), P_PERIPHS_PIN_MUX_2);
	writel(readl(P_PERIPHS_PIN_MUX_6) | (1 << 28), P_PERIPHS_PIN_MUX_6);
	return 0;
}

static void setup_net_chip(void)
{
	eth_aml_reg0_t eth_cus_value;

	eth_cus_value.d32 = 0;
	eth_cus_value.b.phy_intf_sel = 0;
	eth_cus_value.b.rx_delay = 0xf;
	eth_cus_value.b.tx_delay = 0;
	eth_cus_value.b.gmode = 1;
	eth_cus_value.b.rgmii_mode = 0;
	eth_cus_value.b.rgmii_mode_en = 1;
	eth_cus_value.b.adj_enable = 1;
	eth_cus_value.b.calib_enable = 1;
	eth_cus_value.b.calib_start = 0;
	eth_cus_value.b.adj_setup = 0;
	eth_cus_value.b.adj_delay = 0xf;
	eth_cus_value.b.adj_skew = 0;
	eth_cus_value.b.cali_start = 0;
	eth_cus_value.b.cali_rise = 0;
	eth_cus_value.b.cali_sel = 0;
	eth_cus_value.b.rgmii_clk_sel = 0;
	eth_cus_value.b.phy_clk_sel = 0;
	eth_cus_value.b.phy_clk_sel_en = 0;
	eth_cus_value.b.phy_clk_27m = 1;
	eth_cus_value.b.phy_clk_out = 0;
	eth_cus_value.b.phy_clk_out_en = 0;
	eth_cus_value.b.phy_rxclk_sel = 0;
	eth_cus_value.b.phy_rxclk_sel_en = 0;
	eth_cus_value.b.phy_rxclk_inv = 0;

	writel(eth_cus_value.d32, P_ETH_CNTL0);
}

int board_eth_init(bd_t *bis)
{
	setup_net_chip();
	dwmac_meson_cfg_io();
	dwmac_meson_cfg_gpios();
	dwmac_meson_cfg_pll();

	u32 reg = readl(P_ETH_CNTL0);
	reg |= (1 << 0);
	writel(reg, P_ETH_CNTL0);
	udelay(10);
	writel(reg & ~(1 << 0), P_ETH_CNTL0);

	extern int dwc_eth_init(int idx, void *base, int *flags);
	dwc_eth_init(0, (void *)(ETH_BASE), NULL);
	return 0;
}
#endif

#ifdef CONFIG_AML_I2C
void board_i2c_init(void)
{
	// I2C_A initialization for LED Ring
	// Configure I2C_A SDA/SCL pins
	writel(readl(P_PAD_GPIO_BANK_0_REG4) | (0x3 << 2), P_PAD_GPIO_BANK_0_REG4);
}
#endif

static void led_ring_init(void)
{
	#ifdef CONFIG_LED_RING_SUPPORT
	// Initialize IS31FL3236 LED controllers
	u8 data[2];

	// Enable LED controller 1 @ 0x3C
	data[0] = IS31FL3236_REG_SHUTDOWN;
	data[1] = 0x01; // Normal operation
	i2c_probe(IS31FL3236_ADDR1, LED_RING_I2C_BUS);
	i2c_write(IS31FL3236_ADDR1, IS31FL3236_REG_SHUTDOWN, 1, &data[1], 1);

	// Enable LED controller 2 @ 0x3F
	data[0] = IS31FL3236_REG_SHUTDOWN;
	data[1] = 0x01; // Normal operation
	i2c_probe(IS31FL3236_ADDR2, LED_RING_I2C_BUS);
	i2c_write(IS31FL3236_ADDR2, IS31FL3236_REG_SHUTDOWN, 1, &data[1], 1);

	// Set global control (brightness)
	data[0] = IS31FL3236_REG_GLOBAL_CTRL;
	data[1] = 0xFF; // Max brightness
	i2c_write(IS31FL3236_ADDR1, IS31FL3236_REG_GLOBAL_CTRL, 1, &data[1], 1);

	// Initialize all PWM registers to 0 (LEDs off)
	for (int i = 0; i < 36; i++) {
		data[0] = IS31FL3236_REG_PWM1 + i;
		data[1] = 0x00;
		i2c_write(IS31FL3236_ADDR1, data[0], 1, &data[1], 1);
	}

	printf("LED Ring: Initialized IS31FL3236 controllers @ 0x%02X and 0x%02X\n",
		IS31FL3236_ADDR1, IS31FL3236_ADDR2);
	#endif
}

static void board_led_ring_enable(void)
{
	printf("ATR-01: Initializing LED Ring...\n");
	led_ring_init();
}

int board_init(void)
{
	// Configure board-specific hardware
	printf("ATR-01 Board (AtriStation) Initialization\n");

	// Initialize I2C bus (for LED Ring and other peripherals)
#ifdef CONFIG_AML_I2C
	board_i2c_init();
#endif

	// Initialize LED Ring
	board_led_ring_enable();

	// Configure system control registers
	__raw_writel(0x999, P_AO_RTI_PIN_MUX_REG);

	// Power on VPU
	WRITE_VREG(VPU_MEM_PD_REG0, 0x7fffff);
	WRITE_VREG(VPU_MEM_PD_REG1, 0xffff);
	WRITE_VREG(VPU_VCBUS_MEM_PD_REG2, 0xffffffff);
	WRITE_VREG(VPU_VCBUS_MEM_PD_REG3, 0xffffffff);
	WRITE_VREG(VPU_VCBUS_MEM_PD_REG4, 0xffffffff);

	printf("ATR-01 Board init done\n");
	return 0;
}

int misc_init_r(void)
{
	printf("misc_init_r\n");
	return 0;
}

int board_late_init(void)
{
	printf("board_late_init\n");

	// Set board-specific environment variables
	setenv("board_name", "atr_01_v1");
	setenv("device_model", "AtriStation");
	setenv("vendor", "leftymods");

	return 0;
}

u32 get_board_signature(void)
{
	return 0;
}

int do_load_aml_dtb(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	return 0;
}

int board_recover(void)
{
	printf("board_recover called\n");
	return 0;
}

int do_get_cpu_temp(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	return 0;
}
