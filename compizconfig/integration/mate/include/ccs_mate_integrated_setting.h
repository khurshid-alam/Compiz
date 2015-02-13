#ifndef _CCS_MATE_INTEGRATED_SETTING_H
#define _CCS_MATE_INTEGRATED_SETTING_H

#include <ccs-defs.h>
#include <ccs-fwd.h>
#include <ccs_mate_fwd.h>

#include "ccs_mate_integration_types.h"

COMPIZCONFIG_BEGIN_DECLS

typedef struct _CCSMATEIntegratedSettingInfoInterface CCSMATEIntegratedSettingInfoInterface;

typedef SpecialOptionType (*CCSMATEIntegratedSettingInfoGetSpecialOptionType) (CCSMATEIntegratedSettingInfo *);
typedef const char * (*CCSMATEIntegratedSettingInfoGetMATEName) (CCSMATEIntegratedSettingInfo *);
typedef void (*CCSMATEIntegratedSettingInfoFree) (CCSMATEIntegratedSettingInfo *);

struct _CCSMATEIntegratedSettingInfoInterface
{
    CCSMATEIntegratedSettingInfoGetSpecialOptionType getSpecialOptionType;
    CCSMATEIntegratedSettingInfoGetMATEName         getMATEName;
    CCSMATEIntegratedSettingInfoFree                 free;
};

/**
 * @brief The _CCSMATEIntegratedSetting struct
 *
 * CCSMATEIntegratedSetting represents an integrated setting in
 * MATE - it builds upon CCSSharedIntegratedSetting (which it composes
 * and implements) and also adds the concept of an MATE side keyname
 * and option type for that keyname (as the types do not match 1-1)
 */
struct _CCSMATEIntegratedSettingInfo
{
    CCSObject object;
};

unsigned int ccsCCSMATEIntegratedSettingInfoInterfaceGetType ();

Bool
ccsMATEIntegrationFindSettingsMatchingPredicate (CCSIntegratedSetting *setting,
						  void		       *userData);

SpecialOptionType
ccsMATEIntegratedSettingInfoGetSpecialOptionType (CCSMATEIntegratedSettingInfo *);

const char *
ccsMATEIntegratedSettingInfoGetMATEName (CCSMATEIntegratedSettingInfo *);

CCSMATEIntegratedSettingInfo *
ccsMATEIntegratedSettingInfoNew (CCSIntegratedSettingInfo *base,
				  SpecialOptionType    type,
				  const char	   *mateName,
				  CCSObjectAllocationInterface *ai);

void
ccsFreeMATEIntegratedSettingInfo (CCSMATEIntegratedSettingInfo *);

CCSREF_HDR (MATEIntegratedSettingInfo, CCSMATEIntegratedSettingInfo);
CCSLIST_HDR (MATEIntegratedSettingInfo, CCSMATEIntegratedSettingInfo);

COMPIZCONFIG_END_DECLS

#endif
