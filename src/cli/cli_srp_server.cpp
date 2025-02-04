/*
 *  Copyright (c) 2020, The OpenThread Authors.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of the copyright holder nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file
 *   This file implements a simple CLI for the SRP server.
 */

#include "cli_srp_server.hpp"

#include <inttypes.h>

#include "cli/cli.hpp"
#include "common/string.hpp"

#if OPENTHREAD_CONFIG_SRP_SERVER_ENABLE

namespace ot {
namespace Cli {

constexpr SrpServer::Command SrpServer::sCommands[];

otError SrpServer::Process(Arg aArgs[])
{
    otError        error = OT_ERROR_INVALID_COMMAND;
    const Command *command;

    if (aArgs[0].IsEmpty())
    {
        IgnoreError(ProcessHelp(aArgs));
        ExitNow();
    }

    command = Utils::LookupTable::Find(aArgs[0].GetCString(), sCommands);
    VerifyOrExit(command != nullptr);

    error = (this->*command->mHandler)(aArgs + 1);

exit:
    return error;
}

otError SrpServer::ProcessAddrMode(Arg aArgs[])
{
    otError error = OT_ERROR_INVALID_ARGS;

    if (aArgs[0].IsEmpty())
    {
        switch (otSrpServerGetAddressMode(mInterpreter.mInstance))
        {
        case OT_SRP_SREVER_ADDRESS_MODE_UNICAST:
            mInterpreter.OutputLine("unicast");
            break;

        case OT_SRP_SERVER_ADDRESS_MODE_ANYCAST:
            mInterpreter.OutputLine("anycast");
            break;
        }

        error = OT_ERROR_NONE;
    }
    else if (aArgs[0] == "unicast")
    {
        error = otSrpServerSetAddressMode(mInterpreter.mInstance, OT_SRP_SREVER_ADDRESS_MODE_UNICAST);
    }
    else if (aArgs[0] == "anycast")
    {
        error = otSrpServerSetAddressMode(mInterpreter.mInstance, OT_SRP_SERVER_ADDRESS_MODE_ANYCAST);
    }

    return error;
}

otError SrpServer::ProcessDomain(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
    {
        mInterpreter.OutputLine("%s", otSrpServerGetDomain(mInterpreter.mInstance));
    }
    else
    {
        error = otSrpServerSetDomain(mInterpreter.mInstance, aArgs[0].GetCString());
    }

    return error;
}

otError SrpServer::ProcessState(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    switch (otSrpServerGetState(mInterpreter.mInstance))
    {
    case OT_SRP_SERVER_STATE_DISABLED:
        mInterpreter.OutputLine("disabled");
        break;
    case OT_SRP_SERVER_STATE_RUNNING:
        mInterpreter.OutputLine("running");
        break;
    case OT_SRP_SERVER_STATE_STOPPED:
        mInterpreter.OutputLine("stopped");
        break;
    default:
        mInterpreter.OutputLine("invalid state");
        break;
    }

    return OT_ERROR_NONE;
}

otError SrpServer::ProcessEnable(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    otSrpServerSetEnabled(mInterpreter.mInstance, /* aEnabled */ true);

    return OT_ERROR_NONE;
}

otError SrpServer::ProcessDisable(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    otSrpServerSetEnabled(mInterpreter.mInstance, /* aEnabled */ false);

    return OT_ERROR_NONE;
}

otError SrpServer::ProcessLease(Arg aArgs[])
{
    otError                error = OT_ERROR_NONE;
    otSrpServerLeaseConfig leaseConfig;

    if (aArgs[0].IsEmpty())
    {
        otSrpServerGetLeaseConfig(mInterpreter.mInstance, &leaseConfig);
        mInterpreter.OutputLine("min lease: %u", leaseConfig.mMinLease);
        mInterpreter.OutputLine("max lease: %u", leaseConfig.mMaxLease);
        mInterpreter.OutputLine("min key-lease: %u", leaseConfig.mMinKeyLease);
        mInterpreter.OutputLine("max key-lease: %u", leaseConfig.mMaxKeyLease);
    }
    else
    {
        SuccessOrExit(error = aArgs[0].ParseAsUint32(leaseConfig.mMinLease));
        SuccessOrExit(error = aArgs[1].ParseAsUint32(leaseConfig.mMaxLease));
        SuccessOrExit(error = aArgs[2].ParseAsUint32(leaseConfig.mMinKeyLease));
        SuccessOrExit(error = aArgs[3].ParseAsUint32(leaseConfig.mMaxKeyLease));
        VerifyOrExit(aArgs[4].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

        error = otSrpServerSetLeaseConfig(mInterpreter.mInstance, &leaseConfig);
    }

exit:
    return error;
}

otError SrpServer::ProcessHost(Arg aArgs[])
{
    otError                error = OT_ERROR_NONE;
    const otSrpServerHost *host;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    host = nullptr;
    while ((host = otSrpServerGetNextHost(mInterpreter.mInstance, host)) != nullptr)
    {
        const otIp6Address *addresses;
        uint8_t             addressesNum;
        bool                isDeleted = otSrpServerHostIsDeleted(host);

        mInterpreter.OutputLine("%s", otSrpServerHostGetFullName(host));
        mInterpreter.OutputLine(Interpreter::kIndentSize, "deleted: %s", isDeleted ? "true" : "false");
        if (isDeleted)
        {
            continue;
        }

        mInterpreter.OutputSpaces(Interpreter::kIndentSize);
        mInterpreter.OutputFormat("addresses: [");

        addresses = otSrpServerHostGetAddresses(host, &addressesNum);

        for (uint8_t i = 0; i < addressesNum; ++i)
        {
            mInterpreter.OutputIp6Address(addresses[i]);
            if (i < addressesNum - 1)
            {
                mInterpreter.OutputFormat(", ");
            }
        }

        mInterpreter.OutputFormat("]\r\n");
    }

exit:
    return error;
}

void SrpServer::OutputHostAddresses(const otSrpServerHost *aHost)
{
    const otIp6Address *addresses;
    uint8_t             addressesNum;

    addresses = otSrpServerHostGetAddresses(aHost, &addressesNum);

    mInterpreter.OutputFormat("[");
    for (uint8_t i = 0; i < addressesNum; ++i)
    {
        if (i != 0)
        {
            mInterpreter.OutputFormat(", ");
        }

        mInterpreter.OutputIp6Address(addresses[i]);
    }
    mInterpreter.OutputFormat("]");
}

otError SrpServer::ProcessService(Arg aArgs[])
{
    static constexpr char *kAnyServiceName  = nullptr;
    static constexpr char *kAnyInstanceName = nullptr;

    otError                error = OT_ERROR_NONE;
    const otSrpServerHost *host  = nullptr;

    VerifyOrExit(aArgs[0].IsEmpty(), error = OT_ERROR_INVALID_ARGS);

    while ((host = otSrpServerGetNextHost(mInterpreter.mInstance, host)) != nullptr)
    {
        const otSrpServerService *service = nullptr;

        while ((service = otSrpServerHostFindNextService(host, service, OT_SRP_SERVER_FLAGS_BASE_TYPE_SERVICE_ONLY,
                                                         kAnyServiceName, kAnyInstanceName)) != nullptr)
        {
            bool                      isDeleted    = otSrpServerServiceIsDeleted(service);
            const char *              instanceName = otSrpServerServiceGetInstanceName(service);
            const otSrpServerService *subService   = nullptr;
            const uint8_t *           txtData;
            uint16_t                  txtDataLength;
            bool                      hasSubType = false;

            mInterpreter.OutputLine("%s", instanceName);
            mInterpreter.OutputLine(Interpreter::kIndentSize, "deleted: %s", isDeleted ? "true" : "false");

            if (isDeleted)
            {
                continue;
            }

            mInterpreter.OutputFormat(Interpreter::kIndentSize, "subtypes: ");

            while ((subService = otSrpServerHostFindNextService(
                        host, subService, (OT_SRP_SERVER_SERVICE_FLAG_SUB_TYPE | OT_SRP_SERVER_SERVICE_FLAG_ACTIVE),
                        kAnyServiceName, instanceName)) != nullptr)
            {
                char subLabel[OT_DNS_MAX_LABEL_SIZE];

                IgnoreError(otSrpServerServiceGetServiceSubTypeLabel(subService, subLabel, sizeof(subLabel)));
                mInterpreter.OutputFormat("%s%s", hasSubType ? "," : "", subLabel);
                hasSubType = true;
            }

            mInterpreter.OutputLine(hasSubType ? "" : "(null)");

            mInterpreter.OutputLine(Interpreter::kIndentSize, "port: %hu", otSrpServerServiceGetPort(service));
            mInterpreter.OutputLine(Interpreter::kIndentSize, "priority: %hu", otSrpServerServiceGetPriority(service));
            mInterpreter.OutputLine(Interpreter::kIndentSize, "weight: %hu", otSrpServerServiceGetWeight(service));

            txtData = otSrpServerServiceGetTxtData(service, &txtDataLength);
            mInterpreter.OutputFormat(Interpreter::kIndentSize, "TXT: ");
            mInterpreter.OutputDnsTxtData(txtData, txtDataLength);
            mInterpreter.OutputLine("");

            mInterpreter.OutputLine(Interpreter::kIndentSize, "host: %s", otSrpServerHostGetFullName(host));

            mInterpreter.OutputFormat(Interpreter::kIndentSize, "addresses: ");
            OutputHostAddresses(host);
            mInterpreter.OutputLine("");
        }
    }

exit:
    return error;
}

otError SrpServer::ProcessSeqNum(Arg aArgs[])
{
    otError error = OT_ERROR_NONE;

    if (aArgs[0].IsEmpty())
    {
        mInterpreter.OutputLine("%u", otSrpServerGetAnycastModeSequenceNumber(mInterpreter.mInstance));
    }
    else
    {
        uint8_t sequenceNumber;

        SuccessOrExit(error = aArgs[0].ParseAsUint8(sequenceNumber));
        error = otSrpServerSetAnycastModeSequenceNumber(mInterpreter.mInstance, sequenceNumber);
    }

exit:
    return error;
}

otError SrpServer::ProcessHelp(Arg aArgs[])
{
    OT_UNUSED_VARIABLE(aArgs);

    for (const Command &command : sCommands)
    {
        mInterpreter.OutputLine(command.mName);
    }

    return OT_ERROR_NONE;
}

} // namespace Cli
} // namespace ot

#endif // OPENTHREAD_CONFIG_SRP_SERVER_ENABLE
