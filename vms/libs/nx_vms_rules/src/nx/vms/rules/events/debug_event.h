// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#pragma once

#include <nx/vms/rules/basic_event.h>

#include "../data_macros.h"

namespace nx::vms::rules {

class NX_VMS_RULES_API DebugEvent: public BasicEvent
{
    Q_OBJECT
    Q_CLASSINFO("type", "nx.events.debug")

    FIELD(QString, action, setAction)
    FIELD(qint64, value, setValue)

public:
    DebugEvent() = default;
    DebugEvent(
        const QString& caption,
        const QString& description,
        const QString& action,
        qint64 value,
        std::chrono::microseconds timestamp);

    virtual QString caption() const override;
    virtual void setCaption(const QString& caption) override;
    virtual QString description() const override;
    virtual void setDescription(const QString& description) override;

    static FilterManifest filterManifest();
    static const ItemDescriptor& manifest();

private:
    QString m_caption;
    QString m_description;
};

} // namespace nx::vms::rules
