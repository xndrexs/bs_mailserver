#ifndef CONFIG_H
#define CONFIG_H

#define database "database.dat"
#define database_tmp "database_tmp.dat"

#define mailbox_tmp "mailbox/mailbox_tmp"

#define cat_mailbox "mailbox"
#define cat_password "password"
#define cat_smtp "smtp"

#define logging_out "+OK Bye bye.\r\n"

int my_printf(const char *message);

#endif
