#ifndef AJS_DEBUGGER_H_
#define AJS_DEBUGGER_H_
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

#include "duktape.h"
#include "ajs.h"

#define DEBUG_BUFFER_SIZE 512

typedef struct _AJS_DebuggerState {
    duk_context* ctx;
    AJ_IOBuffer* read;      /* IO buffer for AJS_DebuggerRead() */
    AJ_IOBuffer* write;     /* IO buffer for AJS_DebuggerWrite() */
    AJ_Message lastMsg;     /* Holds the last AllJoyn message for method replies */
    uint8_t lastType;       /* Holds the last debug element type (int, string, length etc) */
    uint8_t lastMsgType;    /* Holds the last debug message type (Step in, Out, locals, call stack etc.) */
    uint8_t initialNotify;  /* Flag to signal if the initial notification has happened */
    uint32_t nextChunk;     /* Number of bytes to advance (used if we reach the end of buffer before EOM */
    int8_t curTypePos;      /* Current position in a data type (if length parameter is a uint16 or uint32) */
    uint32_t internal;      /* Place holder for any information while parsing the message (usually a length) */
    uint32_t msgLength;     /* Current messages length */
}AJS_DebuggerState;

/*
 * Builds a simple debug message up to 4 bytes
 */
#define BUILD_DBG_MSG(a, b, c, d) ((a & 0xff) | ((b & 0xff) << 8) | ((c & 0xff) << 16) | ((d & 0xff) << 24))

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
#define AJ_DUK_TYPE_STRING     's' /* <0x60 - 0x7f><data> */
#define AJ_DUK_TYPE_INTSM      'i' /* <0x80 - 0xbf> */
#define AJ_DUK_TYPE_INTLG      't' /* <0xc0 - 0xff><u8> */

/*
 * Debug messages (org.allseen.scriptDebugger)
 */
#define DBG_NOTIF_MSGID     AJ_APP_MESSAGE_ID(0, 2, 0)
#define DBG_BASIC_MSGID     AJ_APP_MESSAGE_ID(0, 2, 1)
#define DBG_TRIGGER_MSGID   AJ_APP_MESSAGE_ID(0, 2, 2)
#define DBG_PAUSE_MSGID     AJ_APP_MESSAGE_ID(0, 2, 3)
#define DBG_RESUME_MSGID    AJ_APP_MESSAGE_ID(0, 2, 4)
#define DBG_STEPIN_MSGID    AJ_APP_MESSAGE_ID(0, 2, 5)
#define DBG_STEPOVER_MSGID  AJ_APP_MESSAGE_ID(0, 2, 6)
#define DBG_STEPOUT_MSGID   AJ_APP_MESSAGE_ID(0, 2, 7)
#define DBG_LISTBREAK_MSGID AJ_APP_MESSAGE_ID(0, 2, 8)
#define DBG_ADDBREAK_MSGID  AJ_APP_MESSAGE_ID(0, 2, 9)
#define DBG_DELBREAK_MSGID  AJ_APP_MESSAGE_ID(0, 2, 10)
#define DBG_GETVAR_MSGID    AJ_APP_MESSAGE_ID(0, 2, 11)
#define DBG_PUTVAR_MSGID    AJ_APP_MESSAGE_ID(0, 2, 12)
#define DBG_GETCALL_MSGID   AJ_APP_MESSAGE_ID(0, 2, 13)
#define DBG_GETLOCALS_MSGID AJ_APP_MESSAGE_ID(0, 2, 14)
#define DBG_DUMPHEAP_MSGID  AJ_APP_MESSAGE_ID(0, 2, 15)
#define DBG_VERSION_MSGID   AJ_APP_MESSAGE_ID(0, 2, 16)
#define DBG_GETSCRIPT_MSGID AJ_APP_MESSAGE_ID(0, 2, 17)
#define DBG_DETACH_MSGID    AJ_APP_MESSAGE_ID(0, 2, 18)
#define DBG_EVAL_MSGID      AJ_APP_MESSAGE_ID(0, 2, 19)
#define DBG_BEGIN_MSGID     AJ_APP_MESSAGE_ID(0, 2, 20)
#define DBG_END_MSGID       AJ_APP_MESSAGE_ID(0, 2, 21)

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

/**
 * Initialize the debugger state.
 *
 * @return          Pointer to the new debugger state
 */
AJS_DebuggerState* AJS_InitDebugger(duk_context* ctx);

/**
 * Deinitialize the debugger state
 *
 * @param state     Pointer to the debugger state
 */
void AJS_DeinitDebugger(AJS_DebuggerState* state);

/**
 * Duktape debugger read callback
 *
 * @param udata     User data (AJS_DebuggerState)
 * @param buffer    Buffer to fill with data the debugger consumes
 * @param length    Requested length of debugger data
 * @return          Number of bytes actually read
 */
duk_size_t AJS_DebuggerRead(void* udata, char*buffer, duk_size_t length);

/**
 * Duktape debugger write callback.
 *
 * @param udata     User data (AJS_DebuggerState)
 * @param buffer    Buffer containing debug data to write
 * @param length    Length of debug data
 * @return          Number of bytes written
 */
duk_size_t AJS_DebuggerWrite(void* udata, const char*buffer, duk_size_t length);

/**
 * Duktape debugger peek callback
 *
 * @param udata     User data (AJS_DebuggerState)
 * @return          Number of bytes available to read
 */
duk_size_t AJS_DebuggerPeek(void* udata);

/**
 * Duktape debugger read flush callback. In AJS implementation read data is
 * always flushed automatically so this callback just returns.
 *
 * @param udata     User data (AJS_DebuggerState)
 */
void AJS_DebuggerReadFlush(void* udata);

/**
 * Duktape debugger write flush callback. In AJS implementation written data is
 * always flushed automatically so this callback just returns.
 *
 * @param udata     User data (AJS_DebuggerState)
 */
void AJS_DebuggerWriteFlush(void* udata);

/**
 * Callback for when the debugger was detached.
 *
 * @param udata     User data (AJS_DebuggerState)
 */
void AJS_DebuggerDetached(void* udata);

#endif /* AJS_DEBUGGER_H_ */
