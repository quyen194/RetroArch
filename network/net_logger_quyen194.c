/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2014 - Hans-Kristian Arntzen
 *  Copyright (C) 2011-2017 - Daniel De Matteis
 *
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

// QuyenNC add start
#if defined(__CELLOS_LV2__) || defined(__PSL1GHT__)
#include "../defines/ps3_defines.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <retro_miscellaneous.h>
#include <net/net_compat.h>
#include <net/net_socket.h>
#include <file/config_file.h>

#include "../configuration.h"
#include "../paths.h"
#include "../verbosity.h"

static int g_sid;
static struct sockaddr_in target;

static bool net_logger_init = false;
static bool net_logger_inited = false;
static char netlogger_ip[32];
static unsigned netlogger_port;

bool aries_logger_config_load()
{
   config_file_t *conf = NULL;

   conf = config_file_new("ux0:/data/retroarch/retroarch.cfg");
   if (!conf)
      return false;

   // get ip
   if (!config_get_array(conf, "netlogger_ip", netlogger_ip, sizeof(netlogger_ip))) {
      return false;
   }

   // get port
   if (!config_get_uint(conf, "netlogger_port", &netlogger_port)) {
      return false;
   }

   if (conf)
      config_file_free(conf);

   return true;
}

void aries_logger_init()
{
   net_logger_init = true;

   if (!aries_logger_config_load()) {
      return;
   }

   socket_target_t in_target;
   const char *server = netlogger_ip;
   unsigned      port = netlogger_port;

   if (!network_init())
   {
      printf("Could not initialize network logger interface.\n");
      return;
   }

   g_sid  = socket_create(
         "ra_netlogger",
         SOCKET_DOMAIN_INET,
         SOCKET_TYPE_DATAGRAM,
         SOCKET_PROTOCOL_NONE);

   in_target.port   = port;
   in_target.server = server;
   in_target.domain = SOCKET_DOMAIN_INET;

   socket_set_target(&target, &in_target);

   net_logger_inited = true;
}

void aries_logger_shutdown()
{
   if (!net_logger_inited) {
      return;
   }

   if (socket_close(g_sid) < 0)
      printf("Could not close socket.\n");

   network_deinit();
}

void aries_logger_send(const char *__format,...)
{
   va_list args;

   va_start(args,__format);
   aries_logger_send_v(__format, args);
   va_end(args);
}

void aries_logger_send_v(const char *__format, va_list args)
{
   if (!net_logger_init) {
      aries_logger_init();
   }

   if (!net_logger_inited) {
      return;
   }

   static char sendbuf[4096];
   int len;

   vsnprintf(sendbuf, 4000, __format, args);
   len = strlen(sendbuf);
   len += snprintf(&sendbuf[len], 4000 - len, "\n");

   sendto(g_sid,
         sendbuf,
         len,
         MSG_DONTWAIT,
         (struct sockaddr*)&target,
         sizeof(target));

   aries_write_log("%s", sendbuf);
}

void aries_write_log(const char *fmt, ...)
{
   va_list ap;
   FILE *log_file = NULL;

   va_start(ap, fmt);

   log_file = (FILE*)fopen("ux0:aries_retroarch.log", "ab");
   if (log_file) {
      vfprintf(log_file, fmt, ap);
      fprintf(log_file, "\n");
      fflush(log_file);
      fclose(log_file);
   }

   va_end(ap);
}
// QuyenNC add end
