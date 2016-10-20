/******************************************************************************
 *
 * Copyright(c) 2007 - 2011 Realtek Corporation. All rights reserved.
 *                                        
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 *
 ******************************************************************************/
#define _RTW_RF_C_

#include <drv_types.h>
#include <hal_data.h>

u8 center_ch_5g_all[CENTER_CH_5G_ALL_NUM] = {
		36, 38, 40, 42, 44, 46, 48,			/* Band 1 */
		52, 54, 56, 58, 60, 62, 64,			/* Band 2 */
		100, 102, 104, 106, 108, 110, 112,	/* Band 3 */
		116, 118, 120, 122, 124, 126, 128,	/* Band 3 */
		132, 134, 136, 138, 140, 142, 144,	/* Band 3 */
		149, 151, 153, 155, 157, 159, 161,	/* Band 4 */
		165, 167, 169, 171, 173, 175, 177};	/* Band 4 */

u8 center_ch_5g_20m[CENTER_CH_5G_20M_NUM] = {
	36, 40, 44, 48,
	52, 56, 60, 64,
	100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140, 144,
	149, 153, 157, 161, 165, 169, 173, 177
};

u8 center_ch_5g_40m[CENTER_CH_5G_40M_NUM] = {38, 46, 54, 62, 102, 110, 118, 126, 134, 142, 151, 159, 167, 175};

u8 center_ch_5g_80m[CENTER_CH_5G_80M_NUM] = {42, 58, 106, 122, 138, 155, 171};

struct center_chs_ent {
	u8 ch_num;
	u8 *chs;
};

struct center_chs_ent center_chs_5g_by_bw[] = {
	{CENTER_CH_5G_20M_NUM, center_ch_5g_20m},
	{CENTER_CH_5G_40M_NUM, center_ch_5g_40m},
	{CENTER_CH_5G_80M_NUM, center_ch_5g_80m},
};

inline u8 center_chs_5g_num(u8 bw)
{
	if (bw >= CHANNEL_WIDTH_160)
		return 0;
	
	return center_chs_5g_by_bw[bw].ch_num;
}

inline u8 center_chs_5g(u8 bw, u8 id)
{
	if (bw >= CHANNEL_WIDTH_160)
		return 0;

	if (id >= center_chs_5g_num(bw))
		return 0;
		
	return center_chs_5g_by_bw[bw].chs[id];
}

int rtw_ch2freq(int chan)
{
	/* see 802.11 17.3.8.3.2 and Annex J
	* there are overlapping channel numbers in 5GHz and 2GHz bands */

	/*
	* RTK: don't consider the overlapping channel numbers: 5G channel <= 14,
	* because we don't support it. simply judge from channel number
	*/

	if (chan >= 1 && chan <= 14) {
		if (chan == 14)
			return 2484;
		else if (chan < 14)
			return 2407 + chan * 5;
	} else if (chan >= 36 && chan <= 177) {
		return 5000 + chan * 5;
	}

	return 0; /* not supported */
}

int rtw_freq2ch(int freq)
{
	/* see 802.11 17.3.8.3.2 and Annex J */
	if (freq == 2484)
		return 14;
	else if (freq < 2484)
		return (freq - 2407) / 5;
	else if (freq >= 4910 && freq <= 4980)
		return (freq - 4000) / 5;
	else if (freq <= 45000) /* DMG band lower limit */
		return (freq - 5000) / 5;
	else if (freq >= 58320 && freq <= 64800)
		return (freq - 56160) / 2160;
	else
		return 0;
}

bool rtw_chbw_to_freq_range(u8 ch, u8 bw, u8 offset, u32 *hi, u32 *lo)
{
	u8 c_ch;
	u32 freq;
	u32 hi_ret = 0, lo_ret = 0;
	int i;
	bool valid = _FALSE;

	if (hi)
		*hi = 0;
	if (lo)
		*lo = 0;

	c_ch = rtw_get_center_ch(ch, bw, offset);
	freq = rtw_ch2freq(c_ch);

	if (!freq) {
		rtw_warn_on(1);
		goto exit;
	}

	if (bw == CHANNEL_WIDTH_80) {
		hi_ret = freq + 40;
		lo_ret = freq - 40;
	} else if (bw == CHANNEL_WIDTH_40) {
		hi_ret = freq + 20;
		lo_ret = freq - 20;
	} else if (bw == CHANNEL_WIDTH_20) {
		hi_ret = freq + 10;
		lo_ret = freq - 10;
	} else {
		rtw_warn_on(1);
	}

	if (hi)
		*hi = hi_ret;
	if (lo)
		*lo = lo_ret;

	valid = _TRUE;

exit:
	return valid;
}

const char * const _ch_width_str[] = {
	"20MHz",
	"40MHz",
	"80MHz",
	"160MHz",
	"80_80MHz",
	"CHANNEL_WIDTH_MAX",
};

const u8 _ch_width_to_bw_cap[] = {
	BW_CAP_20M,
	BW_CAP_40M,
	BW_CAP_80M,
	BW_CAP_160M,
	BW_CAP_80_80M,
	0,
};

const char * const _band_str[] = {
	"2.4G",
	"5G",
	"BOTH",
	"BAND_MAX",
};

const u8 _band_to_band_cap[] = {
	BAND_CAP_2G,
	BAND_CAP_5G,
	0,
	0,
};

struct country_chplan {
	char alpha2[2];
	u8 chplan;
};

static const struct country_chplan country_chplan_map[] = {
	{"AD", 0x26}, /* Andorra */
	{"AE", 0x26}, /* United Arab Emirates */
	{"AF", 0x42}, /* Afghanistan */
	{"AG", 0x30}, /* Antigua & Barbuda */
	{"AI", 0x26}, /* Anguilla(UK) */
	{"AL", 0x26}, /* Albania */
	{"AM", 0x34}, /* Armenia */
	{"AO", 0x26}, /* Angola */
	{"AQ", 0x26}, /* Antarctica */
	{"AR", 0x57}, /* Argentina */
	{"AS", 0x34}, /* American Samoa */
	{"AT", 0x26}, /* Austria */
	{"AU", 0x45}, /* Australia */
	{"AW", 0x34}, /* Aruba */
	{"AZ", 0x26}, /* Azerbaijan */
	{"BA", 0x26}, /* Bosnia & Herzegovina */
	{"BB", 0x34}, /* Barbados */
	{"BD", 0x26}, /* Bangladesh */
	{"BE", 0x26}, /* Belgium */
	{"BF", 0x26}, /* Burkina Faso */
	{"BG", 0x26}, /* Bulgaria */
	{"BH", 0x47}, /* Bahrain */
	{"BI", 0x26}, /* Burundi */
	{"BJ", 0x26}, /* Benin */
	{"BN", 0x47}, /* Brunei */
	{"BO", 0x30}, /* Bolivia */
	{"BR", 0x34}, /* Brazil */
	{"BS", 0x34}, /* Bahamas */
	{"BW", 0x26}, /* Botswana */
	{"BY", 0x26}, /* Belarus */
	{"BZ", 0x34}, /* Belize */
	{"CA", 0x34}, /* Canada */
	{"CC", 0x26}, /* Cocos (Keeling) Islands (Australia) */
	{"CD", 0x26}, /* Congo, Republic of the */
	{"CF", 0x26}, /* Central African Republic */
	{"CG", 0x26}, /* Congo, Democratic Republic of the. Zaire */
	{"CH", 0x26}, /* Switzerland */
	{"CI", 0x26}, /* Cote d'Ivoire */
	{"CK", 0x26}, /* Cook Islands */
	{"CL", 0x30}, /* Chile */
	{"CM", 0x26}, /* Cameroon */
	{"CN", 0x48}, /* China */
	{"CO", 0x34}, /* Colombia */
	{"CR", 0x34}, /* Costa Rica */
	{"CV", 0x26}, /* Cape Verde */
	{"CX", 0x45}, /* Christmas Island (Australia) */
	{"CY", 0x26}, /* Cyprus */
	{"CZ", 0x26}, /* Czech Republic */
	{"DE", 0x26}, /* Germany */
	{"DJ", 0x26}, /* Djibouti */
	{"DK", 0x26}, /* Denmark */
	{"DM", 0x34}, /* Dominica */
	{"DO", 0x34}, /* Dominican Republic */
	{"DZ", 0x26}, /* Algeria */
	{"EC", 0x34}, /* Ecuador */
	{"EE", 0x26}, /* Estonia */
	{"EG", 0x47}, /* Egypt */
	{"EH", 0x47}, /* Western Sahara */
	{"ER", 0x26}, /* Eritrea */
	{"ES", 0x26}, /* Spain */
	{"ET", 0x26}, /* Ethiopia */
	{"FI", 0x26}, /* Finland */
	{"FJ", 0x34}, /* Fiji */
	{"FK", 0x26}, /* Falkland Islands (Islas Malvinas) (UK) */
	{"FM", 0x34}, /* Micronesia, Federated States of (USA) */
	{"FO", 0x26}, /* Faroe Islands (Denmark) */
	{"FR", 0x26}, /* France */
	{"GA", 0x26}, /* Gabon */
	{"GB", 0x26}, /* Great Britain (United Kingdom; England) */
	{"GD", 0x34}, /* Grenada */
	{"GE", 0x26}, /* Georgia */
	{"GF", 0x26}, /* French Guiana */
	{"GG", 0x26}, /* Guernsey (UK) */
	{"GH", 0x26}, /* Ghana */
	{"GI", 0x26}, /* Gibraltar (UK) */
	{"GL", 0x26}, /* Greenland (Denmark) */
	{"GM", 0x26}, /* Gambia */
	{"GN", 0x26}, /* Guinea */
	{"GP", 0x26}, /* Guadeloupe (France) */
	{"GQ", 0x26}, /* Equatorial Guinea */
	{"GR", 0x26}, /* Greece */
	{"GS", 0x26}, /* South Georgia and the Sandwich Islands (UK) */
	{"GT", 0x34}, /* Guatemala */
	{"GU", 0x34}, /* Guam (USA) */
	{"GW", 0x26}, /* Guinea-Bissau */
	{"GY", 0x44}, /* Guyana */
	{"HK", 0x26}, /* Hong Kong */
	{"HM", 0x45}, /* Heard and McDonald Islands (Australia) */
	{"HN", 0x32}, /* Honduras */
	{"HR", 0x26}, /* Croatia */
	{"HT", 0x34}, /* Haiti */
	{"HU", 0x26}, /* Hungary */
	{"ID", 0x54}, /* Indonesia */
	{"IE", 0x26}, /* Ireland */
	{"IL", 0x47}, /* Israel */
	{"IM", 0x26}, /* Isle of Man (UK) */
	{"IN", 0x47}, /* India */
	{"IQ", 0x26}, /* Iraq */
	{"IR", 0x26}, /* Iran */
	{"IS", 0x26}, /* Iceland */
	{"IT", 0x26}, /* Italy */
	{"JE", 0x26}, /* Jersey (UK) */
	{"JM", 0x51}, /* Jamaica */
	{"JO", 0x49}, /* Jordan */
	{"JP", 0x27}, /* Japan- Telec */
	{"KE", 0x47}, /* Kenya */
	{"KG", 0x26}, /* Kyrgyzstan */
	{"KH", 0x26}, /* Cambodia */
	{"KI", 0x26}, /* Kiribati */
	{"KN", 0x34}, /* Saint Kitts and Nevis */
	{"KR", 0x28}, /* South Korea */
	{"KW", 0x47}, /* Kuwait */
	{"KY", 0x34}, /* Cayman Islands (UK) */
	{"KZ", 0x26}, /* Kazakhstan */
	{"LA", 0x26}, /* Laos */
	{"LB", 0x26}, /* Lebanon */
	{"LC", 0x34}, /* Saint Lucia */
	{"LI", 0x26}, /* Liechtenstein */
	{"LK", 0x26}, /* Sri Lanka */
	{"LR", 0x26}, /* Liberia */
	{"LS", 0x26}, /* Lesotho */
	{"LT", 0x26}, /* Lithuania */
	{"LU", 0x26}, /* Luxembourg */
	{"LV", 0x26}, /* Latvia */
	{"LY", 0x26}, /* Libya */
	{"MA", 0x47}, /* Morocco */
	{"MC", 0x26}, /* Monaco */
	{"MD", 0x26}, /* Moldova */
	{"ME", 0x26}, /* Montenegro */
	{"MF", 0x34}, /* Saint Martin */
	{"MG", 0x26}, /* Madagascar */
	{"MH", 0x34}, /* Marshall Islands (USA) */
	{"MK", 0x26}, /* Republic of Macedonia (FYROM) */
	{"ML", 0x26}, /* Mali */
	{"MM", 0x26}, /* Burma (Myanmar) */
	{"MN", 0x26}, /* Mongolia */
	{"MO", 0x26}, /* Macau */
	{"MP", 0x34}, /* Northern Mariana Islands (USA) */
	{"MQ", 0x26}, /* Martinique (France) */
	{"MR", 0x26}, /* Mauritania */
	{"MS", 0x26}, /* Montserrat (UK) */
	{"MT", 0x26}, /* Malta */
	{"MU", 0x26}, /* Mauritius */
	{"MV", 0x26}, /* Maldives */
	{"MW", 0x26}, /* Malawi */
	{"MX", 0x34}, /* Mexico */
	{"MY", 0x47}, /* Malaysia */
	{"MZ", 0x26}, /* Mozambique */
	{"NA", 0x26}, /* Namibia */
	{"NC", 0x26}, /* New Caledonia */
	{"NE", 0x26}, /* Niger */
	{"NF", 0x45}, /* Norfolk Island (Australia) */
	{"NG", 0x50}, /* Nigeria */
	{"NI", 0x34}, /* Nicaragua */
	{"NL", 0x26}, /* Netherlands */
	{"NO", 0x26}, /* Norway */
	{"NP", 0x47}, /* Nepal */
	{"NR", 0x26}, /* Nauru */
	{"NU", 0x45}, /* Niue */
	{"NZ", 0x45}, /* New Zealand */
	{"OM", 0x26}, /* Oman */
	{"PA", 0x34}, /* Panama */
	{"PE", 0x34}, /* Peru */
	{"PF", 0x26}, /* French Polynesia (France) */
	{"PG", 0x26}, /* Papua New Guinea */
	{"PH", 0x26}, /* Philippines */
	{"PK", 0x51}, /* Pakistan */
	{"PL", 0x26}, /* Poland */
	{"PM", 0x26}, /* Saint Pierre and Miquelon (France) */
	{"PR", 0x34}, /* Puerto Rico */
	{"PT", 0x26}, /* Portugal */
	{"PW", 0x34}, /* Palau */
	{"PY", 0x34}, /* Paraguay */
	{"QA", 0x51}, /* Qatar */
	{"RE", 0x26}, /* Reunion (France) */
	{"RO", 0x26}, /* Romania */
	{"RS", 0x26}, /* Serbia */
	{"RU", 0x59}, /* Russia, fac/gost */
	{"RW", 0x26}, /* Rwanda */
	{"SA", 0x26}, /* Saudi Arabia */
	{"SB", 0x26}, /* Solomon Islands */
	{"SC", 0x34}, /* Seychelles */
	{"SE", 0x26}, /* Sweden */
	{"SG", 0x47}, /* Singapore */
	{"SH", 0x26}, /* Saint Helena (UK) */
	{"SI", 0x26}, /* Slovenia */
	{"SJ", 0x26}, /* Svalbard (Norway) */
	{"SK", 0x26}, /* Slovakia */
	{"SL", 0x26}, /* Sierra Leone */
	{"SM", 0x26}, /* San Marino */
	{"SN", 0x26}, /* Senegal */
	{"SO", 0x26}, /* Somalia */
	{"SR", 0x34}, /* Suriname */
	{"ST", 0x34}, /* Sao Tome and Principe */
	{"SV", 0x30}, /* El Salvador */
	{"SX", 0x34}, /* Sint Marteen */
	{"SZ", 0x26}, /* Swaziland */
	{"TC", 0x26}, /* Turks and Caicos Islands (UK) */
	{"TD", 0x26}, /* Chad */
	{"TF", 0x26}, /* French Southern and Antarctic Lands (FR Southern Territories) */
	{"TG", 0x26}, /* Togo */
	{"TH", 0x26}, /* Thailand */
	{"TJ", 0x26}, /* Tajikistan */
	{"TK", 0x45}, /* Tokelau */
	{"TM", 0x26}, /* Turkmenistan */
	{"TN", 0x47}, /* Tunisia */
	{"TO", 0x26}, /* Tonga */
	{"TR", 0x26}, /* Turkey */
	{"TT", 0x42}, /* Trinidad & Tobago */
	{"TW", 0x39}, /* Taiwan */
	{"TZ", 0x26}, /* Tanzania */
	{"UA", 0x26}, /* Ukraine */
	{"UG", 0x26}, /* Uganda */
	{"US", 0x34}, /* United States of America (USA) */
	{"UY", 0x34}, /* Uruguay */
	{"UZ", 0x47}, /* Uzbekistan */
	{"VA", 0x26}, /* Holy See (Vatican City) */
	{"VC", 0x34}, /* Saint Vincent and the Grenadines */
	{"VE", 0x30}, /* Venezuela */
	{"VI", 0x34}, /* United States Virgin Islands (USA) */
	{"VN", 0x26}, /* Vietnam */
	{"VU", 0x26}, /* Vanuatu */
	{"WF", 0x26}, /* Wallis and Futuna (France) */
	{"WS", 0x34}, /* Samoa */
	{"YE", 0x26}, /* Yemen */
	{"YT", 0x26}, /* Mayotte (France) */
	{"ZA", 0x26}, /* South Africa */
	{"ZM", 0x26}, /* Zambia */
	{"ZW", 0x26}, /* Zimbabwe */
};

u16 country_chplan_map_sz = sizeof(country_chplan_map)/sizeof(struct country_chplan);

/*
* rtw_get_chplan_from_country -
* @country_code: string of country code
*
* Return channel_plan index or -1 when unsupported country_code is given
*/
int rtw_get_chplan_from_country(const char *country_code)
{
	int channel_plan = -1;
	int i;

	/* TODO: should consider 3-character country code? */

	for (i = 0; i < country_chplan_map_sz; i++) {
		if (strncmp(country_code, country_chplan_map[i].alpha2, 2) == 0) {
			channel_plan = country_chplan_map[i].chplan;
			break;
		}
	}

	return channel_plan;
}

int rtw_ch_to_bb_gain_sel(int ch)
{
	int sel = -1;

	if (ch >= 1 && ch <= 14)
		sel = BB_GAIN_2G;
#ifdef CONFIG_IEEE80211_BAND_5GHZ
	else if (ch >= 36 && ch < 48)
		sel = BB_GAIN_5GLB1;
	else if (ch >= 52 && ch <= 64)
		sel = BB_GAIN_5GLB2;
	else if (ch >= 100 && ch <= 120)
		sel = BB_GAIN_5GMB1;
	else if (ch >= 124 && ch <= 144)
		sel = BB_GAIN_5GMB2;
	else if (ch >= 149 && ch <= 177)
		sel = BB_GAIN_5GHB;
#endif

	return sel;
}

s8 rtw_rf_get_kfree_tx_gain_offset(_adapter *padapter, u8 path, u8 ch)
{
	s8 kfree_offset = 0;

#ifdef CONFIG_RF_GAIN_OFFSET
	HAL_DATA_TYPE *hal_data = GET_HAL_DATA(padapter);
	struct kfree_data_t *kfree_data = GET_KFREE_DATA(padapter);
	s8 bb_gain_sel = rtw_ch_to_bb_gain_sel(ch);

	if (bb_gain_sel < BB_GAIN_2G || bb_gain_sel >= BB_GAIN_NUM) {
		rtw_warn_on(1);
		goto exit;
	}

	if (kfree_data->flag & KFREE_FLAG_ON) {
		kfree_offset = kfree_data->bb_gain[bb_gain_sel][path];
		if (1)
			DBG_871X("%s path:%u, ch:%u, bb_gain_sel:%d, kfree_offset:%d\n"
				, __func__, path, ch, bb_gain_sel, kfree_offset);
	}
exit:
#endif /* CONFIG_RF_GAIN_OFFSET */
	return kfree_offset;
}

void rtw_rf_set_tx_gain_offset(_adapter *adapter, u8 path, s8 offset)
{
	u8 write_value;

	DBG_871X("kfree gain_offset 0x55:0x%x ", rtw_hal_read_rfreg(adapter, path, 0x55, 0xffffffff));
	switch (rtw_get_chip_type(adapter)) {
#ifdef CONFIG_RTL8703B
	case RTL8703B:
		write_value = RF_TX_GAIN_OFFSET_8703B(offset);
		rtw_hal_write_rfreg(adapter, path, 0x55, 0x0fc000, write_value);
		break;
#endif /* CONFIG_RTL8703B */
#ifdef CONFIG_RTL8188F
	case RTL8188F:
		write_value = RF_TX_GAIN_OFFSET_8188F(offset);
		rtw_hal_write_rfreg(adapter, path, 0x55, 0x0fc000, write_value);
		break;
#endif /* CONFIG_RTL8188F */
#ifdef CONFIG_RTL8192E
	case RTL8192E:
		write_value = RF_TX_GAIN_OFFSET_8192E(offset);
		rtw_hal_write_rfreg(adapter, path, 0x55, 0x0f8000, write_value);
		break;
#endif /* CONFIG_RTL8188F */

#ifdef CONFIG_RTL8821A
	case RTL8821:
		write_value = RF_TX_GAIN_OFFSET_8821A(offset);
		rtw_hal_write_rfreg(adapter, path, 0x55, 0x0f8000, write_value);
		break;
#endif /* CONFIG_RTL8821A */
#ifdef CONFIG_RTL8814A
		case RTL8814A:
		DBG_871X("\nkfree by PhyDM on the sw CH. path %d\n", path);
		break;
#endif /* CONFIG_RTL8821A */

	default:
		rtw_warn_on(1);
		break;
	}

	DBG_871X(" after :0x%x\n", rtw_hal_read_rfreg(adapter, path, 0x55, 0xffffffff));
}

void rtw_rf_apply_tx_gain_offset(_adapter *adapter, u8 ch)
{
	HAL_DATA_TYPE *hal_data = GET_HAL_DATA(adapter);
	s8 kfree_offset = 0;
	s8 tx_pwr_track_offset = 0; /* TODO: 8814A should consider tx pwr track when setting tx gain offset */
	s8 total_offset;
	int i;

	for (i = 0; i < hal_data->NumTotalRFPath; i++) {
		kfree_offset = rtw_rf_get_kfree_tx_gain_offset(adapter, i, ch);
		total_offset = kfree_offset + tx_pwr_track_offset;
		rtw_rf_set_tx_gain_offset(adapter, i, total_offset);
	}
}

bool rtw_is_dfs_range(u32 hi, u32 lo)
{
	return rtw_is_range_overlap(hi, lo, 5720 + 10, 5260 - 10)?_TRUE:_FALSE;
}

bool rtw_is_dfs_ch(u8 ch, u8 bw, u8 offset)
{
	u32 hi, lo;

	if (rtw_chbw_to_freq_range(ch, bw, offset, &hi, &lo) == _FALSE)
		return _FALSE;

	return rtw_is_dfs_range(hi, lo)?_TRUE:_FALSE;
}

bool rtw_is_long_cac_range(u32 hi, u32 lo)
{
	return rtw_is_range_overlap(hi, lo, 5660 + 10, 5600 - 10)?_TRUE:_FALSE;
}

bool rtw_is_long_cac_ch(u8 ch, u8 bw, u8 offset)
{
	u32 hi, lo;

	if (rtw_chbw_to_freq_range(ch, bw, offset, &hi, &lo) == _FALSE)
		return _FALSE;

	return rtw_is_long_cac_range(hi, lo)?_TRUE:_FALSE;
}

