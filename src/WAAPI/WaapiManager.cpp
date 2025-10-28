#include "../stdafx.h"
#include "WaapiManager.h"

#include <AK/WwiseAuthoringAPI/AkAutobahn/Client.h>
#include <AK/WwiseAuthoringAPI/AkAutobahn/Logger.h>
#include <AK/WwiseAuthoringAPI/waapi.h>

#include <iostream>
#include <set>
#include <unordered_set>
#include <sstream>

using namespace AK::WwiseAuthoringAPI;

static CcpLogChannel_t s_ch = CCP_LOG_DEFINE_CHANNEL( "WaapiManager" );

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
{}

WaapiManager::~WaapiManager()
{
    Disconnect();
}

bool WaapiManager::Connect(const std::string& host, int port)
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
            CCP_LOGERR_CH(s_ch, "Could not connect to Wwise Authoring on %s:%d", m_host.c_str(), m_port);
            m_client.reset();
            return false;
        }

        m_connected = true;
        CCP_LOG_CH(s_ch, "Successfully connected to Wwise Authoring on %s:%d", m_host.c_str(), m_port);
        return true;
    }
    catch (const std::exception& e)
    {
        CCP_LOGERR_CH(s_ch, "Exception during connection: %s", e.what());
        m_client.reset();
        m_connected = false;
        return false;
    }
}

void WaapiManager::Disconnect()
{
    if (m_connected && m_client)
    {
        try
        {
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

    // Collect unique target IDs
    std::set<std::string> targetIdSet;
    for (const auto& row : targRes["return"].GetArray())
    {
        if (row.HasKey("target.id"))
            targetIdSet.insert(row["target.id"].GetVariant().GetString());
    }
    if (targetIdSet.empty()) return true;

    AkJson::Array targetIdArray;
    for (const auto& targetId : targetIdSet)
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

bool WaapiManager::GetFirstPlatformId(std::string& outPlatformId)
{

    if (!IsConnected()) return false;
    auto* client = m_client.get();

    AkJson result;
    if (!client->Call(ak::wwise::core::object::get,
                     AkJson::Map{ { "from", AkJson::Map{ { "ofType", AkJson::Array{ AkVariant("Platform") } } } } },
                     AkJson::Map{ { "return", AkJson::Array{ AkVariant("id"), AkVariant("name") } } },
                     result))
        return false;

    if (!result.HasKey("return") || result["return"].GetArray().empty())
        return false;

    outPlatformId = result["return"].GetArray()[0]["id"].GetVariant().GetString();
    return true;
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