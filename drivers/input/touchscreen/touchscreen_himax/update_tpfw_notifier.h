/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _UPDATE_TPFW_NOTIFIER_H
#define _UPDATE_TPFW_NOTIFIER_H

#include <linux/notifier.h>

#define DRM_PANEL_CS_HIGH_BLANK        0x01
#define DRM_PANEL_CS_LOW_BLANK         0x02
#define DRM_PANEL_UPDATE_TPFW_BLANK    0x03

int update_tpfw_register_client(struct notifier_block *nb);
int update_tpfw_unregister_client(struct notifier_block *nb);
int update_tpfw_notifier_call_chain(unsigned long val, void *v);

#endif
