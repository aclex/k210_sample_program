#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include "bsp.h"
#include "sysctl.h"
#include "plic.h"
#include "utils.h"
#include "gpiohs.h"
#include "fpioa.h"
#include "lcd.h"
#include "uarths.h"
#include "kpu.h"
#include "region_layer.h"
#include "image_process.h"
#include "board_config.h"
#include "w25qxx.h"
#define INCBIN_STYLE INCBIN_STYLE_SNAKE
#define INCBIN_PREFIX
#include "incbin.h"
#include "utils.h"
#include "iomem.h"
#include "global_config.h"

#define PLL0_OUTPUT_FREQ 800000000UL
#define PLL1_OUTPUT_FREQ 400000000UL

#define FRAME_WIDTH 28
#define FRAME_HEIGHT 28

#define NUM_BUFFERS 1

volatile uint32_t g_ai_done_flag;
static image_t kpu_image[NUM_BUFFERS];

kpu_model_context_t task;

INCBIN(model, "mnist_cnn_k210.kmodel");

INCBIN(input, "3.bin");

void set_kpu_addr_comps(image_t* image);

static void ai_done(void *ctx)
{
    g_ai_done_flag = 1;
}

static void io_set_power(void)
{
	sysctl_set_power_mode(SYSCTL_POWER_BANK6, SYSCTL_POWER_V18);
	sysctl_set_power_mode(SYSCTL_POWER_BANK7, SYSCTL_POWER_V18);
}

int main(void)
{
    /* Set CPU and dvp clk */
    sysctl_pll_set_freq(SYSCTL_PLL0, PLL0_OUTPUT_FREQ);
    sysctl_pll_set_freq(SYSCTL_PLL1, PLL1_OUTPUT_FREQ);
    sysctl_clock_enable(SYSCTL_CLOCK_AI);
    uarths_init();
	uarths_config(115200, 1);

    io_set_power();
    plic_init();
	uint8_t *model_data_align = model_data;
    kpu_image[0].pixel = 1;
    kpu_image[0].width = FRAME_WIDTH;
    kpu_image[0].height = FRAME_HEIGHT;
    image_init(&kpu_image[0]);
	memcpy(kpu_image[0].addr, input_data, input_size);
    printf("input data uploaded\r\n");

	/* init face detect model */
	if (kpu_load_kmodel(&task, model_data_align) != 0)
	{
		printf("\r\nmodel init error\r\n");
		while (1);
	}
    printf("mnist_cnn_k210.kmodel\r\n");

    /* enable global interrupt */
    sysctl_enable_irq();

    /* system start */
    printf("System start\r\n");
    uint64_t time_last = sysctl_get_time_us();
    uint64_t time_now = sysctl_get_time_us();
    uint64_t kpu_time_measurements = 0;

    int time_count = 0;
	int initial = 1;
	while (1)
	{
		if (!initial)
		{
			/* const uint64_t kpu_time_start = sysctl_get_time_us(); */
			g_ai_done_flag = 0;
			kpu_run_kmodel(&task, kpu_image[0].addr, DMAC_CHANNEL5, ai_done, NULL);
			while(!g_ai_done_flag);
			/* uint8_t *output; */
			/* size_t output_size; */
			/* kpu_get_output(&task, 0, (uint8_t **)&output, &output_size); */
			/* const uint64_t kpu_time_finish = sysctl_get_time_us(); */
			/* kpu_time_measurements += kpu_time_finish - kpu_time_start; */
		}
		else
		{
			initial = 0;
		}

		/* time_count++; */
		/* if(time_count % 100 == 0) */
		/* { */
			/* time_now = sysctl_get_time_us(); */
			/* const float loop_time = (time_now - time_last) / 1000.0 / 100; */
			/* const float fps = 1000 / loop_time; */
			/* printf("loop time: %.03fms, fps: %.02f\r\n", loop_time, fps); */
			/* time_last = time_now; */
			/* kpu_time_measurements = 0; */
		/* } */
	}
}
