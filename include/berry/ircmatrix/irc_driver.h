/* src/irc_driver.h */
#ifndef IRC_DRIVER_H
#define IRC_DRIVER_H

#include "chatlib.h"

/* Handle khusus untuk koneksi IRC */
typedef struct IRCHandle IRCHandle;

/* Membuat koneksi IRC */
IRCHandle* irc_connect(const ChatConfig* config);
/* Mengirim pesan ke channel IRC */
void irc_send_message(IRCHandle* handle, const char *channel, const char *message);
/* Memutus koneksi IRC */
void irc_disconnect(IRCHandle* handle);

#endif // IRC_DRIVER_H