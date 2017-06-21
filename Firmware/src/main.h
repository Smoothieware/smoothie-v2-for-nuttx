#pragma once

#include <mqueue.h>
#include "OutputStream.h"

mqd_t get_message_queue(bool read);
bool send_message_queue(mqd_t mqfd, const char *pline, OutputStream *pos);

// TODO may move to Dispatcher
bool dispatch_line(OutputStream& os, const char *line);
