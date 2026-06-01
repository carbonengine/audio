#include "../stdafx.h"
#include "WaapiManager.h"

#include <AK/WwiseAuthoringAPI/AkAutobahn/Client.h>
#include <AK/WwiseAuthoringAPI/AkAutobahn/Logger.h>
#include <AK/WwiseAuthoringAPI/waapi.h>

#include "../Audio2.h"

#include <iostream>
#include <unordered_set>
#include <sstream>

using namespace AK::WwiseAuthoringAPI;

static CcpLogChannel_t s_ch = CCP_LOG_DEFINE_CHANNEL( "WaapiManager" );

static std::string GetWaapiStringField(const AkJson& objectData, const char* fieldName)
{
    if (!objectData.HasKey(fieldName))
        return "";

    return objectData[fieldName].GetVariant().GetString();
}

static void ReadObjectPositioningData(const AkJson& objectData, WaapiObjectPositioningData& outData)
{
    outData = WaapiObjectPositioningData{};
    outData.id = GetWaapiStringField(objectData, "id");
    outData.name = GetWaapiStringField(objectData, "name");
    outData.type = GetWaapiStringField(objectData, "type");
    outData.path = GetWaapiStringField(objectData, "path");
    outData.parentId = GetWaapiStringField(objectData, "parent.id");
    outData.attenuationId = GetWaapiStringField(objectData, "Attenuation.id");
    outData.attenuationName = GetWaapiStringField(objectData, "Attenuation.name");

    if (objectData.HasKey("OverridePositioning"))
        outData.overridePositioning = static_cast<bool>(objectData["OverridePositioning"].GetVariant());
    if (objectData.HasKey("EnableAttenuation"))
        outData.enableAttenuation = static_cast<bool>(objectData["EnableAttenuation"].GetVariant());
}

#ifndef AK_OPTIMIZED
static uint32_t GetWaapiUInt32Field(const AkJson& objectData, const char* fieldName)
{
    if (!objectData.HasKey(fieldName))
        return 0;

    return static_cast<uint32_t>(objectData[fieldName].GetVariant());
}

static uint64_t GetWaapiUInt64Field(const AkJson& objectData, const char* fieldName)
{
    if (!objectData.HasKey(fieldName))
        return 0;

    return static_cast<uint64_t>(objectData[fieldName].GetVariant());
}

static double GetWaapiDoubleField(const AkJson& objectData, const char* fieldName, double defaultValue = 0.0)
{
    if (!objectData.HasKey(fieldName))
        return defaultValue;

    return static_cast<double>(objectData[fieldName].GetVariant());
}

static bool GetWaapiBoolField(const AkJson& objectData, const char* fieldName)
{
    if (!objectData.HasKey(fieldName))
        return false;

    return static_cast<bool>(objectData[fieldName].GetVariant());
}

static void ReadProfilerVoiceData(const AkJson& objectData, WaapiProfilerVoiceData& outData)
{
    outData = WaapiProfilerVoiceData{};
    outData.pipelineId = GetWaapiUInt32Field(objectData, "pipelineID");
    outData.playingId = GetWaapiUInt32Field(objectData, "playingID");
    outData.soundId = GetWaapiUInt32Field(objectData, "soundID");
    outData.gameObjectId = GetWaapiUInt64Field(objectData, "gameObjectID");
    outData.gameObjectName = GetWaapiStringField(objectData, "gameObjectName");
    outData.objectGuid = GetWaapiStringField(objectData, "objectGUID");
    outData.objectName = GetWaapiStringField(objectData, "objectName");
    outData.playTargetGuid = GetWaapiStringField(objectData, "playTargetGUID");
    outData.playTargetName = GetWaapiStringField(objectData, "playTargetName");
    outData.baseVolume = GetWaapiDoubleField(objectData, "baseVolume", -1000.0);
    outData.isStarted = GetWaapiBoolField(objectData, "isStarted");
    outData.isVirtual = GetWaapiBoolField(objectData, "isVirtual");
    outData.isForcedVirtual = GetWaapiBoolField(objectData, "isForcedVirtual");
}
#endif

std::string WaapiManager::CurveShapeToString(CurveShape shape)
{
    switch (shape)
    {
        case CurveShape::Linear:   return "Linear";
        case CurveShape::Log1:     return "Log1";
        case CurveShape::Log2:     return "Log2";
        case CurveShape::Log3:     return "Log3";
        case CurveShape::Exp1:     return "Exp1";
        case CurveShape::Exp2:     return "Exp2";
        case CurveShape::Exp3:     return "Exp3";
        case CurveShape::SCurve:   return "SCurve";
        case CurveShape::InvSCurve: return "InvSCurve";
        default: return "Linear";
    }
}

CurveShape WaapiManager::StringToCurveShape(const std::string& str)
{
    if (str == "Log1")        return CurveShape::Log1;
    if (str == "Log2")        return CurveShape::Log2;
    if (str == "Log3")        return CurveShape::Log3;
    if (str == "Exp1")        return CurveShape::Exp1;
    if (str == "Exp2")        return CurveShape::Exp2;
    if (str == "Exp3")        return CurveShape::Exp3;
    if (str == "SCurve")      return CurveShape::SCurve;
    if (str == "InvSCurve")   return CurveShape::InvSCurve;
    return CurveShape::Linear;
}

int WaapiManager::CurveShapeToInt(CurveShape shape)
{
    return static_cast<int>(shape);
}

CurveShape WaapiManager::IntToCurveShape(int shapeInt)
{
    if (shapeInt >= 0 && shapeInt <= 8)
        return static_cast<CurveShape>(shapeInt);
    return CurveShape::Linear;
}

WaapiManager::WaapiManager(IRoot* lockobj)
    : m_client(nullptr)
    , m_connected(false)
    , m_host("127.0.0.1")
    , m_port(8080)
#ifndef AK_OPTIMIZED
    , m_profilerVoiceDataEnabled(false)
#endif
{}

WaapiManager::~WaapiManager()
{
    Disconnect();
}

bool WaapiManager::Connect(const std::string& host, int port)
{
    return ConnectInternal(host, port, true);
}

bool WaapiManager::TryConnect(const std::string& host, int port)
{
    return ConnectInternal(host, port, false);
}

bool WaapiManager::ConnectInternal(const std::string& host, int port, bool logFailures)
{
    if (m_connected)
    {
        return true;
    }

    try
    {
        m_host = host;
        m_port = port;

        m_client = std::make_unique<Client>();

        if (!m_client->Connect(m_host.c_str(), m_port))
        {
            if (logFailures)
                CCP_LOGERR_CH(s_ch, "Could not connect to Wwise Authoring on %s:%d", m_host.c_str(), m_port);
            m_client.reset();
            m_connected = false;
#ifndef AK_OPTIMIZED
            m_profilerVoiceDataEnabled = false;
            m_profilerCaptureStartAttempted = false;
#endif
            return false;
        }

        m_connected = true;
        CCP_LOG_CH(s_ch, "Successfully connected to Wwise Authoring on %s:%d", m_host.c_str(), m_port);
        return true;
    }
    catch (const std::exception& e)
    {
        if (logFailures)
            CCP_LOGERR_CH(s_ch, "Exception during connection: %s", e.what());
        m_client.reset();
        m_connected = false;
#ifndef AK_OPTIMIZED
        m_profilerVoiceDataEnabled = false;
        m_profilerCaptureStartAttempted = false;
#endif
        return false;
    }
}

void WaapiManager::MarkConnectionLost(const char* reason)
{
    if (m_connected)
    {
        CCP_LOGWARN_CH(s_ch, "Lost Wwise Authoring WAAPI connection: %s", reason ? reason : "unknown reason");
    }

    m_client.reset();
    m_connected = false;
#ifndef AK_OPTIMIZED
    m_profilerVoiceDataEnabled = false;
    m_profilerCaptureStartAttempted = false;
#endif
}

void WaapiManager::Disconnect()
{
    if (m_connected && m_client)
    {
        try
        {
            // Unsubscribe from all active subscriptions
            {
                std::lock_guard<std::mutex> lock(m_subscriptionMutex);
                for (const auto& pair : m_Subscriptions)
                {
                    AkJson unsubscribeResult;
                    if (!m_client->Unsubscribe(pair.second, unsubscribeResult))
                    {
                        CCP_LOG_CH(s_ch, "Failed to unsubscribe from object '%s' (SubID: %llu) during disconnect",
                                  pair.first.c_str(), static_cast<unsigned long long>(pair.second));
                    }
                }
                m_Subscriptions.clear();
                m_changeCallbacks.clear();
            }

            m_client->Disconnect();
            CCP_LOG_CH(s_ch, "Disconnected from Wwise Authoring");
        }
        catch (const std::exception& e)
        {
            CCP_LOGERR_CH(s_ch, "Exception during disconnection: %s", e.what());
        }
    }

    m_client.reset();
    m_connected = false;
#ifndef AK_OPTIMIZED
    m_profilerVoiceDataEnabled = false;
    m_profilerCaptureStartAttempted = false;
#endif
}

bool WaapiManager::IsConnected() const
{
    return m_connected && m_client != nullptr;
}

Client* WaapiManager::GetClient() const
{
    return IsConnected() ? m_client.get() : nullptr;
}

bool WaapiManager::GetEventReferencedTargets(const std::string& eventName, std::vector<std::string>& outIds, std::vector<std::string>& outNames, std::vector<std::string>& outPathNames)
{

    if (!IsConnected()) return false;
    auto* client = m_client.get();

    outIds.clear();
    outNames.clear();
    outPathNames.clear();

    std::string waql = "from type Event where name = \"" + EscapeWaqlString(eventName) + "\"";
    AkJson evtRes;
    if (!client->Call(ak::wwise::core::object::get,
                     AkJson::Map{ { "waql", AkVariant(waql.c_str()) } },
                     AkJson::Map{ { "return", AkJson::Array{ AkVariant("id") } } },
                     evtRes) || evtRes["return"].GetArray().empty())
        return false;

    const auto eventId = evtRes["return"].GetArray()[0]["id"].GetVariant().GetString();

    // Get Actions under Event using WAQL, then get their targets
    std::string actionsWaql = "$ from object \"" + EscapeWaqlString(eventId) + "\" select children where type = \"Action\"";
    AkJson actRes;
    if (!client->Call(ak::wwise::core::object::get,
                     AkJson::Map{ { "waql", AkVariant(actionsWaql.c_str()) } },
                     AkJson::Map{ { "return", AkJson::Array{ AkVariant("id") } } },
                     actRes))
        return false;

    // Build action ID array and get targets
    AkJson::Array actionIdArray;
    for (const auto& row : actRes["return"].GetArray())
        actionIdArray.emplace_back(AkVariant(row["id"].GetVariant().GetString().c_str()));

    if (actionIdArray.empty()) return true;

    AkJson targRes;
    if (!client->Call(ak::wwise::core::object::get,
                     AkJson::Map{ { "from", AkJson::Map{ { "id", actionIdArray } } } },
                     AkJson::Map{ { "return", AkJson::Array{ AkVariant("target.id") } } },
                     targRes))
        return false;

    std::vector<std::string> targetIds;
    std::unordered_set<std::string> seenTargetIds;
    for (const auto& row : targRes["return"].GetArray())
    {
        if (row.HasKey("target.id"))
        {
            const std::string targetId = row["target.id"].GetVariant().GetString();
            if (!targetId.empty() && seenTargetIds.insert(targetId).second)
                targetIds.push_back(targetId);
        }
    }
    if (targetIds.empty()) return true;

    AkJson::Array targetIdArray;
    for (const auto& targetId : targetIds)
        targetIdArray.emplace_back(AkVariant(targetId.c_str()));

    AkJson targetRes;
    if (!client->Call(ak::wwise::core::object::get,
                     AkJson::Map{ { "from", AkJson::Map{ { "id", targetIdArray } } } },
                     AkJson::Map{ { "return", AkJson::Array{ AkVariant("id"), AkVariant("name"), AkVariant("path") } } },
                     targetRes))
        return false;

    if (!targetRes.HasKey("return")) return true;

    for (const auto& row : targetRes["return"].GetArray()) {
        if (!row.HasKey("id")) continue;

        outIds.push_back(row["id"].GetVariant().GetString());
        outNames.push_back(row.HasKey("name") ? row["name"].GetVariant().GetString() : "");
        outPathNames.push_back(row.HasKey("path") ? row["path"].GetVariant().GetString() : "");
    }

    return true;
}

std::vector<std::string> WaapiManager::GetEventReferencedTargetIds(const std::string& eventName)
{
    std::vector<std::string> ids, names, pathNames;

    if (GetEventReferencedTargets(eventName, ids, names, pathNames))
        return ids;

    return {};
}

std::vector<std::string> WaapiManager::GetEventReferencedTargetNames(const std::string& eventName)
{
    std::vector<std::string> ids, names, pathNames;

    if (GetEventReferencedTargets(eventName, ids, names, pathNames))
        return names;

    return {};
}

std::vector<std::string> WaapiManager::GetEventReferencedTargetPaths(const std::string& eventName)
{
    std::vector<std::string> ids, names, pathNames;

    if (GetEventReferencedTargets(eventName, ids, names, pathNames))
        return pathNames;

    return {};
}

std::string WaapiManager::GetSoundReferencedAttenuationId(const std::string& soundId)
{
    std::string id, name;
    if (GetSoundReferencedAttenuations(soundId, id, name))
        return id;
    return {};
}

std::string WaapiManager::GetSoundReferencedAttenuationName(const std::string& soundId)
{
    std::string id, name;
    if (GetSoundReferencedAttenuations(soundId, id, name))
        return name;
    return {};
}

std::string WaapiManager::GetObjectEffectiveAttenuationId(const std::string& objectId)
{
    WaapiObjectPositioningData source;
    if (ResolveEffectiveAttenuationReference(objectId, source))
        return source.attenuationId;
    return {};
}

#ifndef AK_OPTIMIZED
std::string WaapiManager::GetObjectEffectiveAttenuationConeAttenuationId(const std::string& objectId)
{
    WaapiAttenuationConeData data;
    if (GetObjectEffectiveAttenuationConeData(objectId, data))
        return data.attenuationId;
    return {};
}

double WaapiManager::GetAttenuationConeInnerAngle(const std::string& attenuationId)
{
    WaapiAttenuationConeData data;
    if (GetAttenuationConeData(attenuationId, data))
        return data.innerAngle;
    return 0.0;
}

double WaapiManager::GetAttenuationConeOuterAngle(const std::string& attenuationId)
{
    WaapiAttenuationConeData data;
    if (GetAttenuationConeData(attenuationId, data))
        return data.outerAngle;
    return 0.0;
}
#endif


std::vector<double> WaapiManager::GetAttenuationVolumeCurveDistances(const std::string& attenuationId)
{
    std::vector<double> distances, values;
    std::vector<std::string> stringShapes;

    if (GetAttenuationVolumeCurveInternal(attenuationId, distances, values, stringShapes))
        return distances;
    return {};
}

std::vector<double> WaapiManager::GetAttenuationVolumeCurveValues(const std::string& attenuationId)
{
    std::vector<double> distances, values;
    std::vector<std::string> stringShapes;

    if (GetAttenuationVolumeCurveInternal(attenuationId, distances, values, stringShapes))
        return values;
    return {};
}


std::vector<int> WaapiManager::GetAttenuationVolumeCurveShapeInts(const std::string& attenuationId)
{
    std::vector<double> distances, values;
    std::vector<std::string> stringShapes;

    if (GetAttenuationVolumeCurveInternal(attenuationId, distances, values, stringShapes))
    {
        std::vector<int> intShapes;
        intShapes.reserve(stringShapes.size());
        for (const auto& shapeStr : stringShapes)
        {
            intShapes.push_back(CurveShapeToInt(StringToCurveShape(shapeStr)));
        }
        return intShapes;
    }
    return {};
}

bool WaapiManager::SetAttenuationVolumeCurve(const std::string& attenuationId, const std::vector<double>& distances, const std::vector<double>& values, const std::vector<int>& shapeInts)
{
    if (distances.size() != values.size() || (!shapeInts.empty() && shapeInts.size() != distances.size()))
        return false;

    if (!IsConnected()) return false;

    // Convert integer shapes to strings
    std::vector<std::string> stringShapes;
    stringShapes.reserve(shapeInts.size());
    for (int shapeInt : shapeInts)
    {
        stringShapes.push_back(CurveShapeToString(IntToCurveShape(shapeInt)));
    }
    
    return SetAttenuationVolumeCurveInternal(attenuationId, distances, values, stringShapes);
}

bool WaapiManager::GetSoundReferencedAttenuations(const std::string& soundId, std::string& outAttenuationId, std::string& outAttenuationName)
{

    if (!IsConnected()) return false;
    auto* client = m_client.get();

    outAttenuationId.clear();
    outAttenuationName.clear();

    AkJson args = AkJson::Map{ { "from", AkJson::Map{ { "id", AkJson::Array{ AkVariant(soundId.c_str()) } } } } };
    AkJson opts = AkJson::Map{ { "return", AkJson::Array{ AkVariant("Attenuation.id"), AkVariant("Attenuation.name") } } };
    AkJson result;

    if (!client->Call(ak::wwise::core::object::get, args, opts, result))
        return false;

    if (!result.HasKey("return") || result["return"].GetArray().empty())
        return false;

    const auto& obj = result["return"].GetArray()[0];
    if (obj.HasKey("Attenuation.id"))
        outAttenuationId = obj["Attenuation.id"].GetVariant().GetString();
    if (obj.HasKey("Attenuation.name"))
        outAttenuationName = obj["Attenuation.name"].GetVariant().GetString();

    return !outAttenuationId.empty();
}

bool WaapiManager::GetObjectPositioningData(const std::string& objectId, WaapiObjectPositioningData& outData)
{
    outData = WaapiObjectPositioningData{};

    if (!IsConnected() || objectId.empty()) return false;
    auto* client = m_client.get();

    AkJson args = AkJson::Map{ { "from", AkJson::Map{ { "id", AkJson::Array{ AkVariant(objectId.c_str()) } } } } };
    AkJson opts = AkJson::Map{ { "return", AkJson::Array{
        AkVariant("id"),
        AkVariant("name"),
        AkVariant("type"),
        AkVariant("path"),
        AkVariant("parent.id"),
        AkVariant("OverridePositioning"),
        AkVariant("EnableAttenuation"),
        AkVariant("Attenuation.id"),
        AkVariant("Attenuation.name")
    } } };
    AkJson result;

    if (!client->Call(ak::wwise::core::object::get, args, opts, result))
        return false;

    if (!result.HasKey("return") || result["return"].GetArray().empty())
        return false;

    ReadObjectPositioningData(result["return"].GetArray()[0], outData);
    return !outData.id.empty();
}

bool WaapiManager::ResolveEffectiveAttenuationReference(const std::string& objectId, WaapiObjectPositioningData& outSource)
{
    outSource = WaapiObjectPositioningData{};

    std::string currentObjectId = objectId;
    std::unordered_set<std::string> visitedObjectIds;
    while (!currentObjectId.empty() && visitedObjectIds.find(currentObjectId) == visitedObjectIds.end())
    {
        visitedObjectIds.insert(currentObjectId);

        WaapiObjectPositioningData objectData;
        if (!GetObjectPositioningData(currentObjectId, objectData))
            return false;

        if (objectData.overridePositioning)
        {
            if (objectData.enableAttenuation && !objectData.attenuationId.empty())
            {
                outSource = objectData;
                return true;
            }

            return false;
        }

        currentObjectId = objectData.parentId;
    }

    return false;
}

bool WaapiManager::GetAttenuationVolumeCurveInternal(const std::string& attenuationId, std::vector<double>& outDistances, std::vector<double>& outValues, std::vector<std::string>& outShapes)
{

    if (!IsConnected()) return false;
    auto* client = m_client.get();

    outDistances.clear();
    outValues.clear();
    outShapes.clear();

    AkJson result;
    if (!client->Call("ak.wwise.core.object.getAttenuationCurve",
                     AkJson::Map{
                         { "object", AkVariant(attenuationId.c_str()) },
                         { "curveType", AkVariant("VolumeDryUsage") }
                     },
                     AkJson::Map{},
                     result))
        return false;

    if (!result.HasKey("points") || !result["points"].IsArray())
        return false;

    for (const auto& point : result["points"].GetArray())
    {
        if (!point.IsMap()) continue;

        double distance = 0.0;
        double value = 0.0;
        std::string shape = "Linear";

        if (point.HasKey("x")) {
            distance = static_cast<double>(point["x"].GetVariant());
        }
        if (point.HasKey("y")) {
            value = static_cast<double>(point["y"].GetVariant());
        }
        if (point.HasKey("shape")) {
            shape = point["shape"].GetVariant().GetString();
        }

        outDistances.push_back(distance);
        outValues.push_back(value);
        outShapes.push_back(shape);
    }

    return !outDistances.empty();
}


std::string WaapiManager::GetAttenuationName(const std::string& attenuationId)
{

    if (!IsConnected()) return "";
    auto* client = m_client.get();

    std::string waql = "$ where id=\"" + EscapeWaqlString(attenuationId) + "\"";
    AkJson result;
    if (!client->Call(ak::wwise::core::object::get,
                     AkJson::Map{ { "waql", AkVariant(waql.c_str()) } },
                     AkJson::Map{ { "return", AkJson::Array{ AkVariant("name") } } },
                     result))
        return "";

    if (!result.HasKey("return") || result["return"].GetArray().empty())
        return "";

    return result["return"].GetArray()[0]["name"].GetVariant().GetString();
}

bool WaapiManager::SetAttenuationVolumeCurveInternal(const std::string& attenuationId, const std::vector<double>& distances, const std::vector<double>& values, const std::vector<std::string>& shapes)
{

    if (!IsConnected()) return false;
    auto* client = m_client.get();

    if (distances.size() != values.size() || (!shapes.empty() && shapes.size() != distances.size()))
        return false;

    // Build curve points array
    AkJson::Array pointsArray;
    for (size_t i = 0; i < distances.size(); ++i)
    {
        const char* shape = "Linear";
        if (!shapes.empty() && i < shapes.size() && !shapes[i].empty())
            shape = shapes[i].c_str();

        pointsArray.emplace_back(AkJson::Map{
            { "x", AkVariant(distances[i]) },
            { "y", AkVariant(values[i]) },
            { "shape", AkVariant(shape) }
        });
    }

    // Set volume curve
    AkJson args = AkJson::Map{
        { "object", AkVariant(attenuationId.c_str()) },
        { "curveType", AkVariant("VolumeDryUsage") },
        { "points", pointsArray },
        { "use", AkVariant("Custom") }
    };
    AkJson opts = AkJson::Map{};
    AkJson result;

    return client->Call("ak.wwise.core.object.setAttenuationCurve", args, opts, result);
}


double WaapiManager::GetAttenuationMaxRadius(const std::string& attenuationId)
{
    double radius;
    if (GetAttenuationMaxRadiusInternal(attenuationId, radius))
        return radius;
    return 0.0;
}

#ifndef AK_OPTIMIZED
bool WaapiManager::GetAttenuationConeData(const std::string& attenuationId, WaapiAttenuationConeData& outData)
{
    outData = WaapiAttenuationConeData{};

    if (!IsConnected()) return false;
    auto* client = m_client.get();

    AkJson args = AkJson::Map{ { "from", AkJson::Map{ { "id", AkJson::Array{ AkVariant(attenuationId.c_str()) } } } } };
    AkJson opts = AkJson::Map{ { "return", AkJson::Array{
        AkVariant("id"),
        AkVariant("name"),
        AkVariant("ConeUse"),
        AkVariant("ConeInnerAngle"),
        AkVariant("ConeOuterAngle"),
        AkVariant("ConeAttenuation")
    } } };
    AkJson result;

    if (!client->Call(ak::wwise::core::object::get, args, opts, result))
        return false;

    if (!result.HasKey("return") || result["return"].GetArray().empty())
        return false;

    const auto& obj = result["return"].GetArray()[0];
    outData.attenuationId = attenuationId;
    if (obj.HasKey("name"))
        outData.attenuationName = obj["name"].GetVariant().GetString();
    if (obj.HasKey("ConeUse"))
        outData.coneEnabled = static_cast<bool>(obj["ConeUse"].GetVariant());
    if (obj.HasKey("ConeInnerAngle"))
        outData.innerAngle = static_cast<double>(obj["ConeInnerAngle"].GetVariant());
    if (obj.HasKey("ConeOuterAngle"))
        outData.outerAngle = static_cast<double>(obj["ConeOuterAngle"].GetVariant());
    if (obj.HasKey("ConeAttenuation"))
        outData.coneAttenuation = static_cast<double>(obj["ConeAttenuation"].GetVariant());

    GetAttenuationMaxRadiusInternal(attenuationId, outData.maxRadius);

    return true;
}

bool WaapiManager::GetObjectEffectiveAttenuationConeData(const std::string& objectId, WaapiAttenuationConeData& outData)
{
    outData = WaapiAttenuationConeData{};

    if (!IsConnected()) return false;

    WaapiObjectPositioningData sourceData;
    if (!ResolveEffectiveAttenuationReference(objectId, sourceData))
        return false;

    WaapiAttenuationConeData coneData;
    if (!GetAttenuationConeData(sourceData.attenuationId, coneData))
        return false;

    coneData.sourceObjectId = sourceData.id;
    coneData.sourceObjectName = sourceData.name;
    coneData.sourceObjectPath = sourceData.path;
    outData = coneData;
    return true;
}

bool WaapiManager::EnableProfilerVoiceData()
{
    if (!IsConnected()) return false;
    if (m_profilerVoiceDataEnabled) return true;

    auto* client = m_client.get();
    AkJson result;
    const bool success = client->Call(ak::wwise::core::profiler::enableProfilerData,
        AkJson::Map{
            { "dataTypes", AkJson::Array{
                AkJson::Map{
                    { "dataType", AkVariant("voices") },
                    { "enable", AkVariant(true) }
                }
            } }
        },
        AkJson::Map{},
        result);

    m_profilerVoiceDataEnabled = success;
    if (!success)
    {
        CCP_LOGWARN_CH(s_ch, "Failed to enable Wwise profiler voice data for audio debug cone resolution.");
        MarkConnectionLost("enableProfilerData failed");
    }

    return success;
}

bool WaapiManager::StartProfilerCaptureForVoiceData()
{
    if (!IsConnected() || m_profilerCaptureStartAttempted) return false;

    m_profilerCaptureStartAttempted = true;

    auto* client = m_client.get();
    AkJson result;
    const bool success = client->Call(ak::wwise::core::profiler::startCapture, AkJson::Map{}, AkJson::Map{}, result);
    if (success)
        CCP_LOG_CH(s_ch, "Started Wwise profiler capture for audio debug cone voice resolution.");
    else
        CCP_LOGWARN_CH(s_ch, "Failed to start Wwise profiler capture for audio debug cone voice resolution.");

    return success;
}

bool WaapiManager::GetProfilerVoices(std::vector<WaapiProfilerVoiceData>& outVoices)
{
    outVoices.clear();

    if (!EnableProfilerVoiceData()) return false;

    auto* client = m_client.get();
    AkJson result;
    auto getVoices = [&]() {
        result = AkJson();
        return client->Call(ak::wwise::core::profiler::getVoices,
        AkJson::Map{ { "time", AkVariant("capture") } },
        AkJson::Map{ { "return", AkJson::Array{
            AkVariant("pipelineID"),
            AkVariant("playingID"),
            AkVariant("soundID"),
            AkVariant("gameObjectID"),
            AkVariant("gameObjectName"),
            AkVariant("objectGUID"),
            AkVariant("objectName"),
            AkVariant("playTargetGUID"),
            AkVariant("playTargetName"),
            AkVariant("baseVolume"),
            AkVariant("isStarted"),
            AkVariant("isVirtual"),
            AkVariant("isForcedVirtual")
        } } },
        result);
    };

    if (!getVoices())
    {
        if (!StartProfilerCaptureForVoiceData() || !getVoices())
        {
            MarkConnectionLost("profiler getVoices failed");
            return false;
        }
    }

    if (!result.HasKey("return"))
        return true;

    for (const auto& row : result["return"].GetArray())
    {
        WaapiProfilerVoiceData voiceData;
        ReadProfilerVoiceData(row, voiceData);
        outVoices.push_back(voiceData);
    }

    return true;
}

std::string WaapiManager::SelectBestProfilerVoiceObjectId(const std::vector<WaapiProfilerVoiceData>& voices, uint64_t gameObjectId, uint32_t playingId)
{
    const WaapiProfilerVoiceData* bestVoice = nullptr;
    double bestScore = -1000000.0;

    for (const WaapiProfilerVoiceData& voice : voices)
    {
        if (voice.gameObjectId != gameObjectId || voice.playingId != playingId || voice.objectGuid.empty())
            continue;

        double score = voice.baseVolume;
        if (voice.isStarted)
            score += 1000.0;
        if (!voice.isVirtual)
            score += 100.0;
        if (!voice.isForcedVirtual)
            score += 50.0;

        if (bestVoice == nullptr || score > bestScore)
        {
            bestVoice = &voice;
            bestScore = score;
        }
    }

    return bestVoice ? bestVoice->objectGuid : "";
}

std::string WaapiManager::GetBestProfilerVoiceObjectIdForPlayingEvent(uint64_t gameObjectId, uint32_t playingId)
{
    std::vector<WaapiProfilerVoiceData> voices;
    if (!GetProfilerVoices(voices))
        return "";

    return SelectBestProfilerVoiceObjectId(voices, gameObjectId, playingId);
}

std::string WaapiManager::SelectDebugProfilerVoiceObjectIdForTest(long long gameObjectId,
                                                                  int playingId,
                                                                  const std::vector<std::string>& objectGuids,
                                                                  const std::vector<int>& playingIds,
                                                                  const std::vector<long long>& gameObjectIds,
                                                                  const std::vector<int>& isStarted,
                                                                  const std::vector<int>& isVirtual,
                                                                  const std::vector<double>& baseVolumes)
{
    const size_t voiceCount = objectGuids.size();
    if (playingIds.size() != voiceCount ||
        gameObjectIds.size() != voiceCount ||
        isStarted.size() != voiceCount ||
        isVirtual.size() != voiceCount ||
        baseVolumes.size() != voiceCount ||
        voiceCount == 0)
    {
        return "";
    }

    std::vector<WaapiProfilerVoiceData> voices;
    voices.reserve(voiceCount);
    for (size_t index = 0; index < voiceCount; ++index)
    {
        WaapiProfilerVoiceData voice;
        voice.objectGuid = objectGuids[index];
        voice.playingId = static_cast<uint32_t>(playingIds[index]);
        voice.gameObjectId = static_cast<uint64_t>(gameObjectIds[index]);
        voice.isStarted = isStarted[index] != 0;
        voice.isVirtual = isVirtual[index] != 0;
        voice.baseVolume = baseVolumes[index];
        voices.push_back(voice);
    }

    return SelectBestProfilerVoiceObjectId(voices, static_cast<uint64_t>(gameObjectId), static_cast<uint32_t>(playingId));
}
#endif

bool WaapiManager::GetAttenuationMaxRadiusInternal(const std::string& attenuationId, double& outRadius)
{

    if (!IsConnected()) return false;
    auto* client = m_client.get();

    outRadius = 0.0;

    std::vector<double> distances, values;
    std::vector<std::string> shapes;
    if (GetAttenuationVolumeCurveInternal(attenuationId, distances, values, shapes) && !distances.empty()) {
        outRadius = distances.back();
        return true;
    }
    return false;
}

bool WaapiManager::SetAttenuationMaxRadius(const std::string& attenuationId, double radius)
{

    if (!IsConnected()) return false;
    auto* client = m_client.get();

    AkJson args = AkJson::Map{
        { "object", AkVariant(attenuationId.c_str()) },
        { "property", AkVariant("RadiusMax") },
        { "value", AkVariant(radius) }
    };
    AkJson opts = AkJson::Map{};
    AkJson result;

    return client->Call(ak::wwise::core::object::setProperty, args, opts, result);
}

std::string WaapiManager::EscapeWaqlString(const std::string& str)
{
    std::string result;
    result.reserve(str.size() * 2);
    for (char c : str)
    {
        if (c == '\\' || c == '"') result += '\\';
        result += c;
    }
    return result;
}

// ============================================================================
// Subscription and Monitoring
// ============================================================================

bool WaapiManager::IsSubscribedToProperty(const std::string& objectId) const
{
    std::lock_guard<std::mutex> lock(m_subscriptionMutex);
    return m_Subscriptions.find(objectId) != m_Subscriptions.end();
}

bool WaapiManager::SubscribeToPropertyChanges(const std::string& objectId,
                                              const std::string& propertyName,
                                              const BlueScriptCallback& callback)
{
    if (!IsConnected()) {
        CCP_LOG_CH(s_ch, "Cannot subscribe: not connected to Wwise");
        return false;
    }

    if (objectId.empty()) {
        CCP_LOG_CH(s_ch, "Cannot subscribe: object ID is empty");
        return false;
    }

    if (propertyName.empty()) {
        CCP_LOG_CH(s_ch, "Cannot subscribe: property name is empty");
        return false;
    }

    std::lock_guard<std::mutex> lock(m_subscriptionMutex);

    // Check if already subscribed
    if (m_Subscriptions.find(objectId) != m_Subscriptions.end()) {
        CCP_LOG_CH(s_ch, "Already subscribed to object: %s", objectId.c_str());
        return false;
    }

    auto* client = m_client.get();
    uint64_t subscriptionId = 0;
    AkJson subscribeResult;

    WaapiManagerPtr selfPtr = this;

    auto onPropertyChanged = [selfPtr, objectId](uint64_t subId, const JsonProvider& jsonProvider) {
        auto jsonData = jsonProvider.GetAkJson();
        std::string propertyNameStr = jsonData["property"].GetVariant().GetString();

        CallbackData* pData = new CallbackData(selfPtr, objectId, propertyNameStr);

        try {
            g_mainThreadQueue->Add(PropagatePropertyChangeCallback, pData, IBlueCallbackMan::BCBF_NONE, nullptr);
        }
        catch (...) {
            delete pData;
            throw;
        }
    };

    // Subscribe to the specified property changes
    AkJson options(AkJson::Map{
        { "object", AkVariant(objectId.c_str()) },
        { "property", AkVariant(propertyName.c_str()) }
    });

    if (!client->Subscribe(ak::wwise::core::object::propertyChanged,
                          options,
                          onPropertyChanged,
                          subscriptionId,
                          subscribeResult))
    {
        CCP_LOG_CH(s_ch, "Failed to subscribe to property '%s' for object: %s",
                  propertyName.c_str(), objectId.c_str());
        return false;
    }
    m_Subscriptions[objectId] = subscriptionId;
    m_changeCallbacks[objectId] = std::make_shared<BlueScriptCallback>(callback);

    CCP_LOG_CH(s_ch, "Successfully subscribed to property '%s' for object: %s (SubID: %llu)",
              propertyName.c_str(), objectId.c_str(), static_cast<unsigned long long>(subscriptionId));

    return true;
}

bool WaapiManager::SubscribeToAttenuationMaxRadius(const std::string& attenuationId,
                                                   const BlueScriptCallback& callback)
{
    return SubscribeToPropertyChanges(attenuationId, "RadiusMax", callback);
}

bool WaapiManager::UnsubscribeFromPropertyChanges(const std::string& objectId)
{
    if (!IsConnected()) {
        CCP_LOG_CH(s_ch, "Cannot unsubscribe: not connected to Wwise");
        return false;
    }

    uint64_t subscriptionId = 0;

    // Find and remove the subscription
    {
        std::lock_guard<std::mutex> lock(m_subscriptionMutex);

        auto it = m_Subscriptions.find(objectId);
        if (it == m_Subscriptions.end()) {
            CCP_LOG_CH(s_ch, "Not subscribed to object: %s", objectId.c_str());
            return false;
        }

        subscriptionId = it->second;
        m_Subscriptions.erase(it);
        m_changeCallbacks.erase(objectId);
    }

    auto* client = m_client.get();
    AkJson unsubscribeResult;

    if (!client->Unsubscribe(subscriptionId, unsubscribeResult)) {
        CCP_LOG_CH(s_ch, "Failed to unsubscribe from WAAPI (SubID: %llu)",
                  static_cast<unsigned long long>(subscriptionId));
        return false;
    }

    CCP_LOG_CH(s_ch, "Successfully unsubscribed from object: %s (SubID: %llu)",
              objectId.c_str(), static_cast<unsigned long long>(subscriptionId));

    return true;
}

void WaapiManager::PropagatePropertyChangeCallback(void* pCallbackData)
{
    CallbackData* pData = reinterpret_cast<CallbackData*>(pCallbackData);
    if (!pData) return;

    WaapiManager* pManager = pData->manager;
    std::string objectId = pData->objectId;
    std::string propertyName = pData->propertyName;

    delete pData;

    // Find and invoke the Python callback
    // We copy the shared_ptr while holding the lock. This increments
    // the reference count, guaranteeing the callback remains valid even if another
    // thread unsubscribes before we invoke it.
    std::shared_ptr<BlueScriptCallback> callbackPtr;
    {
        std::lock_guard<std::mutex> lock(pManager->m_subscriptionMutex);
        auto it = pManager->m_changeCallbacks.find(objectId);
        if (it != pManager->m_changeCallbacks.end()) {
            callbackPtr = it->second;  // Copy shared_ptr, increments refcount
        }
    }

    // Invoke the Python callback if it exists
    if (callbackPtr) {
        try {
            callbackPtr->CallVoid(objectId, propertyName);
        }
        catch (const std::exception& e) {
            CCP_LOG_CH(s_ch, "Exception in property change callback: %s", e.what());
        }
    }
}

std::string WaapiManager::CreateAttenuation(const std::string& attenuationName,
                                             const std::string& parentPath)
{
    if (!IsConnected()) {
        CCP_LOGERR_CH(s_ch, "Cannot create attenuation: not connected to Wwise");
        return "";
    }

    if (attenuationName.empty()) {
        CCP_LOGERR_CH(s_ch, "Cannot create attenuation: name is empty");
        return "";
    }

    if (parentPath.empty()) {
        CCP_LOGERR_CH(s_ch, "Cannot create attenuation: parent path is empty");
        return "";
    }

    auto* client = m_client.get();

    AkJson parentResult;
    AkVariant parentId;

    // Check if parentPath is a GUID or a path
    if (parentPath[0] == '{') {
        parentId = AkVariant(parentPath.c_str());
    } else {
        AkJson getArgs(AkJson::Map{
            { "from", AkJson::Map{
                { "path", AkJson::Array{ AkVariant(parentPath.c_str()) } }
            }}
        });

        AkJson getOptions(AkJson::Map{
            { "return", AkJson::Array{ AkVariant("id") } }
        });

        if (!client->Call(ak::wwise::core::object::get, getArgs, getOptions, parentResult)) {
            CCP_LOGERR_CH(s_ch, "Failed to resolve parent path: %s", parentPath.c_str());
            return "";
        }

        if (!parentResult.HasKey("return") || parentResult["return"].GetArray().empty()) {
            CCP_LOGERR_CH(s_ch, "Parent path not found: %s", parentPath.c_str());
            return "";
        }

        parentId = parentResult["return"].GetArray()[0]["id"].GetVariant();
    }

    // Create the attenuation object
    AkJson args(AkJson::Map{
        { "parent", parentId },
        { "type", AkVariant("Attenuation") },
        { "name", AkVariant(attenuationName.c_str()) },
        { "onNameConflict", AkVariant("rename") },
    });

    AkJson createResult;
    if (!client->Call(ak::wwise::core::object::create, args, AkJson(AkJson::Type::Map), createResult)) {
        CCP_LOGERR_CH(s_ch, "Failed to create attenuation: %s", attenuationName.c_str());
        return "";
    }

    // Extract the object ID
    if (createResult.HasKey("id")) {
        std::string attenuationId = createResult["id"].GetVariant().GetString();
        CCP_LOG_CH(s_ch, "Successfully created attenuation '%s' with ID: %s",
                  attenuationName.c_str(), attenuationId.c_str());
        return attenuationId;
    }

    CCP_LOGERR_CH(s_ch, "Create succeeded but no ID returned for attenuation: %s", attenuationName.c_str());
    return "";
}

bool WaapiManager::SetReference(const std::string& objectId, 
                                 const std::string& referenceName, 
                                 const std::string& referenceId)
{
    if (!IsConnected()) {
        CCP_LOGERR_CH(s_ch, "Cannot set reference: not connected to Wwise");
        return false;
    }

    if (objectId.empty()) {
        CCP_LOGERR_CH(s_ch, "Cannot set reference: object ID is empty");
        return false;
    }

    if (referenceName.empty()) {
        CCP_LOGERR_CH(s_ch, "Cannot set reference: reference name is empty");
        return false;
    }

    auto* client = m_client.get();

    AkJson args(AkJson::Map{
        { "object", AkVariant(objectId.c_str()) },
        { "reference", AkVariant(referenceName.c_str()) },
        { "value", AkVariant(referenceId.c_str()) }
    });

    AkJson opts = AkJson::Map{};
    AkJson result;

    if (!client->Call(ak::wwise::core::object::setReference, args, opts, result)) {
        CCP_LOGERR_CH(s_ch, "Failed to set reference '%s' on object: %s", 
                     referenceName.c_str(), objectId.c_str());
        return false;
    }

    if (referenceId.empty()) {
        CCP_LOG_CH(s_ch, "Successfully cleared reference '%s' from object: %s", 
                  referenceName.c_str(), objectId.c_str());
    } else {
        CCP_LOG_CH(s_ch, "Successfully set reference '%s' = '%s' on object: %s", 
                  referenceName.c_str(), referenceId.c_str(), objectId.c_str());
    }

    return true;
}

