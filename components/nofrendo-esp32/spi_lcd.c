// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <string.h>
#include <stdio.h>
#include "sdkconfig.h"
#include "rom/ets_sys.h"
#include "rom/gpio.h"
#include "soc/gpio_reg.h"
#include "soc/gpio_sig_map.h"
#include "soc/gpio_struct.h"
#include "soc/io_mux_reg.h"
#include "soc/spi_reg.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/periph_ctrl.h"
#include "spi_lcd.h"

#define LCD_WIDTH       160
#define LCD_HEIGHT      128

#define SPI_NUM         0x3

#define PIN_NUM_MOSI    CONFIG_HW_LCD_MOSI_GPIO
#define PIN_NUM_CLK     CONFIG_HW_LCD_CLK_GPIO
#define PIN_NUM_CS      CONFIG_HW_LCD_CS_GPIO
#define PIN_NUM_DC      CONFIG_HW_LCD_DC_GPIO
#define PIN_NUM_RST     CONFIG_HW_LCD_RESET_GPIO
#define PIN_NUM_BCKL    CONFIG_HW_LCD_BL_GPIO

#define LCD_SEL_CMD()   GPIO.out_w1tc = (1 << PIN_NUM_DC) // Low to send command
#define LCD_SEL_DATA()  GPIO.out_w1ts = (1 << PIN_NUM_DC) // High to send data
#define LCD_RST_SET()   GPIO.out_w1ts = (1 << PIN_NUM_RST)
#define LCD_RST_CLR()   GPIO.out_w1tc = (1 << PIN_NUM_RST)

#if CONFIG_HW_INV_BL
#define LCD_BKG_ON()    GPIO.out_w1tc = (1 << PIN_NUM_BCKL) // Backlight ON
#define LCD_BKG_OFF()   GPIO.out_w1ts = (1 << PIN_NUM_BCKL) //Backlight OFF
#else
#define LCD_BKG_ON()    GPIO.out_w1ts = (1 << PIN_NUM_BCKL) // Backlight ON
#define LCD_BKG_OFF()   GPIO.out_w1tc = (1 << PIN_NUM_BCKL) //Backlight OFF
#endif

#define ST7735_NOP      0x00
#define ST7735_SWRESET  0x01
#define ST7735_RDDID    0x04
#define ST7735_RDDST    0x09

#define ST7735_SLPIN    0x10
#define ST7735_SLPOUT   0x11
#define ST7735_PTLON    0x12
#define ST7735_NORON    0x13

#define ST7735_INVOFF   0x20
#define ST7735_INVON    0x21
#define ST7735_DISPOFF  0x28
#define ST7735_DISPON   0x29
#define ST7735_CASET    0x2A
#define ST7735_RASET    0x2B
#define ST7735_RAMWR    0x2C
#define ST7735_RAMRD    0x2E

#define ST7735_PTLAR    0x30
#define ST7735_COLMOD   0x3A
#define ST7735_MADCTL   0x36

#define ST7735_FRMCTR1  0xB1
#define ST7735_FRMCTR2  0xB2
#define ST7735_FRMCTR3  0xB3
#define ST7735_INVCTR   0xB4
#define ST7735_DISSET5  0xB6

#define ST7735_PWCTR1   0xC0
#define ST7735_PWCTR2   0xC1
#define ST7735_PWCTR3   0xC2
#define ST7735_PWCTR4   0xC3
#define ST7735_PWCTR5   0xC4
#define ST7735_VMCTR1   0xC5

#define ST7735_RDID1    0xDA
#define ST7735_RDID2    0xDB
#define ST7735_RDID3    0xDC
#define ST7735_RDID4    0xDD

#define ST7735_PWCTR6   0xFC

#define ST7735_GMCTRP1  0xE0
#define ST7735_GMCTRN1  0xE1

#define MADCTL_MY       0x80
#define MADCTL_MX       0x40
#define MADCTL_MV       0x20
#define MADCTL_ML       0x10
#define MADCTL_RGB      0x00
#define MADCTL_BGR      0x08
#define MADCTL_MH       0x04

static void spi_write_byte(const uint8_t data){
    SET_PERI_REG_BITS(SPI_MOSI_DLEN_REG(SPI_NUM), SPI_USR_MOSI_DBITLEN, 0x7, SPI_USR_MOSI_DBITLEN_S);
    WRITE_PERI_REG((SPI_W0_REG(SPI_NUM)), data);
    SET_PERI_REG_MASK(SPI_CMD_REG(SPI_NUM), SPI_USR);
    while (READ_PERI_REG(SPI_CMD_REG(SPI_NUM))&SPI_USR);
}

static void LCD_WriteCommand(const uint8_t cmd)
{
    LCD_SEL_CMD();
    spi_write_byte(cmd);
}

static void LCD_WriteData(const uint8_t data)
{
    LCD_SEL_DATA();
    spi_write_byte(data);
}

static void  ILI9341_INITIAL ()
{
    LCD_BKG_ON();
    //------------------------------------Reset Sequence-----------------------------------------//

    LCD_RST_SET();
    ets_delay_us(100000);

    LCD_RST_CLR();
    ets_delay_us(200000);

    LCD_RST_SET();
    ets_delay_us(200000);

    // Software Reset + delay 50ms
    LCD_WriteCommand(ST7735_SWRESET);
    ets_delay_us(200000);
    // Out of sleep mode + delay 500ms
    LCD_WriteCommand(ST7735_SLPOUT);
    ets_delay_us(200000);
    // Frame Rate Control - normal mode
    LCD_WriteCommand(ST7735_FRMCTR1);
    LCD_WriteData(0x01);
    LCD_WriteData(0x2C);
    LCD_WriteData(0x2D);
    // Frame Rate Control - idle mode
    LCD_WriteCommand(ST7735_FRMCTR2);
    LCD_WriteData(0x01);
    LCD_WriteData(0x2C);
    LCD_WriteData(0x2D);
    // Frame Rate Control - paertial mode
    LCD_WriteCommand(ST7735_FRMCTR3);
    LCD_WriteData(0x01);
    LCD_WriteData(0x2C);
    LCD_WriteData(0x2D);
    LCD_WriteData(0x01);
    LCD_WriteData(0x2C);
    LCD_WriteData(0x2D);
    // Display inversion ctrl
    LCD_WriteCommand(ST7735_INVCTR);
    LCD_WriteData(0x07);
    // Power Control -4.6V AUTO mode
    LCD_WriteCommand(ST7735_PWCTR1);
    LCD_WriteData(0xA2);
    LCD_WriteData(0x02);
    LCD_WriteData(0x84);
    // Power Control VGH25 = 2.4C VGSEL = -10 VGH  = 3*AVDD
    LCD_WriteCommand(ST7735_PWCTR2);
    LCD_WriteData(0xC5);
    // Power Control Opamp current small Boost frequency
    LCD_WriteCommand(ST7735_PWCTR3);
    LCD_WriteData(0x0A);
    LCD_WriteData(0x00);
    // Power Control BCLK/2, Opamp current small & Medium low
    LCD_WriteCommand(ST7735_PWCTR4);
    LCD_WriteData(0x8A);
    LCD_WriteData(0x2A);
    // Power Control
    LCD_WriteCommand(ST7735_PWCTR5);
    LCD_WriteData(0x8A);
    LCD_WriteData(0xEE);
    // Power Control
    LCD_WriteCommand(ST7735_VMCTR1);
    LCD_WriteData(0x0E);
    // Don't invert display
    LCD_WriteCommand(ST7735_INVOFF);
    // Memory access control (directions) row addr/col addr, bottom to top refresh
    LCD_WriteCommand(ST7735_MADCTL);
    LCD_WriteData(MADCTL_MV | MADCTL_MX);
    // Set Color mode 16 bit color
    LCD_WriteCommand(ST7735_COLMOD);
    LCD_WriteData(0x05);
    ets_delay_us(100000);
    // Column addr set
    LCD_WriteCommand(ST7735_CASET);
    LCD_WriteData(0x00);
    LCD_WriteData(0x02);
    LCD_WriteData(0x00);
    LCD_WriteData(0x81);
    // Row addr set
    LCD_WriteCommand(ST7735_RASET);
    LCD_WriteData(0x00);
    LCD_WriteData(0x01);
    LCD_WriteData(0x00);
    LCD_WriteData(0xA0);
    // Magical unicorn dust
    LCD_WriteCommand(ST7735_GMCTRP1);
    LCD_WriteData(0x02);
    LCD_WriteData(0x1C);
    LCD_WriteData(0x07);
    LCD_WriteData(0x12);
    LCD_WriteData(0x37);
    LCD_WriteData(0x32);
    LCD_WriteData(0x29);
    LCD_WriteData(0x2D);
    LCD_WriteData(0x29);
    LCD_WriteData(0x25);
    LCD_WriteData(0x2B);
    LCD_WriteData(0x39);
    LCD_WriteData(0x00);
    LCD_WriteData(0x01);
    LCD_WriteData(0x03);
    LCD_WriteData(0x10);
    // Sparkles and rainbows
    LCD_WriteCommand(ST7735_GMCTRN1);
    LCD_WriteData(0x03);
    LCD_WriteData(0x1D);
    LCD_WriteData(0x07);
    LCD_WriteData(0x06);
    LCD_WriteData(0x2C);
    LCD_WriteData(0x2C);
    LCD_WriteData(0x29);
    LCD_WriteData(0x2D);
    LCD_WriteData(0x2E);
    LCD_WriteData(0x2E);
    LCD_WriteData(0x37);
    LCD_WriteData(0x3F);
    LCD_WriteData(0x00);
    LCD_WriteData(0x00);
    LCD_WriteData(0x02);
    LCD_WriteData(0x10);
    // Normal display on
    LCD_WriteCommand(ST7735_NORON);
    ets_delay_us(10000);
    // Main screen turn on
    LCD_WriteCommand(ST7735_DISPON);
    ets_delay_us(100000);
}
//.............LCD API END----------

static void ili_gpio_init()
{
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[PIN_NUM_DC], 2);
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[PIN_NUM_RST], 2);
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[PIN_NUM_BCKL], 2);
    WRITE_PERI_REG(GPIO_ENABLE_W1TS_REG, BIT(PIN_NUM_DC)|BIT(PIN_NUM_RST)|BIT(PIN_NUM_BCKL));
}

static void spi_master_init()
{
    periph_module_enable(PERIPH_VSPI_MODULE);
    periph_module_enable(PERIPH_SPI_DMA_MODULE);

    ets_printf("lcd spi pin mux init ...\r\n");
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[PIN_NUM_MOSI], 2);
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[PIN_NUM_CLK], 2);
    PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[PIN_NUM_CS], 2);
    WRITE_PERI_REG(GPIO_ENABLE_W1TS_REG, BIT(PIN_NUM_MOSI)|BIT(PIN_NUM_CLK)|BIT(PIN_NUM_CS));

    ets_printf("lcd spi signal init\r\n");
    gpio_matrix_out(PIN_NUM_MOSI, VSPID_OUT_IDX,0,0);
    gpio_matrix_out(PIN_NUM_CLK, VSPICLK_OUT_IDX,0,0);
    gpio_matrix_out(PIN_NUM_CS, VSPICS0_OUT_IDX,0,0);
    ets_printf("Hspi config\r\n");

    CLEAR_PERI_REG_MASK(SPI_SLAVE_REG(SPI_NUM), SPI_TRANS_DONE << 5);
    SET_PERI_REG_MASK(SPI_USER_REG(SPI_NUM), SPI_CS_SETUP);
    CLEAR_PERI_REG_MASK(SPI_PIN_REG(SPI_NUM), SPI_CK_IDLE_EDGE);
    CLEAR_PERI_REG_MASK(SPI_USER_REG(SPI_NUM),  SPI_CK_OUT_EDGE);
    CLEAR_PERI_REG_MASK(SPI_CTRL_REG(SPI_NUM), SPI_WR_BIT_ORDER);
    CLEAR_PERI_REG_MASK(SPI_CTRL_REG(SPI_NUM), SPI_RD_BIT_ORDER);
    CLEAR_PERI_REG_MASK(SPI_USER_REG(SPI_NUM), SPI_DOUTDIN);
    WRITE_PERI_REG(SPI_USER1_REG(SPI_NUM), 0);
    SET_PERI_REG_BITS(SPI_CTRL2_REG(SPI_NUM), SPI_MISO_DELAY_MODE, 0, SPI_MISO_DELAY_MODE_S);
    CLEAR_PERI_REG_MASK(SPI_SLAVE_REG(SPI_NUM), SPI_SLAVE_MODE);

    // Using 40MHz SPI Speed - Maximum display ST7735R can handle
    WRITE_PERI_REG(SPI_CLOCK_REG(SPI_NUM), (1 << SPI_CLKCNT_N_S) | (1 << SPI_CLKCNT_L_S));

    SET_PERI_REG_MASK(SPI_USER_REG(SPI_NUM), SPI_CS_SETUP | SPI_CS_HOLD | SPI_USR_MOSI);
    SET_PERI_REG_MASK(SPI_CTRL2_REG(SPI_NUM), ((0x4 & SPI_MISO_DELAY_NUM) << SPI_MISO_DELAY_NUM_S));
    CLEAR_PERI_REG_MASK(SPI_USER_REG(SPI_NUM), SPI_USR_COMMAND);
    SET_PERI_REG_BITS(SPI_USER2_REG(SPI_NUM), SPI_USR_COMMAND_BITLEN, 0, SPI_USR_COMMAND_BITLEN_S);
    CLEAR_PERI_REG_MASK(SPI_USER_REG(SPI_NUM), SPI_USR_ADDR);
    SET_PERI_REG_BITS(SPI_USER1_REG(SPI_NUM), SPI_USR_ADDR_BITLEN, 0, SPI_USR_ADDR_BITLEN_S);
    CLEAR_PERI_REG_MASK(SPI_USER_REG(SPI_NUM), SPI_USR_MISO);
    SET_PERI_REG_MASK(SPI_USER_REG(SPI_NUM), SPI_USR_MOSI);
    char i;
    for (i = 0; i < 16; ++i) {
        WRITE_PERI_REG((SPI_W0_REG(SPI_NUM) + (i << 2)), 0);
    }
}

#define U16x2toU32(m,l) ((((uint32_t)(l>>8|(l&0xFF)<<8))<<16)|(m>>8|(m&0xFF)<<8))

extern uint16_t myPalette[];

#define AVERAGE(a, b)   ( ((((a) ^ (b)) & 0xf7deU) >> 1) + ((a) & (b)) )
static uint16_t averageSamples(const uint8_t * data[], int dx, int dy, const uint16_t width, const uint16_t height)
{
    uint16_t a,b,result;
    uint16_t samples[4];
    samples[0] = myPalette[(unsigned char) (data[dy*height/LCD_HEIGHT][dx*width/LCD_WIDTH])];
    samples[1] = myPalette[(unsigned char) (data[dy*height/LCD_HEIGHT][(dx*width/LCD_WIDTH) + 1])];
    samples[2] = myPalette[(unsigned char) (data[(dy*height/LCD_HEIGHT) + 1][(dx*width/LCD_WIDTH)])];
    samples[3] = myPalette[(unsigned char) (data[(dy*height/LCD_HEIGHT) + 1][(dx*width/LCD_WIDTH) + 1])];
    a = AVERAGE(samples[0],samples[1]);
    b = AVERAGE(samples[2],samples[3]);
    result = AVERAGE(a,b);
    return result;
}

void ili9341_write_frame(const uint16_t xs, const uint16_t ys, const uint16_t width, const uint16_t height, const uint8_t * data[]){
    int x, y;
    int i;
    uint16_t x1, y1;
    uint32_t xv, yv, dc;
    uint32_t temp[16];
    dc = (1 << PIN_NUM_DC);
    for (y=0; y<LCD_HEIGHT; y++) {
        //start line
        x1 = xs+(LCD_WIDTH-1);
        y1 = ys+y+(LCD_HEIGHT-1);
        xv = U16x2toU32(xs,x1);
        yv = U16x2toU32((ys+y),y1);
        while (READ_PERI_REG(SPI_CMD_REG(SPI_NUM))&SPI_USR);
        GPIO.out_w1tc = dc;
        SET_PERI_REG_BITS(SPI_MOSI_DLEN_REG(SPI_NUM), SPI_USR_MOSI_DBITLEN, 7, SPI_USR_MOSI_DBITLEN_S);
        WRITE_PERI_REG((SPI_W0_REG(SPI_NUM)), 0x2A);
        SET_PERI_REG_MASK(SPI_CMD_REG(SPI_NUM), SPI_USR);
        while (READ_PERI_REG(SPI_CMD_REG(SPI_NUM))&SPI_USR);
        GPIO.out_w1ts = dc;
        SET_PERI_REG_BITS(SPI_MOSI_DLEN_REG(SPI_NUM), SPI_USR_MOSI_DBITLEN, 31, SPI_USR_MOSI_DBITLEN_S);
        WRITE_PERI_REG((SPI_W0_REG(SPI_NUM)), xv);
        SET_PERI_REG_MASK(SPI_CMD_REG(SPI_NUM), SPI_USR);
        while (READ_PERI_REG(SPI_CMD_REG(SPI_NUM))&SPI_USR);
        GPIO.out_w1tc = dc;
        SET_PERI_REG_BITS(SPI_MOSI_DLEN_REG(SPI_NUM), SPI_USR_MOSI_DBITLEN, 7, SPI_USR_MOSI_DBITLEN_S);
        WRITE_PERI_REG((SPI_W0_REG(SPI_NUM)), 0x2B);
        SET_PERI_REG_MASK(SPI_CMD_REG(SPI_NUM), SPI_USR);
        while (READ_PERI_REG(SPI_CMD_REG(SPI_NUM))&SPI_USR);
        GPIO.out_w1ts = dc;
        SET_PERI_REG_BITS(SPI_MOSI_DLEN_REG(SPI_NUM), SPI_USR_MOSI_DBITLEN, 31, SPI_USR_MOSI_DBITLEN_S);
        WRITE_PERI_REG((SPI_W0_REG(SPI_NUM)), yv);
        SET_PERI_REG_MASK(SPI_CMD_REG(SPI_NUM), SPI_USR);
        while (READ_PERI_REG(SPI_CMD_REG(SPI_NUM))&SPI_USR);
        GPIO.out_w1tc = dc;
        SET_PERI_REG_BITS(SPI_MOSI_DLEN_REG(SPI_NUM), SPI_USR_MOSI_DBITLEN, 7, SPI_USR_MOSI_DBITLEN_S);
        WRITE_PERI_REG((SPI_W0_REG(SPI_NUM)), 0x2C);
        SET_PERI_REG_MASK(SPI_CMD_REG(SPI_NUM), SPI_USR);
        while (READ_PERI_REG(SPI_CMD_REG(SPI_NUM))&SPI_USR);

        x = 0;
        GPIO.out_w1ts = dc;
        SET_PERI_REG_BITS(SPI_MOSI_DLEN_REG(SPI_NUM), SPI_USR_MOSI_DBITLEN, 511, SPI_USR_MOSI_DBITLEN_S);
        while (x<LCD_WIDTH) {
            for (i=0; i<16; i++) {
                if(data == NULL){
                    temp[i] = 0;
                    x += 2;
                    continue;
                }
                x1 = averageSamples(data, x, y, width, height); x++;
                y1 = averageSamples(data, x, y, width, height); x++;
                temp[i] = U16x2toU32(x1,y1);
            }
            while (READ_PERI_REG(SPI_CMD_REG(SPI_NUM))&SPI_USR);
            for (i=0; i<16; i++) {
                WRITE_PERI_REG((SPI_W0_REG(SPI_NUM) + (i << 2)), temp[i]);
            }
            SET_PERI_REG_MASK(SPI_CMD_REG(SPI_NUM), SPI_USR);
        }
    }
    while (READ_PERI_REG(SPI_CMD_REG(SPI_NUM))&SPI_USR);
}

void ili9341_init()
{
    spi_master_init();
    ili_gpio_init();
    ILI9341_INITIAL ();
}
