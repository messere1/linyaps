// SPDX-FileCopyrightText: 2026 messere1
// SPDX-License-Identifier: LGPL-3.0-or-later

#include "linglong/utils/namespace.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include <pwd.h>
#include <unistd.h>

namespace {
int calls = 0;
int mode = 0;
} // namespace

extern "C" long __real_sysconf(int);

extern "C" long __wrap_sysconf(int key)
{
    return key == _SC_GETPW_R_SIZE_MAX ? 32 : __real_sysconf(key);
}

extern "C" int __wrap_getpwuid_r(uid_t, passwd *pw, char *buffer, size_t size, passwd **result)
{
    ++calls;
    *result = nullptr;
    if (mode == 1)
        return EIO;
    if (mode == 2)
        return 0;
    // NSS records can exceed the sysconf hint, for example a large LDAP GECOS.
    if (size < 8192)
        return ERANGE;
    std::strcpy(buffer, "directory-user");
    pw->pw_name = buffer;
    *result = pw;
    return 0;
}

void require(bool value)
{
    if (!value) {
        std::cerr << "NSS lookup regression failed\n";
        std::exit(1);
    }
}

int main()
{
    auto result = linglong::utils::detail::getUserName(1000);
    require(result && *result == "directory-user");
    require(calls > 1);
    mode = 1;
    calls = 0;
    require(!linglong::utils::detail::getUserName(1000));
    require(calls == 1);
    mode = 2;
    calls = 0;
    require(!linglong::utils::detail::getUserName(1000));
    require(calls == 1);
    std::cout << "NSS ERANGE retry, hard errors and missing entries: passed\n";
}
