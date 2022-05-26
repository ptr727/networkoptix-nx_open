// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "metatypes.h"

#include <common/common_meta_types.h>
#include <nx/fusion/serialization/json_functions.h>
#include <nx/utils/metatypes.h>
#include <nx/vms/rules/camera_conflict_list.h>

#include "basic_event.h"
#include "field_types.h"

namespace nx::vms::rules {

// TODO: Consider moving this registration to the API library.
void Metatypes::initialize()
{
    static std::atomic_bool initialized = false;

    if (initialized.exchange(true))
        return;

    QnCommonMetaTypes::initialize();

    qRegisterMetaType<nx::vms::api::rules::State>();
    QnJsonSerializer::registerSerializer<State>();

    qRegisterMetaType<nx::vms::rules::EventPtr>();

    qRegisterMetaType<nx::vms::rules::CameraConflictList>();

    qRegisterMetaType<QnUuidList>("QnUuidList");
    QnJsonSerializer::registerSerializer<QnUuidList>();

    qRegisterMetaType<QnUuidSet>("QnUuidSet");
    QnJsonSerializer::registerSerializer<QnUuidSet>();

    QnJsonSerializer::registerSerializer<std::chrono::seconds>();
};

} // namespace nx::vms::rules
