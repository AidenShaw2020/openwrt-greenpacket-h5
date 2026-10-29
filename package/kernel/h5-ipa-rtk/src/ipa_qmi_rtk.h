/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _IPA_QMI_RTK_H_
#define _IPA_QMI_RTK_H_

#include <linux/types.h>
struct ipa;
struct h5_install_fltr_rule_req_msg;
struct h5_fltr_installed_notif_req_msg;

/* Returns a coherent snapshot; generation changes on rule update or SSR.
 * A successful QMI reply acknowledges caching, not hardware installation.
 */
int ipa_qmi_rtk_rules_copy(struct ipa *ipa, struct h5_install_fltr_rule_req_msg *rules,
			 u32 *generation);
/* Call from process context after installing/removing the real hardware rules.
 * An update/SSR during the transaction returns ESTALE; caller must quiesce
 * stale hardware state. This function never creates or authorizes a flow.
 */
int ipa_qmi_rtk_rules_notify(struct ipa *ipa,
		const struct h5_fltr_installed_notif_req_msg *notification, u32 generation);
#endif
