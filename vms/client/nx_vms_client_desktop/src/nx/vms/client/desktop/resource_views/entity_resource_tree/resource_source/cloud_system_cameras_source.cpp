// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "cloud_system_cameras_source.h"

#include <QtCore/QPointer>

#include <core/resource/camera_resource.h>
#include <nx/utils/scoped_connections.h>
#include <nx/vms/client/desktop/application_context.h>
#include <nx/vms/client/desktop/cross_system/cloud_cross_system_context.h>
#include <nx/vms/client/desktop/cross_system/cloud_cross_system_manager.h>

namespace nx::vms::client::desktop {
namespace entity_resource_tree {

struct CloudSystemCamerasSource::Private
{
    Private(CloudSystemCamerasSource* owner, const QString& systemId):
        q(owner),
        systemId(systemId)
    {
    }

    CloudSystemCamerasSource* const q;
    const QString systemId;
    QPointer<CloudCrossSystemContext> systemContext;
    nx::utils::ScopedConnections connections;

    void connectToContext()
    {
        systemContext = appContext()->cloudCrossSystemManager()->systemContext(systemId);
        if (!NX_ASSERT(systemContext))
            return;

        connections.reset();
        connections << QObject::connect(systemContext,
            &CloudCrossSystemContext::camerasAdded,
            [this](const QnVirtualCameraResourceList& cameras)
            {
                auto handler = q->addKeyHandler;
                for (auto& camera: cameras)
                    (*handler)(camera);
            });

        connections << QObject::connect(systemContext,
            &CloudCrossSystemContext::camerasRemoved,
            [this](const QnVirtualCameraResourceList& cameras)
            {
                auto handler = q->removeKeyHandler;
                for (auto& camera: cameras)
                    (*handler)(camera);
            });

        auto cameras = systemContext->cameras();
        q->setKeysHandler(QVector<QnResourcePtr>(cameras.cbegin(), cameras.cend()));
    };
};

CloudSystemCamerasSource::CloudSystemCamerasSource(const QString& systemId):
    d(new Private(this, systemId))
{
    initializeRequest =
        [this, systemId]
        {
            if (appContext()->cloudCrossSystemManager()->systemContext(systemId))
            {
                d->connectToContext();
            }
            else
            {
                d->connections << QObject::connect(appContext()->cloudCrossSystemManager(),
                    &CloudCrossSystemManager::systemFound,
                    [this, systemId](const QString& foundSystemId)
                    {
                        if (systemId == foundSystemId)
                            d->connectToContext();
                    });
            }
        };
}

CloudSystemCamerasSource::~CloudSystemCamerasSource()
{
}

} // namespace entity_resource_tree
} // namespace nx::vms::client::desktop