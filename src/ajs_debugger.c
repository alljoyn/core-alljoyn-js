/**
 * @file
 */
/******************************************************************************
 * Copyright AllSeen Alliance. All rights reserved.
 *
 *    Permission to use, copy, modify, and/or distribute this software for any
 *    purpose with or without fee is hereby granted, provided that the above
 *    copyright notice and this permission notice appear in all copies.
 *
 *    THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 *    WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 *    MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 *    ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 *    WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 *    ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 *    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 ******************************************************************************/

#define AJ_MODULE DEBUGGER

#include "ajs_debugger.h"
/**
 * Controls debug output for this module
 */
#ifndef NDEBUG
uint8_t dbgDEBUGGER = 0;
#endif

static uint8_t readBuffer[DEBUG_BUFFER_SIZE];
static uint8_t writeBuffer[DEBUG_BUFFER_SIZE];

void AJS_DebuggerHandleMessage(AJS_DebuggerState* state);

static void printDebugMsg(const void* buffer, uint32_t length)
{
    int i = 0;
    AJ_AlwaysPrintf(("DEBUG MESSAGE:\n"));
    while (i < length) {
        if (*((uint8_t*)buffer + i) == DBG_TYPE_EOM) {
            AJ_AlwaysPrintf(("<EOM>\n"));
        } else if (*((uint8_t*)buffer + i) == DBG_TYPE_REQ) {
            AJ_AlwaysPrintf(("<REQ> "));
        } else if (*((uint8_t*)buffer + i) == DBG_TYPE_REP) {
            AJ_AlwaysPrintf(("<REP> "));
        } else if (*((uint8_t*)buffer + i) == DBG_TYPE_NFY) {
            AJ_AlwaysPrintf(("<NFY> "));
        } else if (*((uint8_t*)buffer + i) == DBG_TYPE_ERR) {
            AJ_AlwaysPrintf(("<ERR> "));
        } else {
            AJ_AlwaysPrintf(("0x%02x, ", *((uint8_t*)buffer + i)));
        }
        i++;
    }
    AJ_AlwaysPrintf(("\n"));
}

/*
 * Unmarshal a buffer (or debug message). The offset parameter signifies where
 * to start unmarshalling. This function will return the number of bytes that
 * was unmarshalled. This allows you to call this multiple times on the same
 * buffer but keep your place, similar to unmarshalling an AllJoyn message.
 * Note: Strings and buffers are allocated internally and it is required that they
 * are freed or this will leak memory.
 */
static uint16_t unmarhsalBuffer(uint8_t* buffer, uint32_t length, uint16_t offset, char* sig, ...)
{
    va_list args;
    uint8_t* ptr;
    uint16_t numBytes = offset;
    va_start(args, sig);
    ptr = (uint8_t*)(buffer + offset);
    while (*sig && (numBytes <= length)) {
        switch (*sig++) {
        case (AJ_DUK_TYPE_BYTE):
        case (AJ_DUK_TYPE_UNUSED):
        case (AJ_DUK_TYPE_UNDEF):
        case (AJ_DUK_TYPE_NULL):
        case (AJ_DUK_TYPE_TRUE):
        case (AJ_DUK_TYPE_FALSE):
            {
                uint8_t* u8;
                u8 = (uint8_t*)va_arg(args, uint8_t*);
                memcpy(u8, ptr, sizeof(uint8_t));
                ptr += 1;
                numBytes += 1;
            }
            break;

        case (AJ_DUK_TYPE_UINT32):
            {
                uint32_t* u32;
                u32 = (uint32_t*)va_arg(args, uint32_t*);
                if (*ptr != DBG_TYPE_INTEGER4) {
                    goto ErrorUnmarshal;
                }
                /* Advance past identifier */
                ptr++;
                numBytes++;
                memcpy(u32, ptr, sizeof(uint32_t));
                ptr += sizeof(uint32_t);
                numBytes += sizeof(uint32_t);
            }
            break;

        case (AJ_DUK_TYPE_STRING4):
            {
                char** str;
                uint32_t size;
                if (*ptr != DBG_TYPE_STRING4) {
                    goto ErrorUnmarshal;
                }
                /* Advance past identifier */
                ptr++;
                numBytes++;
                memcpy(&size, ptr, sizeof(uint32_t));
                ptr += sizeof(uint32_t);
                numBytes += sizeof(uint32_t);
                str = (char**)va_arg(args, char*);
                *str = (char*)AJ_Malloc(sizeof(char) * size + 1);
                memcpy(*str, ptr, sizeof(char) * size + 1);
                (*str)[size] = '\0';
                ptr += size;
                numBytes += size;
            }
            break;

        case (AJ_DUK_TYPE_STRING2):
            {
                char** str;
                uint16_t size;
                if (*ptr != DBG_TYPE_STRING2) {
                    goto ErrorUnmarshal;
                }
                /* Advance past identifier */
                ptr++;
                numBytes++;
                memcpy(&size, ptr, sizeof(uint16_t));
                ptr += sizeof(uint16_t);
                numBytes += sizeof(uint16_t);
                str = (char**)va_arg(args, char*);
                *str = (char*)AJ_Malloc(sizeof(char) * size + 1);
                memcpy(*str, ptr, sizeof(char) * size + 1);
                (*str)[size] = '\0';
                ptr += size;
                numBytes += size;
            }
            break;

        case (AJ_DUK_TYPE_BUFFER4):
            {
                uint8_t** buf;
                uint32_t size;
                if (*ptr != DBG_TYPE_BUFFER4) {
                    goto ErrorUnmarshal;
                }
                ptr++;
                numBytes++;
                memcpy(&size, ptr, sizeof(uint32_t));
                ptr += sizeof(uint32_t);
                numBytes += sizeof(uint32_t);
                buf = (uint8_t**)va_arg(args, uint8_t*);
                *buf = (uint8_t*)AJ_Malloc(sizeof(uint8_t) * size);
                memcpy(*buf, ptr, sizeof(uint8_t) * size);
                ptr += size;
                numBytes += size;
            }
            break;

        case (AJ_DUK_TYPE_BUFFER2):
            {
                uint8_t** buf;
                uint16_t size;
                if (*ptr != DBG_TYPE_BUFFER4) {
                    goto ErrorUnmarshal;
                }
                ptr++;
                numBytes++;
                memcpy(&size, ptr, sizeof(uint16_t));
                ptr += sizeof(uint16_t);
                numBytes += sizeof(uint16_t);
                buf = (uint8_t**)va_arg(args, uint8_t*);
                *buf = (uint8_t*)AJ_Malloc(sizeof(uint8_t) * size);
                memcpy(*buf, ptr, sizeof(uint8_t) * size);
                ptr += size;
                numBytes += size;
            }
            break;

        case (AJ_DUK_TYPE_NUMBER):
            {
                uint64_t* number;
                if (*ptr != DBG_TYPE_NUMBER) {
                    goto ErrorUnmarshal;
                }
                ptr++;
                numBytes++;
                number = (uint64_t*)va_arg(args, uint64_t*);
                memcpy(number, ptr, sizeof(uint64_t));
                ptr += sizeof(uint64_t);
                numBytes += sizeof(uint64_t);
            }
            break;

        case (AJ_DUK_TYPE_OBJECT):
        case (AJ_DUK_TYPE_POINTER):
        case (AJ_DUK_TYPE_LIGHTFUNC):
        case (AJ_DUK_TYPE_HEAPPTR):
            //TODO: Implement object, pointer, light func, and heap pointer types
            AJ_ErrPrintf(("Currently object, pointer, lightfunc and heap pointer types are not implemented\n"));
            goto ErrorUnmarshal;

        case (AJ_DUK_TYPE_STRING):
            {
                char** str;
                uint8_t size;
                if ((*ptr < DBG_TYPE_STRLOW) || (*ptr > DBG_TYPE_STRHIGH)) {
                    goto ErrorUnmarshal;
                }
                size = (*ptr) - 0x60;
                ptr++;
                numBytes++;
                str = (char**)va_arg(args, char*);
                *str = (char*)AJ_Malloc(sizeof(char) * size + 1);
                memcpy(*str, ptr, sizeof(char) * size + 1);
                (*str)[size] = '\0';
                ptr += size;
                numBytes += size;
            }
            break;

        case (AJ_DUK_TYPE_INTSM):
            {
                uint8_t* u8;
                if ((*ptr < 0x80) || (*ptr > 0xbf)) {
                    goto ErrorUnmarshal;
                }
                u8 = (uint8_t*)va_arg(args, uint8_t*);
                memcpy(u8, ptr, sizeof(uint8_t));
                *u8 -= 0x80;
                ptr += 1;
                numBytes += 1;
            }
            break;

        case (AJ_DUK_TYPE_INTLG):
            {
                uint16_t* u16;
                if ((*ptr < 0xc0) || (*ptr > 0xff)) {
                    goto ErrorUnmarshal;
                }
                u16 = (uint16_t*)va_arg(args, uint16_t*);
                /* ((byte0 - 0xc0) << 8) + byte1 */
                memcpy(u16, ptr, sizeof(uint16_t));
                //*u16 = ((((*ptr) - 0xc0) << 8) | *(ptr + 1));
                ptr += 2;
                numBytes += 2;
            }
            break;
        }
    }
    va_end(args);
    return numBytes;

ErrorUnmarshal:
    AJ_ErrPrintf(("Signature did not match bytes identifier in buffer, current byte: 0x%02x, numBytes: %u\n", *ptr, numBytes));
    return numBytes;
}

AJS_DebuggerState* AJS_InitDebugger(duk_context* ctx)
{
    AJS_DebuggerState* state = AJ_Malloc(sizeof(AJS_DebuggerState));
    state->read = AJ_Malloc(sizeof(AJ_IOBuffer));
    state->write = AJ_Malloc(sizeof(AJ_IOBuffer));
    /* Init write buffer */
    memset(writeBuffer, 0, sizeof(writeBuffer));
    AJ_IOBufInit(state->write, writeBuffer, sizeof(writeBuffer), 0, NULL);

    /* Init read buffer */
    memset(readBuffer, 0, sizeof(readBuffer));
    AJ_IOBufInit(state->read, readBuffer, sizeof(readBuffer), 0, NULL);

    state->ctx = ctx;
    state->initialNotify = FALSE;
    state->lastType = 0;
    state->nextChunk = 0;
    state->msgLength = 0;
    state->curTypePos = -1;
    memset(&state->lastMsg, 0, sizeof(AJ_Message));

    return state;
}

void AJS_DeinitDebugger(AJS_DebuggerState* state)
{
    if (state) {
        if (state->write) {
            AJ_Free(state->write);
        }
        if (state->read) {
            AJ_Free(state->read);
        }
        AJ_Free(state);
    }
}

/*
 * Marshal the tval (variant) portion of a message. The type provided is
 * what decides how the buffer provided will be parsed. It is assumed
 * that the message already has been marshalled enough to where the next argument
 * is the tval. Returned is the size of the data that was marshalled.
 */
static uint32_t marshalTvalMsg(AJ_Message* msg, uint8_t valType, uint8_t* buffer)
{
    AJ_Status status = AJ_OK;
    uint32_t size = 0;
    if (valType == DBG_TYPE_NUMBER) {
        uint64_t value;
        memcpy(&value, buffer, sizeof(uint64_t));
        if (status == AJ_OK) {
            status = AJ_MarshalVariant(msg, "t");
        }
        if (status == AJ_OK) {
            status = AJ_MarshalArgs(msg, "t", value);
        }
        size += sizeof(uint64_t);
    } else if ((valType == DBG_TYPE_UNUSED) ||
               (valType == DBG_TYPE_UNDEFINED) ||
               (valType == DBG_TYPE_NULL) ||
               (valType == DBG_TYPE_TRUE) ||
               (valType == DBG_TYPE_FALSE)) {
        uint8_t value = valType;
        if (status == AJ_OK) {
            status = AJ_MarshalVariant(msg, "y");
        }
        if (status == AJ_OK) {
            status = AJ_MarshalArgs(msg, "y", value);
        }
    } else if ((valType == DBG_TYPE_BUFFER2) || (valType == DBG_TYPE_LIGHTFUNC)) {
        AJ_Arg struct1;
        AJ_Arg array1;
        AJ_Status status = AJ_OK;
        int j;
        uint16_t sz;
        uint16_t flags;

        if (valType == DBG_TYPE_BUFFER2) {
            sz = buffer[0] << 8 | buffer[1];
            size += (2 + sz);
        } else {
            flags = buffer[0];
            sz = buffer[2];
            size = (3 + sz);
        }
        if (status == AJ_OK) {
            status = AJ_MarshalVariant(msg, "(qay)");
        }
        if (status == AJ_OK) {
            status = AJ_MarshalContainer(msg, &struct1, AJ_ARG_STRUCT);
        }
        if (status == AJ_OK) {
            if (valType == DBG_TYPE_BUFFER2) {
                status = AJ_MarshalArgs(msg, "q", sz);
            } else {
                status = AJ_MarshalArgs(msg, "q", flags);
            }
        }
        if (status == AJ_OK) {
            status = AJ_MarshalContainer(msg, &array1, AJ_ARG_ARRAY);
        }
        for (j = 0; j < sz; j++) {
            if (status == AJ_OK) {
                if (valType == DBG_TYPE_BUFFER2) {
                    status = AJ_MarshalArgs(msg, "y", buffer[2 + j]);
                } else {
                    status = AJ_MarshalArgs(msg, "y", buffer[3 + j]);
                }
            }
        }
        if (status == AJ_OK) {
            status = AJ_MarshalCloseContainer(msg, &array1);
        }
        if (status == AJ_OK) {
            status = AJ_MarshalCloseContainer(msg, &struct1);
        }
    } else if ((valType >= DBG_TYPE_STRLOW) && (valType <= DBG_TYPE_STRHIGH)) {
        uint8_t valLen;
        char* str;
        valLen = (valType - 0x60);
        str = AJ_Malloc(sizeof(char) * valLen + 1);
        memcpy(str, buffer, valLen);
        str[valLen] = '\0';
        if (status == AJ_OK) {
            status = AJ_MarshalVariant(msg, "s");
        }
        if (status == AJ_OK) {
            status = AJ_MarshalArgs(msg, "s", str);
        }
        AJ_Free(str);
        size += valLen;
    } else if ((valType == DBG_TYPE_OBJECT) || (valType == DBG_TYPE_POINTER) || (valType == DBG_TYPE_HEAPPTR)) {
        AJ_Arg struct1;
        AJ_Arg array1;
        AJ_Status status = AJ_OK;
        int j;
        /* Class number for object, optional for pointer/heapptr */
        uint8_t optional = 0;
        uint8_t sz;

        if (valType == DBG_TYPE_OBJECT) {
            optional = buffer[0];
            sz = buffer[1];
            size += (2 + sz);
        } else {
            sz = buffer[0];
            size += (1 + sz);
        }

        if (status == AJ_OK) {
            status = AJ_MarshalVariant(msg, "(yyay)");
        }
        if (status == AJ_OK) {
            status = AJ_MarshalContainer(msg, &struct1, AJ_ARG_STRUCT);
        }
        if (status == AJ_OK) {
            status = AJ_MarshalArgs(msg, "yy", valType, optional);
        }
        if (status == AJ_OK) {
            status = AJ_MarshalContainer(msg, &array1, AJ_ARG_ARRAY);
        }
        for (j = 0; j < sz; j++) {
            if (status == AJ_OK) {
                if (valType == DBG_TYPE_OBJECT) {
                    status = AJ_MarshalArgs(msg, "y", buffer[2 + j]);
                } else {
                    status = AJ_MarshalArgs(msg, "y", buffer[1 + j]);
                }
            }
        }
        if (status == AJ_OK) {
            status = AJ_MarshalCloseContainer(msg, &array1);
        }
        if (status == AJ_OK) {
            status = AJ_MarshalCloseContainer(msg, &struct1);
        }
    } else if (valType == DBG_TYPE_STRING2) {
        char* str;
        uint16_t sz;
        memcpy(&sz, buffer, sizeof(uint16_t));
        sz = AJ_ByteSwap16(sz);
        str = (char*)AJ_Malloc(sizeof(char) * sz + 1);
        memcpy(str, (buffer + 2), sz);
        str[sz] = '\0';
        status = AJ_MarshalVariant(msg, "s");
        if (status == AJ_OK) {
            status = AJ_MarshalArgs(msg, "s", str);
        }
        size += sizeof(uint16_t) + sz;
    } else {
        AJ_ErrPrintf(("AJS_DebuggerHandleMessage(): Unknown data type: 0x%x\n", valType));
        return 0;
    }
    if (status != AJ_OK) {
        AJ_ErrPrintf(("Error marshalling tval, status = %s\n", AJ_StatusText(status)));
    }
    return size;
}

/*
 * Copies message data into the read buffer and then copies
 * some requested number of bytes into the debug read buffer
 * and re-bases this buffer, since we know a full message is there
 */
static uint32_t copyToBuffers(AJS_DebuggerState* state, uint8_t* to, uint8_t* from, uint32_t fromLen, uint32_t requested)
{
    /* Either give the requested bytes, or the message length */
    uint32_t newMinLen = min(fromLen, requested);
    if (AJ_IO_BUF_SPACE(state->read) >= fromLen) {
        memcpy(state->read->writePtr, from, fromLen);
        state->read->writePtr += fromLen;
    } else {
        AJ_ErrPrintf(("No space to write debug message\n"));
        return 0;
    }

    if (AJ_IO_BUF_AVAIL(state->read) >= newMinLen) {
        /* Copy the *requested* data into the buffer */
        memcpy(to, state->read->readPtr, newMinLen);
        state->read->readPtr += newMinLen;
        AJ_IOBufRebase(state->read, 0);
    } else {
        AJ_ErrPrintf(("Not enough data to read from buffer\n"));
        return 0;
    }

    return newMinLen;
}

duk_size_t AJS_DebuggerRead(void* udata, char*buffer, duk_size_t length)
{
    AJS_DebuggerState* state = (AJS_DebuggerState*)udata;
    AJ_Status status;
    AJ_Message msg;
    AJ_BusAttachment* bus = AJS_GetBusAttachment();
    uint32_t minLen = min(length, AJ_IO_BUF_AVAIL(state->read));
    if (length > 0 && minLen == 0) {
        /* No data is currently available, must wait for another message to come through */
        while (TRUE) {
            uint32_t newMinLen;
            uint8_t type = 0;

            status = AJ_UnmarshalMsg(bus, &msg, AJ_TIMER_FOREVER);
            if (status != AJ_OK) {
                if ((status == AJ_ERR_INTERRUPTED) || (status == AJ_ERR_TIMEOUT)) {
                    status = AJ_OK;
                    continue;
                }
                if (status == AJ_ERR_NO_MATCH) {
                    status = AJ_OK;
                }
                AJ_CloseMsg(&msg);
                continue;
            }
            switch (msg.msgId) {
            /*
             * Some AllJoyn messages must be handled here or else
             * the router will time out.
             */
            case AJ_SIGNAL_SESSION_LOST:
            case AJ_SIGNAL_SESSION_LOST_WITH_REASON:
                AJ_ErrPrintf(("AJS_DebuggerRead(): Session lost\n"));
                /* Session was lost so we need to end the debug session */
                AJS_SessionLost(state->ctx, &msg);
                AJS_DeinitDebugger(state);
                AJ_CloseMsg(&msg);
                return 0;

            case AJ_SIGNAL_PROBE_ACK:
            case AJ_SIGNAL_PROBE_REQ:
            case AJ_METHOD_PING:
            case AJ_METHOD_BUS_PING:
                status = AJ_BusHandleBusMessage(&msg);
                AJ_CloseMsg(&msg);
                break;

            case DBG_ADDBREAK_MSGID:
                {
                    char* file;
                    uint8_t line;
                    uint8_t* tmp;
                    uint8_t msgLen;
                    status = AJ_UnmarshalArgs(&msg, "sy", &file, &line);

                    if (status == AJ_OK) {
                        msgLen = strlen(file) + 5;
                        tmp = AJ_Malloc(msgLen);
                        memset(tmp, DBG_TYPE_REQ, 1);
                        memset(tmp + 1, (ADD_BREAK_REQ + 0x80), 1);
                        memset(tmp + 2, (strlen(file) + 0x60), 1);
                        memcpy(tmp + 3, file, strlen(file));
                        memset(tmp + 3 + strlen(file), (line + 0x80), 1);
                        memset(tmp + 3 + strlen(file) + 1, DBG_TYPE_EOM, 1);

                        /* Copy the bytes to the static buffer as well as the read buffer */
                        newMinLen = copyToBuffers(state, (uint8_t*)buffer, tmp, (uint32_t)msgLen, (uint32_t)length);

                        memcpy(&state->lastMsg, &msg, sizeof(AJ_Message));
                        state->lastMsgType = ADD_BREAK_REQ;

                        AJ_CloseMsg(&msg);
                        AJ_Free(tmp);
                        return newMinLen;
                    }
                }
                break;

            case DBG_DELBREAK_MSGID:
                {
                    uint8_t index;
                    uint8_t msgLen = 4;
                    status = AJ_UnmarshalArgs(&msg, "y", &index);
                    if (status == AJ_OK) {
                        uint32_t tmp = BUILD_DBG_MSG(DBG_TYPE_REQ, (DEL_BREAK_REQ + 0x80), (index + 0x80), DBG_TYPE_EOM);

                        /* Copy the bytes to the static buffer as well as the read buffer */
                        newMinLen = copyToBuffers(state, (uint8_t*)buffer, (uint8_t*)&tmp, msgLen, length);

                        memcpy(&state->lastMsg, &msg, sizeof(AJ_Message));
                        state->lastMsgType = DEL_BREAK_REQ;

                        AJ_CloseMsg(&msg);
                        return newMinLen;
                    }
                }
                break;

            case DBG_GETVAR_MSGID:
                {
                    char* var;
                    uint8_t varLen;
                    uint8_t* tmp;
                    uint8_t msgLen;
                    status = AJ_UnmarshalArgs(&msg, "s", &var);
                    if (status == AJ_OK) {
                        varLen = strlen(var);
                        msgLen = (varLen + 4);
                        tmp = AJ_Malloc(msgLen);
                        memset(tmp, DBG_TYPE_REQ, 1);
                        memset(tmp + 1, (GET_VAR_REQ + 0x80), 1);
                        memset(tmp + 2, (varLen + 0x60), 1);
                        memcpy(tmp + 3, var, varLen);
                        memset(tmp + 3 + varLen, DBG_TYPE_EOM, 1);

                        /* Copy the bytes to the static buffer as well as the read buffer */
                        newMinLen = copyToBuffers(state, (uint8_t*)buffer, tmp, msgLen, length);

                        memcpy(&state->lastMsg, &msg, sizeof(AJ_Message));
                        state->lastMsgType = GET_VAR_REQ;

                        AJ_Free(tmp);
                        AJ_CloseMsg(&msg);
                        return newMinLen;
                    }
                }
                break;

            case DBG_PUTVAR_MSGID:
                {
                    char* name;
                    uint8_t type;
                    uint32_t size;
                    size_t sz;
                    uint8_t nameLen;
                    uint32_t runningLength = 0;
                    const void* raw;
                    status = AJ_UnmarshalArgs(&msg, "sy", &name, &type);
                    if (status == AJ_OK) {
                        status = AJ_UnmarshalRaw(&msg, &raw, sizeof(size), &sz);
                    }
                    if (status == AJ_OK) {
                        memcpy(&size, raw, sizeof(size));
                    }
                    nameLen = strlen(name);
                    /*
                     * Check for room:
                     * <REQ><PUTVAR><NAMELEN><NAME><TYPE>
                     *   1  +  1   +   1   +   N  +  1
                     */
                    if (AJ_IO_BUF_SPACE(state->read) < (nameLen + 4)) {
                        AJ_ErrPrintf(("No space to write debug message\n"));
                        return 0;
                    }
                    /*
                     * Formulate the debug message header
                     * <REQ><0x1b><NAME LEN><NAME STR><VAL TYPE><VALUE BYTES><EOM>
                     */
                    memset(state->read->writePtr, DBG_TYPE_REQ, 1);
                    runningLength++;
                    state->read->writePtr++;
                    memset(state->read->writePtr, (PUT_VAR_REQ + 0x80), 1);
                    runningLength++;
                    state->read->writePtr++;
                    memset(state->read->writePtr, (nameLen + 0x60), 1);
                    runningLength++;
                    state->read->writePtr++;
                    memcpy(state->read->writePtr, name, nameLen);
                    runningLength += nameLen;
                    state->read->writePtr += nameLen;
                    memset(state->read->writePtr, type, 1);
                    runningLength++;
                    state->read->writePtr++;

                    /* Number (double) needs an endian swap */
                    if (type == DBG_TYPE_NUMBER) {
                        uint64_t tmp = 0;
                        uint8_t* t = (uint8_t*)&tmp;
                        uint8_t pos = 0;
                        while (size) {
                            status = AJ_UnmarshalRaw(&msg, &raw, size, &sz);
                            if (status != AJ_OK) {
                                AJ_ErrPrintf(("AJS_DebuggerRead(): Error unmarshalling\n"));
                                return 0;
                            }

                            memcpy((t + pos), raw, sz);
                            pos += sz;
                            runningLength += sz;
                            size -= sz;
                        }

                        tmp = (double)AJ_ByteSwap64(tmp);
                        if (AJ_IO_BUF_SPACE(state->read) < 8) {
                            AJ_ErrPrintf(("No space to write debug message\n"));
                            return 0;
                        }
                        memcpy(state->read->writePtr, &tmp, 8);
                        state->read->writePtr += 8;
                    } else {
                        /*
                         * Copy value data into the read buffer
                         */
                        while (size) {
                            status = AJ_UnmarshalRaw(&msg, &raw, size, &sz);
                            if (status != AJ_OK) {
                                AJ_ErrPrintf(("AJS_DebuggerRead(): Error unmarshalling\n"));
                                return 0;
                            }
                            if (AJ_IO_BUF_SPACE(state->read) < sz) {
                                AJ_ErrPrintf(("No space to write debug message\n"));
                                return 0;
                            }
                            memcpy(state->read->writePtr, raw, sz);
                            state->read->writePtr += sz;
                            runningLength += sz;
                            size -= sz;
                        }
                    }
                    if (AJ_IO_BUF_SPACE(state->read) < 1) {
                        AJ_ErrPrintf(("No space to write debug message\n"));
                        return 0;
                    }
                    memset(state->read->writePtr, DBG_TYPE_EOM, 1);
                    runningLength++;
                    state->read->writePtr++;
                    /*
                     * Copy the requested number of bytes into the supplied buffer
                     */
                    newMinLen = min(length, runningLength);

                    if (AJ_IO_BUF_SPACE(state->read) < newMinLen) {
                        AJ_ErrPrintf(("No space to write debug message\n"));
                        return 0;
                    }
                    memcpy(buffer, state->read->readPtr, newMinLen);
                    state->read->readPtr += newMinLen;
                    AJ_IOBufRebase(state->read, 0);

                    memcpy(&state->lastMsg, &msg, sizeof(AJ_Message));
                    state->lastMsgType = PUT_VAR_REQ;

                    AJ_CloseMsg(&msg);
                    return newMinLen;
                }
                break;

            case DBG_EVAL_MSGID:
                {
                    char* var;
                    uint8_t varLen;
                    uint8_t* tmp;
                    uint8_t msgLen;
                    status = AJ_UnmarshalArgs(&msg, "s", &var);
                    if (status == AJ_OK) {
                        varLen = strlen(var);
                        msgLen = (varLen + 4);
                        tmp = AJ_Malloc(msgLen);
                        memset(tmp, DBG_TYPE_REQ, 1);
                        memset(tmp + 1, (EVAL_REQ + 0x80), 1);
                        memset(tmp + 2, (varLen + 0x60), 1);
                        memcpy(tmp + 3, var, varLen);
                        memset(tmp + 3 + varLen, DBG_TYPE_EOM, 1);

                        /* Copy the bytes to the static buffer as well as the read buffer */
                        newMinLen = copyToBuffers(state, (uint8_t*)buffer, tmp, msgLen, length);

                        memcpy(&state->lastMsg, &msg, sizeof(AJ_Message));
                        state->lastMsgType = EVAL_REQ;

                        AJ_CloseMsg(&msg);
                        return newMinLen;
                    }
                }
                break;

            case DBG_GETSCRIPT_MSGID:
                {
                    AJ_Message reply;
                    const uint8_t* script;
                    AJ_NV_DATASET* ds = NULL;
                    AJ_NV_DATASET* dsize = NULL;
                    uint32_t sz = AJS_GetScriptSize();

                    /* If the script was previously installed on another boot the size will be zero */
                    if (!sz) {
                        dsize = AJ_NVRAM_Open(AJS_SCRIPT_SIZE_ID, "r", 0);
                        if (dsize) {
                            AJ_NVRAM_Read(&sz, sizeof(sz), dsize);
                            AJ_NVRAM_Close(dsize);
                        }
                    }

                    ds = AJ_NVRAM_Open(AJS_SCRIPT_NVRAM_ID, "r", 0);
                    if (ds) {
                        script = AJ_NVRAM_Peek(ds);

                        status = AJ_MarshalReplyMsg(&msg, &reply);
                        if (status == AJ_OK) {
                            status = AJ_DeliverMsgPartial(&reply, sz + sizeof(uint32_t));
                        }
                        if (status == AJ_OK) {
                            status = AJ_MarshalRaw(&reply, &sz, sizeof(uint32_t));
                        }
                        if (status == AJ_OK) {
                            status = AJ_MarshalRaw(&reply, script, sz);
                        }
                        if (status == AJ_OK) {
                            status = AJ_DeliverMsg(&reply);
                        }

                        AJ_NVRAM_Close(ds);
                    } else {
                        AJ_ErrPrintf(("Error opening script NVRAM entry\n"));
                    }
                }
                AJ_CloseMsg(&msg);
                continue;

            /*
             * Message format is the same for all simple commands
             */
            case DBG_BASIC_MSGID:
                type = BASIC_INFO_REQ;
                break;

            case DBG_TRIGGER_MSGID:
                type = TRIGGER_STATUS_REQ;
                break;

            case DBG_PAUSE_MSGID:
                type = PAUSE_REQ;
                break;

            case DBG_RESUME_MSGID:
                type = RESUME_REQ;
                break;

            case DBG_STEPIN_MSGID:
                type = STEP_INTO_REQ;
                break;

            case DBG_STEPOVER_MSGID:
                type = STEP_OVER_REQ;
                break;

            case DBG_STEPOUT_MSGID:
                type = STEP_OUT_REQ;
                break;

            case DBG_LISTBREAK_MSGID:
                type = LIST_BREAK_REQ;
                break;

            case DBG_GETCALL_MSGID:
                type = GET_CALL_STACK_REQ;
                break;

            case DBG_GETLOCALS_MSGID:
                type = GET_LOCALS_REQ;
                break;

            case DBG_DETACH_MSGID:
                type = DETACH_REQ;
                break;
            }
            if (type != 0) {
                {
                    uint32_t dbgMsg = BUILD_DBG_MSG(DBG_TYPE_REQ, (type + 0x80), DBG_TYPE_EOM, 0);

                    newMinLen = copyToBuffers(state, (uint8_t*)buffer, (uint8_t*)&dbgMsg, 3, length);

                    /* Save away the reply for when write is called */
                    memcpy(&state->lastMsg, &msg, sizeof(AJ_Message));
                    state->lastMsgType = type;
                }
                AJ_CloseMsg(&msg);
                return newMinLen;
            }
            AJ_CloseMsg(&msg);
        }
    } else if (minLen) {
        /* Read operation */
        memcpy(buffer, state->read->readPtr, minLen);
        state->read->readPtr += minLen;
        AJ_IOBufRebase(state->read, 0);
        return minLen;
    } else {
        /* Peek operation */
        if (AJ_IO_BUF_AVAIL(state->read) > 0) {
            return 1;
        } else {
            return 0;
        }
    }
}

static AJ_Status Handle2ByteType(AJS_DebuggerState* state, uint8_t* pos)
{
    if ((state->lastType == DBG_TYPE_BUFFER2) || (state->lastType == DBG_TYPE_STRING2)) {
        if (state->curTypePos >= 0) {
            state->internal = (*pos) << (state->curTypePos * 8);
            state->curTypePos++;
            if (state->curTypePos >= 2) {
                state->curTypePos = -1;
                state->nextChunk = AJ_ByteSwap16((uint16_t)state->internal);
                state->internal = 0;
                state->lastType = 0;
            }
        } else {
            AJ_ErrPrintf(("There was some error parsing BUFFER2 type, *pos = 0x%02x\n", *pos));
            return AJ_ERR_INVALID;
        }
    } else {
        /* 2 Byte buffer identifier */
#ifdef DBG_PRINT_CHUNKS
        AJ_AlwaysPrintf(("<BUFFER2 0x%02x >, ", *pos));
#endif
        /* Advance past identifier */
        state->lastType = *pos;
        state->curTypePos = 0;
    }
    return AJ_OK;
}

static AJ_Status Handle4ByteType(AJS_DebuggerState* state, uint8_t* pos)
{
    if ((state->lastType == DBG_TYPE_BUFFER4) || (state->lastType == DBG_TYPE_STRING4)) {
        if (state->curTypePos >= 0) {
            /*
             * In the middle of the buffer/strings 4 byte length. This means
             * that whatever position were in corresponds to the byte
             * shift for this byte of the length
             */
            state->internal = (*pos) << (state->curTypePos * 8);
            state->curTypePos++;
            /* End of the 4 byte length so set that to nextChunk */
            if (state->curTypePos >= 4) {
                state->curTypePos = -1;
                state->nextChunk = AJ_ByteSwap32(state->internal);
                state->internal = 0;
                state->lastType = 0;
            }
        } else {
            AJ_ErrPrintf(("There was some error parsing BUFFER4 type, *pos = 0x%02x\n", *pos));
            return AJ_ERR_INVALID;
        }
    } else {
#ifdef DBG_PRINT_CHUNKS
        AJ_AlwaysPrintf(("<BUFFER4 0x%02x >, ", *pos));
#endif
        /* Advance past the identifier */
        state->lastType = *pos;
        state->curTypePos = 0;
    }
    return AJ_OK;
}

static AJ_Status HandleObjectType(AJS_DebuggerState* state, uint8_t* pos)
{
    if (state->lastType == DBG_TYPE_OBJECT) {
        if (state->curTypePos == 0) {
#ifdef DBG_PRINT_CHUNKS
            AJ_AlwaysPrintf(("<CLASS=0x%02x> ", *pos));
#endif
            /* Advance past class number */
            state->curTypePos++;
        } else if (state->curTypePos == 1) {
#ifdef DBG_PRINT_CHUNKS
            AJ_AlwaysPrintf(("<PTR LEN=0x%02x> ", *pos));
#endif
            /* Pointer length */
            state->nextChunk = *pos;
            state->curTypePos = 0;
        } else {
            AJ_ErrPrintf(("There was some error parsing OBJECT type, *pos = 0x%02x\n", *pos));
            return AJ_ERR_INVALID;
        }
    } else {
        /* First iteration of object type */
#ifdef DBG_PRINT_CHUNKS
        AJ_AlwaysPrintf(("<OBJECT> "));
#endif
        state->lastType = DBG_TYPE_OBJECT;
        state->curTypePos = 0;
    }
    return AJ_OK;
}

static AJ_Status HandlePointerType(AJS_DebuggerState* state, uint8_t* pos)
{
    /* Pointer or heap pointer data type */
    if ((state->lastType == DBG_TYPE_POINTER) || (state->lastType == DBG_TYPE_HEAPPTR)) {
#ifdef DBG_PRINT_CHUNKS
        AJ_AlwaysPrintf(("<PTR/HEAPPTR LEN = 0x%02x>, ", *pos));
#endif
        state->lastType = 0;
        state->nextChunk = *pos;
    } else {
#ifdef DBG_PRINT_CHUNKS
        AJ_AlwaysPrintf(("<PTR/HEAPPTR>, "));
#endif
        state->lastType = *pos;
    }
    return AJ_OK;
}

duk_size_t AJS_DebuggerWrite(void* udata, const char*buffer, duk_size_t length)
{
    AJS_DebuggerState* state = (AJS_DebuggerState*)udata;
    uint8_t* pos;
    uint8_t advance = 0;
    AJ_Status status = AJ_OK;
    AJ_BusAttachment* bus = AJS_GetBusAttachment();
    uint32_t len = length;
    /*
     * Upon initial debugger startup version information is sent out. This does
     * not follow the same REQ/REP EOM format as other messages so it needs to be
     * handled differently.
     */
    if (!state->initialNotify) {
        AJ_Message versionMsg;
        char* versionString = AJ_Malloc(sizeof(char) * length + 1);
        memcpy(versionString, buffer, length);
        versionString[length] = '\0';
        status = AJ_MarshalSignal(bus, &versionMsg, DBG_VERSION_MSGID, AJS_GetConsoleBusName(), AJS_GetConsoleSession(), 0, 0);
        if (status == AJ_OK) {
            status = AJ_MarshalArgs(&versionMsg, "s", versionString);
        }
        if (status == AJ_OK) {
            AJ_DeliverMsg(&versionMsg);
        }
        state->initialNotify = TRUE;
        AJ_Free(versionString);
    } else {
        if (AJ_IO_BUF_SPACE(state->write) < length) {
            AJ_ErrPrintf(("No space to write debug message\n"));
            return 0;
        }
        /* Add the new data to the write buffer */
        memcpy(state->write->writePtr, buffer, length);
        state->write->writePtr += length;
        /* Set the position to the read pointer + current message length */
        pos = state->write->readPtr + state->msgLength;

        /*
         * This state machine processes debug data and decides when there is a full message
         * to parse and process. It must be able to process debug messages 1 byte at a time.
         * To do this a state needs to be kept. The general flow is as follows:
         *  - Check if 'nextChunk' is > 0. This means that a data type length has been found
         *    previously and we need to advance the pointer by that amount (1 byte at a time)
         *  - Check for a data type or that we are currently in the middle of processing some
         *    arbitrary data type.
         *  - If we found a data type header set the 'lastType' field for that type and continue.
         *  - If we are in a data type ('lastType' is set) we need to continue processing this
         *    data type. This processing is specific to each data types but generally it will
         *    consist of an ID then length followed by the data. The if statements for each
         *    type will usually follow this basic format. When a length is found 'nextChunk' is
         *    set and the next time around the pointer will be advanced by that amount.
         *  - If the byte we are on is EOM (0x00) then an entire message has been found. The
         *    state machine then calls AJS_DebuggerHandleMessage() which essentially
         *    does the same thing but it is known that a full message is contained in the buffer.
         *    After the debug message is unmarshalled the resulting data can be sent back to the
         *    debug client. This is done via a method reply, since all this was initiated by a
         *    method call from a debug command.
         */
        while (len--) {
            /*
             * Must advance pointer by "nextChunk", this must be done 1 byte at a time
             * to conform to the stream protocol. (in case the target is slow).
             */
            if (state->nextChunk) {
                state->nextChunk--;
                pos++;
                advance++;
                continue;
            }
            if (((*pos == DBG_TYPE_REP) || (*pos == DBG_TYPE_NFY) || (*pos == DBG_TYPE_ERR)) && (state->lastType == 0)) {
                /* Reply or notify header types, advance 1 byte */
#ifdef DBG_PRINT_CHUNKS
                AJ_AlwaysPrintf(("<REP or NFY, 0x%02x >, ", *pos));
#endif
                /* Message header, advance 1 byte */
                state->lastType = 0;
                pos++;
                advance++;
                continue;
            } else if ((*pos == DBG_TYPE_BUFFER2) || (*pos == DBG_TYPE_STRING2) || (state->lastType == DBG_TYPE_BUFFER2) || (state->lastType == DBG_TYPE_STRING2)) {
                status = Handle2ByteType(state, pos);
                if (status == AJ_OK) {
                    pos++;
                    advance++;
                    continue;
                } else {
                    return 0;
                }
            } else if ((*pos == DBG_TYPE_BUFFER4) || (*pos == DBG_TYPE_STRING4) || (state->lastType == DBG_TYPE_BUFFER4) || (state->lastType == DBG_TYPE_STRING4)) {
                status = Handle4ByteType(state, pos);
                if (status == AJ_OK) {
                    pos++;
                    advance++;
                    continue;
                } else {
                    return 0;
                }
            } else if ((*pos >= DBG_TYPE_INTSMLOW) && (*pos <= DBG_TYPE_INTSMHIGH)) {
                /* Small integer data type */
#ifdef DBG_PRINT_CHUNKS
                AJ_AlwaysPrintf(("<INTEGER, 0x%02x >, ", *pos));
#endif
                /* Small integer type, advance 1 byte */
                pos++;
                advance++;
                continue;
            } else if (((*pos >= DBG_TYPE_STRLOW) && (*pos <= DBG_TYPE_STRHIGH)) || (state->lastType == DBG_TYPE_STRLOW)) {

#ifdef DBG_PRINT_CHUNKS
                AJ_AlwaysPrintf(("<STRING>,<LENGTH=0x%02x>,", (*pos) - DBG_TYPE_STRLOW));
#endif
                /* String length type, advance 1 byte and next is the string length */
                state->nextChunk = (*pos) - DBG_TYPE_STRLOW;
                state->lastType = 0;
                pos++;
                advance++;
                continue;
            } else if (*pos == DBG_TYPE_EOM) {
                /* EOM found, handle the message */
#ifdef DBG_PRINT_CHUNKS
                AJ_AlwaysPrintf(("<EOM>\n"));
#endif
                state->nextChunk = 0;
                state->lastType = 0;
                state->lastType = 0;
                pos++;
                AJS_DebuggerHandleMessage(state);
                state->msgLength = 0;
                break;
            } else if (*pos == DBG_TYPE_NUMBER) {
                /* Number type, next 8 bytes are its value */
#ifdef DBG_PRINT_CHUNKS
                AJ_AlwaysPrintf(("<NUMBER, advance 8 bytes >, "));
#endif
                state->nextChunk = 8;
                pos++;
                advance++;
                continue;
            } else if ((*pos == DBG_TYPE_UNUSED) ||
                       (*pos == DBG_TYPE_UNDEFINED) ||
                       (*pos == DBG_TYPE_NULL) ||
                       (*pos == DBG_TYPE_TRUE) ||
                       (*pos == DBG_TYPE_FALSE)) {
                /* All these types have no data to follow */
#ifdef DBG_PRINT_CHUNKS
                AJ_AlwaysPrintf(("<UNUSED, UNDEF, NULL, TRUE, FALSE>, "));
#endif
                state->nextChunk = 0;
                pos++;
                advance++;
                continue;
            } else if (((*pos >= DBG_TYPE_INTLGLOW) && (*pos <= DBG_TYPE_INTLGHIGH)) || state->lastType == DBG_TYPE_INTLGLOW) {
                /* Large integer data type */
#ifdef DBG_PRINT_CHUNKS
                AJ_AlwaysPrintf(("<INT LARGE>, "));
#endif
                /* Parsed past the identifier, just 1 byte left in this data type */
                if (state->lastType == DBG_TYPE_INTLGLOW) {
                    state->lastType = 0;
                    pos++;
                    advance++;
                } else {
                    state->lastType = DBG_TYPE_INTLGLOW;
                    pos++;
                    advance++;
                }
                continue;
            } else if ((*pos == DBG_TYPE_OBJECT) || (state->lastType == DBG_TYPE_OBJECT)) {
                status = HandleObjectType(state, pos);
                if (status == AJ_OK) {
                    pos++;
                    advance++;
                    continue;
                } else {
                    return 0;
                }
            } else if ((*pos == DBG_TYPE_POINTER) || (*pos == DBG_TYPE_HEAPPTR) || (state->lastType == DBG_TYPE_POINTER) || (state->lastType == DBG_TYPE_HEAPPTR)) {
                status = HandlePointerType(state, pos);
                if (status == AJ_OK) {
                    pos++;
                    advance++;
                    continue;
                } else {
                    return 0;
                }
            } else if (*pos == DBG_TYPE_LIGHTFUNC) {
                /* First iteration of light function data */
#ifdef DBG_PRINT_CHUNKS
                AJ_AlwaysPrintf(("<LIGHT FUNC>, "));
#endif
                pos++;
                advance++;
                continue;
            } else {
                AJ_ErrPrintf(("Unhandled, byte = 0x%02x\n", *pos));
                break;
            }
        }
#ifdef DBG_PRINT_CHUNKS
        AJ_AlwaysPrintf(("END SEGMENT\n"));
#endif
        state->msgLength += advance;
    }
    /* Bytes provided were processed */
    if (length > 0 && buffer) {
        return length;
    }

    /* Write flush, writes are always flushed automatically so just return */
    return 1;
}

duk_size_t AJS_DebuggerPeek(void* udata)
{
    AJS_DebuggerState* state = (AJS_DebuggerState*)udata;
    return AJ_IO_BUF_AVAIL(state->read);
}

void AJS_DebuggerReadFlush(void* udata)
{
    return;
}

void AJS_DebuggerWriteFlush(void* udata)
{
    return;
}

void AJS_DebuggerDetached(void* udata)
{
    AJS_DebuggerState* state = (AJS_DebuggerState*)udata;
    AJ_AlwaysPrintf(("AJS_DebuggerDetached(): Debugger was detached\n"));
    AJS_DeinitDebugger(state);
}

/*
 * Called when a complete debug message is in the buffer
 * and ready for the final parse and delivery
 */
void AJS_DebuggerHandleMessage(AJS_DebuggerState* state)
{
    AJ_Message reply;
    AJ_Status status = AJ_OK;
    uint8_t* pos = state->write->bufStart;
    if (*pos == DBG_TYPE_NFY) {
        switch (*(pos + 1) - 0x80) {
        case STATUS_NOTIFICATION:
            {
                /*
                 * Status notification format:
                 * <NFY><state><file name len><file name data><func name length><func name data><line><PC><EOM>
                 */
                AJ_BusAttachment* bus = AJS_GetBusAttachment();
                AJ_Message msg;
                AJ_Status status;
                uint8_t type, st, lineNumber, pc;
                char* fileName, *funcName;

                unmarhsalBuffer(state->write->bufStart, AJ_IO_BUF_AVAIL(state->write), 1, "iissii", &type, &st, &fileName, &funcName, &lineNumber, &pc);

                AJ_InfoPrintf(("NOTIFICATION: state = %u lineNum = %u, pc = %u\n", state, lineNumber, pc));
                AJ_InfoPrintf(("NOTIFICATION: File Name = %s, Function Name = %s\n", fileName, funcName));

                status = AJ_MarshalSignal(bus, &msg, DBG_NOTIF_MSGID,  AJS_GetConsoleBusName(), AJS_GetConsoleSession(), 0, 0);

                if (status == AJ_OK) {
                    status = AJ_MarshalArgs(&msg, "yyssyy", STATUS_NOTIFICATION, st, fileName, funcName, lineNumber, pc);
                }
                if (status == AJ_OK) {
                    AJ_DeliverMsg(&msg);
                }
                AJ_Free(fileName);
                AJ_Free(funcName);
                break;
            }

        //TODO: Handle print, alert and log notifications
        case PRINT_NOTIFICATION:
        case ALERT_NOTIFICATION:
        case LOG_NOTIFICATION:
            {
                AJ_InfoPrintf(("PRINT/ALERT/LOG NOTIFICATION\n"));
                break;
            }
        }
    } else if (*pos == DBG_TYPE_ERR) {
        /*
         * Some kind of error, for now just send AJ_ERR_INVALID
         * TODO: Find the cause of the error and give a more meaningful reply
         */
        AJ_MarshalStatusMsg(&state->lastMsg, &reply, AJ_ERR_INVALID);
        AJ_DeliverMsg(&reply);

    } else if (*pos == DBG_TYPE_REP) {
        switch (state->lastMsgType) {

        case EVAL_REQ:
        case GET_VAR_REQ:
            {
                /*
                 * Get variable/Eval request format:
                 * <REP><valid?><type><value><EOM>
                 */
                uint8_t* i = pos;
                uint8_t valid, valType;

                unmarhsalBuffer(state->write->bufStart, AJ_IO_BUF_AVAIL(state->write), 1, "i", &valid);
                /*
                 * The rest must be unmarshalled manually since it is unknown
                 * what the tval type is.
                 */
                valType = *(i + 2);
                status = AJ_MarshalReplyMsg(&state->lastMsg, &reply);
                if (status == AJ_OK) {
                    status = AJ_MarshalArgs(&reply, "yy", valid, valType);
                }
                /* GetVar valid is 1, eval valid is 0 */
                if (((valid) && (state->lastMsgType == GET_VAR_REQ)) || ((state->lastMsgType == EVAL_REQ) && (valid == 0))) {
                    marshalTvalMsg(&reply, valType, (i + 3));
                } else {
                    if (status == AJ_OK) {
                        status = AJ_MarshalVariant(&reply, "y");
                    }
                    if (status == AJ_OK) {
                        status = AJ_MarshalArgs(&reply, "y", 0);
                    }
                }
                if (status == AJ_OK) {
                    status = AJ_DeliverMsg(&reply);
                }
                AJ_CloseMsg(&reply);
            }
            break;

        case BASIC_INFO_REQ:
            {
                /*
                 * Basic info request format:
                 * <REP><duk version><description len><description><targ len><targ info><endianness><EOM>
                 */
                uint16_t dukVersion;
                uint8_t endianness;
                char* describe, *targInfo;

                unmarhsalBuffer(state->write->bufStart, AJ_IO_BUF_AVAIL(state->write), 1, "tssi", &dukVersion, &describe, &targInfo, &endianness);

                AJ_InfoPrintf(("BASIC INFO: Version: %u, Describe: %s, Info: %s, Endian: %u\n", dukVersion, describe, targInfo, endianness));

                status = AJ_MarshalReplyMsg(&state->lastMsg, &reply);
                if (status == AJ_OK) {
                    status = AJ_MarshalArgs(&reply, "yssy", dukVersion, describe, targInfo, endianness);
                }
                if (status == AJ_OK) {
                    status = AJ_DeliverMsg(&reply);
                }
                if (status != AJ_OK) {
                    AJ_ErrPrintf(("Error: %s\n", AJ_StatusText(status)));
                }
                AJ_CloseMsg(&reply);
                AJ_Free(describe);
                AJ_Free(targInfo);
            }
            break;

        case GET_CALL_STACK_REQ:
            {
                /*
                 * Get call stack request format:
                 * <REP>[<file name len><file name><func name len><func name><line><PC>]*<EOM>
                 */
                AJ_Arg array;
                uint8_t* i = pos;
                uint16_t numBytes = 0; //Skip past <REP>

                AJ_MarshalReplyMsg(&state->lastMsg, &reply);
                status = AJ_MarshalContainer(&reply, &array, AJ_ARG_ARRAY);
                /*
                 * Loop over message, marshalling the call stack until EOM
                 */
                i++;
                while (*i != DBG_TYPE_EOM) {
                    uint8_t line, pc;
                    char* fname, *funcName;
                    AJ_Arg struct1;

                    status = AJ_MarshalContainer(&reply, &struct1, AJ_ARG_STRUCT);

                    numBytes = unmarhsalBuffer(i, AJ_IO_BUF_AVAIL(state->write), 0, "ssii", &fname, &funcName, &line, &pc);
                    i += numBytes;

                    AJ_InfoPrintf(("Callstack; File: %s, Function: %s, Line: %u, PC: %u\n", fname, funcName, line, pc));
                    status = AJ_MarshalArgs(&reply, "ssyy", fname, funcName, line, pc);
                    if (status == AJ_OK) {
                        status = AJ_MarshalCloseContainer(&reply, &struct1);
                    }
                    AJ_Free(fname);
                    AJ_Free(funcName);
                }
                status = AJ_MarshalCloseContainer(&reply, &array);
                if (status == AJ_OK) {
                    status = AJ_DeliverMsg(&reply);
                }
                AJ_CloseMsg(&reply);
            }
            break;

        case LIST_BREAK_REQ:
            {
                /*
                 * List breakpoints request format:
                 * <REP>[<file name len><file name><line>]*<EOM>
                 */
                AJ_Arg array;
                uint8_t* i = pos;
                uint16_t numBytes = 0; //Skip over REP

                AJ_MarshalReplyMsg(&state->lastMsg, &reply);
                status = AJ_MarshalContainer(&reply, &array, AJ_ARG_ARRAY);
                /*
                 * Loop over message, marshalling the call stack until EOM
                 */
                i++;
                while (*i != DBG_TYPE_EOM) {
                    uint8_t line;
                    char* fname;
                    AJ_Arg struct1;

                    status = AJ_MarshalContainer(&reply, &struct1, AJ_ARG_STRUCT);
                    if ((*i == DBG_TYPE_REP) && (*(i + 1) == DBG_TYPE_EOM)) {
                        /* No breakpoints */
                        status = AJ_MarshalArgs(&reply, "sy", "N/A", 0);
                        if (status == AJ_OK) {
                            status = AJ_MarshalCloseContainer(&reply, &struct1);
                        }
                        break;
                    }
                    numBytes = unmarhsalBuffer(i, AJ_IO_BUF_AVAIL(state->write), 0, "si", &fname, &line);
                    i += numBytes;

                    AJ_InfoPrintf(("Breakpoints; File: %s, Line: %u\n", fname, line));
                    status = AJ_MarshalArgs(&reply, "sy", fname, line);
                    if (status == AJ_OK) {
                        status = AJ_MarshalCloseContainer(&reply, &struct1);
                    }
                    AJ_Free(fname);
                }
                status = AJ_MarshalCloseContainer(&reply, &array);
                if (status == AJ_OK) {
                    status = AJ_DeliverMsg(&reply);
                }
                AJ_CloseMsg(&reply);
            }
            break;

        case GET_LOCALS_REQ:
            {
                /*
                 * Get locals request format:
                 * <REP>[<name length><name><value type><value data>]*<EOM>
                 */
                AJ_Message reply;
                AJ_Arg array;
                uint8_t* i = pos;
                /*
                 * Case of no local variables. No need to go on further, just marshal "N/A" and deliver the reply
                 */
                if ((*(i) == DBG_TYPE_REP) && (*(i + 1) == DBG_TYPE_EOM)) {
                    AJ_Arg struct1;
                    status = AJ_MarshalReplyMsg(&state->lastMsg, &reply);
                    status = AJ_MarshalContainer(&reply, &array, AJ_ARG_ARRAY);
                    status = AJ_MarshalContainer(&reply, &struct1, AJ_ARG_STRUCT);
                    if (status == AJ_OK) {
                        status = AJ_MarshalArgs(&reply, "ys", 0, "N/A");
                    }
                    if (status == AJ_OK) {
                        status = AJ_MarshalVariant(&reply, "s");
                    }
                    if (status == AJ_OK) {
                        status = AJ_MarshalArgs(&reply, "s", "N/A");
                    }
                    if (status == AJ_OK) {
                        status = AJ_MarshalCloseContainer(&reply, &struct1);
                    }
                    if (status == AJ_OK) {
                        status = AJ_MarshalCloseContainer(&reply, &array);
                    }
                    if (status == AJ_OK) {
                        status = AJ_DeliverMsg(&reply);
                    }
                    AJ_CloseMsg(&reply);
                    break;
                }

                AJ_MarshalReplyMsg(&state->lastMsg, &reply);
                status = AJ_MarshalContainer(&reply, &array, AJ_ARG_ARRAY);
                /*
                 * Loop over message, marshalling the call stack until EOM. This type
                 * of message cannot be used with the unmarshaller because it contains
                 * tval's with unknown types.
                 * TODO: Make unmarshaller able to handle this type of message
                 */
                while (state->msgLength) {
                    uint8_t advance = 0;
                    uint8_t valType;
                    char*name;
                    AJ_Arg struct1;

                    status = AJ_MarshalContainer(&reply, &struct1, AJ_ARG_STRUCT);
                    advance = unmarhsalBuffer(i + 1, AJ_IO_BUF_AVAIL(state->write), 0, "sy", &name, &valType);
                    status = AJ_MarshalArgs(&reply, "ys", valType, name);
                    advance += marshalTvalMsg(&reply, valType, (i + 1 + strlen(name) + 2));
                    i += advance;
                    state->msgLength -= advance;
                    if (status == AJ_OK) {
                        status = AJ_MarshalCloseContainer(&reply, &struct1);
                    }
                    AJ_Free(name);
                    if (state->msgLength <= 1) {
                        break;
                    }
                }
                status = AJ_MarshalCloseContainer(&reply, &array);

                if (status == AJ_OK) {
                    status = AJ_DeliverMsg(&reply);
                }
                AJ_CloseMsg(&reply);
            }
            break;

        /*
         * Simple request formats:
         * <REP><EOM>
         */
        case PUT_VAR_REQ:
        case PAUSE_REQ:
        case ADD_BREAK_REQ:
        case TRIGGER_STATUS_REQ:
        case RESUME_REQ:
        case STEP_INTO_REQ:
        case STEP_OUT_REQ:
        case STEP_OVER_REQ:
        case DEL_BREAK_REQ:
        case DETACH_REQ:
            status = AJ_MarshalReplyMsg(&state->lastMsg, &reply);
            if (status == AJ_OK) {
                status = AJ_MarshalArgs(&reply, "y", 1);
            }
            if (status == AJ_OK) {
                status = AJ_DeliverMsg(&reply);
            }
            AJ_CloseMsg(&reply);
            break;

        default:
            AJ_ErrPrintf(("Unknown debug command, command = 0x%02x\n", state->lastMsgType));
            break;
        }
    }
    memset(writeBuffer, 0, sizeof(writeBuffer));
    AJ_IOBufInit(state->write, writeBuffer, sizeof(writeBuffer), 0, NULL);
}
