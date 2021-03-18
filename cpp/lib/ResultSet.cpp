/*
 * Copyright (c) 2021 Snowflake Computing, Inc. All rights reserved.
 */

#include <cstdio>
#include <cstring>
#include <ctime>

#include "../logger/SFLogger.hpp"
#include "snowflake/platform.h"
#include "snowflake/Simba_CRTFunctionSafe.h"
#include "DataConversion.hpp"
#include "ResultSet.hpp"

namespace Snowflake
{
namespace Client
{

ResultSet::ResultSet() :
    m_binaryOutputFormat("HEX"),
    m_dateOutputFormat("YYYY-MM-DD"),
    m_timeOutputFormat("HH24:MI:SS"),
    m_timestampOutputFormat("YYYY-MM-DD HH24:MI:SS.FF3 TZHTZM"),
    m_timestampLtzOutputFormat("YYYY-MM-DD HH24:MI:SS.FF3 TZHTZM"),
    m_timestampNtzOutputFormat("YYYY-MM-DD HH24:MI:SS.FF3"),
    m_timestampTzOutputFormat("YYYY-MM-DD HH24:MI:SS.FF3 TZHTZM"),
    m_currChunkIdx(0),
    m_currChunkRowIdx(0),
    m_currColumnIdx(0),
    m_currRowIdx(0)
{
    ;
}

ResultSet::ResultSet(
    cJSON * data,
    cJSON * rowset,
    SF_COLUMN_DESC * metadata,
    std::string tzString
) :
    m_currChunkIdx(0),
    m_currChunkRowIdx(0),
    m_currColumnIdx(0),
    m_currRowIdx(0),
    m_metadata(metadata),
    m_totalChunkCount(0),
    m_totalColumnCount(0),
    m_tzString(tzString)
{
    // Determine session parameter values.
    cJSON * params = snowflake_cJSON_GetObjectItem(data, "parameters");
    for (int i = 0; i < snowflake_cJSON_GetArraySize(params); ++i)
    {
        cJSON * curr = snowflake_cJSON_GetArrayItem(params, i);
        const char * paramName = snowflake_cJSON_GetObjectItem(curr, "name")->valuestring;

        if (paramName == "BINARY_OUTPUT_FORMAT")
            m_binaryOutputFormat = snowflake_cJSON_GetObjectItem(curr, "value")->valuestring;
        else if (paramName == "DATE_OUTPUT_FORMAT")
            m_dateOutputFormat = snowflake_cJSON_GetObjectItem(curr, "value")->valuestring;
        else if (paramName == "TIME_OUTPUT_FORMAT")
            m_timeOutputFormat = snowflake_cJSON_GetObjectItem(curr, "value")->valuestring;
        else if (paramName == "TIMESTAMP_OUTPUT_FORMAT")
            m_timestampOutputFormat = snowflake_cJSON_GetObjectItem(curr, "value")->valuestring;
        else if (paramName == "TIMESTAMP_LTZ_OUTPUT_FORMAT")
            m_timestampLtzOutputFormat = snowflake_cJSON_GetObjectItem(curr, "value")->valuestring;
        else if (paramName == "TIMESTAMP_NTZ_OUTPUT_FORMAT")
            m_timestampNtzOutputFormat = snowflake_cJSON_GetObjectItem(curr, "value")->valuestring;
        else if (paramName == "TIMESTAMP_TZ_OUTPUT_FORMAT")
            m_timestampTzOutputFormat = snowflake_cJSON_GetObjectItem(curr, "value")->valuestring;
    }
}

// Public getter methods ===========================================================================

std::string ResultSet::getBinaryOutputFormat()
{
    return m_binaryOutputFormat;
}

std::string ResultSet::getDateOutputFormat()
{
    return m_dateOutputFormat;
}

std::string ResultSet::getTimeOutputFormat()
{
    return m_timeOutputFormat;
}

std::string ResultSet::getTimestampOutputFormat()
{
    return m_timestampOutputFormat;
}

std::string ResultSet::getTimestampLtzOutputFormat()
{
    return m_timestampLtzOutputFormat;
}

std::string ResultSet::getTimestampNtzOutputFormat()
{
    return m_timestampNtzOutputFormat;
}

std::string ResultSet::getTimestampTzOutputFormat()
{
    return m_timestampTzOutputFormat;
}

QueryResultFormat ResultSet::getQueryResultFormat()
{
    return m_queryResultFormat;
}

size_t ResultSet::getTotalChunkCount()
{
    return m_totalChunkCount;
}

size_t ResultSet::getTotalColumnCount()
{
    return m_totalColumnCount;
}

size_t ResultSet::getTotalRowCount()
{
    return m_totalRowCount;
}

// Protected methods ===============================================================================

void ResultSet::initTzString()
{
    int zeroOffset = 1440;
    bool isPositiveOffset = zeroOffset >= m_tzOffset;

    char signChar = isPositiveOffset ? '+' : '-';

    // Extract HH and MM values from time offset.
    int absOffset =
        isPositiveOffset ? m_tzOffset - zeroOffset : zeroOffset - m_tzOffset;
    int hh = (int) absOffset / 60;
    int mm = absOffset % 60;

    char buffer[6];
    sb_sprintf(buffer, 6, "%c%02d:%02d", signChar, hh, mm);

    m_tzString = std::string(buffer);
}

} // namespace Client
} // namespace Snowflake
