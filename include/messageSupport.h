
#ifndef _MESSAGE_SUPPORT_H
#define _MESSAGE_SUPPORT_H

typedef void (*updateMessageCallback_t)(char const*);
typedef void (*setPriorityMessageCallback_t)(char const*);
typedef void (*clearPriorityMessageCallback_t)(void);
typedef void (*publishMessageCallback_t)(char const*);

#endif