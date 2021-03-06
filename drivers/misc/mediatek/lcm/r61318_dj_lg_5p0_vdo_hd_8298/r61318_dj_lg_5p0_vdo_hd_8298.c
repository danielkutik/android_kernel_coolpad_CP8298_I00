#ifndef BUILD_LK
#include <linux/string.h>
#endif


#include "lcm_drv.h"
#ifdef BUILD_LK
    #include <platform/mt_gpio.h>
#elif defined(BUILD_UBOOT)
    #include <asm/arch/mt_gpio.h>
#else
    #include <mach/mt_gpio.h>
#endif

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH                                         (720)
#define FRAME_HEIGHT                                        (1280)

#define REGFLAG_DELAY                                       0xFE
#define REGFLAG_END_OF_TABLE                                0xFF   // END OF REGISTERS MARKER

#define LCM_DSI_CMD_MODE                                    0

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    (lcm_util.set_reset_pin((v)))

#define UDELAY(n) (lcm_util.udelay(n))
#define MDELAY(n) (lcm_util.mdelay(n))


// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------

#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)    lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)       lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)                                  lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)              lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg                                            lcm_util.dsi_read_reg()


static struct LCM_setting_table {
    unsigned cmd;
    unsigned char count;
    unsigned char para_list[64];
};


static struct LCM_setting_table lcm_backlight_level_setting[] = {
    {0x51, 1, {0xFF}},
    {REGFLAG_END_OF_TABLE, 0x00, {}}
};

static int transform[] = {
    255,253,252,250,249,247,246,244,243,241,
    240,238,237,236,234,233,231,230,228,227,
    225,224,222,221,219,218,217,215,214,212,
    211,209,208,206,205,203,202,200,199,198,
    196,195,193,192,190,189,187,186,184,183,
    181,180,179,177,176,174,173,171,170,168,
    167,165,164,163,161,160,158,157,155,154,
    152,151,149,148,146,145,144,142,141,139,
    138,136,135,133,132,130,129,127,126,125,
    124,123,122,121,120,119,118,117,116,115,
    114,113,112,111,109,107,105,103,101,99,
    97, 95, 93, 91, 89, 87, 85, 83, 81, 79,
    77, 75, 73, 71, 69, 67, 65, 63, 61, 59,
    57, 56, 55, 54, 53, 52, 51, 50, 49, 47,
    46, 45, 44, 43, 42, 41, 40, 39, 38, 36,
    35, 34, 33, 33, 33, 32, 32, 32, 32, 31, //105
    31, 31, 30, 30, 30, 30, 29, 29, 29, 28,
    28, 28, 27, 27, 27, 27, 26, 26, 26, 25,
    25, 25, 25, 24, 24, 24, 23, 23, 23, 23,
    22, 22, 22, 21, 21, 21, 21, 20, 20, 20,
    19, 19, 19, 19, 18, 18, 18, 17, 17, 17, //55
    17, 16, 16, 16, 15, 15, 15, 15, 14, 14,
    14, 13, 13, 13, 13, 12, 12, 12, 11, 11,
    11, 11, 10, 10, 10, 9, 9, 8, 8, 7,
    6, 5, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 0
};
static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
    unsigned int i;

    for(i = 0; i < count; i++)
    {
        unsigned cmd;
        cmd = table[i].cmd;

        switch (cmd)
        {
            case REGFLAG_DELAY :
            {
                MDELAY(table[i].count);
                break;
            }
            case REGFLAG_END_OF_TABLE :
            {
                break;
            }
            default:
            {
                dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
            }
        }
    }
}


// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------

static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
    memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}

static void lcm_get_params(LCM_PARAMS *params)
{
    memset(params, 0, sizeof(LCM_PARAMS));

    params->type   = LCM_TYPE_DSI;

    params->width  = FRAME_WIDTH;
    params->height = FRAME_HEIGHT;

    #if (LCM_DSI_CMD_MODE)
    params->dsi.mode   = CMD_MODE;
    #else
    params->dsi.mode   = BURST_VDO_MODE; //SYNC_PULSE_VDO_MODE;//BURST_VDO_MODE;
    #endif

    // DSI
    /* Command mode setting */
    //1 Three lane or Four lane
    params->dsi.LANE_NUM                = LCM_FOUR_LANE;
    //The following defined the fomat for data coming from LCD engine.
    params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

    // Video mode setting
    params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;

    params->dsi.vertical_sync_active                = 2;
    params->dsi.vertical_backporch                  = 13;
    params->dsi.vertical_frontporch                 = 17;
    params->dsi.vertical_active_line                = FRAME_HEIGHT;

    params->dsi.horizontal_sync_active              = 12;// 10  2
    params->dsi.horizontal_backporch                = 80;//100;
    params->dsi.horizontal_frontporch               = 100;//86;
    params->dsi.horizontal_active_pixel             = FRAME_WIDTH;

    params->physical_width = 68;
    params->physical_height = 121;

    params->dsi.PLL_CLOCK=224;
    params->dsi.ssc_disable = 1;

    params->dsi.noncont_clock = 1;
    params->dsi.noncont_clock_period = 2;

    params->dsi.esd_check_enable = 1;
    params->dsi.customization_esd_check_enable = 1;
    params->dsi.lcm_esd_check_table[0].cmd          = 0x0a;
    params->dsi.lcm_esd_check_table[0].count        = 1;
    params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;
}

static void init_lcm_registers(void)
{
    unsigned int data_array[33];

    data_array[0] = 0x00010500;
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(10);

    data_array[0] = 0x00022902;
    data_array[1] = 0x000000B0;
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00022902;
    data_array[1] = 0x000001E3;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x00022902;
    data_array[1] = 0x000000B3;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x00022902;
    data_array[1] = 0x000032B6;
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00072902;
    data_array[1] = 0x10B223C0;
    data_array[2] = 0x007F0210;
    dsi_set_cmdq(data_array, 3, 1);
    data_array[0] = 0x00082902;
    data_array[1] = 0x33A8A8C1;
    data_array[2] = 0x33101033;
    dsi_set_cmdq(data_array, 3, 1);

    data_array[0] = 0x00022902;
    data_array[1] = 0x000020C3;
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00042902;
    data_array[1] = 0x82D9B4C4;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x00042902;
    data_array[1] = 0x130502C5;
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00022902;
    data_array[1] = 0x000021C6;
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x001d2902;
    data_array[1] = 0x188200C8;
    data_array[2] = 0x928398A4;
    data_array[3] = 0x07B9AC5A;
    data_array[4] = 0x56B49CA3;
    data_array[5] = 0xCF84653A;
    data_array[6] = 0x4254B635;
    data_array[7] = 0x443214C7;
    data_array[8] = 0x00000000;
    dsi_set_cmdq(data_array, 9, 1);

//GAMMA2.2
    data_array[0] = 0x001f2902;
    data_array[1] = 0x0B0702CA;
    data_array[2] = 0x1D1B1710;
    data_array[3] = 0x2125271B;
    data_array[4] = 0x020C1219;
    data_array[5] = 0x0E0A0702;
    data_array[6] = 0x1A1C1914;
    data_array[7] = 0x19212527;
    data_array[8] = 0x00020C12;
    dsi_set_cmdq(data_array, 9, 1);

    data_array[0] = 0x00142902;
    data_array[1] = 0xFC0001CB;
    data_array[2] = 0x00C302FF;
    data_array[3] = 0x06FFFC00;
    data_array[4] = 0xF90000A5;
    data_array[5] = 0x00FC0E06;
    dsi_set_cmdq(data_array, 6, 1);
    data_array[0] = 0x00042902;
    data_array[1] = 0x4B1005D0;
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00022902;
    data_array[1] = 0x000003D1;
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00032902;
    data_array[1] = 0x001F01D2;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x00032902;
    data_array[1] = 0x003333D3;
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00022902;
    data_array[1] = 0x000002E5;
    dsi_set_cmdq(data_array, 2, 1);

    //VCOM
    //data_array[0] = 0x69D42300;
    //dsi_set_cmdq(data_array, 1, 1);

    data_array[0] = 0x00032902;
    data_array[1] = 0x003232D5;
    dsi_set_cmdq(data_array, 2, 1);

    data_array[0] = 0x00022902;
    data_array[1] = 0x000001D6;
    dsi_set_cmdq(data_array, 2, 1);
    //MDELAY(20);

    //open TE
    //data_array[0] = 0x00023902;
    //data_array[1] = 0x00000035;
    //dsi_set_cmdq(data_array, 2, 1);

    //CABC
    /*
    data_array[0] = 0x00023902;
    data_array[1] = 0x00000051;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x00023902;
    data_array[1] = 0x00002453;
    dsi_set_cmdq(data_array, 2, 1);
    data_array[0] = 0x00023902;
    data_array[1] = 0x00000055;
    dsi_set_cmdq(data_array, 2, 1);
    */

    data_array[0] = 0x00110500;      //Sleep Out
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(120);
    data_array[0] = 0x00290500;     //Display On
    dsi_set_cmdq(data_array, 1, 1);
    MDELAY(10);
}


static struct LCM_setting_table lcm_sleep_out_setting[] = {
    // Sleep Out
    {0x11, 0, {0x00}},
    {REGFLAG_DELAY, 200, {}},

    // Display ON
    {0x29, 0, {0x00}},
    {REGFLAG_DELAY, 20, {}},

    {REGFLAG_END_OF_TABLE, 0x00, {}}
};


static struct LCM_setting_table lcm_sleep_in_setting[] = {
    // Display off sequence
    {0x28, 20, {0x00}},

    // Sleep Mode On
    {0x10, 100, {0x00}},

    {REGFLAG_END_OF_TABLE, 0x00, {}}
};

static void lcm_init(void)
{
    mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ZERO);
    MDELAY(10);

    mt_set_gpio_mode(GPIO_LCM_PWR_EN,GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_PWR_EN,GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_PWR_EN,GPIO_OUT_ONE);
    MDELAY(10);

    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
    MDELAY(20);

    init_lcm_registers();
}



static void lcm_suspend(void)
{
    push_table(lcm_sleep_in_setting, sizeof(lcm_sleep_in_setting) / sizeof(struct LCM_setting_table), 1);

    mt_set_gpio_mode(GPIO_LCM_PWR_EN,GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_PWR_EN,GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_PWR_EN,0);
    MDELAY(10); 

    mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ZERO);
    MDELAY(10);
}


static void lcm_resume(void)
{
    mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ZERO);
    MDELAY(10);

    mt_set_gpio_mode(GPIO_LCM_PWR_EN,GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_PWR_EN,GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_PWR_EN,GPIO_OUT_ONE);
    MDELAY(10);

    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
    MDELAY(20);

    init_lcm_registers();
}


static void lcm_update(unsigned int x, unsigned int y,
                       unsigned int width, unsigned int height)
{
    unsigned int x0 = x;
    unsigned int y0 = y;
    unsigned int x1 = x0 + width - 1;
    unsigned int y1 = y0 + height - 1;

    unsigned char x0_MSB = ((x0>>8)&0xFF);
    unsigned char x0_LSB = (x0&0xFF);
    unsigned char x1_MSB = ((x1>>8)&0xFF);
    unsigned char x1_LSB = (x1&0xFF);
    unsigned char y0_MSB = ((y0>>8)&0xFF);
    unsigned char y0_LSB = (y0&0xFF);
    unsigned char y1_MSB = ((y1>>8)&0xFF);
    unsigned char y1_LSB = (y1&0xFF);

    unsigned int data_array[16];

    data_array[0]= 0x00053902;
    data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
    data_array[2]= (x1_LSB);
    data_array[3]= 0x00053902;
    data_array[4]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
    data_array[5]= (y1_LSB);
    data_array[6]= 0x002c3909;

    dsi_set_cmdq(data_array, 7, 0);

}


static void lcm_setbacklight(unsigned int level)
{
    //unsigned int default_level = 0;//145;
    unsigned int mapped_level = 0;

    //for LGE backlight IC mapping table
    if(level > 255)
            level = 255;

    if(level >0)
            //mapped_level = default_level+(level)*(255-default_level)/(255);
            mapped_level = (unsigned char)transform[255-level];
    else
            mapped_level=0;

    // Refresh value of backlight level.
    lcm_backlight_level_setting[0].para_list[0] = mapped_level;

    push_table(lcm_backlight_level_setting, sizeof(lcm_backlight_level_setting) / sizeof(struct LCM_setting_table), 1);
}

#define read_reg_v2(cmd, buffer, buffer_size)               lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)
#define LCM_COMPARE_ID_DJ_LG 0x40
static unsigned int lcm_compare_id(void)
{
    unsigned int id=0;
    unsigned char buffer[5];
    unsigned int array[16];

    mt_set_gpio_mode(GPIO_LCM_RST, GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_RST, GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ZERO);
    MDELAY(10);

    mt_set_gpio_mode(GPIO_LCM_PWR_EN,GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_PWR_EN,GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_PWR_EN,GPIO_OUT_ONE);
    MDELAY(11);

    mt_set_gpio_out(GPIO_LCM_RST, GPIO_OUT_ONE);
    MDELAY(20);

    array[0] = 0x04B02300;
    dsi_set_cmdq(array, 1, 1);

    array[0] = 0x00013700;
    dsi_set_cmdq(array, 1, 1);

    read_reg_v2(0xDA, buffer, 1);
    id = buffer[0];
    #ifdef BUILD_LK
        printf("lk debug: DJ_LG id = 0x%x\n", id);
    #endif

    if(LCM_COMPARE_ID_DJ_LG == id)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

static void lcm_vddi_power_on(void)
{
    #ifdef BUILD_LK
        printf("lk  lcm_vddi_power_on\n");
    #else
        printk("kernel lcm_vddi_power_on\n");
    #endif
    mt_set_gpio_mode(GPIO_LCM_PWR2_EN,GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_PWR2_EN,GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_PWR2_EN,GPIO_OUT_ONE);
    MDELAY(10);
}
static void lcm_vddi_power_off(void)
{
    #ifdef BUILD_LK
        printf("lk  lcm_vddi_power_off\n");
    #else
        printk("kernel lcm_vddi_power_off\n");
    #endif
    mt_set_gpio_mode(GPIO_LCM_PWR2_EN,GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_PWR2_EN,GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_PWR2_EN,GPIO_OUT_ZERO);
    MDELAY(10);
}
static void lcm_vddi_power_init(void)
{
    #ifdef BUILD_LK
        printf("lk  lcm_vddi_power_init\n");
    #else
        printk("kernel lcm_vddi_power_init\n");
    #endif
    mt_set_gpio_mode(GPIO_LCM_PWR2_EN,GPIO_MODE_00);
    mt_set_gpio_dir(GPIO_LCM_PWR2_EN,GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_LCM_PWR2_EN,GPIO_OUT_ONE);
}
LCM_DRIVER r61318_dsi_dj_lg_lcm_drv =
{
    .name           = "r61318_dsi_dj_lg",
    .set_util_funcs = lcm_set_util_funcs,
    .get_params     = lcm_get_params,
    .init           = lcm_init,
    .suspend        = lcm_suspend,
    .resume         = lcm_resume,
    .compare_id     = lcm_compare_id,
    .set_backlight  = lcm_setbacklight,
    .vddi_power_on  = lcm_vddi_power_on,
    .vddi_power_off  = lcm_vddi_power_off,
    .vddi_power_init = lcm_vddi_power_init,
#if (LCM_DSI_CMD_MODE)
    .update         = lcm_update
#endif
};
