#ifndef __NR_SNPE_HEADER__
#define __NR_SNPE_HEADER__

#define SNPE_1550 (1550)
#define SNPE_1610 (1610)
#define SNPE_1660 (1660)
#define SNPE_1680 (1680)
#define SNPE_2070 (2070)
#define SNPE_2170 (2170)

#if SNPE_VERSION < 2000

#include "DiagLog/IDiagLog.hpp"
#include "DiagLog/Options.hpp"
#include "DlContainer/IDlContainer.hpp"
#include "DlSystem/DlEnums.hpp"
#include "DlSystem/DlError.hpp"
#include "DlSystem/ITensorFactory.hpp"
#include "DlSystem/IUserBuffer.hpp"
#include "DlSystem/RuntimeList.hpp"
#include "DlSystem/UserBufferMap.hpp"
#include "SNPE/SNPE.hpp"
#include "SNPE/SNPEBuilder.hpp"
#include "SNPE/SNPEFactory.hpp"

#else

#include "DlContainer/DlContainer.h"
#include "DlSystem/DlEnums.h"
#include "DlSystem/DlError.h"
#include "DlSystem/DlVersion.h"
#include "DlSystem/TensorShape.h"
#include "SNPE/SNPE.h"
#include "SNPE/SNPEBuilder.h"
#include "SNPE/SNPEUtil.h"

#endif


#endif