// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include <gtest/gtest.h>

#include <nx/utils/range_adapters.h>
#include <nx/utils/qobject.h>
#include <nx/vms/common/test_support/test_context.h>
#include <nx/vms/rules/action_fields/builtin_fields.h>
#include <nx/vms/rules/actions/builtin_actions.h>
#include <nx/vms/rules/action_builder.h>
#include <nx/vms/rules/engine.h>
#include <nx/vms/rules/event_fields/builtin_fields.h>
#include <nx/vms/rules/events/builtin_events.h>
#include <nx/vms/rules/plugin.h>
#include <nx/vms/rules/utils/serialization.h>

#include "test_event.h"
#include "test_router.h"

namespace nx::vms::rules::test {

class BuiltinTypesTest:
    public common::test::ContextBasedTest,
    public TestEngineHolder,
    public Plugin
{
public:
    BuiltinTypesTest()
    {
        initialize(engine.get());
    }

    template<class T>
    void testManifestValidity()
    {
        const auto& manifest = T::manifest();
        const auto& meta = T::staticMetaObject;

        EXPECT_FALSE(manifest.id.isEmpty());
        EXPECT_FALSE(manifest.displayName.isEmpty());

        // Check all manifest fields correspond to properties with the same name.
        for (const auto& field: manifest.fields)
        {
            SCOPED_TRACE(field.id.toStdString());

            EXPECT_FALSE(field.id.isEmpty());
            EXPECT_FALSE(field.displayName.isEmpty());
            EXPECT_FALSE(field.fieldName.isEmpty());

            const auto propIndex = meta.indexOfProperty(field.fieldName.toUtf8().data());
            EXPECT_GE(propIndex, 0);

            const auto prop = meta.property(propIndex);
            EXPECT_TRUE(prop.isValid());
            EXPECT_NE(prop.userType(), QMetaType::UnknownType);
        }
    }

    template<class T, class... Args>
    void testActionFieldRegistration(Args... args)
    {
        SCOPED_TRACE(fieldMetatype<T>().toStdString());

        EXPECT_TRUE(m_engine->registerActionField(
            fieldMetatype<T>(),
            [args...]{ return new T(args...); }));
    }

    template<class T, class... Args>
    void testEventFieldRegistration(Args... args)
    {
        SCOPED_TRACE(fieldMetatype<T>().toStdString());

        EXPECT_TRUE(m_engine->registerEventField(
            fieldMetatype<T>(),
            [args...]{ return new T(args...); }));

        // Check for serialization assertions.
        const auto field = m_engine->buildEventField(fieldMetatype<T>());
        const auto data = serializeProperties(field.get(), nx::utils::propertyNames(field.get()));
        deserializeProperties(data, field.get());
    }

    template<class T>
    void testEventRegistration()
    {
        SCOPED_TRACE(T::manifest().id.toStdString());

        testManifestValidity<T>();

        EXPECT_TRUE(registerEvent<T>());

        // Check for serialization assertions.
        const auto event = QSharedPointer<T>::create();
        event->setState(State::instant);

        const auto data = serializeProperties(event.get(), nx::utils::propertyNames(event.get()));
        deserializeProperties(data, event.get());
    }

    template<class T>
    void testActionRegistration()
    {
        const auto& manifest = T::manifest();
        SCOPED_TRACE(manifest.id.toStdString());

        testManifestValidity<T>();
        ASSERT_TRUE(registerAction<T>());

        const auto builder = engine->buildActionBuilder(manifest.id);
        ASSERT_TRUE(builder);

        const auto testEvent = QSharedPointer<TestEvent>::create();
        const auto& meta = T::staticMetaObject;
        const auto fields = builder->fields();

        // Test property type compatibility.
        for(const auto [name, field]: nx::utils::constKeyValueRange(fields))
        {
            SCOPED_TRACE(name.toStdString());
            const auto value = field->build(testEvent);
            EXPECT_TRUE(value.isValid());

            const auto prop = meta.property(meta.indexOfProperty(name.toUtf8().data()));
            EXPECT_EQ(prop.userType(), value.userType());
        }
    }
};

TEST_F(BuiltinTypesTest, BuiltinEvents)
{
    // Event fields need to be registered first.
    testEventFieldRegistration<AnalyticsEngineField>();
    testEventFieldRegistration<AnalyticsEventLevelField>();
    testEventFieldRegistration<AnalyticsEventTypeField>(systemContext());
    testEventFieldRegistration<AnalyticsObjectAttributesField>();
    testEventFieldRegistration<AnalyticsObjectTypeField>(systemContext());
    testEventFieldRegistration<CustomizableIconField>();
    testEventFieldRegistration<CustomizableTextField>();
    testEventFieldRegistration<InputPortField>();
    testEventFieldRegistration<StateField>();
    testEventFieldRegistration<EventTextField>();
    testEventFieldRegistration<ExpectedUuidField>();
    testEventFieldRegistration<IntField>();
    testEventFieldRegistration<KeywordsField>();
    testEventFieldRegistration<SourceCameraField>();
    testEventFieldRegistration<SourceServerField>();
    testEventFieldRegistration<SourceUserField>();
    testEventFieldRegistration<UniqueIdField>();

    // TODO: #amalov Uncomment all types after manifest definition.
    testEventRegistration<AnalyticsEvent>();
    testEventRegistration<AnalyticsObjectEvent>();
    //testEventRegistration<BackupFinishedEvent>(); // Deprecated
    testEventRegistration<CameraInputEvent>();
    testEventRegistration<DebugEvent>();
    testEventRegistration<DeviceDisconnectedEvent>();
    testEventRegistration<DeviceIpConflictEvent>();
    testEventRegistration<FanErrorEvent>();
    testEventRegistration<GenericEvent>();
    testEventRegistration<LicenseIssueEvent>();
    testEventRegistration<MotionEvent>();
    testEventRegistration<NetworkIssueEvent>();
    testEventRegistration<PluginDiagnosticEvent>();
    testEventRegistration<PoeOverBudgetEvent>();
    testEventRegistration<ServerCertificateErrorEvent>();
    testEventRegistration<ServerConflictEvent>();
    testEventRegistration<ServerFailureEvent>();
    testEventRegistration<ServerStartedEvent>();
    testEventRegistration<SoftTriggerEvent>();
    testEventRegistration<StorageIssueEvent>();
}

TEST_F(BuiltinTypesTest, BuiltinActions)
{
    // Action fields need to be registered first.
    testActionFieldRegistration<ActionTextField>();
    testActionFieldRegistration<ContentTypeField>();
    testActionFieldRegistration<EmailMessageField>(systemContext());
    testActionFieldRegistration<FlagField>();
    testActionFieldRegistration<HttpMethodField>();
    testActionFieldRegistration<OptionalTimeField>();
    testActionFieldRegistration<PasswordField>();
    testActionFieldRegistration<Substitution>();
    testActionFieldRegistration<TargetDeviceField>();
    testActionFieldRegistration<TargetUserField>();
    testActionFieldRegistration<TextWithFields>(systemContext());
    testActionFieldRegistration<VolumeField>();

    // TODO: #amalov Uncomment all types after manifest definition.
    //testActionRegistration<BookmarkAction>();
    //testActionRegistration<DeviceOutputAction>();
    //testActionRegistration<DeviceRecordingAction>();
    //testActionRegistration<EnterFullscreenAction>();
    //testActionRegistration<ExitFullscreenAction>();
    testActionRegistration<HttpAction>();
    testActionRegistration<NotificationAction>();
    //testActionRegistration<OpenLayoutAction>();
    //testActionRegistration<PanicRecordingAction>();
    //testActionRegistration<PlaySoundAction>();
    //testActionRegistration<PtzPresetAction>();
    //testActionRegistration<PushNotificationAction>();
    //testActionRegistration<RepeatSoundAction>();
    testActionRegistration<SendEmailAction>();
    //testActionRegistration<ShowOnAlarmLayoutAction>();
    testActionRegistration<SpeakAction>();
    testActionRegistration<TextOverlayAction>();
    //testActionRegistration<WriteToLogAction>();
}

} // namespace nx::vms::rules::test