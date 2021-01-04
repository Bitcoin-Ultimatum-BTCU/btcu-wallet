/* Cable: CMake Bootstrap Library.
 * Copyright 2018 Pawel Bylica.
 * Licensed under the Apache License, Version 2.0. See the LICENSE file.
 */

#include "buildinfo.h"

const struct buildinfo* aleth_get_buildinfo()
{
    static const struct buildinfo buildinfo = {
        .project_name = "cpp-ethereum",
        .project_version = "1.6.0-alpha.1",
        .project_name_with_version = "cpp-ethereum-1.6.0-alpha.1",
        .git_commit_hash = "d5cbe3c5934f0b9ae4b0dbbe4dc264389d5281c5",
        .system_name = "",
        .system_processor = "",
        .compiler_id = "",
        .compiler_version = "",
        .build_type = "",
    };
    return &buildinfo;
}
