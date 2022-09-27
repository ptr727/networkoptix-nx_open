// Copyright 2018-present Network Optix, Inc. Licensed under MPL 2.0: www.mozilla.org/MPL/2.0/

#include "data.h"

#include <algorithm>

#include <nx/sdk/cloud_storage/helpers/algorithm.h>
#include <nx/sdk/helpers/device_info.h>

#include "media_data_packet.h"

namespace nx {
namespace sdk {
namespace cloud_storage {

using namespace std::chrono;

namespace {

template<typename Data>
Data getObjectValue(const nx::kit::Json& json, const std::string& key)
{
    const auto value = json[key];
    if (!value.is_object())
        throw std::logic_error("Value '" + key + "' is not an object");

    return Data(value);
}

template<typename Data>
std::optional<Data> getOptionalObjectValue(
    const nx::kit::Json& json, const std::string& key)
{
    const auto value = json[key];
    if (!value.is_object())
        return std::nullopt;

    return std::optional<Data>(Data(value));
}

std::string getStringValue(const nx::kit::Json& json, const std::string& key)
{
    const auto value = json[key];
    if (!value.is_string())
        throw std::logic_error("Value '" + key + "' is not a string");

    return value.string_value();
}

std::optional<std::string> getOptionalStringValue(
    const nx::kit::Json& json, const std::string& key)
{
    const auto value = json[key];
    if (!value.is_string())
        return std::nullopt;

    return value.string_value();
}

int64_t getIntValue(const nx::kit::Json& json, const std::string& key)
{
    const auto value = json[key];
    if (!value.is_number())
        throw std::logic_error("Value '" + key + "' is not a number");

    return (int64_t) value.number_value();
}

bool getBoolValue(const nx::kit::Json& json, const std::string& key)
{
    const auto value = json[key];
    if (!value.is_bool())
        throw std::logic_error("Value '" + key + "' is not a boolean");

    return value.bool_value();
}

std::optional<int64_t> getOptionalIntValue(const nx::kit::Json& json, const std::string& key)
{
    const auto value = json[key];
    if (!value.is_number())
        return std::nullopt;

    return (int64_t) value.number_value();
}

template<typename Duration>
Duration getDurationValue(const nx::kit::Json& json, const std::string& key)
{
    const auto value = json[key];
    if (!value.is_number())
        throw std::logic_error("Value '" + key + "' is not a number");

    return Duration((int64_t) value.number_value());
}

template<typename Duration>
std::optional<Duration> getOptionalDurationValue(
    const nx::kit::Json& json, const std::string& key)
{
    const auto value = json[key];
    if (!value.is_number())
        return std::nullopt;

    return Duration((int64_t) value.number_value());
}

std::vector<std::string> getStringListValue(const nx::kit::Json& json, const std::string& key)
{
    const auto value = json[key];
    if (!value.is_array())
        throw std::logic_error("Value '" + key + "' is not an array");

    std::vector<std::string> result;
    for (const auto& entry: value.array_items())
    {
        if (!entry.is_string())
            throw std::logic_error("Value '" + key + "' is not an array of strings");

        result.push_back(entry.string_value());
    }

    return result;
}

template<typename Data>
std::vector<Data> objectListFromJson(nx::kit::Json json)
{
    if (!json.is_array())
        throw std::logic_error("Object list is not an array");

    std::vector<Data> result;
    for (const auto& oJson : json.array_items())
    {
        if (!oJson.is_object())
            throw std::logic_error("Object is not a json object");

        result.push_back(Data(oJson));
    }

    return result;
}

template<typename Data>
std::vector<Data> getObjectListValue(const nx::kit::Json& json, const std::string& key)
{
    const auto value = json[key];
    return objectListFromJson<Data>(value);
}

nx::kit::Json parseJson(const std::string& jsonStr)
{
    std::string parseError;
    const auto json = nx::kit::Json::parse(jsonStr, parseError);
    if (!parseError.empty())
    {
        throw std::logic_error(
            "Failed to parse json. Error: " + parseError + ". Original string: " + jsonStr);
    }

    return json;
}

template<typename Data>
std::vector<Data> objectListFromJson(const char* data)
{
    return objectListFromJson<Data>(parseJson(data));
}

template<typename T>
nx::kit::Json objectListToJson(const std::vector<T>& objectList)
{
    auto result = nx::kit::Json::array();
    for (const auto& o: objectList)
        result.push_back(nx::kit::Json(o));

    return result;
}

} // namespace

PluginManifest::PluginManifest(const char* jsonStr): PluginManifest(parseJson(jsonStr))
{
}

PluginManifest::PluginManifest(const nx::kit::Json& json)
{
    id = getStringValue(json, "id");
    name = getStringValue(json, "name");
    description = getStringValue(json, "description");
    version = getStringValue(json, "version");
    vendor = getStringValue(json, "vendor");
}

PluginManifest::PluginManifest(
    const std::string& id,
    const std::string& name,
    const std::string& description,
    const std::string& version,
    const std::string& vendor)
    :
    id(id),
    name(name),
    description(description),
    version(version),
    vendor(vendor)
{
}

nx::kit::Json PluginManifest::to_json() const
{
    return nx::kit::Json::object({
        {"id", id},
        {"name", name},
        {"description", description},
        {"version", version},
        {"vendor", vendor},
        });
}

bool TimePeriod::contains(std::chrono::milliseconds timestamp) const
{
    return (timestamp >= startTimestamp)
        && (isInfinite() || timestamp < startTimestamp + duration);
}

bool TimePeriod::isInfinite() const
{
    return duration == -1ms;
}

bool TimePeriod::isNull() const
{
    return startTimestamp == -1ms;
}

bool TimePeriod::intersects(const TimePeriod& other) const
{
    if (isNull() || other.isNull())
        return false;

    if (startTimestamp == other.startTimestamp)
        return true;

    if (startTimestamp > other.startTimestamp)
    {
        if (other.isInfinite() || other.endTimestamp() > startTimestamp)
            return true;
    }

    if (other.startTimestamp > startTimestamp)
    {
        if (isInfinite() || endTimestamp() > other.startTimestamp)
            return true;
    }

    return false;
}

std::optional<std::chrono::milliseconds> TimePeriod::endTimestamp() const
{
    if (isInfinite())
        return std::nullopt;

    return startTimestamp + duration;
}

nx::kit::Json TimePeriod::to_json() const
{
    return nx::kit::Json::object(
        {{"startTimestamp", nx::kit::Json((double) startTimestamp.count())},
        {"duration", (double) duration.count()} });
}

bool TimePeriod::operator<(const TimePeriod& other) const
{
    return this->startTimestamp < other.startTimestamp;
}

bool TimePeriod::operator==(const TimePeriod& other) const
{
    return startTimestamp == other.startTimestamp && duration == other.duration;
}

KeyValuePair::KeyValuePair(const char* jsonStr): KeyValuePair(parseJson(jsonStr))
{
}

KeyValuePair::KeyValuePair(const nx::kit::Json& json)
{
    if (!json.is_object())
        throw std::logic_error("KeyValuePair is not an object");

    name = getStringValue(json, "name");
    value = getStringValue(json, "value");
}

KeyValuePair::KeyValuePair(const std::string& name, const std::string& value):
    name(name), value(value)
{
}

nx::kit::Json KeyValuePair::to_json() const
{
    return nx::kit::Json::object({
        {"name", name},
        {"value", value},
        });
}

nx::kit::Json keyValuePairsToJson(const std::vector<KeyValuePair>& keyValuePairList)
{
    auto result = nx::kit::Json::array();
    for (const auto& p: keyValuePairList)
        result.push_back(nx::kit::Json(p));

    return result;
}

nx::kit::Json DeviceDescription::to_json() const
{
    return nx::kit::Json::object({
        {"parameters", keyValuePairsToJson(parameters)}
        });
}

DeviceDescription::DeviceDescription(const char* jsonData): DeviceDescription(parseJson(jsonData))
{
}

DeviceDescription::DeviceDescription(const nx::kit::Json& json)
{
    parameters = getObjectListValue<KeyValuePair>(json, "parameters");
}

DeviceDescription::DeviceDescription(const nx::sdk::IDeviceInfo* info)
{
    parameters.push_back(DeviceParameter{ "id", info->id() });
    parameters.push_back(
        DeviceParameter{ "channelNumber", std::to_string(info->channelNumber()) });

    parameters.push_back(DeviceParameter{ "firmware", info->firmware() });
    parameters.push_back(DeviceParameter{ "logicalId", info->logicalId() });
    parameters.push_back(DeviceParameter{ "login", info->login() });
    parameters.push_back(DeviceParameter{ "model", info->model() });
    parameters.push_back(DeviceParameter{ "name", info->name() });
    parameters.push_back(DeviceParameter{ "password", info->password() });
    parameters.push_back(DeviceParameter{ "sharedId", info->sharedId() });
    parameters.push_back(DeviceParameter{ "url", info->url() });
    parameters.push_back(DeviceParameter{ "vendor", info->vendor() });
}

std::optional<std::string> DeviceDescription::deviceId() const
{
    const std::string result = getParamValue("id");
    if (result.empty())
        throw std::logic_error("No id in device description");

    return result.empty() ? std::nullopt : std::optional<std::string>(result);
}

std::string deviceId(nx::sdk::cloud_storage::IDeviceAgent* deviceAgent)
{
    return nx::sdk::toPtr(deviceAgent->deviceInfo().value())->id();
}

int toStreamIndex(nxcip::MediaStreamQuality quality)
{
    switch (quality)
    {
        case nxcip::msqDefault:
        case nxcip::msqHigh:
            return 0;
        case nxcip::msqLow:
            return 1;
    }

    throw std::logic_error("Unknown MediaStreamQuality");
}

std::string toString(sdk::cloud_storage::MetadataType metadataType)
{
    switch (metadataType)
    {
        case sdk::cloud_storage::MetadataType::analytics: return "analytics";
        case sdk::cloud_storage::MetadataType::motion: return "motion";
        case sdk::cloud_storage::MetadataType::bookmark: return "bookmark";
    }

    throw std::logic_error("Unknown MetadataType");
}

Bookmark::Bookmark(const char* jsonData): Bookmark(parseJson(jsonData))
{}

Bookmark::Bookmark(const nx::kit::Json& json)
{
    id = getStringValue(json, "id");
    creatorId = getStringValue(json, "creatorId");
    creationTimestamp = milliseconds(getIntValue(json, "creationTimestamp"));
    name = getStringValue(json, "name");
    description = getStringValue(json, "description");
    timeout = milliseconds(getIntValue(json, "timeout"));
    startTimestamp = milliseconds(getIntValue(json, "startTimestamp"));
    duration = milliseconds(getIntValue(json, "duration"));
    tags = getStringListValue(json, "tags");
    deviceId = getStringValue(json, "deviceId");
}

nx::kit::Json Bookmark::to_json() const
{
    return nx::kit::Json::object({
        {"id", id},
        {"creatorId", creatorId},
        {"creationTimestamp", (double) creationTimestamp.count()},
        {"name", name},
        {"description", description},
        {"timeout", (double) timeout.count()},
        {"startTimestamp", (double) startTimestamp.count()},
        {"duration", (double) duration.count()},
        {"tags", tags},
        {"deviceId", deviceId}
        });
}

BookmarkList bookmarkListFromJson(const char* data)
{
    const auto json = parseJson(data);
    if (!json.is_array())
        throw std::logic_error("Bookmark list is not an array");

    BookmarkList result;
    for (const auto& bookmarkJson: json.array_items())
    {
        if (!bookmarkJson.is_object())
            throw std::logic_error("Bookmark list is not an object");

        result.push_back(Bookmark(bookmarkJson));
    }

    return result;
}
std::string sortOrderToString(SortOrder order)
{
    switch (order)
    {
        case SortOrder::ascending:
            return "ascending";
        case SortOrder::descending:
            return "descending";
    }

    throw std::logic_error("Unexpected value of the SortOrder");
}

SortOrder sortOrderFromString(const std::string& s)
{
    if (s == "ascending")
        return SortOrder::ascending;

    if (s == "descending")
        return SortOrder::descending;

    throw std::logic_error("Unexpected value of the SortOrder string: " + s);
}

std::string BookmarkFilter::sortColumnToString(SortColumn column)
{
    switch (column)
    {
        case SortColumn::creationTime:
            return "creationTime";
        case SortColumn::description:
            return "description";
        case SortColumn::duration:
            return "duration";
        case SortColumn::name:
            return "name";
        case SortColumn::startTime:
            return "startTime";
        case SortColumn::tags:
            return "tags";
    }

    throw std::logic_error("Unexpected value of the SortColumn");
}

BookmarkFilter::SortColumn BookmarkFilter::sortColumnFromString(const std::string& s)
{
    if (s == "creationTime")
        return SortColumn::creationTime;
    if (s == "description")
        return SortColumn::description;
    if (s == "duration")
        return SortColumn::duration;
    if (s == "name")
        return SortColumn::name;
    if (s == "startTime")
        return SortColumn::startTime;
    if (s == "tags")
        return SortColumn::tags;

    throw std::logic_error("Unexpected value of the SortColumn string: " + s);
}


BookmarkFilter::BookmarkFilter(const char* jsonData): BookmarkFilter(parseJson(jsonData))
{}

BookmarkFilter::BookmarkFilter(const nx::kit::Json& json)
{
    id = getOptionalStringValue(json, "id");
    startTimestamp = getOptionalDurationValue<milliseconds>(json, "startTimestamp");
    endTimestamp = getOptionalDurationValue<milliseconds>(json, "endTimestamp");
    text = getOptionalStringValue(json, "text");
    const auto optionalLimit = getOptionalIntValue(json, "limit");
    limit = optionalLimit ? std::optional<int>((int) *optionalLimit) : std::nullopt;
    order = sortOrderFromString(getStringValue(json, "order"));
    column = sortColumnFromString(getStringValue(json, "column"));
    minVisibleLength = getOptionalDurationValue<milliseconds>(json, "minVisibleLength");
    std::vector<std::string> deviceIds = getStringListValue(json, "deviceIds");
    creationStartTimestamp = getOptionalDurationValue<milliseconds>(json, "creationStartTimestamp");
    creationEndTimestamp = getOptionalDurationValue<milliseconds>(json, "creationEndTimestamp");
}

nx::kit::Json BookmarkFilter::to_json() const
{
    auto result =  nx::kit::Json::object({
        {"order", sortOrderToString(order)},
        {"column", sortColumnToString(column)},
        });

    if (id)
        result["id"] = *id;

    if (startTimestamp)
        result["startTimestamp"] = (double) startTimestamp->count();

    if (endTimestamp)
        result["endTimestamp"] = (double) endTimestamp->count();

    if (text)
        result["text"] = *text;

    if (limit)
        result["limit"] = *limit;

    if (minVisibleLength)
        result["minVisibleLength"] = (double) minVisibleLength->count();

    if (creationStartTimestamp)
        result["creationStartTimestamp"] = (double) creationStartTimestamp->count();

    if (creationEndTimestamp)
        result["creationEndTimestamp"] = (double) creationEndTimestamp->count();

    result["deviceIds"] = objectListToJson(deviceIds);
    return result;
}

Motion::Motion(const char* jsonData): Motion(parseJson(jsonData))
{
}

Motion::Motion(const nx::kit::Json& json)
{
    channel = getIntValue(json, "channel");
    startTimestamp = milliseconds(getIntValue(json, "startTimestamp"));
    duration = milliseconds(getIntValue(json, "duration"));
    deviceId = getStringValue(json, "deviceId");
    dataBase64 = getStringValue(json, "dataBase64");
}

nx::kit::Json Motion::to_json() const
{
    return nx::kit::Json::object({
        {"channel", channel},
        {"startTimestamp", (double) startTimestamp.count()},
        {"duration", (double) duration.count()},
        {"deviceId", deviceId},
        {"dataBase64", dataBase64},
        });
}

Rect::Rect(const char* jsonData): Rect(parseJson(jsonData))
{
}

Rect::Rect(const nx::kit::Json& json)
{
    x = getIntValue(json, "x");
    y = getIntValue(json, "y");
    w = getIntValue(json, "w");
    h = getIntValue(json, "h");
}

Rect::Rect(double x, double y, double w, double h):
    x(x), y(y), w(w), h(h)
{}

bool Rect::isInside(double x, double y) const
{
    return x >= this->x && x <= this->x + this->w && y <= this->y && y >= this->y - this->w;
}

bool Rect::isEmpty() const
{
    return w <= 0 || h <= 0;
}

bool Rect::intersectsWith(const Rect& other) const
{
    return isInside(other.left(), other.top()) || isInside(other.left(), other.bottom())
        || isInside(other.right(), other.top()) || isInside(other.right(), other.bottom());
}

double Rect::left() const
{
    return x;
}

double Rect::top() const
{
    return y;
}

double Rect::right() const
{
    return x + w;
}

double Rect::bottom() const
{
    return y - h;
}

nx::kit::Json Rect::to_json() const
{
    return nx::kit::Json::object({
        {"x", x},
        {"y", y},
        {"w", w},
        {"h", h},
        });
}

TimePeriod::TimePeriod(const char* jsonStr): TimePeriod(parseJson(jsonStr))
{
}

TimePeriod::TimePeriod(const nx::kit::Json& json)
{
    startTimestamp = milliseconds(getIntValue(json, "startTimestamp"));
    duration = milliseconds(getIntValue(json, "duration"));
}

TimePeriod::TimePeriod(
    std::chrono::milliseconds startTimestamp,
    std::chrono::milliseconds duration)
    :
    startTimestamp(startTimestamp),
    duration(duration)
{
}

MotionFilter::MotionFilter(const char* jsonData): MotionFilter(parseJson(jsonData))
{
}

MotionFilter::MotionFilter(const nx::kit::Json& json)
{
    deviceIds = getStringListValue(json, "deviceIds");
    timePeriod = getObjectValue<TimePeriod>(json, "timePeriod");
    regions = getObjectListValue<Rect>(json, "regions");
    const auto optionalLimit = getOptionalIntValue(json, "limit");
    limit = optionalLimit ? std::optional<int>((int)*optionalLimit) : std::nullopt;
    order = sortOrderFromString(getStringValue(json, "order"));
    detailLevel = milliseconds(getIntValue(json, "detailLevel"));
}

nx::kit::Json MotionFilter::to_json() const
{
    auto result = nx::kit::Json::object({
        {"timePeriod", timePeriod},
        {"order", sortOrderToString(order)},
        {"detailLevel", (double) detailLevel.count() },
        });

    auto deviceIdJson = nx::kit::Json::array();
    for (const auto id : deviceIds)
        deviceIdJson.push_back(nx::kit::Json(id));

    result["deviceIds"] = objectListToJson(deviceIds);
    result["regions"] = objectListToJson(regions);
    if (limit)
        result["limit"] = *limit;

    return result;
}

nx::kit::Json timePeriodListToJson(const TimePeriodList& timePeriods)
{
    auto result = nx::kit::Json::array();
    for (const auto& tp: timePeriods)
        result.push_back(nx::kit::Json(tp));

    return result;
}

TimePeriodList timePeriodListFromJson(const char* data)
{
    return objectListFromJson<TimePeriod>(data);
}

CodecInfoData::CodecInfoData()
{
}

const char* DeviceDescription::getParamValue(const std::string& key) const
{
    const auto it = std::find_if(
        parameters.cbegin(), parameters.cend(),
        [&key](const auto& p) { return p.name == key; });

    if (it == parameters.cend())
        return "";

    return it->value.data();
}

IDeviceInfo* deviceInfo(const DeviceDescription& deviceDescription)
{
    auto result = new nx::sdk::DeviceInfo();

    auto get = [&](const std::string& key) { return deviceDescription.getParamValue(key); };
    const std::string channelNumberStr = get("channelNumber");
    int channelNumber = channelNumberStr == "" ? -1 : std::stoi(channelNumberStr);

    result->setChannelNumber(channelNumber);
    result->setFirmware(get("firmware"));
    result->setId(get("id"));
    result->setLogicalId(get("logicalId"));
    result->setLogin(get("login"));
    result->setModel(get("model"));
    result->setName(get("name"));
    result->setPassword(get("password"));
    result->setSharedId(get("sharedId"));
    result->setUrl(get("url"));
    result->setVendor(get("vendor"));

    return result;
}

std::string AnalyticsFilter::optionsToString(int options)
{
    if (options == 0)
        return "none";

    std::string result;
    if (options & Option::ignoreBoundingBox)
        result = "ignoreBoundingBox";

    if (options & Option::ignoreTextFilter)
    {
        if (!result.empty())
            result += "|";

        result += "ignoreTextFilter";
    }

    if (options & Option::ignoreTimePeriod)
    {
        if (!result.empty())
            result += "|";

        result += "ignoreTimePeriod";
    }

    return result;
}

int AnalyticsFilter::optionsFromString(const std::string& s)
{
    if (s.empty())
        throw std::logic_error("Options string is unexpectedly empty");

    if (s == "none")
        return 0;

    int result = 0;
    if (s.find("ignoreBoundingBox") != std::string::npos)
        result |= Option::ignoreBoundingBox;

    if (s.find("ignoreTextFilter") != std::string::npos)
        result |= Option::ignoreTextFilter;

    if (s.find("ignoreTimePeriod") != std::string::npos)
        result |= Option::ignoreTimePeriod;

    if (result == 0)
        throw std::logic_error("Unexpected value of the options string: " + s);

    return result;
}


AnalyticsFilter::AnalyticsFilter(const char* jsonData): AnalyticsFilter(parseJson(jsonData))
{
}

AnalyticsFilter::AnalyticsFilter(const nx::kit::Json& json)
{
    deviceIds = getStringListValue(json, "deviceIds");
    objectTypeIds = getStringListValue(json, "objectTypeIds");
    objectTrackId = getOptionalStringValue(json, "objectTrackId");
    timePeriod = getObjectValue<TimePeriod>(json, "timePeriod");
    boundingBox = getOptionalObjectValue<Rect>(json, "boundingBox");
    const auto maybeMaxObjectTracksToSelect = getOptionalIntValue(json, "maxObjectTracksToSelect");
    if (maybeMaxObjectTracksToSelect)
        maxObjectTracksToSelect = (int) *maybeMaxObjectTracksToSelect;

    order = sortOrderFromString(getStringValue(json, "order"));
    withBestShotOnly = getBoolValue(json, "withBestShotOnly");
    analyticsEngineId = getOptionalStringValue(json, "analyticsEngineId");
    options = optionsFromString(getStringValue(json, "options"));
    detailLevel = getDurationValue<milliseconds>(json, "detailLevel");
}

nx::kit::Json AnalyticsFilter::to_json() const
{
    auto result = nx::kit::Json::object();
    result["deviceIds"] = objectListToJson(deviceIds);
    result["objectTypeIds"] = objectListToJson(objectTypeIds);
    if (objectTrackId)
        result["objectTrackId"] = *objectTrackId;

    result["timePeriod"] = timePeriod;
    if (boundingBox)
        result["boundingBox"] = *boundingBox;

    if (maxObjectTracksToSelect)
        result["maxObjectTracksToSelect"] = *maxObjectTracksToSelect;

    result["order"] = sortOrderToString(order);
    result["withBestShotOnly"] = withBestShotOnly;
    if (analyticsEngineId)
        result["analyticsEngineId"] = *analyticsEngineId;

    result["options"] = optionsToString(options);
    result["detailLevel"] = (double) detailLevel.count();

    return result;
}

ObjectRegion::ObjectRegion(const nx::kit::Json& json)
{
    boundingBoxGridBase64 = getStringValue(json, "boundingBoxGridBase64");
}

nx::kit::Json ObjectRegion::to_json() const
{
    return nx::kit::Json::object({{"boundingBoxGridBase64", boundingBoxGridBase64}});
}

BestShot::BestShot(const nx::kit::Json& json)
{
    timestamp = microseconds(getIntValue(json, "timestamp"));
    rect = getObjectValue<Rect>(json, "rect");
    streamIndex = getIntValue(json, "streamIndex");
}

BestShot::BestShot(const char* jsonData): BestShot(parseJson(jsonData))
{
}

nx::kit::Json BestShot::to_json() const
{
    return nx::kit::Json::object({
        {"timestamp", (double) timestamp.count()},
        {"rect", rect},
        {"streamIndex", streamIndex},
        });
}

ObjectTrack::ObjectTrack(const char* jsonData): ObjectTrack(parseJson(jsonData))
{
}

ObjectTrack::ObjectTrack(const nx::kit::Json & json)
{
    id = getStringValue(json, "id");
    deviceId = getStringValue(json, "deviceId");
    objectTypeId = getStringValue(json, "objectTypeId");
    attributes = getObjectListValue<KeyValuePair>(json, "attributes");
    firstAppearanceTimestamp = microseconds(getIntValue(json, "firstAppearanceTimestamp"));
    lastAppearanceTimestamp = microseconds(getIntValue(json, "lastAppearanceTimestamp"));
    objectPosition = getObjectValue<ObjectRegion>(json, "objectPosition");
    analyticsEngineId = getStringValue(json, "analyticsEngineId");
}

nx::kit::Json ObjectTrack::to_json() const
{
    return nx::kit::Json::object({
        {"id", id},
        {"deviceId", deviceId},
        {"objectTypeId", objectTypeId},
        {"attributes", attributes},
        {"firstAppearanceTimestamp", (double) firstAppearanceTimestamp.count()},
        {"lastAppearanceTimestamp", (double) lastAppearanceTimestamp.count()},
        {"objectPosition", objectPosition},
        {"analyticsEngineId", analyticsEngineId},
        });
}

BestShotImage::BestShotImage(const nx::kit::Json& json)
{
    objectTrackId = getStringValue(json, "objectTrackId");
    format = getStringValue(json, "format");
    data64 = getStringValue(json, "data64");
}

BestShotImage::BestShotImage(const char* jsonData): BestShotImage(parseJson(jsonData))
{
}

nx::kit::Json BestShotImage::to_json() const
{
    return nx::kit::Json::object({
        {"objectTrackId", objectTrackId},
        {"format", format},
        {"data64", data64},
        });
}

AnalyticsLookupResult analyticsLookupResultFromJson(const char* data)
{
    return objectListFromJson<ObjectTrack>(data);
}

CodecInfoData::CodecInfoData(const char* jsonData): CodecInfoData(parseJson(jsonData))
{}

CodecInfoData::CodecInfoData(const nx::kit::Json& json)
{
    compressionType = (nxcip::CompressionType) getIntValue(json, "compressionType");
    pixelFormat = (nxcip::PixelFormat) getIntValue(json, "pixelFormat");
    mediaType = (nxcip::MediaType) getIntValue(json, "mediaType");
    width = getIntValue(json, "width");
    height = getIntValue(json, "height");
    codecTag = getIntValue(json, "codecTag");
    bitRate = getIntValue(json, "bitRate");
    channels = getIntValue(json, "channels");
    frameSize = getIntValue(json, "frameSize");
    blockAlign = getIntValue(json, "blockAlign");
    sampleRate = getIntValue(json, "sampleRate");
    sampleFormat = (nxcip::SampleFormat) getIntValue(json, "sampleFormat");
    bitsPerCodedSample = getIntValue(json, "bitsPerCodedSample");
    channelLayout = getIntValue(json, "channelLayout");
    channelNumber = getIntValue(json, "channelNumber");
    extradataBase64 = getStringValue(json, "extradataBase64");
}

nx::kit::Json CodecInfoData::to_json() const
{
    return nx::kit::Json::object({
        {"compressionType", (int) compressionType},
        {"pixelFormat", (int) pixelFormat},
        {"mediaType", (int) mediaType},
        {"width", width},
        {"height", height},
        {"codecTag", (double) codecTag},
        {"bitRate", (double) bitRate},
        {"channels", channels},
        {"frameSize", frameSize},
        {"blockAlign", blockAlign},
        {"sampleRate", sampleRate},
        {"sampleFormat", (int) sampleFormat},
        {"bitsPerCodedSample", bitsPerCodedSample},
        {"channelLayout", (double) channelLayout},
        {"channelNumber", channelNumber},
        {"extradataBase64", extradataBase64}
        });
}

CodecInfoData::CodecInfoData(const ICodecInfo* info)
{
    compressionType = info->compressionType();
    pixelFormat = info->pixelFormat();
    mediaType = info->mediaType();
    width = info->width();
    height = info->height();
    codecTag = info->codecTag();
    bitRate = info->bitRate();
    channels = info->channels();
    frameSize = info->frameSize();
    blockAlign = info->blockAlign();
    sampleRate = info->sampleRate();
    sampleFormat = info->sampleFormat();
    bitsPerCodedSample = info->bitsPerCodedSample();
    channelLayout = info->channelLayout();
    if (info->extradataSize() > 0)
    {
        extradataBase64 = nx::sdk::cloud_storage::toBase64(
            info->extradata(), info->extradataSize());
    }

    channelNumber = info->channelNumber();
}

MediaPacketData::MediaPacketData(
    const IMediaDataPacket* mediaPacket, std::chrono::milliseconds startTime)
{
    data.assign(
        (const uint8_t*) mediaPacket->data(),
        (const uint8_t*) mediaPacket->data() + mediaPacket->dataSize());

    dataSize = mediaPacket->dataSize();
    compressionType = mediaPacket->codecType();
    timestampUs = mediaPacket->timestampUs() - startTime.count() * 1000;
    type = mediaPacket->type();
    channelNumber = mediaPacket->channelNumber();
    isKeyFrame = mediaPacket->isKeyFrame();

    if (mediaPacket->encryptionDataSize() > 0)
    {
        encryptionData.assign((const uint8_t*) mediaPacket->encryptionData(),
            (const uint8_t*) mediaPacket->encryptionData() + mediaPacket->encryptionDataSize());
    }
}

} // namespace cloud_storage
} // namespace sdk
} // namespace nx
