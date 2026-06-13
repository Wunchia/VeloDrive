#include "AuthServiceImpl.h"
#include <workflow/MySQLResult.h>
#include <workflow/Workflow.h>
#include <workflow/WFTaskFactory.h>
#include <workflow/mysql_types.h>
#include <string>

using namespace protocol;

const std::string AuthServiceImpl::DB_URL="mysql://root:123456@localhost/VeloDrive";
