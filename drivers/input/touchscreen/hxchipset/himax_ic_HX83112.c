/* SPDX-License-Identifier: GPL-2.0 */
/*  Himax Android Driver Sample Code for HX83112 chipset
 *
 *  Copyright (C) 2019 Himax Corporation.
 *
 *  This software is licensed under the terms of the GNU General Public
 *  License version 2,  as published by the Free Software Foundation,  and
 *  may be copied,  distributed,  and modified under those terms.
 *
 *  This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */

#include "himax_ic_HX83112.h"
#include "himax_modular.h"

static void hx83112_chip_init(void)
{
	(*kp_private_ts)->chip_cell_type = CHIP_IS_IN_CELL;
	I("%s:IC cell type = %d\n",  __func__,
		(*kp_private_ts)->chip_cell_type);
	(*kp_IC_CHECKSUM) = HX_TP_BIN_CHECKSUM_CRC;
	/*Himax: Set FW and CFG Flash Address*/
	(*kp_FW_VER_MAJ_FLASH_ADDR) = 49157;  /*0x00C005*/
	(*kp_FW_VER_MIN_FLASH_ADDR) = 49158;  /*0x00C006*/
	(*kp_CFG_VER_MAJ_FLASH_ADDR) = 49408;  /*0x00C100*/
	(*kp_CFG_VER_MIN_FLASH_ADDR) = 49409;  /*0x00C101*/
	(*kp_CID_VER_MAJ_FLASH_ADDR) = 49154;  /*0x00C002*/
	(*kp_CID_VER_MIN_FLASH_ADDR) = 49155;  /*0x00C003*/
	(*kp_CFG_TABLE_FLASH_ADDR) = 0x10000;
}

static bool hx83112_sense_off(bool check_en)
{
	uint8_t cnt = 0;
	uint8_t tmp_data[DATA_LEN_4];
	int ret = 0;

	do {
		if (cnt == 0
		|| (tmp_data[0] != 0xA5
		&& tmp_data[0] != 0x00
		&& tmp_data[0] != 0x87))
			kp_g_core_fp->fp_register_write(
				(*kp_pfw_op)->addr_ctrl_fw_isr,
				DATA_LEN_4,
				(*kp_pfw_op)->data_fw_stop,
				0);

		/*msleep(20);*/
		usleep_range(10000, 10001);
		/* check fw status */
		kp_g_core_fp->fp_register_read(
			(*kp_pic_op)->addr_cs_central_state,
			ADDR_LEN_4, tmp_data, 0);

		if (tmp_data[0] != 0x05) {
			I("%s: Do not need wait FW, Status = 0x%02X!\n",
					__func__, tmp_data[0]);
			break;
		}

		kp_g_core_fp->fp_register_read((*kp_pfw_op)->addr_ctrl_fw_isr,
			4, tmp_data, false);
		I("%s: cnt = %d, data[0] = 0x%02X!\n", __func__,
			cnt, tmp_data[0]);
	} while (tmp_data[0] != 0x87 && (++cnt < 10) && check_en == true);

	cnt = 0;

	do {
		/**
		 * I2C_password[7:0] set Enter safe mode : 0x31 ==> 0x27
		 */
		tmp_data[0] = (*kp_pic_op)->data_i2c_psw_lb[0];

		ret = kp_himax_bus_write((*kp_pic_op)->adr_i2c_psw_lb[0],
				tmp_data, 1, HIMAX_I2C_RETRY_TIMES);
		if (ret < 0) {
			E("%s: i2c access fail!\n", __func__);
			return false;
		}

		/**
		 * I2C_password[15:8] set Enter safe mode :0x32 ==> 0x95
		 */
		tmp_data[0] = (*kp_pic_op)->data_i2c_psw_ub[0];

		ret = kp_himax_bus_write((*kp_pic_op)->adr_i2c_psw_ub[0],
				tmp_data, 1, HIMAX_I2C_RETRY_TIMES);
		if (ret < 0) {
			E("%s: i2c access fail!\n", __func__);
			return false;
		}

		/**
		 * Check enter_save_mode
		 */
		kp_g_core_fp->fp_register_read(
			(*kp_pic_op)->addr_cs_central_state,
			ADDR_LEN_4,
			tmp_data,
			0);
		I("%s: Check enter_save_mode data[0]=%X\n", __func__,
			tmp_data[0]);

		if (tmp_data[0] == 0x0C) {
			/**
			 * Reset TCON
			 */
			kp_g_core_fp->fp_register_write(
				(*kp_pic_op)->addr_tcon_on_rst,
				DATA_LEN_4,
				(*kp_pic_op)->data_rst,
				0);
			usleep_range(1000, 1001);
			tmp_data[3] = (*kp_pic_op)->data_rst[3];
			tmp_data[2] = (*kp_pic_op)->data_rst[2];
			tmp_data[1] = (*kp_pic_op)->data_rst[1];
			tmp_data[0] = (*kp_pic_op)->data_rst[0] | 0x01;
			kp_g_core_fp->fp_register_write(
				(*kp_pic_op)->addr_tcon_on_rst,
				DATA_LEN_4,
				tmp_data,
				0);
			/**
			 * Reset ADC
			 */
			kp_g_core_fp->fp_register_write(
				(*kp_pic_op)->addr_adc_on_rst,
				DATA_LEN_4,
				(*kp_pic_op)->data_rst,
				0);
			usleep_range(1000, 1001);
			tmp_data[3] = (*kp_pic_op)->data_rst[3];
			tmp_data[2] = (*kp_pic_op)->data_rst[2];
			tmp_data[1] = (*kp_pic_op)->data_rst[1];
			tmp_data[0] = (*kp_pic_op)->data_rst[0] | 0x01;
			kp_g_core_fp->fp_register_write(
				(*kp_pic_op)->addr_adc_on_rst,
				DATA_LEN_4,
				tmp_data,
				0);
			goto SUCCEED;
		} else {
			/*msleep(10);*/
#if defined(HX_RST_PIN_FUNC)
			kp_g_core_fp->fp_ic_reset(false, false);
#else
			kp_g_core_fp->fp_system_reset();
#endif
		}
	} while (cnt++ < 5);

	return false;
SUCCEED:
	return true;
}

static bool hx83112ab_sense_off(bool check_en)
{
	uint8_t cnt = 0;
	uint8_t tmp_data[DATA_LEN_4];
	int ret = 0;

	do {
		if (cnt == 0
		|| (tmp_data[0] != 0xA5
		&& tmp_data[0] != 0x00
		&& tmp_data[0] != 0x87))
			kp_g_core_fp->fp_register_write(
				(*kp_pfw_op)->addr_ctrl_fw_isr,
				DATA_LEN_4,
				(*kp_pfw_op)->data_fw_stop,
				0);

		/*msleep(20);*/
		usleep_range(10000, 10001);
		/* check fw status */
		kp_g_core_fp->fp_register_read(
			(*kp_pic_op)->addr_cs_central_state,
			ADDR_LEN_4, tmp_data, 0);

		if (tmp_data[0] != 0x05) {
			I("%s: Do not need wait FW, Status = 0x%02X!\n",
					__func__, tmp_data[0]);
			break;
		}

		kp_g_core_fp->fp_register_read((*kp_pfw_op)->addr_ctrl_fw_isr,
			4, tmp_data, false);
		I("%s: cnt = %d, data[0] = 0x%02X!\n", __func__,
			cnt, tmp_data[0]);
	} while (tmp_data[0] != 0x87 && (++cnt < 10) && check_en == true);

	cnt = 0;

	do {
		/**
		 * I2C_password[7:0] set Enter safe mode : 0x31 ==> 0x27
		 */
		tmp_data[0] = (*kp_pic_op)->data_i2c_psw_lb[0];

		ret = kp_himax_bus_write((*kp_pic_op)->adr_i2c_psw_lb[0],
				tmp_data, 1, HIMAX_I2C_RETRY_TIMES);
		if (ret < 0) {
			E("%s: i2c access fail!\n", __func__);
			return false;
		}

		/**
		 * I2C_password[15:8] set Enter safe mode :0x32 ==> 0x95
		 */
		tmp_data[0] = (*kp_pic_op)->data_i2c_psw_ub[0];

		ret = kp_himax_bus_write((*kp_pic_op)->adr_i2c_psw_ub[0],
				tmp_data, 1, HIMAX_I2C_RETRY_TIMES);
		if (ret < 0) {
			E("%s: i2c access fail!\n", __func__);
			return false;
		}

		/**
		 * I2C_password[7:0] set Enter safe mode : 0x31 ==> 0x00
		 */
		tmp_data[0] = 0x00;

		ret = kp_himax_bus_write((*kp_pic_op)->adr_i2c_psw_lb[0],
				tmp_data, 1, HIMAX_I2C_RETRY_TIMES);
		if (ret < 0) {
			E("%s: i2c access fail!\n", __func__);
			return false;
		}

		/**
		 * I2C_password[7:0] set Enter safe mode : 0x31 ==> 0x27
		 */
		tmp_data[0] = (*kp_pic_op)->data_i2c_psw_lb[0];

		ret = kp_himax_bus_write((*kp_pic_op)->adr_i2c_psw_lb[0],
				tmp_data, 1, HIMAX_I2C_RETRY_TIMES);
		if (ret < 0) {
			E("%s: i2c access fail!\n", __func__);
			return false;
		}

		/**
		 * I2C_password[15:8] set Enter safe mode :0x32 ==> 0x95
		 */
		tmp_data[0] = (*kp_pic_op)->data_i2c_psw_ub[0];

		ret = kp_himax_bus_write((*kp_pic_op)->adr_i2c_psw_ub[0],
				tmp_data, 1, HIMAX_I2C_RETRY_TIMES);
		if (ret < 0) {
			E("%s: i2c access fail!\n", __func__);
			return false;
		}

		/**
		 * Check enter_save_mode
		 */
		kp_g_core_fp->fp_register_read(
			(*kp_pic_op)->addr_cs_central_state,
			ADDR_LEN_4,
			tmp_data,
			0);
		I("%s: Check enter_save_mode data[0]=%X\n", __func__,
			tmp_data[0]);

		if (tmp_data[0] == 0x0C) {
			/**
			 * Reset TCON
			 */
			kp_g_core_fp->fp_register_write(
				(*kp_pic_op)->addr_tcon_on_rst,
				DATA_LEN_4,
				(*kp_pic_op)->data_rst,
				0);
			usleep_range(1000, 1001);
			tmp_data[3] = (*kp_pic_op)->data_rst[3];
			tmp_data[2] = (*kp_pic_op)->data_rst[2];
			tmp_data[1] = (*kp_pic_op)->data_rst[1];
			tmp_data[0] = (*kp_pic_op)->data_rst[0] | 0x01;
			kp_g_core_fp->fp_register_write(
				(*kp_pic_op)->addr_tcon_on_rst,
				DATA_LEN_4,
				tmp_data,
				0);
			/**
			 * Reset ADC
			 */
			kp_g_core_fp->fp_register_write(
				(*kp_pic_op)->addr_adc_on_rst,
				DATA_LEN_4,
				(*kp_pic_op)->data_rst,
				0);
			usleep_range(1000, 1001);
			tmp_data[3] = (*kp_pic_op)->data_rst[3];
			tmp_data[2] = (*kp_pic_op)->data_rst[2];
			tmp_data[1] = (*kp_pic_op)->data_rst[1];
			tmp_data[0] = (*kp_pic_op)->data_rst[0] | 0x01;
			kp_g_core_fp->fp_register_write(
				(*kp_pic_op)->addr_adc_on_rst,
				DATA_LEN_4,
				tmp_data,
				0);
			goto SUCCEED;
		} else {
			/*msleep(10);*/
#if defined(HX_RST_PIN_FUNC)
			kp_g_core_fp->fp_ic_reset(false, false);
#else
			kp_g_core_fp->fp_system_reset();
#endif
		}
	} while (cnt++ < 5);

	return false;
SUCCEED:
	return true;
}

#if defined(HX_ZERO_FLASH)
/*
 * HX83112F firmware layout differs from the generic HX83112 zero-flash
 * one assumed by himax_zf_part_info() in himax_ic_incell_core.c:
 *
 *   [0x00000 .. 0x003FF]  1K header (skipped during SRAM upload)
 *   [0x00400 .. 0x103FF]  64K main firmware
 *   [0x10400 .. ]         config partition table + partition payloads
 *
 * The partition table at 0x10400 also uses a different entry encoding
 * than the generic parser expects: 2-byte write_size and 3-byte
 * fw_addr instead of 4 and 4. Mirror the touchscreen_himax/ reference
 * (hx83112f_noflash.c: hx_parse_bin_cfg_data + hx83112f_nf_zf_part_info)
 * here and install this as fp_firmware_update_0f for the F revision.
 */
static int himax_hx83112f_parse_cfg(const struct firmware *fw_entry,
		uint8_t sram_min[4], uint8_t **fw_buf_out,
		uint32_t *cfg_sz_out, int *cfg_crc_out)
{
	const uint32_t cfg_table_pos = HX64K + HX1K;
	struct zf_info *zf_info_arr;
	int part_num, i, i_min = 0, i_max = 0;
	uint32_t dsram_base = 0xFFFFFFFF, dsram_max = 0;
	uint8_t *fw_buf;
	uint32_t cfg_sz;
	uint8_t buf[16];
	int ret = 0;

	if (fw_entry->size < cfg_table_pos + 16) {
		E("%s: FW too small for F config table (size=%zu)\n",
			__func__, fw_entry->size);
		return -EINVAL;
	}

	part_num = fw_entry->data[cfg_table_pos + 12];
	I("%s: partition count = %d\n", __func__, part_num);
	if (part_num <= 1) {
		E("%s: bad partition count %d at 0x%X\n",
			__func__, part_num, cfg_table_pos);
		return -EINVAL;
	}

	zf_info_arr = kcalloc(part_num, sizeof(*zf_info_arr), GFP_KERNEL);
	if (!zf_info_arr)
		return -ENOMEM;

	for (i = 0; i < part_num; i++) {
		memcpy(buf, &fw_entry->data[i * 0x10 + cfg_table_pos], 16);

		memcpy(zf_info_arr[i].sram_addr, buf, 4);
		zf_info_arr[i].write_size = buf[5] << 8 | buf[4];
		zf_info_arr[i].fw_addr = buf[10] << 16 | buf[9] << 8 | buf[8];
		zf_info_arr[i].cfg_addr = zf_info_arr[i].sram_addr[0];
		zf_info_arr[i].cfg_addr += zf_info_arr[i].sram_addr[1] << 8;
		zf_info_arr[i].cfg_addr += zf_info_arr[i].sram_addr[2] << 16;
		zf_info_arr[i].cfg_addr += zf_info_arr[i].sram_addr[3] << 24;

		if (i == 0)
			continue;
		if (dsram_base > zf_info_arr[i].cfg_addr) {
			dsram_base = zf_info_arr[i].cfg_addr;
			i_min = i;
		} else if (dsram_max < zf_info_arr[i].cfg_addr) {
			dsram_max = zf_info_arr[i].cfg_addr;
			i_max = i;
		}
	}

	memcpy(sram_min, zf_info_arr[i_min].sram_addr, 4);
	cfg_sz = (dsram_max - dsram_base) + zf_info_arr[i_max].write_size;
	cfg_sz += cfg_sz % 16;

	I("%s: cfg_sz=%u dsram_base=0x%X dsram_max=0x%X\n",
		__func__, cfg_sz, dsram_base, dsram_max);

	fw_buf = kzalloc(cfg_sz, GFP_KERNEL);
	if (!fw_buf) {
		ret = -ENOMEM;
		goto out_free_arr;
	}

	for (i = 1; i < part_num; i++) {
		uint32_t off = zf_info_arr[i].cfg_addr - dsram_base;

		if (off + zf_info_arr[i].write_size > cfg_sz ||
		    zf_info_arr[i].fw_addr + zf_info_arr[i].write_size >
				fw_entry->size) {
			E("%s: part %d out of bounds (off=%u sz=%u fw=%u)\n",
				__func__, i, off, zf_info_arr[i].write_size,
				zf_info_arr[i].fw_addr);
			kfree(fw_buf);
			ret = -EINVAL;
			goto out_free_arr;
		}
		memcpy(fw_buf + off,
			&fw_entry->data[zf_info_arr[i].fw_addr],
			zf_info_arr[i].write_size);
	}

	*fw_buf_out = fw_buf;
	*cfg_sz_out = cfg_sz;
	*cfg_crc_out = kp_g_core_fp->fp_Calculate_CRC_with_AP(fw_buf, 0, cfg_sz);

out_free_arr:
	kfree(zf_info_arr);
	return ret;
}

static void himax_hx83112f_firmware_update_0f(const struct firmware *fw_entry)
{
	uint8_t sram_min[4];
	uint8_t *fw_buf = NULL;
	uint32_t cfg_sz = 0;
	int cfg_crc_sw = 0, cfg_crc_hw;
	int crc, ret;
	uint8_t tmp_data[DATA_LEN_4] = {0x01, 0x00, 0x00, 0x00};

	I("%s: enter, fw size=%zu\n", __func__, fw_entry->size);

	if (fw_entry->size <= HX64K) {
		E("%s: FW size %zu <= HX64K, refusing\n",
			__func__, fw_entry->size);
		return;
	}

	ret = himax_hx83112f_parse_cfg(fw_entry, sram_min,
			&fw_buf, &cfg_sz, &cfg_crc_sw);
	if (ret) {
		E("%s: parse_cfg failed (%d)\n", __func__, ret);
		return;
	}

	kp_g_core_fp->fp_register_write((*kp_pzf_op)->addr_system_reset, 4,
			(*kp_pzf_op)->data_system_reset, 0);
	kp_g_core_fp->fp_sense_off(false);

	/* main firmware: 64K from byte HX1K (skip 1K header) into SRAM */
	crc = kp_g_core_fp->fp_write_sram_0f_crc(fw_entry,
			(*kp_pzf_op)->data_sram_start_addr, HX1K, HX64K);
	if (crc != 0)
		E("%s: main 64K CRC fail (%X)\n", __func__, crc);

	/* config partitions: contiguous blob at dsram_base */
	kp_g_core_fp->fp_register_write(sram_min, cfg_sz, fw_buf, 0);
	cfg_crc_hw = kp_g_core_fp->fp_check_CRC(sram_min, cfg_sz);
	if (cfg_crc_hw != cfg_crc_sw)
		E("%s: config CRC mismatch hw=%X sw=%X\n",
			__func__, cfg_crc_hw, cfg_crc_sw);

	/* reset n_frame back to default for normal mode */
	kp_g_core_fp->fp_register_write((*kp_pfw_op)->addr_set_frame_addr,
			4, tmp_data, 0);

	kfree(fw_buf);
	I("%s: done\n", __func__);
}

static void himax_hx83112f_reload_to_active(void)
{
	uint8_t addr[DATA_LEN_4] = {0};
	uint8_t data[DATA_LEN_4] = {0};
	uint8_t retry_cnt = 0;

	addr[3] = 0x90;
	addr[2] = 0x00;
	addr[1] = 0x00;
	addr[0] = 0x48;

	do {
		data[3] = 0x00;
		data[2] = 0x00;
		data[1] = 0x00;
		data[0] = 0xEC;
		kp_g_core_fp->fp_register_write(addr, DATA_LEN_4, data, 0);
		usleep_range(1000, 1100);
		kp_g_core_fp->fp_register_read(addr, DATA_LEN_4, data, 0);
		I("%s: data[1]=%d, data[0]=%d, retry_cnt=%d\n", __func__,
				data[1], data[0], retry_cnt);
		retry_cnt++;
	} while ((data[1] != 0x01
		|| data[0] != 0xEC)
		&& retry_cnt < HIMAX_REG_RETRY_TIMES);
}

static void himax_hx83112f_resume_ic_action(void)
{
#if !defined(HX_RESUME_HW_RESET)
	himax_hx83112f_reload_to_active();
#endif
}

static void himax_hx83112f_sense_on(uint8_t FlashMode)
{
	uint8_t tmp_data[DATA_LEN_4];
	int retry = 0;
	int ret = 0;

	I("Enter %s\n", __func__);
	(*kp_private_ts)->notouch_frame = (*kp_private_ts)->ic_notouch_frame;
	kp_g_core_fp->fp_interface_on();
	kp_g_core_fp->fp_register_write((*kp_pfw_op)->addr_ctrl_fw_isr,
		sizeof((*kp_pfw_op)->data_clear), (*kp_pfw_op)->data_clear, 0);
	/*msleep(20);*/
	usleep_range(10000, 10001);
	if (!FlashMode) {
#if defined(HX_RST_PIN_FUNC)
		kp_g_core_fp->fp_ic_reset(false, false);
#else
		kp_g_core_fp->fp_system_reset();
#endif
	} else {
		do {
			kp_g_core_fp->fp_register_read(
				(*kp_pfw_op)->addr_flag_reset_event,
				DATA_LEN_4, tmp_data, 0);
			I("%s:Read status from IC = %X,%X\n", __func__,
					tmp_data[0], tmp_data[1]);
		} while ((tmp_data[1] != 0x01
			|| tmp_data[0] != 0x00)
			&& retry++ < 5);

		if (retry >= 5) {
			E("%s: Fail:\n", __func__);
#if defined(HX_RST_PIN_FUNC)
			kp_g_core_fp->fp_ic_reset(false, false);
#else
			kp_g_core_fp->fp_system_reset();
#endif
		} else {
			I("%s:OK and Read status from IC = %X,%X\n", __func__,
				tmp_data[0], tmp_data[1]);
			/* reset code*/
			tmp_data[0] = 0x00;

			ret = kp_himax_bus_write(
				(*kp_pic_op)->adr_i2c_psw_lb[0],
				tmp_data, 1, HIMAX_I2C_RETRY_TIMES);
			if (ret < 0)
				E("%s: i2c access fail!\n", __func__);

				ret = kp_himax_bus_write(
					(*kp_pic_op)->adr_i2c_psw_ub[0],
					tmp_data, 1, HIMAX_I2C_RETRY_TIMES);
			if (ret < 0)
				E("%s: i2c access fail!\n", __func__);
		}
	}
	himax_hx83112f_reload_to_active();
}

#endif

static void hx83112_func_re_init(void)
{
	kp_g_core_fp->fp_sense_off = hx83112_sense_off;
	kp_g_core_fp->fp_chip_init = hx83112_chip_init;
}

static void hx83112_reg_re_init(void)
{
	(*kp_private_ts)->ic_notouch_frame = hx83112_notouch_frame;
}

static void hx83112f_reg_re_init(void)
{
	kp_himax_parse_assign_cmd(hx83112f_fw_addr_raw_out_sel,
		(*kp_pfw_op)->addr_raw_out_sel,
		sizeof((*kp_pfw_op)->addr_raw_out_sel));
	(*kp_private_ts)->ic_notouch_frame = hx83112f_notouch_frame;
}

static void hx83112f_func_re_init(void)
{
#if defined(HX_ZERO_FLASH)
	kp_g_core_fp->fp_resume_ic_action = himax_hx83112f_resume_ic_action;
	kp_g_core_fp->fp_sense_on = himax_hx83112f_sense_on;
	kp_g_core_fp->fp_0f_reload_to_active = himax_hx83112f_reload_to_active;
	kp_g_core_fp->fp_firmware_update_0f = himax_hx83112f_firmware_update_0f;
#endif
}

static void hx83112ab_func_re_init(void)
{
	kp_g_core_fp->fp_sense_off = hx83112ab_sense_off;
}

static bool hx83112_chip_detect(void)
{
	uint8_t tmp_data[DATA_LEN_4];
	bool ret_data = false;
	int ret = 0;
	int i = 0;

	if (himax_ic_setup_external_symbols())
		return false;

	ret = kp_himax_mcu_in_cmd_struct_init();
	if (ret < 0) {
		ret_data = false;
		E("%s:cmd_struct_init Fail:\n", __func__);
		return ret_data;
	}

	kp_himax_mcu_in_cmd_init();

	hx83112_reg_re_init();
	hx83112_func_re_init();

	ret = kp_himax_bus_read((*kp_pic_op)->addr_conti[0],
			tmp_data, 1, HIMAX_I2C_RETRY_TIMES);
	if (ret < 0) {
		E("%s: i2c access fail!\n", __func__);
		return false;
	}

	if (kp_g_core_fp->fp_sense_off(false) == false) {
		ret_data = false;
		E("%s:fp_sense_off Fail:\n", __func__);
		return ret_data;
	}
	for (i = 0; i < 5; i++) {
		ret = kp_g_core_fp->fp_register_read(
			(*kp_pfw_op)->addr_icid_addr,
			DATA_LEN_4,
			tmp_data,
			false);

		if (ret != 0) {
			ret_data = false;
			E("%s:fp_register_read Fail:\n", __func__);
			return ret_data;
		}
		I("%s:Read driver IC ID = %X, %X, %X\n", __func__,
				tmp_data[3], tmp_data[2], tmp_data[1]);

		if ((tmp_data[3] == 0x83)
		&& (tmp_data[2] == 0x11)
		&& ((tmp_data[1] == 0x2a)
		|| (tmp_data[1] == 0x2b)
		|| (tmp_data[1] == 0x2e)
		|| (tmp_data[1] == 0x2f))) {
			if (tmp_data[1] == 0x2a) {
				strscpy((*kp_private_ts)->chip_name,
					HX_83112A_SERIES_PWON, 30);
				(*kp_ic_data)->ic_adc_num =
					hx83112a_data_adc_num;
				hx83112ab_func_re_init();
			} else if (tmp_data[1] == 0x2b) {
				strscpy((*kp_private_ts)->chip_name,
					HX_83112B_SERIES_PWON, 30);
				(*kp_ic_data)->ic_adc_num =
					hx83112b_data_adc_num;
				hx83112ab_func_re_init();
			} else if (tmp_data[1] == 0x2e) {
				strscpy((*kp_private_ts)->chip_name,
					HX_83112E_SERIES_PWON, 30);
				(*kp_ic_data)->ic_adc_num =
					hx83112e_data_adc_num;
				hx83112ab_func_re_init();
			} else if (tmp_data[1] == 0x2f) {
				strscpy((*kp_private_ts)->chip_name,
					HX_83112F_SERIES_PWON, 30);
				(*kp_ic_data)->ic_adc_num =
					hx83112f_data_adc_num;
				hx83112f_reg_re_init();
				hx83112f_func_re_init();
			}

			I("%s:IC name = %s\n", __func__,
				(*kp_private_ts)->chip_name);

			I("Himax IC package %x%x%x in\n", tmp_data[3],
					tmp_data[2], tmp_data[1]);
			ret_data = true;
			goto FINAL;
		} else {
			ret_data = false;
			E("%s:Read driver ID register Fail:\n", __func__);
			E("Could NOT find Himax Chipset\n");
			E("Please check 1.VCCD,VCCA,VSP,VSN\n");
			E("2. LCM_RST,TP_RST\n");
			E("3. Power On Sequence\n");
		}
	}
FINAL:

	return ret_data;
}

DECLARE(HX_MOD_KSYM_HX83112);

static int himax_hx83112_probe(void)
{
	I("%s:Enter\n", __func__);
	himax_add_chip_dt(hx83112_chip_detect);
	return 0;
}

static int himax_hx83112_remove(void)
{
	free_chip_dt_table();
	return 0;
}

static int __init himax_hx83112_init(void)
{
	int ret = 0;

	I("%s\n", __func__);
	ret = himax_hx83112_probe();
	return 0;
}

static void __exit himax_hx83112_exit(void)
{
	himax_hx83112_remove();
}

module_init(himax_hx83112_init);
module_exit(himax_hx83112_exit);

MODULE_DESCRIPTION("HIMAX HX83112 touch driver");
MODULE_LICENSE("GPL");


