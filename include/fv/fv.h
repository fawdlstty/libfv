#ifndef __FV_H__
#define __FV_H__



//
// Project Name:   libfv
// Repository URL: https://github.com/fawdlstty/libfv
// Author:         fawdlstty
// License:        MIT
// Version:        0.0.8
// Last updated:   Aug 06, 2022
//



namespace fv {
inline const char *version = "libfv-0.0.8";
}



#include "declare.hpp"
#include "structs.hpp"
#include "common.hpp"
#include "common_funcs.hpp"
#include "conn.hpp"
#include "conn_impl.hpp"
#include "ioctx_pool.hpp"
#include "req_res.hpp"
#include "req_res_impl.hpp"
#include "server.hpp"
#include "session.hpp"



#endif //__FV_H__
