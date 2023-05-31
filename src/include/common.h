#ifndef COMMON_H
#define COMMON_H

#define ERROR_CATCH(msg, loc) error_msg = msg; goto loc;
#define ATTACHMENT(key) ((struct client_connection_data *)(key)->data)

#endif
