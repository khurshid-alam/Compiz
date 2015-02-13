#ifndef _CCS_COMPIZCONFIG_MATE_GCONF_INTEGRATION
#define _CCS_COMPIZCONFIG_MATE_GCONF_INTEGRATION

#include <ccs-defs.h>
#include <ccs-fwd.h>
#include <ccs_mate_fwd.h>
#include "ccs_mate_integration_types.h"

COMPIZCONFIG_BEGIN_DECLS

struct _CCSMATEValueChangeData
{
    CCSIntegration *integration;
    CCSIntegratedSettingsStorage *storage;
    CCSIntegratedSettingFactory *factory;
    CCSContext     *context;
};

/**
 * @brief ccsMATEIntegrationBackendNew
 * @param backend
 * @param context
 * @param factory
 * @param storage
 * @param ai
 * @return A new CCSIntegration
 *
 * The MATE implementation of desktop environment integration - requires
 * a method to create new integrated settings, and a method to store them
 * as well.
 *
 * CCSMATEIntegration is a pure composition in most respects - it just
 * represents the process as to which settings should be written to
 * what keys and vice versa, it doesn't represent how those keys should
 * be written.
 */
CCSIntegration *
ccsMATEIntegrationBackendNew (CCSBackend *backend,
			       CCSContext *context,
			       CCSIntegratedSettingFactory *factory,
			       CCSIntegratedSettingsStorage *storage,
			       CCSObjectAllocationInterface *ai);

COMPIZCONFIG_END_DECLS

#endif
