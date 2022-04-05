// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "event_list_model.h"

#include <QtCore/QDateTime>

#include <core/resource/camera_resource.h>
#include <nx/utils/metatypes.h>
#include <nx/vms/client/desktop/ui/actions/action_manager.h>
#include <utils/common/delayed.h>
#include <utils/common/synctime.h>

#include "private/event_list_model_p.h"

namespace nx::vms::client::desktop {

using namespace std::chrono;

EventListModel::EventListModel(QnWorkbenchContext* context, QObject* parent):
    base_type(context, parent),
    d(new Private(this))
{
}

EventListModel::~EventListModel()
{
}

int EventListModel::rowCount(const QModelIndex& /*parent*/) const
{
    return d->count();
}

QVariant EventListModel::data(const QModelIndex& index, int role) const
{
    if (!isValid(index))
        return {};

    const auto& event = d->getEvent(index.row());
    switch (role)
    {
        case Qt::DisplayRole:
            return QVariant::fromValue(event.title);

        case Qt::DecorationRole:
            return QVariant::fromValue(event.icon);

        case Qn::UuidRole:
            return QVariant::fromValue(event.id);

        case Qn::TimestampRole:
            return QVariant::fromValue(event.timestamp);

        case Qn::PreviewTimeRole:
            return QVariant::fromValue(event.previewTime);

        case Qn::ForcePrecisePreviewRole:
            return event.previewTime > 0us && event.previewTime.count() != DATETIME_NOW;

        case Qt::ToolTipRole:
            return QVariant::fromValue(event.toolTip);

        case Qn::DescriptionTextRole:
            return QVariant::fromValue(event.description);

        case Qn::NotificationLevelRole:
            return QVariant::fromValue(event.level);

        case Qt::ForegroundRole:
            return event.titleColor.isValid()
                ? QVariant::fromValue(event.titleColor)
                : QVariant();

        case Qn::ResourceRole:
            return QVariant::fromValue<QnResourcePtr>(d->previewCamera(event));

        case Qn::ResourceListRole:
            return QVariant::fromValue<QnResourceList>(d->accessibleCameras(event));

        case Qn::DisplayedResourceListRole:
            return event.source ? QVariant::fromValue<QnResourceList>({event.source}) : QVariant();

        case Qn::RemovableRole:
            return event.removable;

        case Qn::CommandActionRole:
            return QVariant::fromValue(event.extraAction);

        case Qn::TimeoutRole:
            return event.removable ? QVariant::fromValue(event.lifetime) : QVariant();

        case Qn::ObjectTrackIdRole:
            return QVariant::fromValue(event.objectTrackId);

        case Qn::GroupedAttributesRole:
            return QVariant::fromValue(event.attributes);

        default:
            return base_type::data(index, role);
    }
}

bool EventListModel::addEvent(const EventData& event, Position where)
{
    return where == Position::front
        ? d->addFront(event)
        : d->addBack(event);
}

bool EventListModel::updateEvent(const EventData& event)
{
    return d->updateEvent(event);
}

QModelIndex EventListModel::indexOf(const QnUuid& id) const
{
    return index(d->indexOf(id));
}

EventListModel::EventData EventListModel::getEvent(int row) const
{
    return d->getEvent(row);
}

void EventListModel::clear()
{
    d->clear();
}

bool EventListModel::removeEvent(const QnUuid& id)
{
    return d->removeEvent(id);
}

bool EventListModel::removeRows(int row, int count, const QModelIndex& parent)
{
    if (parent.isValid() || row < 0 || count < 0 || row + count > rowCount())
        return false;

    d->removeEvents(row, count);
    return true;
}

bool EventListModel::defaultAction(const QModelIndex& index)
{
    const auto& event = getEvent(index.row());
    if (event.actionId == ui::action::NoAction)
        return false;

    menu()->triggerIfPossible(event.actionId, event.actionParameters);
    return true;
}

} // namespace nx::vms::client::desktop
