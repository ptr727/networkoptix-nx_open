// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "server_certificate_error_event.h"

#include "../utils/event_details.h"
#include "../utils/string_helper.h"
#include "../utils/type.h"

namespace nx::vms::rules {

ServerCertificateErrorEvent::ServerCertificateErrorEvent(
    std::chrono::microseconds timestamp,
    QnUuid serverId)
    :
    base_type(timestamp),
    m_serverId(serverId)
{
}

QVariantMap ServerCertificateErrorEvent::details(common::SystemContext* context) const
{
    auto result = BasicEvent::details(context);

    utils::insertIfNotEmpty(result, utils::kExtendedCaptionDetailName, extendedCaption(context));
    result.insert(utils::kEmailTemplatePathDetailName, manifest().emailTemplatePath);

    return result;
}

QString ServerCertificateErrorEvent::extendedCaption(common::SystemContext* context) const
{
    if (totalEventCount() == 1)
    {
        const auto resourceName = utils::StringHelper(context).resource(m_serverId, Qn::RI_WithUrl);
        return tr("Server \"%1\" certificate error").arg(resourceName);
    }

    return BasicEvent::extendedCaption();
}

const ItemDescriptor& ServerCertificateErrorEvent::manifest()
{
    static const auto kDescriptor = ItemDescriptor{
        .id = utils::type<ServerCertificateErrorEvent>(),
        .displayName = tr("Server Certificate Error"),
        .description = "",
        .emailTemplatePath = ":/email_templates/server_certificate_error.mustache"
    };
    return kDescriptor;
}

} // namespace nx::vms::rules