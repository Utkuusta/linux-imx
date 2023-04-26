// SPDX-License-Identifier: GPL-2.0
/*
 * BOE B3 panel driver
 *
 * Copyright 2023 Inventron
 */

#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/regulator/consumer.h>

#include <video/mipi_display.h>
#include <video/of_videomode.h>
#include <video/videomode.h>

#include <drm/drm_crtc.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_panel.h>
#include <drm/drm_print.h>

/* Panel specific color-format bits */
#define COL_FMT_16BPP 0x55
#define COL_FMT_18BPP 0x66
#define COL_FMT_24BPP 0x77

/* Write Manufacture Command Set Control */
#define WRMAUCCTR 0xFE

/* Manufacturer Command Set pages (CMD2) */
struct cmd_set_entry {
	u8 cmd;
	u8 param;
};

static const u32 boe_bus_formats[] = {
	MEDIA_BUS_FMT_RGB888_1X24,
	MEDIA_BUS_FMT_RGB666_1X18,
	MEDIA_BUS_FMT_RGB565_1X16,
};

static const u32 boe_bus_flags = DRM_BUS_FLAG_DE_LOW |
				 DRM_BUS_FLAG_PIXDATA_NEGEDGE;

struct boe_panel {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;

	struct backlight_device *backlight;

	struct regulator_bulk_data *supplies;
	unsigned int num_supplies;

	bool prepared;
	bool enabled;
};

// "1920x1080": 60 148500 1920 2008 2052 2200 1080 1084 1089 1125 0x40 0x5
/*
static const struct drm_display_mode default_mode = {
	.clock = 74250,
	.hdisplay = 1920,
	.hsync_start = 2008,
	.hsync_end = 2052,
	.htotal = 2200,
	.vdisplay = 1080,
	.vsync_start = 1084,
	.vsync_end = 1089,
	.vtotal = 1125,
	.vrefresh = 30,
	.width_mm = 217,
	.height_mm = 136,
	.flags = 0x5
};*/

// 1280x720
// static const struct drm_display_mode default_mode = {
// 	.clock = 74250,
// 	.hdisplay = 1280,
// 	.hsync_start = 1280 + 110,
// 	.hsync_end = 1280 + 110 + 40,
// 	.htotal = 1280 + 110 + 40 + 220,
// 	.vdisplay = 720,
// 	.vsync_start = 720 + 5,
// 	.vsync_end = 720 + 5 + 5,
// 	.vtotal = 720 + 5 + 5 + 20,
// 	.vrefresh = 60,
// 	.width_mm = 217,
// 	.height_mm = 136,
// 	.flags = 0x5
// };

 // 1280x800 bizim panel
 /*
 static const struct drm_display_mode default_mode = {
 	.clock = 71000,
 	.hdisplay = 1280,
 	.hsync_start = 1280 + 28,
 	.hsync_end = 1280 + 28 + 32,
 	.htotal = 1280 + 28 + 32 + 100,
 	.vdisplay = 800,
 	.vsync_start = 800 + 2,
 	.vsync_end = 800 + 2 + 6,
 	.vtotal = 800 + 2 + 6 + 15,
 	.vrefresh = 60,
 	.width_mm = 217,
 	.height_mm = 136,
 	.flags = DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC
 };*/
 
 //1280x800 vesa
 static const struct drm_display_mode default_mode = {
 	.clock = 74250,
 	.hdisplay = 1280,
 	.hsync_start = 1280 + 72,
 	.hsync_end = 1280 + 72 + 128,
 	.htotal = 1280 + 72 + 128 + 200,
 	.vdisplay = 800,
 	.vsync_start = 800 + 3,
 	.vsync_end = 800 + 3 + 6,
 	.vtotal = 800 + 3 + 6 + 22,
 	.vrefresh = 60,
 	.width_mm = 217,
 	.height_mm = 136,
 	.flags = DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC
 };

// 1280x800 panel
// static const struct drm_display_mode default_mode = {
// 	.clock = 83500,
// 	.hdisplay = 1280,
// 	.hsync_start = 1280 + 48,
// 	.hsync_end = 1280 + 48 + 32,
// 	.htotal = 1280 + 48 + 32 + 80,
// 	.vdisplay = 800,
// 	.vsync_start = 800 + 3,
// 	.vsync_end = 800 + 3 + 6,
// 	.vtotal = 800 + 3 + 6 + 14,
// 	.vrefresh = 30,
// 	.width_mm = 217,
// 	.height_mm = 136,
// 	.flags = 0x5
// };

static inline struct boe_panel *to_boe_panel(struct drm_panel *panel)
{
	return container_of(panel, struct boe_panel, panel);
}

static inline int st7796_dsi_write(struct mipi_dsi_device *dsi, const void *seq,
				   size_t len)
{
	return mipi_dsi_dcs_write_buffer(dsi, seq, len);
}

#define ST7796_DSI(dsi, seq...)                              \
        {                                                       \
                const u8 d[] = { seq };                         \
                dev_err(&dsi->dev, "Writing command %02x, length: %d\n", d[0], ARRAY_SIZE(d)); \
                st7796_dsi_write(dsi, d, ARRAY_SIZE(d));     \
        }

static int boe_panel_push_cmd_list(struct mipi_dsi_device *dsi)
{
	return 0;
};

static int boe_panel_push_cmd_list_after_exit_sleep(struct mipi_dsi_device *dsi)
{
	int ret = 0;
	return ret;
};

static int boe_panel_prepare(struct drm_panel *panel)
{
	struct boe_panel *boe = to_boe_panel(panel);
	int ret;
	struct mipi_dsi_device *dsi = boe->dsi;
	struct device *dev = &dsi->dev;

	dev_err(dev, "lt8912panel prepare\n");
	if (boe->prepared)
		return 0;

	ret = regulator_bulk_enable(boe->num_supplies, boe->supplies);
	if (ret)
		return ret;

	boe->prepared = true;

	return 0;
}

static int boe_panel_unprepare(struct drm_panel *panel)
{
	struct boe_panel *boe = to_boe_panel(panel);
	int ret;
	struct mipi_dsi_device *dsi = boe->dsi;
	struct device *dev = &dsi->dev;

	dev_info(dev, "panel unprepare\n");

	if (!boe->prepared)
		return 0;

	ret = regulator_bulk_disable(boe->num_supplies, boe->supplies);
	if (ret)
		return ret;

	boe->prepared = false;

	return 0;
}

static int boe_panel_enable(struct drm_panel *panel)
{
	struct boe_panel *boe = to_boe_panel(panel);
	struct mipi_dsi_device *dsi = boe->dsi;
	struct device *dev = &dsi->dev;
	int color_format = COL_FMT_24BPP;
	int ret;

	if (boe->enabled)
		return 0;

	dsi->mode_flags &= ~MIPI_DSI_CLOCK_NON_CONTINUOUS;
	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;
	dsi->mode_flags |= MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_SYNC_PULSE;

	ret = boe_panel_push_cmd_list(dsi);
	if (ret < 0) {
		DRM_DEV_ERROR(dev, "Failed to send MCS (%d)\n", ret);
		goto fail;
	}

	/* Software reset */
	ret = mipi_dsi_dcs_soft_reset(dsi);
	if (ret < 0) {
		DRM_DEV_ERROR(dev, "Failed to do Software Reset (%d)\n", ret);
		goto fail;
	}

	usleep_range(15000, 17000);

	/* Set pixel format */
	ret = mipi_dsi_dcs_set_pixel_format(dsi, color_format);
	DRM_DEV_DEBUG_DRIVER(dev, "Interface color format set to 0x%x\n",
			     color_format);
	if (ret < 0) {
		DRM_DEV_ERROR(dev, "Failed to set pixel format (%d)\n", ret);
		goto fail;
	}

	ret = mipi_dsi_dcs_write(dsi, MIPI_DCS_ENTER_INVERT_MODE, NULL, 0);
	if (ret < 0) {
		DRM_DEV_ERROR(dev, "Failed to exit sleep mode (%d)\n", ret);
		goto fail;
	}

	/* Exit sleep mode */
	ret = mipi_dsi_dcs_exit_sleep_mode(dsi);
	if (ret < 0) {
		DRM_DEV_ERROR(dev, "Failed to exit sleep mode (%d)\n", ret);
		goto fail;
	}

	usleep_range(120000, 125000);

	ret = mipi_dsi_dcs_set_display_on(dsi);
	if (ret < 0) {
		DRM_DEV_ERROR(dev, "Failed to set display ON (%d)\n", ret);
		goto fail;
	}

	usleep_range(20000, 22000);

	ret = boe_panel_push_cmd_list_after_exit_sleep(dsi);
	if (ret < 0) {
		DRM_DEV_ERROR(dev, "Failed to send MCS2 (%d)\n", ret);
		goto fail;
	}

	usleep_range(200000, 202000);
	backlight_enable(boe->backlight);

	boe->enabled = true;

	return 0;

fail:
	return ret;
}

static int boe_panel_disable(struct drm_panel *panel)
{
	struct boe_panel *boe = to_boe_panel(panel);
	struct mipi_dsi_device *dsi = boe->dsi;
	struct device *dev = &dsi->dev;
	int ret;

	if (!boe->enabled)
		return 0;

	dsi->mode_flags |= MIPI_DSI_MODE_LPM;

	backlight_disable(boe->backlight);

	usleep_range(10000, 12000);

	ret = mipi_dsi_dcs_set_display_off(dsi);
	if (ret < 0) {
		DRM_DEV_ERROR(dev, "Failed to set display OFF (%d)\n", ret);
		return ret;
	}

	usleep_range(120000, 125000);

	ret = mipi_dsi_dcs_enter_sleep_mode(dsi);
	if (ret < 0) {
		DRM_DEV_ERROR(dev, "Failed to enter sleep mode (%d)\n", ret);
		return ret;
	}

	usleep_range(120000, 125000);

	boe->enabled = 0;

	return 0;
}

static int boe_panel_get_modes(struct drm_panel *panel)
{
	struct drm_connector *connector = panel->connector;
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(panel->drm, &default_mode);
	if (!mode) {
		DRM_DEV_ERROR(panel->dev, "failed to add mode %ux%ux@%u\n",
			      default_mode.hdisplay, default_mode.vdisplay,
			      default_mode.vrefresh);
		return -ENOMEM;
	}

	drm_mode_set_name(mode);
	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_probed_add(panel->connector, mode);

	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;
	connector->display_info.bus_flags = boe_bus_flags;

	drm_display_info_set_bus_formats(&connector->display_info,
					 boe_bus_formats,
					 ARRAY_SIZE(boe_bus_formats));
	return 1;
}

static int boe_bl_get_brightness(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	struct boe_panel *boe = mipi_dsi_get_drvdata(dsi);
	u16 brightness;
	int ret;

	if (!boe->prepared)
		return 0;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_get_display_brightness(dsi, &brightness);
	if (ret < 0)
		return ret;

	bl->props.brightness = brightness;

	return brightness & 0xff;
}

static int boe_bl_update_status(struct backlight_device *bl)
{
	struct mipi_dsi_device *dsi = bl_get_data(bl);
	struct boe_panel *boe = mipi_dsi_get_drvdata(dsi);
	int ret = 0;

	if (!boe->prepared)
		return 0;

	dsi->mode_flags &= ~MIPI_DSI_MODE_LPM;

	ret = mipi_dsi_dcs_set_display_brightness(dsi, bl->props.brightness);
	if (ret < 0)
		return ret;

	return 0;
}

static const struct backlight_ops boe_bl_ops = {
	.update_status = boe_bl_update_status,
	.get_brightness = boe_bl_get_brightness,
};

static const struct drm_panel_funcs boe_panel_funcs = {
	.prepare = boe_panel_prepare,
	.unprepare = boe_panel_unprepare,
	.enable = boe_panel_enable,
	.disable = boe_panel_disable,
	.get_modes = boe_panel_get_modes,
};

static const char * const boe_supply_names[] = {
	"v3p3",
	"v1p8",
};

static int boe_init_regulators(struct boe_panel *boe)
{
	struct device *dev = &boe->dsi->dev;
	int i;

	boe->num_supplies = ARRAY_SIZE(boe_supply_names);
	boe->supplies = devm_kcalloc(dev, boe->num_supplies,
				     sizeof(*boe->supplies), GFP_KERNEL);
	if (!boe->supplies)
		return -ENOMEM;

	for (i = 0; i < boe->num_supplies; i++)
		boe->supplies[i].supply = boe_supply_names[i];

	return devm_regulator_bulk_get(dev, boe->num_supplies, boe->supplies);
};

static int boe_panel_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct device_node *np = dev->of_node;
	struct boe_panel *panel;
	struct backlight_properties bl_props;
	int ret;
	u32 video_mode;

	usleep_range(2000000, 2500000);

	dev_info(dev, "panel probe\n");

	panel = devm_kzalloc(&dsi->dev, sizeof(*panel), GFP_KERNEL);
	if (!panel)
		return -ENOMEM;

	mipi_dsi_set_drvdata(dsi, panel);

	panel->dsi = dsi;

	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags =  /*MIPI_DSI_MODE_VIDEO_HSE | */ MIPI_DSI_MODE_VIDEO;

	// ret = of_property_read_u32(np, "video-mode", &video_mode);
	// if (!ret) {
	// 	switch (video_mode) {
	// 	case 0:
	// 		/* burst mode */
	// 		dsi->mode_flags |= MIPI_DSI_MODE_VIDEO_BURST;
	// 		break;
	// 	case 1:
	// 		/* non-burst mode with sync event */
	// 		break;
	// 	case 2:
	// 		/* non-burst mode with sync pulse */
	// 		dsi->mode_flags |= MIPI_DSI_MODE_VIDEO_SYNC_PULSE;
	// 		break;
	// 	default:
	// 		dev_warn(dev, "invalid video mode %d\n", video_mode);
	// 		break;
	// 	}
	// }
	dsi->mode_flags |= MIPI_DSI_MODE_VIDEO_SYNC_PULSE;

	ret = of_property_read_u32(np, "dsi-lanes", &dsi->lanes);
	if (ret) {
		dev_err(dev, "Failed to get dsi-lanes property (%d)\n", ret);
		return ret;
	}

	ret = boe_init_regulators(panel);
	if (ret)
		return ret;

	drm_panel_init(&panel->panel);
	panel->panel.funcs = &boe_panel_funcs;
	panel->panel.dev = dev;
	dev_set_drvdata(dev, panel);

	ret = drm_panel_add(&panel->panel);
	if (ret)
		return ret;

	ret = mipi_dsi_attach(dsi);
	if (ret)
		drm_panel_remove(&panel->panel);

	dev_info(dev, "panel probe end\n");

	return ret;
}

static int boe_panel_remove(struct mipi_dsi_device *dsi)
{
	struct boe_panel *boe = mipi_dsi_get_drvdata(dsi);
	struct device *dev = &dsi->dev;
	int ret;

	ret = mipi_dsi_detach(dsi);
	if (ret)
		DRM_DEV_ERROR(dev, "Failed to detach from host (%d)\n",
			      ret);

	drm_panel_remove(&boe->panel);

	return 0;
}

static void boe_panel_shutdown(struct mipi_dsi_device *dsi)
{
	struct boe_panel *boe = mipi_dsi_get_drvdata(dsi);

	boe_panel_disable(&boe->panel);
	boe_panel_unprepare(&boe->panel);
}

static const struct of_device_id boe_of_match[] = {
	{ .compatible = "boe,b3", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, boe_of_match);

static struct mipi_dsi_driver boe_panel_driver = {
	.driver = {
		.name = "panel-boe-b3",
		.of_match_table = boe_of_match,
	},
	.probe = boe_panel_probe,
	.remove = boe_panel_remove,
	.shutdown = boe_panel_shutdown,
};
module_mipi_dsi_driver(boe_panel_driver);

MODULE_AUTHOR("Ceyhun Sen <ceyhun.sen@inventron.com.tr>");
MODULE_DESCRIPTION("DRM Driver for BOE B3 panel");
MODULE_LICENSE("GPL v2");
