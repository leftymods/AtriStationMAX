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
#define IS31FL3236_REG_SHUTDOWN		0x00
#define IS31FL3236_REG_PWM1		0x01
#define IS31FL3236_REG_PWM36		0x24
#define IS31FL3236_REG_UPDATE		0x25
#define IS31FL3236_REG_LED_CTRL1	0x26
#define IS31FL3236_REG_LED_CTRL2	0x27
#define IS31FL3236_REG_LED_CTRL3	0x28
#define IS31FL3236_REG_LED_CTRL4	0x29
#define IS31FL3236_REG_LED_CTRL5	0x2A
#define IS31FL3236_REG_GLOBAL_CTRL	0x4B

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

static int led_ring_write_reg(u8 addr, u8 reg, u8 val)
{
	int ret;
	u8 buf[2];

	buf[0] = reg;
	buf[1] = val;
	ret = i2c_write(addr, reg, 1, &val, 1);
	if (ret != 0)
		printf("LED Ring: FAIL write 0x%02X to reg 0x%02X @ 0x%02X (ret=%d)\n",
		       val, reg, addr, ret);
	else
		printf("LED Ring: OK write 0x%02X to reg 0x%02X @ 0x%02X\n",
		       val, reg, addr);
	udelay(200);
	return ret;
}

static void led_ring_init(void)
{
#ifdef CONFIG_LED_RING_SUPPORT
	int ret1, ret2, i;
	int controllers_found = 0;

	printf("LED Ring: IS31FL3236 initialization starting...\n");

	// Probe I2C devices
	ret1 = i2c_probe(IS31FL3236_ADDR1, LED_RING_I2C_BUS);
	ret2 = i2c_probe(IS31FL3236_ADDR2, LED_RING_I2C_BUS);
	printf("LED Ring: Probe addr 0x%02X=%d, addr 0x%02X=%d\n",
	       IS31FL3236_ADDR1, ret1, IS31FL3236_ADDR2, ret2);

	if (ret1 == 0) controllers_found++;
	if (ret2 == 0) controllers_found++;

	if (controllers_found == 0) {
		printf("LED Ring: ERROR - No controllers found on I2C bus %d!\n", LED_RING_I2C_BUS);
		return;
	}

	// Initialize each controller
	for (int c = 0; c < 2; c++) {
		u8 addr = (c == 0) ? IS31FL3236_ADDR1 : IS31FL3236_ADDR2;
		int ret = (c == 0) ? ret1 : ret2;

		if (ret != 0) {
			printf("LED Ring: Skipping controller @ 0x%02X (not present)\n", addr);
			continue;
		}

		printf("LED Ring: Initializing controller @ 0x%02X...\n", addr);

		// Step 1: Software shutdown (0x00 = shutdown mode)
		led_ring_write_reg(addr, IS31FL3236_REG_SHUTDOWN, 0x00);

		// Step 2: Enable all LED outputs (registers 0x26-0x2A)
		led_ring_write_reg(addr, IS31FL3236_REG_LED_CTRL1, 0xFF);
		led_ring_write_reg(addr, IS31FL3236_REG_LED_CTRL2, 0xFF);
		led_ring_write_reg(addr, IS31FL3236_REG_LED_CTRL3, 0xFF);
		led_ring_write_reg(addr, IS31FL3236_REG_LED_CTRL4, 0xFF);
		led_ring_write_reg(addr, IS31FL3236_REG_LED_CTRL5, 0x0F);

		// Step 3: Set global current control (0x4B - 0xFF = max)
		led_ring_write_reg(addr, IS31FL3236_REG_GLOBAL_CTRL, 0xFF);

		// Step 4: Initialize all PWM registers to 0 (off)
		for (i = 0; i < 36; i++) {
			u8 pwm_reg = IS31FL3236_REG_PWM1 + i;
			led_ring_write_reg(addr, pwm_reg, 0x00);
		}

		// Step 5: Update PWM registers
		led_ring_write_reg(addr, IS31FL3236_REG_UPDATE, 0x00);

		// Step 6: Exit shutdown mode (0x01 = normal mode)
		led_ring_write_reg(addr, IS31FL3236_REG_SHUTDOWN, 0x01);

		printf("LED Ring: Controller @ 0x%02X initialized successfully\n", addr);
	}

	// Turn on a test LED to indicate success
	if (ret1 == 0) {
		led_ring_write_reg(IS31FL3236_ADDR1, IS31FL3236_REG_PWM1, 0x80);
		led_ring_write_reg(IS31FL3236_ADDR1, IS31FL3236_REG_UPDATE, 0x00);
		printf("LED Ring: TEST - LED 0 on @ 0x%02X (PWM 0x80 = 50%%)\n", IS31FL3236_ADDR1);
	}

	printf("LED Ring: Init done. Found %d/2 controllers (0x%02X=%s, 0x%02X=%s)\n",
	       controllers_found, IS31FL3236_ADDR1, ret1 == 0 ? "OK" : "FAIL",
	       IS31FL3236_ADDR2, ret2 == 0 ? "OK" : "FAIL");
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
