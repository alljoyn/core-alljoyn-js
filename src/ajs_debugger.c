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

/* If AJS_CONSOLE_LOCKDOWN is compiled in this whole file is unused */
#if !defined(AJS_CONSOLE_LOCKDOWN)

#define AJ_MODULE DEBUGGER

#include "ajs_debugger.h"
#include "ajs_util.h"
#include "ajs_storage.h"

/**
 * Controls debug output for this module
 */
#ifndef NDEBUG
uint8_t dbgDEBUGGER = 0;
#endif

#define SCRIPT_DEBUG_STARTED        6  /* the debugger was started successfully */
#define SCRIPT_DEBUG_STOPPED        7  /* the debugger was not started successfully, or stopped */

#define DEBUG_BUFFER_SIZE 512

/*
 * Below describes the states that the debugger can be in.
 *
 * ATTACHED_PAUSED:     In this state the debugger can accept any debug commands.
 *
 * ATTACHED_RUNNING:    In this state the debugger is running within the JS execution engine.
 *                      It can be either executing bytecode or waiting for bytecode
 *                      to execute. In this state only a limited number of debug commands
 *                      are accepted:
 *                      Pause, listBreak, addBreak, delBreak and Eval are all accepted
 *
 * DETACHED             The debug client has not attached and duktape is not in debug
 *                      mode. The only debug command accepted in this state is attach.
 */
typedef enum {
    AJS_DEBUG_ATTACHED_PAUSED = 0,      /* Debugger is attached and execution is paused */
    AJS_DEBUG_ATTACHED_RUNNING = 1,     /* Debugger is attached and execution is running */
    AJS_DEBUG_DETACHED = 2              /* Debugger is detached */
} AJS_DebugStatus;

typedef struct {
    AJ_MsgHeader hdr;
    uint32_t msgId;
    uint32_t sessionId;
    char sender[16];
} SavedMsg;

typedef struct _AJS_DebuggerState {
    duk_context* ctx;
    AJ_IOBuffer* read;      /* IO buffer for AJS_DebuggerRead() */
    AJ_IOBuffer* write;     /* IO buffer for AJS_DebuggerWrite() */
    AJ_Message* currentMsg; /* Holds pointer to current unmarshaled AllJoyn message */
    SavedMsg lastMsg;       /* Holds information about the AllJoyn message for method replies */
    uint8_t lastType;       /* Holds the last debug element type (int, string, length etc) */
    uint8_t lastMsgType;    /* Holds the last debug message type (Step in, Out, locals, call stack etc.) */
    uint8_t initialNotify;  /* Flag to signal if the initial notification has happened */
    uint32_t nextChunk;     /* Number of bytes to advance (used if we reach the end of buffer before EOM */
    int8_t curTypePos;      /* Current position in a data type (if length parameter is a uint16 or uint32) */
    uint32_t internal;      /* Place holder for any information while parsing the message (usually a length) */
    uint32_t msgLength;     /* Current messages length */
    AJS_DebugStatus status; /* Current status of the debugger */
} AJS_DebuggerState;

/*
 * Duktape dvalues for unmashalling/marshalling
 */
#define AJ_DUK_TYPE_BYTE       'y' /* REP/REQ/ERR/NFY */
#define AJ_DUK_TYPE_UINT32     '0' /* 0x10 <u32> */
#define AJ_DUK_TYPE_STRING4    '1' /* 0x11 <u32><data> */
#define AJ_DUK_TYPE_STRING2    '2' /* 0x12 <u16><data> */
#define AJ_DUK_TYPE_BUFFER4    '3' /* 0x13 <u32><data> */
#define AJ_DUK_TYPE_BUFFER2    '4' /* 0x14 <u16><data> */
#define AJ_DUK_TYPE_UNUSED     '5' /* 0x15 */
#define AJ_DUK_TYPE_UNDEF      '6' /* 0x16 */
#define AJ_DUK_TYPE_NULL       '7' /* 0x17 */
#define AJ_DUK_TYPE_TRUE       '8' /* 0x18 */
#define AJ_DUK_TYPE_FALSE      '9' /* 0x19 */
#define AJ_DUK_TYPE_NUMBER     'a' /* 0x1a <8 bytes> */
#define AJ_DUK_TYPE_OBJECT     'b' /* 0x1b <u8><u8><data> */
#define AJ_DUK_TYPE_POINTER    'c' /* 0x1c <u8><data> */
#define AJ_DUK_TYPE_LIGHTFUNC  'd' /* 0x1d <u16><u8><data> */
#define AJ_DUK_TYPE_HEAPPTR    'e' /* 0x1e <u8><data> */
#define AJ_DUK_TYPE_INTSM      'i' /* <0x80 - 0xbf> */
#define AJ_DUK_TYPE_INTLG      't' /* <0xc0 - 0xff><u8> */

/*
 * Any duktape integer type (1, 2 or 4 byte)
 */
#define AJ_DUK_TYPE_INTEGER    'i'
/*
 * Any duktape string type (1, 2, or 4 byte)
 */
#define AJ_DUK_TYPE_STRING     's'
/*
 * Any duktape buffer type (4 or 2 byte)
 */
#define AJ_DUK_TYPE_BUFFER     'B'

typedef enum {
    STATUS_NOTIFICATION     = 0x01,
    PRINT_NOTIFICATION      = 0x02,
    ALERT_NOTIFICATION      = 0x03,
    LOG_NOTIFICATION        = 0x04,
    BASIC_INFO_REQ          = 0x10,
    TRIGGER_STATUS_REQ      = 0x11,
    PAUSE_REQ               = 0x12,
    RESUME_REQ              = 0x13,
    STEP_INTO_REQ           = 0x14,
    STEP_OVER_REQ           = 0x15,
    STEP_OUT_REQ            = 0x16,
    LIST_BREAK_REQ          = 0x17,
    ADD_BREAK_REQ           = 0x18,
    DEL_BREAK_REQ           = 0x19,
    GET_VAR_REQ             = 0x1a,
    PUT_VAR_REQ             = 0x1b,
    GET_CALL_STACK_REQ      = 0x1c,
    GET_LOCALS_REQ          = 0x1d,
    EVAL_REQ                = 0x1e,
    DETACH_REQ              = 0x1f,
    DUMP_HEAP_REQ           = 0x20
} DEBUG_REQUESTS;

typedef enum {
    DBG_TYPE_EOM        = 0x00,
    DBG_TYPE_REQ        = 0x01,
    DBG_TYPE_REP        = 0x02,
    DBG_TYPE_ERR        = 0x03,
    DBG_TYPE_NFY        = 0x04,
    DBG_TYPE_INTEGER4   = 0x10,
    DBG_TYPE_STRING4    = 0x11,
    DBG_TYPE_STRING2    = 0x12,
    DBG_TYPE_BUFFER4    = 0x13,
    DBG_TYPE_BUFFER2    = 0x14,
    DBG_TYPE_UNUSED     = 0x15,
    DBG_TYPE_UNDEFINED  = 0x16,
    DBG_TYPE_NULL       = 0x17,
    DBG_TYPE_TRUE       = 0x18,
    DBG_TYPE_FALSE      = 0x19,
    DBG_TYPE_NUMBER     = 0x1a,
    DBG_TYPE_OBJECT     = 0x1b,
    DBG_TYPE_POINTER    = 0x1c,
    DBG_TYPE_LIGHTFUNC  = 0x1d,
    DBG_TYPE_HEAPPTR    = 0x1e,
    DBG_TYPE_STRLOW     = 0x60,
    DBG_TYPE_STRHIGH    = 0x7f,
    DBG_TYPE_INTSMLOW   = 0x80,
    DBG_TYPE_INTSMHIGH  = 0xbf,
    DBG_TYPE_INTLGLOW   = 0xc0,
    DBG_TYPE_INTLGHIGH  = 0xff
} DEBUG_TYPES;

static uint8_t readBuffer[DEBUG_BUFFER_SIZE];
static uint8_t writeBuffer[DEBUG_BUFFER_SIZE];

void AJS_DebuggerHandleMessage(AJS_DebuggerState* state);

#ifdef DBG_PRINT_CHUNKS
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
#endif

#define AJS_DUK_INT1BYTE_MAX    63
#define AJS_DUK_INT2BYTE_MAX    16383

/*
 * Encode any integer value (<= 4 bytes) to a duktape compliant tval.
 * @param in        Integer value to encode
 * @param out[out]  Output buffer with duktape tval, *out must be freed
 * @return          Byte length of *out buffer
 */
static uint8_t IntEncode(uint32_t in, uint8_t** out)
{
    /* 1 byte integer */
    if (in <= AJS_DUK_INT1BYTE_MAX) {
        *out = AJ_Malloc(1);
        **out = (uint8_t)in | DBG_TYPE_INTSMLOW;
        return 1;
        /* 2 byte integer */
    } else if (in <= AJS_DUK_INT2BYTE_MAX) {
        *out = AJ_Malloc(2);
        /*
         * To encode (from debugger.rst):
         * ((IB >> 8) | 0xc0) + followup_byte
         */
        (*out)[0] = ((in & 0xff00) >> 8) | DBG_TYPE_INTLGLOW;
        (*out)[1] = (uint8_t)(in & 0x00ff);
        return 2;
        /* 4 byte integer (5 including type byte) */
    } else {
        *out = AJ_Malloc(5);
        *out[0] = DBG_TYPE_INTEGER4;
        memcpy((*out) + 1, &in, 4);
        return 5;
    }
}

/*
 * Decode a duktape integer tval
 *
 * @param in        Input buffer containing duktape integer
 * @param out       Output integer as uint32
 *
 * @return          Number of bytes decoded, zero on error
 */
static uint8_t IntDecode(uint8_t* in, uint32_t* out)
{
    /* 1 byte integer */
    if (((uint8_t)*in >= DBG_TYPE_INTSMLOW) && ((uint8_t)*in <= DBG_TYPE_INTSMHIGH)) {
        *out = *in - DBG_TYPE_INTSMLOW;
        return 1;
        /* 2 byte integer */
    } else if (((uint8_t)*in >= DBG_TYPE_INTLGLOW) && ((uint8_t)*in <= DBG_TYPE_INTLGHIGH)) {
        /*
         * To decode(from debugger.rst):
         * ((IB - 0xc0) << 8) + followup_byte
         */
        *out = (((*in & 0x00ff) - DBG_TYPE_INTLGLOW) << 8) | (uint8_t)*(in + 1);
        return 2;
    } else if ((uint8_t)*in == DBG_TYPE_INTEGER4) {
        memcpy(out, in + 1, 4);
        return 5;
    } else {
        AJ_ErrPrintf(("IntDecode(): Error, invalid integer data\n"));
        return 0;
    }
}

static uint8_t StringDecode(uint8_t* in, char** out)
{
    uint8_t adv = 0;
    uint32_t sz = 0;
    /* 4 Byte string */
    if (*in == DBG_TYPE_STRING4) {
        adv = 5;
        ++in;
        memcpy(&sz, in, 4);
        in += 4;
        /* 2 Byte string */
    } else if (*in == DBG_TYPE_STRING2) {
        adv = 3;
        ++in;
        memcpy(&sz, in, 2);
        in += 2;
        /* 1 Byte string */
    } else if ((*in >= DBG_TYPE_STRLOW) && (*in <= DBG_TYPE_STRHIGH)) {
        adv = 1;
        sz = *in - DBG_TYPE_STRLOW;
        ++in;
    } else {
        AJ_ErrPrintf(("StringDecode(): Error, invalid identifier\n"));
        return 0;
    }
    *out = (char*)AJ_Malloc(sizeof(char) * sz + 1);
    memcpy(*out, in, sz);
    (*out)[sz] = '\0';
    return sz + adv;
}

static uint8_t BufferDecode(uint8_t* in, uint8_t** out)
{
    uint8_t adv = 0;
    uint32_t sz = 0;
    /* 4 Byte string */
    if (*in == DBG_TYPE_BUFFER4) {
        adv = 5;
        ++in;
        memcpy(&sz, in, 4);
        in += 4;
        /* 2 Byte buffer */
    } else if (*in == DBG_TYPE_BUFFER2) {
        adv = 3;
        ++in;
        memcpy(&sz, in, 2);
        in += 2;
    } else {
        AJ_ErrPrintf(("BufferDecode(): Error, invalid identifier\n"));
        return 0;
    }
    *out = (uint8_t*)AJ_Malloc(sizeof(char) * sz);
    memcpy(*out, in, sz);
    return sz + adv;
}

/*
 * Uses the cached last message information to compose a reply message
 */
static AJ_Status MarshalLastMsgStatus(AJS_DebuggerState* dbgState, AJ_Message* reply, AJ_Status status)
{
    AJ_Message lastMsg;

    AJ_ASSERT(dbgState->lastMsg.msgId != 0);

    memset(&lastMsg, 0, sizeof(AJ_Message));
    lastMsg.hdr = &dbgState->lastMsg.hdr;
    lastMsg.sender = dbgState->lastMsg.sender;
    lastMsg.bus = AJS_GetBusAttachment();
    lastMsg.sessionId = dbgState->lastMsg.sessionId;
    lastMsg.msgId = dbgState->lastMsg.msgId;

    if (status == AJ_OK) {
        status = AJ_MarshalReplyMsg(&lastMsg, reply);
    } else {
        status = AJ_MarshalStatusMsg(&lastMsg, reply, status);
    }
    memset(&dbgState->lastMsg, 0, sizeof(dbgState->lastMsg));
    return status;
}

#define MarshalLastMsgReply(dbgState, reply) MarshalLastMsgStatus(dbgState, reply, AJ_OK)

/*
 * Unmarshal a buffer (or debug message). The offset parameter signifies where
 * to start unmarshalling. This function will return the number of bytes that
 * was unmarshalled. This allows you to call this multiple times on the same
 * buffer but keep your place, similar to unmarshalling an AllJoyn message.
 * Note: Strings and buffers are allocated internally and it is required that they
 * are freed or this will leak memory.
 */
static uint16_t UnmarshalBuffer(uint8_t* buffer, uint32_t length, uint16_t offset, char* sig, ...)
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

        case (AJ_DUK_TYPE_INTEGER):
            {
                uint8_t adv = IntDecode(ptr, (uint32_t*)va_arg(args, uint32_t*));
                if (adv == 0) {
                    goto ErrorUnmarshal;
                }
                ptr += adv;
                numBytes += adv;
            }
            break;

        case (AJ_DUK_TYPE_STRING):
            {
                uint32_t adv = StringDecode(ptr, (char**)va_arg(args, char*));
                if (adv == 0) {
                    goto ErrorUnmarshal;
                }
                ptr += adv;
                numBytes += adv;
            }
            break;

        case (AJ_DUK_TYPE_BUFFER):
            {
                uint32_t adv = BufferDecode(ptr, (uint8_t**)va_arg(args, uint8_t*));
                if (adv == 0) {
                    goto ErrorUnmarshal;
                }
                ptr += adv;
                numBytes += adv;
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
        }
    }
    va_end(args);
    return numBytes;

ErrorUnmarshal:
    AJ_ErrPrintf(("Signature did not match bytes identifier in buffer, current byte: 0x%02x, numBytes: %u\n", *ptr, numBytes));
    return numBytes;
}

static AJS_DebuggerState* dbgState = NULL;

/**
 * Get the debuggers current status
 */
uint8_t AJS_DebuggerIsAttached(void)
{
    if (dbgState && (dbgState->status == AJS_DEBUG_ATTACHED_PAUSED || dbgState->status == AJS_DEBUG_ATTACHED_RUNNING)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

static AJS_DebuggerState* AllocDebuggerState(duk_context* ctx)
{
    AJ_InfoPrintf(("AllocDebuggerState()"));

    AJ_ASSERT(!dbgState);
    dbgState = AJ_Malloc(sizeof(AJS_DebuggerState));

    memset(dbgState, 0, sizeof(*dbgState));
    dbgState->read = AJ_Malloc(sizeof(AJ_IOBuffer));
    dbgState->write = AJ_Malloc(sizeof(AJ_IOBuffer));
    /* Init write buffer */
    memset(writeBuffer, 0, sizeof(writeBuffer));
    AJ_IOBufInit(dbgState->write, writeBuffer, sizeof(writeBuffer), 0, NULL);
    /* Init read buffer */
    memset(readBuffer, 0, sizeof(readBuffer));
    AJ_IOBufInit(dbgState->read, readBuffer, sizeof(readBuffer), 0, NULL);

    dbgState->ctx = ctx;
    dbgState->initialNotify = FALSE;
    dbgState->curTypePos = -1;
    /* State will change to paused when bytecode executes */
    dbgState->status = AJS_DEBUG_ATTACHED_RUNNING;

    return dbgState;
}

static AJ_Status DebugAddBreak(AJS_DebuggerState* state, AJ_Message* msg, const char* file, uint16_t line)
{
    uint8_t* dbgMsg;
    size_t flen = strlen(file);
    uint8_t msgLen = flen + 4;
    uint8_t* tmp;
    uint8_t szline;

    /* Determine the complete message length */
    if (line <= AJS_DUK_INT1BYTE_MAX) {
        msgLen += 1;
    } else if (line <= AJS_DUK_INT2BYTE_MAX) {
        msgLen += 2;
    } else {
        AJ_ErrPrintf(("DebugAddBreak(): Invalid line number: %u\n", line));
        return AJ_ERR_RESOURCES;
    }

    AJ_InfoPrintf(("DebugAddBreak(): file=%s, line=%i\n", file, line));
    szline = IntEncode(line, &tmp);
    if (szline >= 5) {
        AJ_ErrPrintf(("DebugAddBreak(): Max line number is 16838\n"));
        AJ_Free(tmp);
        return AJ_ERR_RESOURCES;
    }
    dbgMsg = AJ_Malloc(msgLen);
    dbgMsg[0] = DBG_TYPE_REQ;
    dbgMsg[1] = ADD_BREAK_REQ | DBG_TYPE_INTSMLOW;
    dbgMsg[2] = flen | DBG_TYPE_STRLOW;
    memcpy(dbgMsg + 3, file, flen);
    memcpy(dbgMsg + 3 + flen, tmp, szline);
    dbgMsg[3 + szline + flen] = DBG_TYPE_EOM;

    if (AJ_IO_BUF_SPACE(state->read) >= msgLen) {
        memcpy(state->read->writePtr, dbgMsg, msgLen);
        state->read->writePtr += msgLen;
    } else {
        AJ_ErrPrintf(("No space to write debug message\n"));
        AJ_Free(dbgMsg);
        AJ_Free(tmp);
        return AJ_ERR_RESOURCES;
    }
    AJ_Free(dbgMsg);
    AJ_Free(tmp);
    state->lastMsgType = ADD_BREAK_REQ;
    return AJ_OK;
}

static AJ_Status DebugDelBreak(AJS_DebuggerState* state, AJ_Message* msg, uint8_t index)
{
    uint8_t dbgMsg[4];

    dbgMsg[0] = DBG_TYPE_REQ;
    dbgMsg[1] = DEL_BREAK_REQ | DBG_TYPE_INTSMLOW;
    dbgMsg[2] = index | DBG_TYPE_INTSMLOW;
    dbgMsg[3] = DBG_TYPE_EOM;

    AJ_InfoPrintf(("DebugDelBreak(): index=%i\n", index));
    if (AJ_IO_BUF_SPACE(state->read) >= sizeof(dbgMsg)) {
        memcpy(state->read->writePtr, dbgMsg, sizeof(dbgMsg));
        state->read->writePtr += sizeof(dbgMsg);
    } else {
        AJ_ErrPrintf(("No space to write debug message\n"));
        return AJ_ERR_RESOURCES;
    }
    state->lastMsgType = DEL_BREAK_REQ;
    return AJ_OK;
}

static AJ_Status DebugGetVar(AJS_DebuggerState* state, AJ_Message* msg, char* var)
{
    uint8_t* dbgMsg;
    uint8_t varLen = strlen(var);
    uint8_t msgLen = varLen + 4;

    AJ_InfoPrintf(("DebugGetVar(): var=%s\n", var));

    dbgMsg = AJ_Malloc(msgLen);
    dbgMsg[0] = DBG_TYPE_REQ;
    dbgMsg[1] = GET_VAR_REQ | DBG_TYPE_INTSMLOW;
    dbgMsg[2] = varLen | DBG_TYPE_STRLOW;
    memcpy(dbgMsg + 3, var, varLen);
    dbgMsg[3 + varLen] = DBG_TYPE_EOM;

    if (AJ_IO_BUF_SPACE(state->read) >= msgLen) {
        memcpy(state->read->writePtr, dbgMsg, msgLen);
        state->read->writePtr += msgLen;
    } else {
        AJ_ErrPrintf(("No space to write debug message\n"));
        AJ_Free(dbgMsg);
        return AJ_ERR_RESOURCES;
    }
    AJ_Free(dbgMsg);
    state->lastMsgType = GET_VAR_REQ;
    return AJ_OK;
}

static AJ_Status DebugPutVar(AJS_DebuggerState* state, AJ_Message* msg, char* name, uint8_t type, uint8_t* varData, uint32_t varLen)
{
    uint8_t nameLen;
    uint32_t runningLength = 0;

    AJ_InfoPrintf(("DebugPutVar(): name=%s, type=0x%x, varData=0x%p, varLen=%u\n", name, type, (void*)varData, varLen));

    nameLen = strlen(name);
    /*
     * Formulate the debug message header
     * <REQ><0x1b><NAME LEN><NAME STR><VAL TYPE><VALUE BYTES><EOM>
     */
    *state->read->writePtr++ = DBG_TYPE_REQ;
    runningLength++;
    *state->read->writePtr++ = PUT_VAR_REQ | DBG_TYPE_INTSMLOW;
    runningLength++;
    *state->read->writePtr++ = nameLen | DBG_TYPE_STRLOW;
    runningLength++;
    memcpy(state->read->writePtr, name, nameLen);
    runningLength += nameLen;
    state->read->writePtr += nameLen;
    *state->read->writePtr++ = type;
    runningLength++;

    /* Number requires a byte swap */
    if (type == DBG_TYPE_NUMBER) {
        uint64_t tmp = 0;
        memcpy(&tmp, varData, 8);
        tmp = AJ_ByteSwap64(tmp);
        memcpy(state->read->writePtr, &tmp, 8);
        state->read->writePtr += 8;
    } else {
        memcpy(state->read->writePtr, varData, varLen);
        state->read->writePtr += varLen;
    }
    runningLength += varLen;

    if (AJ_IO_BUF_SPACE(state->read) < 1) {
        AJ_ErrPrintf(("No space to write debug message\n"));
        return AJ_ERR_RESOURCES;
    }
    *state->read->writePtr++ = DBG_TYPE_EOM;
    state->lastMsgType = PUT_VAR_REQ;
    return AJ_OK;
}

static AJ_Status DebugEval(AJS_DebuggerState* state, AJ_Message* msg, char* eval)
{
    uint8_t* dbgMsg;
    uint8_t varLen = strlen(eval);
    uint8_t msgLen = varLen + 4;

    AJ_InfoPrintf(("DebugEval(): eval=%s\n", eval));

    dbgMsg = AJ_Malloc(msgLen);
    dbgMsg[0] = DBG_TYPE_REQ;
    dbgMsg[1] = EVAL_REQ | DBG_TYPE_INTSMLOW;
    dbgMsg[2] = varLen | DBG_TYPE_STRLOW;
    memcpy(dbgMsg + 3, eval, varLen);
    dbgMsg[3 + varLen] = DBG_TYPE_EOM;

    if (AJ_IO_BUF_SPACE(state->read) >= msgLen) {
        memcpy(state->read->writePtr, dbgMsg, msgLen);
        state->read->writePtr += msgLen;
    } else {
        AJ_ErrPrintf(("No space to write debug message\n"));
        AJ_Free(dbgMsg);
        return AJ_ERR_RESOURCES;
    }
    AJ_Free(dbgMsg);
    state->lastMsgType = EVAL_REQ;
    return AJ_OK;
}

static AJ_Status DebugSimpleCommand(AJS_DebuggerState* state, AJ_Message* msg, DEBUG_REQUESTS command)
{
    uint8_t dbgMsg[3];

    dbgMsg[0] = DBG_TYPE_REQ;
    dbgMsg[1] = command | DBG_TYPE_INTSMLOW;
    dbgMsg[2] = DBG_TYPE_EOM;

    AJ_InfoPrintf(("DebugSimpleCommand(): command=%i msg=%s\n", command, msg->member));

    if (AJ_IO_BUF_SPACE(state->read) >= sizeof(dbgMsg)) {
        memcpy(state->read->writePtr, &dbgMsg, sizeof(dbgMsg));
        state->read->writePtr += sizeof(dbgMsg);
    } else {
        AJ_ErrPrintf(("No space to write debug message\n"));
        return AJ_ERR_RESOURCES;
    }
    state->lastMsgType = command;

    return AJ_OK;
}

/*
 * Marshal the tval (variant) portion of a message. The type provided is
 * what decides how the buffer provided will be parsed. It is assumed
 * that the message already has been marshalled enough to where the next argument
 * is the tval. Returned is the size of the data that was marshalled.
 */
static uint32_t MarshalTvalMsg(AJ_Message* msg, uint8_t valType, uint8_t* buffer)
{
    AJ_Status status = AJ_OK;
    uint32_t size = 0;
    if (valType == DBG_TYPE_NUMBER) {
        uint64_t value;
        memcpy(&value, buffer, sizeof(uint64_t));
#if HOST_IS_BIG_ENDIAN
        value = AJ_ByteSwap64(value);
#endif
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
        valLen = (valType - DBG_TYPE_STRLOW);
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
        AJ_Free(str);
    } else {
        AJ_ErrPrintf(("AJS_DebuggerHandleMessage(): Unknown data type: 0x%x\n", valType));
        return 0;
    }
    if (status != AJ_OK) {
        AJ_ErrPrintf(("Error marshalling tval, status = %s\n", AJ_StatusText(status)));
    }
    return size;
}

static int DebugCommandAllowed(AJS_DebugStatus dbgStatus, uint32_t msgId)
{
    /* All commands allowed when the debugger is paused */
    if (dbgStatus == AJS_DEBUG_ATTACHED_PAUSED) {
        return TRUE;
    }

    switch (msgId) {
    case DBG_DELBREAK_MSGID:
    case DBG_LISTBREAK_MSGID:
    case DBG_ADDBREAK_MSGID:
    case DBG_PAUSE_MSGID:
    case DBG_GETVAR_MSGID:
    case DBG_PUTVAR_MSGID:
    case DBG_EVAL_MSGID:
    case DBG_DETACH_MSGID:
    case EVAL_MSGID:
        /* These can always be executed */
        return TRUE;

    case DBG_RESUME_MSGID:
        /* Can only resume if already paused */
        return FALSE;

    case DBG_BASIC_MSGID:
    case DBG_TRIGGER_MSGID:
    case DBG_STEPIN_MSGID:
    case DBG_STEPOVER_MSGID:
    case DBG_STEPOUT_MSGID:
    case DBG_GETCALL_MSGID:
    case DBG_GETLOCALS_MSGID:
        /* These can only be executed when the debugger is running */
        return (dbgStatus == AJS_DEBUG_ATTACHED_RUNNING);

    default:
        /* For completeness */
        return FALSE;
    }
}

/*
 * Unmarshals a debugger message.
 */
AJ_Status DebugMsgUnmarshal(AJS_DebuggerState* dbgState, AJ_Message* msg)
{
    AJ_Message reply;
    AJ_Status status = AJ_OK;

    /*
     * Reject the request if the command is not allowed in this state
     */
    if (!DebugCommandAllowed(dbgState->status, msg->msgId)) {
        AJ_ErrPrintf(("DebugMsgUnmarshal command %s not allowed in state %d\n", msg->member, dbgState->status));
        status = AJ_MarshalStatusMsg(msg, &reply, AJ_ERR_BUSY);
        AJ_CloseMsg(msg);
        if (status == AJ_OK) {
            status = AJ_DeliverMsg(&reply);
        }
        return status;
    }

    switch (msg->msgId) {
    case DBG_ADDBREAK_MSGID:
        {
            char* file;
            uint16_t line;
            status = AJ_UnmarshalArgs(msg, "sq", &file, &line);
            if (status == AJ_OK) {
                status = DebugAddBreak(dbgState, msg, file, line);
            }
        }
        break;

    case DBG_DELBREAK_MSGID:
        {
            uint8_t index;
            status = AJ_UnmarshalArgs(msg, "y", &index);
            if (status == AJ_OK) {
                status = DebugDelBreak(dbgState, msg, index);
            }
        }
        break;

    case DBG_GETVAR_MSGID:
        {
            char* var;
            status = AJ_UnmarshalArgs(msg, "s", &var);
            if (status == AJ_OK) {
                status = DebugGetVar(dbgState, msg, var);
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
            uint8_t* data = NULL;
            uint32_t dataLen = 0;

            status = AJ_UnmarshalArgs(msg, "sy", &name, &type);
            if (status == AJ_OK) {
                status = AJ_UnmarshalRaw(msg, &raw, sizeof(size), &sz);
            }
            if (status != AJ_OK) {
                break;
            }
            memcpy(&size, raw, sizeof(size));
            nameLen = strlen(name);
            /*
             * Check for room:
             * <REQ><PUTVAR><NAMELEN><NAME><TYPE>
             *   1  +  1   +   1   +   N  +  1
             */
            if (AJ_IO_BUF_SPACE(dbgState->read) < (nameLen + 4)) {
                AJ_ErrPrintf(("No space to write debug message\n"));
                status = AJ_ERR_RESOURCES;
                break;
            }
            /*
             * Get the variables new data
             */
            while (size) {
                status = AJ_UnmarshalRaw(msg, &raw, size, &sz);
                if (status != AJ_OK) {
                    AJ_ErrPrintf(("DebuggerRead(): Error unmarshalling\n"));
                    break;
                }
                runningLength += sz;
                size -= sz;
                data = AJ_Realloc(data, dataLen + sz);
                memcpy(data + dataLen, raw, sz);
                dataLen += sz;
            }
            if (status == AJ_OK && dataLen) {
                status = DebugPutVar(dbgState, msg, name, type, data, dataLen);
            }
            AJ_Free(data);
        }
        break;

    case DBG_EVAL_MSGID:
        {
            char* eval;
            status = AJ_UnmarshalArgs(msg, "s", &eval);
            if (status == AJ_OK) {
                status = DebugEval(dbgState, msg, eval);
            }
        }
        break;

    case DBG_BASIC_MSGID:
        status = DebugSimpleCommand(dbgState, msg, BASIC_INFO_REQ);
        break;

    case DBG_TRIGGER_MSGID:
        status = DebugSimpleCommand(dbgState, msg, TRIGGER_STATUS_REQ);
        break;

    case DBG_PAUSE_MSGID:
        status = DebugSimpleCommand(dbgState, msg, PAUSE_REQ);
        break;

    case DBG_RESUME_MSGID:
        status = DebugSimpleCommand(dbgState, msg, RESUME_REQ);
        break;

    case DBG_STEPIN_MSGID:
        status = DebugSimpleCommand(dbgState, msg, STEP_INTO_REQ);
        break;

    case DBG_STEPOVER_MSGID:
        status = DebugSimpleCommand(dbgState, msg, STEP_OVER_REQ);
        break;

    case DBG_STEPOUT_MSGID:
        status = DebugSimpleCommand(dbgState, msg, STEP_OUT_REQ);
        break;

    case DBG_LISTBREAK_MSGID:
        status = DebugSimpleCommand(dbgState, msg, LIST_BREAK_REQ);
        break;

    case DBG_GETCALL_MSGID:
        status = DebugSimpleCommand(dbgState, msg, GET_CALL_STACK_REQ);
        break;

    case DBG_GETLOCALS_MSGID:
        status = DebugSimpleCommand(dbgState, msg, GET_LOCALS_REQ);
        break;

    case DBG_DETACH_MSGID:
        status = DebugSimpleCommand(dbgState, msg, DETACH_REQ);
        break;

    case EVAL_MSGID:
        AJS_Eval(dbgState->ctx, msg);
        /*
         * The status must be set to not AJ_OK or AJ_ERR_NO_MATCH because:
         *
         * 1. We have no debug command to push to the buffer (eval is done differently, with duk_pcall)
         *    so the AJ_OK path will not work.
         * 2. AJS_Eval closes the message so delivering a status message will fail
         *
         * This means we need to bypass both 1 and 2 but still must return AJ_OK.
         */
        status = AJ_ERR_BUSY;
        break;

    default:
        /*
         * Not a debugger message
         */
        return AJ_ERR_NO_MATCH;
    }
    if (status == AJ_OK) {
        /*
         * Save enough of the message to generate the reply later
         */
        size_t len = strlen(msg->sender) + 1;
        /*
         * Sender strings will be less than space we allocated but need to check anyway
         */
        len = min(len, sizeof(dbgState->lastMsg.sender));
        memcpy(dbgState->lastMsg.sender, msg->sender, len);
        dbgState->lastMsg.sender[len - 1] = 0;
        memcpy(&dbgState->lastMsg.hdr, msg->hdr, sizeof(AJ_MsgHeader));
        dbgState->lastMsg.sessionId = msg->sessionId;
        dbgState->lastMsg.msgId = msg->msgId;
    } else if (status != AJ_ERR_BUSY) {
        AJ_MarshalStatusMsg(msg, &reply, status);
        status = AJ_DeliverMsg(&reply);
    } else {
        /*
         * Special case for standard Eval. Since it does not use the duktape debug
         * buffer we need to bypass the if statements above but still return OK.
         */
        status = AJ_OK;
    }
    return status;
}

static duk_size_t DebuggerRead(void* udata, char* buffer, duk_size_t length)
{
    AJS_DebuggerState* state = (AJS_DebuggerState*)udata;
    AJ_Status status = AJ_OK;
    uint32_t avail = AJ_IO_BUF_AVAIL(state->read);

    AJ_InfoPrintf(("DebuggerRead() buffer=%p, length=%u, avail=%u\n", (void*)buffer, length, avail));

    /*
     * Peek operation if request length == 0
     */
    if (length == 0) {
        return ((avail > 0) || state->currentMsg) ? 1 : 0;
    }
    /*
     * Maximize the space in the read buffer
     */
    AJ_IOBufRebase(state->read, 0);

    /*
     * Unmarshal messages until we have data to return
     */
    while ((status == AJ_OK) && (avail == 0)) {
        AJ_Message msg;
        if (!state->currentMsg) {
            status = AJ_UnmarshalMsg(AJS_GetBusAttachment(), &msg, AJ_TIMER_FOREVER);
            if (status != AJ_OK) {
                if ((status == AJ_ERR_INTERRUPTED) || (status == AJ_ERR_TIMEOUT)) {
                    status = AJ_OK;
                } else {
                    AJ_CloseMsg(&msg);
                    if (status != AJ_ERR_READ) {
                        /*
                         * Silently ignore unknown or bad messages or security failures
                         */
                        status = AJ_OK;
                    }
                }
                continue;
            }
            state->currentMsg = &msg;
        }
        switch (state->currentMsg->msgId) {
        case AJ_SIGNAL_SESSION_LOST_WITH_REASON:
            AJ_ErrPrintf(("DebuggerRead(): Session lost\n"));
            /*
             * Maybe the debugger session was lost
             */
            status = AJS_ConsoleMsgHandler(state->ctx, state->currentMsg);
            if (status != AJ_ERR_NO_MATCH) {
                /*
                 * This will have stopped the debugger. We must close the message
                 * because if a debug client attaches again we need to have a clean
                 * unmarshal state.
                 */
                AJ_CloseMsg(state->currentMsg);
                return 0;
            }
            break;

        case AJ_SIGNAL_PROBE_ACK:
        case AJ_SIGNAL_PROBE_REQ:
        case AJ_METHOD_PING:
        case AJ_METHOD_BUS_PING:
            /*
             * These AllJoyn messages must be handled here or else the router will time out.
             */
            status = AJ_BusHandleBusMessage(state->currentMsg);
            break;

        case DBG_END_MSGID:
            (void)AJS_StopDebugger(state->ctx, state->currentMsg);
            /*
             * Debugger state has been freed - need to return
             */
            return 0;

        case DBG_GETSTATUS_MSGID:
            status = AJS_DebuggerGetStatus(state->ctx, state->currentMsg);
            break;

        case DBG_GETSCRIPTNAME_MSGID:
            status = AJS_DebuggerGetScriptName(state->ctx, state->currentMsg);
            break;

        case DBG_GETSCRIPT_MSGID:
            status = AJS_DebuggerGetScript(state->ctx, state->currentMsg);
            break;

        case LOCK_CONSOLE_MSGID:
            status = AJS_LockConsole(state->currentMsg);
            break;

        default:
            status = DebugMsgUnmarshal(state, state->currentMsg);
            break;
        }
        AJ_CloseMsg(state->currentMsg);
        state->currentMsg = NULL;
        avail = AJ_IO_BUF_AVAIL(state->read);
        if (status == AJ_ERR_NO_MATCH) {
            status = AJ_OK;
        }
    }

    if (status == AJ_OK) {
        if (buffer) {
            length = min(length, avail);
            AJ_InfoPrintf(("DebuggerRead(): Pushing %u bytes into duktape debug buffer\n", length));
            memcpy(buffer, state->read->readPtr, length);
            state->read->readPtr += length;
            return length;
        } else {
            /*
             * No buffer means this function was called from peek
             */
            return avail;
        }
    } else {
        /*
         * Returning zero will cause the debugger to detach
         */
        AJ_InfoPrintf(("DebuggerRead(): status=%s\n", AJ_StatusText(status)));
        return 0;
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

static duk_size_t DebuggerWrite(void* udata, const char* buffer, duk_size_t length)
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

        AJ_InfoPrintf(("DebuggerWrite(): Sending initial notify message, \"%s\"\n", versionString));

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

static duk_size_t DebuggerPeek(void* udata)
{
    AJS_DebuggerState* state = (AJS_DebuggerState*)udata;
    if (state->currentMsg) {
        return DebuggerRead(udata, NULL, DEBUG_BUFFER_SIZE);
    } else {
        return AJ_IO_BUF_AVAIL(state->read);
    }
}

static void DebuggerReadFlush(void* udata)
{
    return;
}

static void DebuggerWriteFlush(void* udata)
{
    return;
}

static void DebuggerDetached(void* udata)
{
    AJ_InfoPrintf(("DebuggerDetached(): Debugger was detached\n"));
    if (dbgState) {
        AJ_InfoPrintf(("Free dbgState=%p\n", dbgState));
        if (dbgState->write) {
            AJ_Free(dbgState->write);
        }
        if (dbgState->read) {
            AJ_Free(dbgState->read);
        }
        AJ_Free(dbgState);
        dbgState = NULL;
    }
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

    AJ_InfoPrintf(("AJS_DebuggerHandleMessage(): Buffer position=0x%p\n", pos));

    if (*pos == DBG_TYPE_NFY) {
        switch (*(pos + 1) - DBG_TYPE_INTSMLOW) {
        case STATUS_NOTIFICATION:
            {
                /*
                 * Status notification format:
                 * <NFY><state><file name len><file name data><func name length><func name data><line><PC><EOM>
                 */
                AJ_BusAttachment* bus = AJS_GetBusAttachment();
                AJ_Message msg;
                AJ_Status status;
                uint8_t* i = state->write->readPtr;
                uint32_t lineNumber, pc;
                uint32_t type, st;
                char* fileName = NULL;
                char* funcName = NULL;
                uint8_t dummy;

                /*
                 * Unmarshal the type and state first. fileName/funcName may be null in case of a cooperate call
                 * so those need to be checked before unmarshalling further
                 */
                i += UnmarshalBuffer(i, AJ_IO_BUF_AVAIL(state->write), 1, "ii", &type, &st);

                status = AJ_MarshalSignal(bus, &msg, DBG_NOTIF_MSGID,  AJS_GetConsoleBusName(), AJS_GetConsoleSession(), 0, 0);

                /* fileName and funcName are null so just send back N/A and the line/pc number */
                if (*i == DBG_TYPE_UNDEFINED) {
                    UnmarshalBuffer(i, AJ_IO_BUF_AVAIL(state->write), 0, "yyii", &dummy, &dummy, &lineNumber, &pc);
                    AJ_InfoPrintf(("AJS_DebuggerHandleMessage(): Debug notification, no bytecode executing, state = %u lineNum = %u, pc = %u, file=%s, function=%s\n", st, lineNumber, pc, fileName, funcName));
                    if (status == AJ_OK) {
                        status = AJ_MarshalArgs(&msg, "yyssqy", STATUS_NOTIFICATION, st, "N/A", "N/A", lineNumber, pc);
                    }
                    /* Notification with undefined as file/function means no bytecode is executing == busy */
                    state->status = AJS_DEBUG_ATTACHED_RUNNING;
                } else {
                    /* Regular notification */
                    i += UnmarshalBuffer(i, AJ_IO_BUF_AVAIL(state->write), 0, "ssii", &fileName, &funcName, &lineNumber, &pc);

                    AJ_InfoPrintf(("AJS_DebuggerHandleMessage(): Debug notification: state = %u lineNum = %u, pc = %u, file=%s, function=%s\n", st, lineNumber, pc, fileName, funcName));

                    if (status == AJ_OK) {
                        status = AJ_MarshalArgs(&msg, "yyssqy", STATUS_NOTIFICATION, st, fileName, funcName, lineNumber, pc);
                    }
                    AJ_Free(fileName);
                    AJ_Free(funcName);
                    if (st == 0) {
                        /* one = running */
                        state->status = AJS_DEBUG_ATTACHED_RUNNING;
                    } else {
                        /* zero = paused */
                        state->status = AJS_DEBUG_ATTACHED_PAUSED;

                    }
                }

                if (status == AJ_OK) {
                    AJ_DeliverMsg(&msg);
                }
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
        MarshalLastMsgStatus(state, &reply, AJ_ERR_INVALID);
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

                memcpy(&valid, state->write->readPtr + 1, 1);
                AJ_InfoPrintf(("AJS_DebuggerHandleMessage(): Eval/GetVar reply, valid=%i\n", valid));
                /*
                 * The rest must be unmarshalled manually since it is unknown
                 * what the tval type is.
                 */
                valType = *(i + 2);
                status = MarshalLastMsgReply(state, &reply);
                valid -= DBG_TYPE_INTSMLOW;
                if (status == AJ_OK) {
                    status = AJ_MarshalArgs(&reply, "yy", valid, valType);
                }
                /* GetVar valid is 1, eval valid is 0 */
                if (((valid) && (state->lastMsgType == GET_VAR_REQ)) || ((state->lastMsgType == EVAL_REQ) && (valid == 0))) {
                    MarshalTvalMsg(&reply, valType, (i + 3));
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
                uint32_t dukVersion;
                uint32_t endianness;
                char* describe, *targInfo;

                UnmarshalBuffer(state->write->bufStart, AJ_IO_BUF_AVAIL(state->write), 1, "issi", &dukVersion, &describe, &targInfo, &endianness);

                AJ_InfoPrintf(("AJS_DebuggerHandleMessage(): Basic Info reply, Version: %u, Describe: %s, Info: %s, Endian: %u\n", dukVersion, describe, targInfo, endianness));

                status = MarshalLastMsgReply(state, &reply);
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

                MarshalLastMsgReply(state, &reply);
                status = AJ_MarshalContainer(&reply, &array, AJ_ARG_ARRAY);
                /*
                 * Loop over message, marshalling the call stack until EOM
                 */
                i++;
                while (*i != DBG_TYPE_EOM) {
                    uint32_t line;
                    uint32_t pc;
                    char* fname, *funcName;
                    AJ_Arg struct1;

                    status = AJ_MarshalContainer(&reply, &struct1, AJ_ARG_STRUCT);

                    i += UnmarshalBuffer(i, AJ_IO_BUF_AVAIL(state->write), 0, "ssii", &fname, &funcName, &line, &pc);

                    AJ_InfoPrintf(("AJS_DebuggerHandleMessage(): Get Callstack, File: %s, Function: %s, Line: %u, PC: %u\n", fname, funcName, line, pc));
                    status = AJ_MarshalArgs(&reply, "ssqy", fname, funcName, (uint16_t)line, pc);
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

                MarshalLastMsgReply(state, &reply);
                status = AJ_MarshalContainer(&reply, &array, AJ_ARG_ARRAY);
                /*
                 * Loop over message, marshalling the call stack until EOM
                 */
                i++;
                AJ_InfoPrintf(("AJS_DebuggerHandleMessage(): Get breakpoints\n"));
                while (*i != DBG_TYPE_EOM) {
                    uint32_t line;
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
                    i += UnmarshalBuffer(i, AJ_IO_BUF_AVAIL(state->write), 0, "si", &fname, &line);

                    AJ_InfoPrintf(("AJS_DebuggerHandleMessage Breakpoint: File: %s, Line: %u\n", fname, line));
                    status = AJ_MarshalArgs(&reply, "sq", fname, (uint16_t)line);
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
                AJ_InfoPrintf(("AJS_DebuggerHandleMessage(): Get locals reply\n"));
                if ((*(i) == DBG_TYPE_REP) && (*(i + 1) == DBG_TYPE_EOM)) {
                    AJ_Arg struct1;
                    status = MarshalLastMsgReply(state, &reply);
                    if (status != AJ_OK) {
                        break;
                    }
                    status = AJ_MarshalContainer(&reply, &array, AJ_ARG_ARRAY);
                    if (status != AJ_OK) {
                        break;
                    }
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

                MarshalLastMsgReply(state, &reply);
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
                    advance = UnmarshalBuffer(i + 1, AJ_IO_BUF_AVAIL(state->write), 0, "sy", &name, &valType);
                    status = AJ_MarshalArgs(&reply, "ys", valType, name);
                    advance += MarshalTvalMsg(&reply, valType, (i + 1 + strlen(name) + 2));
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
        case RESUME_REQ:
        case PUT_VAR_REQ:
        case PAUSE_REQ:
        case ADD_BREAK_REQ:
        case TRIGGER_STATUS_REQ:
        case STEP_INTO_REQ:
        case STEP_OUT_REQ:
        case STEP_OVER_REQ:
        case DEL_BREAK_REQ:
        case DETACH_REQ:

            AJ_InfoPrintf(("AJS_DebuggerHandleMessage(): Simple command reply, command=%u\n", state->lastMsgType));
            /*
             * Consume the buffer
             */
            status = MarshalLastMsgReply(state, &reply);
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

AJ_Status AJS_StartDebugger(duk_context* ctx, AJ_Message* msg)
{
    AJ_Status status = AJ_OK;
    AJ_Message reply;
    uint8_t quiet;
    status = AJ_UnmarshalArgs(msg, "y", &quiet);
    if (quiet) {
        AJS_ConsoleSetQuiet(quiet);
    }
    AJ_InfoPrintf(("StartDebugger()\n"));
    if (!dbgState) {
        AJ_MarshalReplyMsg(msg, &reply);
        AJ_MarshalArgs(&reply, "y", SCRIPT_DEBUG_STARTED);
        status = AJ_DeliverMsg(&reply);
    } else {
        return AJ_ERR_INVALID;
    }
    if (status == AJ_OK) {
        AJS_DisableWatchdogTimer();
        dbgState = AllocDebuggerState(ctx);
        /* Start the debugger */
        duk_debugger_attach(ctx,
                            DebuggerRead,
                            DebuggerWrite,
                            DebuggerPeek,
                            DebuggerReadFlush,
                            DebuggerWriteFlush,
                            DebuggerDetached,
                            (void*)dbgState);
    }
    return status;
}

AJ_Status AJS_StopDebugger(duk_context* ctx, AJ_Message* msg)
{
    AJ_Status status = AJ_OK;

    AJ_InfoPrintf(("StopDebugger()\n"));
    /*
     * Maybe called because the session was lost
     */
    if (msg) {
        AJ_Message reply;
        AJ_MarshalReplyMsg(msg, &reply);
        AJ_MarshalArgs(&reply, "y", SCRIPT_DEBUG_STOPPED);
        status = AJ_DeliverMsg(&reply);
    }
    if (dbgState) {
        DebuggerDetached(dbgState);
        duk_debugger_detach(ctx);
        AJS_EnableWatchdogTimer();
    }
    return status;
}

AJ_Status AJS_DebuggerGetStatus(duk_context* ctx, AJ_Message* msg)
{
    AJ_Message reply;
    AJ_Status status = AJ_MarshalReplyMsg(msg, &reply);
    if (status == AJ_OK) {
        status = AJ_MarshalArgs(&reply, "y", dbgState ? dbgState->status : AJS_DEBUG_DETACHED);
    }
    if (status == AJ_OK) {
        status = AJ_DeliverMsg(&reply);
    }
    return status;
}

AJ_Status AJS_DebuggerGetScriptName(duk_context* ctx, AJ_Message* msg)
{
    AJ_Status status;
    AJ_Message reply;
    AJ_NV_DATASET* ds;

    ds = AJ_NVRAM_Open(AJS_SCRIPT_NAME_NVRAM_ID, "r", 0);
    if (ds) {
        const char* name = (const char*)AJ_NVRAM_Peek(ds);
        if (!name) {
            name = "<unknown>";
        }
        status = AJ_MarshalReplyMsg(msg, &reply);
        if (status == AJ_OK) {
            status = AJ_MarshalArgs(&reply, "s", name);
        }
        if (status == AJ_OK) {
            status = AJ_DeliverMsg(&reply);
        }
        AJ_NVRAM_Close(ds);
    } else {
        AJ_ErrPrintf(("Error opening script name NVRAM entry\n"));
        status = AJ_MarshalStatusMsg(msg, &reply, AJ_ERR_INVALID);
        if (status == AJ_OK) {
            status = AJ_DeliverMsg(&reply);
        }
    }
    return status;
}

AJ_Status AJS_DebuggerGetScript(duk_context* ctx, AJ_Message* msg)
{
    AJ_Status status;
    AJ_Message reply;
    AJ_NV_DATASET* ds = NULL;
    const char* script;
    uint32_t sz;
    void* sctx;

    AJ_InfoPrintf(("DebuggerGetScript()\n"));

    /* If the script was previously installed on another boot the size will be zero */
    if (!sz) {
        ds = AJ_NVRAM_Open(AJS_SCRIPT_SIZE_ID, "r", 0);
        if (ds) {
            AJ_NVRAM_Read(&sz, sizeof(sz), ds);
            AJ_NVRAM_Close(ds);
        }
    }
    status = AJS_OpenScript(0, &sctx);
    if (status != AJ_OK) {
        AJ_ErrPrintf(("AJS_DebuggerGetScript(): Error opening script\n"));
        goto ErrorReply;
    } else {
        status = AJS_ReadScript((uint8_t**)&script, &sz, sctx);
        if (status == AJ_OK) {
            if (!script) {
                script = "";
            }
            status = AJ_MarshalReplyMsg(msg, &reply);
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
            AJS_CloseScript(sctx);
            return status;
        } else {
            AJ_ErrPrintf(("AJS_DebuggerGetScript(): Error reading script\n"));
            goto ErrorReply;
        }
    }

ErrorReply:
    status = AJ_MarshalStatusMsg(msg, &reply, AJ_ERR_INVALID);
    if (status == AJ_OK) {
        status = AJ_DeliverMsg(&reply);
    }
    return status;
}

AJ_Status AJS_DebuggerCommand(duk_context* ctx, AJ_Message* msg)
{
    AJ_Status status;

    /*
     * We are going to let the debugger read handler process this message
     * So we need to save the message pointer in the debugger state structure
     */
    if (dbgState && (dbgState->status != AJS_DEBUG_DETACHED)) {
        dbgState->currentMsg = msg;
        /*
         * The debugger read callback will process the message
         */
        duk_debugger_cooperate(ctx);
        AJ_ASSERT(!dbgState || !dbgState->currentMsg);
        status = AJ_OK;
    } else {
        AJ_Message reply;
        AJ_ErrPrintf(("Debug command received when no debugger attached\n"));
        status = AJ_MarshalStatusMsg(msg, &reply, AJ_ERR_INVALID);
        if (status == AJ_OK) {
            status = AJ_DeliverMsg(&reply);
        }
    }
    return status;
}

#endif // AJS_CONSOLE_LOCKDOWN
