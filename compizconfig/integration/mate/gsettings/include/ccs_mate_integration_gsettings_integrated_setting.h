#ifndef _CCS_MATE_GSETTINGS_INTEGRATED_SETTING_H
#define _CCS_MATE_GSETTINGS_INTEGRATED_SETTING_H

#include <ccs-defs.h>
#include <ccs-fwd.h>
#include <ccs_mate_fwd.h>
#include <ccs_gsettings_backend_fwd.h>

COMPIZCONFIG_BEGIN_DECLS

/**
 * @brief ccsGSettingsIntegratedSettingNew
 * @param base
 * @param wrapper
 * @param ai
 * @return
 *
 * The GSettings implementation of CCSIntegratedSetting *, which will
 * write using a CCSGSettingsWrapper * object when the read and write
 * methods are called.
 */
CCSIntegratedSetting *
ccsGSettingsIntegratedSettingNew (CCSMATEIntegratedSettingInfo *base,
				  CCSGSettingsWrapper       *wrapper,
				  CCSObjectAllocationInterface *ai);

/**
 * @brief ccsGSettingsIntegratedSettingsTranslateOldMATEKeyForGSettings
 * @param key the old style mate key to translate
 * @return new-style key. Caller should free
 *
 * This translates old style keys (eg foo_bar) to new style keys
 * foo-bar and special cases a few keys
 */
char *
ccsGSettingsIntegratedSettingsTranslateOldMATEKeyForGSettings (const char *key);

COMPIZCONFIG_END_DECLS

#endif
